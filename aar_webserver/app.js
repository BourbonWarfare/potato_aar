var createError = require('http-errors');
var express = require('express');
var path = require('path');
var adaro = require('adaro');
var cookieParser = require('cookie-parser');
var logger = require('morgan');
var AdmZip = require('adm-zip');
var BSON = require('bson');
const WebSocket = require('ws');
const uuid = require('uuid');

var indexRouter = require('./routes/index');

var app = express();

// view engine setup
app.engine('dust', adaro.dust());
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'dust');

app.use(logger('dev'));
app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

app.use('/', indexRouter);

// catch 404 and forward to error handler
app.use(function(req, res, next) {
  next(createError(404));
});

// error handler
app.use(function(err, req, res, next) {
  // set locals, only providing error in development
  res.locals.message = err.message;
  res.locals.error = req.app.get('env') === 'development' ? err : {};

  // render the error page
  res.status(err.status || 500);
  res.render('error');
});

module.exports = app;

const zip = new AdmZip('./08-08-21_19-00-07_Latofel_TVT35_Patrol_V2.zip');
const metaInfo = JSON.parse(zip.getEntry('meta.json').getData().toString());
const eventQueue = BSON.deserialize(zip.getEntry('events.bson').getData()).events;

const projectiles = BSON.deserialize(zip.getEntry('projectilesTeeny.bson').getData());
var missionObjectsStates = new Map();

var last = Date.now();
var accumulator = 0;

const timestep = 1 / 5;
const overallUpdateRate = 1/20;

function GameObject(uid) {
  this.uid = uid;
  this.overview = BSON.deserialize(zip.getEntry(`${uid.replace(':', '_')}.bson`).getData());
  this.states = this.overview.generalStates;
  this.currentStateIndex = 0;

  this.getLatestState = function(time) {
    while (time >= this.states[this.currentStateIndex].time) {
      this.currentStateIndex += 1;
      if (this.currentStateIndex >= this.states.length) {
        this.currentStateIndex = this.states.length - 1;
        break;
      }
    }
    return this.states[this.currentStateIndex];
  }

  this.isNewState = function(time) {
    return time >= this.states[this.currentStateIndex].time
  }
}

var clients = new Map();
function Client(ws, uid) {
  this.socket = ws;
  this.currentEvent = 0;
  this.timeConnected = Date.now();
  this.activeObjects = new Map();
  this.activeProjectiles = new Map();

  this.send = function(type, json) {
    const packet = {
      type: type,
      data: json
    };
    this.socket.send(JSON.stringify(packet));
  }

  this.addObjectToTrack = function(event) {
    let uid = JSON.parse(event.arguments[0]);
    if (!missionObjectsStates.has(uid)) {
      missionObjectsStates.set(uid, new GameObject(uid));
    }
    this.activeObjects.set(uid, missionObjectsStates.get(uid));
  }

  this.removeObjectToTrack = function(event) {
    let uid = JSON.parse(event.arguments[0]);
    this.activeObjects.delete(uid);
  }
};

const wss = new WebSocket.Server({ port: 8082 });
wss.on('connection', ws => {
  ws.id = uuid.v4();

  thisClient = new Client(ws);
  clients.set(ws.id, thisClient);

  console.log(ws.id);

  thisClient.send('init', metaInfo);

  ws.on('close', () => {
    console.log("client has disconnected");
    clients.delete(ws.id);
  });
});

const update = function() {
  let current = Date.now();
  let delta = (current - last) / 1000;
  last = current;

  accumulator += delta;
  while (accumulator >= timestep) {
    accumulator -= timestep;
    clients.forEach(client => {
      const clientRunTime = (Date.now() - client.timeConnected) / 1000;
      console.log(clientRunTime);

      client.activeObjects.forEach(object => {
        if (object.isNewState(clientRunTime)) {
          const latestState = object.getLatestState(clientRunTime);
          const update = {
            object: object.uid,
            state: latestState
          };
          client.send('object_update', update);
        }
      });

      while (client.currentEvent < eventQueue.length && eventQueue[client.currentEvent].time <= clientRunTime) {
        let frontEvent = eventQueue[client.currentEvent];
        console.log(frontEvent.type);
        switch (frontEvent.type) {
          case "Object Created":
            client.addObjectToTrack(frontEvent);
            break;
          case "Object Killed":
            client.removeObjectToTrack(frontEvent);
            break;
          case "Fired":
            const uid = JSON.parse(frontEvent.arguments[0]);
            frontEvent.metaInfo = {
              lifetime: projectiles[uid.toString()].lifetime
            };
          default:
            break;
        }

        client.send('event', frontEvent);
        client.currentEvent += 1;
      }
    });
  }

  setTimeout(update, overallUpdateRate);
}
update();
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
const sqlite3 = require('sqlite3');

var indexRouter = require('./routes/index');
const { time } = require('console');

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

let cachedMissions = new Map();

const overallUpdateRate = 1 / 20;

function GameObject(uid, zip) {
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
  this.currentTime = 0;
  this.activeObjects = new Map();
  this.activeProjectiles = new Map();
  this.uid = uid;
  this.missionInfo = null;
  this.state = 'init';

  this.send = function(type, json) {
    const packet = {
      type: type,
      data: json
    };
    this.socket.send(JSON.stringify(packet));
  }

  this.addObjectToTrack = function(event) {
    const uid = JSON.parse(event.arguments[0]);
    this.activeObjects.set(uid, new GameObject(uid, this.missionInfo.zip));
  }

  this.removeObjectToTrack = function(event) {
    let uid = JSON.parse(event.arguments[0]);
    this.activeObjects.delete(uid);
  }

  this.update = function(deltaTime) {
    if (this.state != 'playing') { return; }
    this.currentTime += deltaTime;

    this.activeObjects.forEach(object => {
      if (object.isNewState(this.currentTime)) {
        const latestState = object.getLatestState(this.currentTime);
        const update = {
          object: object.uid,
          state: latestState
        };
        this.send('object_update', update);
      }
    });

    while (this.currentEvent < this.eventQueue.length && this.eventQueue[this.currentEvent].time <= this.currentTime) {
      let frontEvent = this.eventQueue[this.currentEvent];
      switch (frontEvent.type) {
        case "Object Created":
          this.addObjectToTrack(frontEvent);
          break;
        case "Object Killed":
          this.removeObjectToTrack(frontEvent);
          break;
        case "Fired":
          const uid = JSON.parse(frontEvent.arguments[0]);
          frontEvent.arguments.push(this.projectiles[uid.toString()].lifetime);
        default:
          break;
      }
      
      this.send('event', frontEvent);
      this.currentEvent += 1;
    };
  }

  this.playMission = function() {
    let db = new sqlite3.Database('missions.db', sqlite3.OPEN_READONLY, (err) => {
      if (err) {
        console.error(err.message);
        return;
      }
    });
    
    db.each('SELECT * FROM missions WHERE ReplayPath="18-08-21_21-04-03_gn502_TvT25_Discotheque_v1.zip";', (err, row) => {
      if (err) {
        console.log(err);
        return;
      }
      let path = row.ReplayPath;
      let map = row.Map;

      if (!cachedMissions.has(path)) {
        const zip = new AdmZip(`./${path}`);
        const metaInfo = JSON.parse(zip.getEntry('meta.json').getData().toString());
        const eventQueue = BSON.deserialize(zip.getEntry('events.bson').getData()).events;

        const projectiles = BSON.deserialize(zip.getEntry('projectilesTeeny.bson').getData());

        cachedMissions.set(path, {
          eventQueue: eventQueue,
          projectiles: projectiles,
          metaInfo: metaInfo,
          zip: zip,
          lastAccess: Date.now()
        });
      }

      let missionMetaInfo = cachedMissions.get(path);

      this.eventQueue = missionMetaInfo.eventQueue;
      this.projectiles = missionMetaInfo.projectiles;
      missionMetaInfo.lastAccess = Date.now();
      
      if (missionMetaInfo.metaInfo.endTime == 0) {
        let maxTimeSeen = 0;
        this.eventQueue.forEach(event => {
          maxTimeSeen = Math.max(maxTimeSeen, event.time);
        });
    
        let adjustedMetaInfo = missionMetaInfo.metaInfo;
        adjustedMetaInfo.endTime = maxTimeSeen;
        thisClient.send('init', adjustedMetaInfo);
      } else {
        thisClient.send('init', missionMetaInfo.metaInfo);
      }

      this.missionInfo = missionMetaInfo;
      this.state = 'playing';
    });
    
    db.close((err) => {
      if (err) {
        console.error(err.message);
      }
    });
  }
};

const wss = new WebSocket.Server({ port: 8082 });
wss.on('connection', ws => {
  ws.id = uuid.v4();

  thisClient = new Client(ws, ws.id);
  clients.set(ws.id, thisClient);
  console.log(`${ws.id} connected`);

  thisClient.playMission();

  ws.on('close', () => {
    console.log(`${ws.id} has disconnected`);
    clients.delete(ws.id);
  });
});

const update = function() {
  // for each tick, update client 15 times as fast
  clients.forEach(client => { client.update(overallUpdateRate * 15) });
  setTimeout(update, overallUpdateRate);
}
update();
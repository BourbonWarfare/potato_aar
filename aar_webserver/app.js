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

var zip = new AdmZip('./test.zip');
const metaInfo = JSON.parse(zip.getEntry('meta.json').getData().toString());
const eventQueue = BSON.deserialize(zip.getEntry('events.bson').getData()).events;

var last = Date.now();
var accumulator = 0;

const timestep = 1 / 5;

var clients = new Map();
function Client(ws, uid) {
  this.socket = ws;
  this.currentEvent = 0;
  this.timeConnected = Date.now();

  this.send = function(type, json) {
    const packet = {
      type: type,
      data: json
    };
    this.socket.send(JSON.stringify(packet));
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

const a = function() {
  var current = Date.now();
  var delta = (current - last) / 1000;
  last = current;

  accumulator += delta;
  while (accumulator >= timestep) {
    accumulator -= timestep;
    clients.forEach(client => {
      const clientRunTime = (Date.now() - client.timeConnected) / 1000;

      while (client.currentEvent < eventQueue.length && eventQueue[client.currentEvent].time <= clientRunTime) {
        client.send('event', eventQueue[client.currentEvent]);
        client.currentEvent += 1;
      }
    });
  }

  setImmediate(a);
}
a();
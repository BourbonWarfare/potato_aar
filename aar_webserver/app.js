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
const config = require('./server_config.json');

console.log('Started server with settings: ', config);

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

function MissionCache(timeUntilRemoval) {
  this.cache = new Map();
  this.timeUntilRemoval = timeUntilRemoval; // in seconds

  this.get = function(path) {
    if (!this.cache.has(path)) {
      let filepath = `./${config.mission_replay_path_relative}`;
      if (filepath == './') {
        filepath = `./${path}`;
      } else {
        filepath += `/${path}`;
      }

      const zip = new AdmZip(filepath);
      const metaInfo = JSON.parse(zip.getEntry('meta.json').getData().toString());
      const eventQueue = BSON.deserialize(zip.getEntry('events.bson').getData()).events;

      const projectiles = BSON.deserialize(zip.getEntry('projectilesTeeny.bson').getData());

      this.cache.set(path, {
        eventQueue: eventQueue,
        projectiles: projectiles,
        metaInfo: metaInfo,
        zip: zip,
        lastAccess: Date.now()
      });

      console.log(`Adding ${path} to cache`);
    } else {
      console.log(`Retrieving ${path} from cache`);
    }

    let mission = this.cache.get(path);
    mission.lastAccess = Date.now();

    return mission;
  }

  this.update = function() {
    this.cache.forEach((mission, key) => {
      const timeDelta = (Date.now() - mission.lastAccess) / 1000;

      if (timeDelta >= this.timeUntilRemoval) {
        console.log(`Removing ${key} from cache (timeDelta: ${timeDelta})`);
        this.cache.delete(key);
      }
    });
  }
}
const cachedMissions = new MissionCache(config.keep_mission_cached_for);
const overallUpdateRate = config.tick_rate;

function mysql_real_escape_string(str) {
  if (typeof str != 'string')
      return str;

  return str.replace(/[\0\x08\x09\x1a\n\r"'\\\%]/g, function (char) {
      switch (char) {
          case "\0":
              return "\\0";
          case "\x08":
              return "\\b";
          case "\x09":
              return "\\t";
          case "\x1a":
              return "\\z";
          case "\n":
              return "\\n";
          case "\r":
              return "\\r";
          case "\"":
          case "'":
          case "\\":
          case "%":
              return "\\"+char; // prepends a backslash to backslash, percent,
                                // and double/single quotes
      }
  });
}

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

  this.playMission = function(file) {
    this.state = 'init';

    this.currentEvent = 0;
    this.currentTime = 0;
    this.activeObjects = new Map();
    this.activeProjectiles = new Map();

    let db = new sqlite3.Database('missions.db', sqlite3.OPEN_READONLY, (err) => {
      if (err) {
        console.error(err.message);
        return;
      }
    });
    
    db.each(`SELECT * FROM missions WHERE ReplayPath="${file}";`, (err, row) => {
      if (err) {
        console.error(err);
        return;
      }
      let path = row.ReplayPath;

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

const wss = new WebSocket.Server({ port: config.web_rtc_port });
wss.on('connection', ws => {
  ws.id = uuid.v4();

  thisClient = new Client(ws, ws.id);
  clients.set(ws.id, thisClient);
  console.log(`${ws.id} connected`);

  ws.on('message', (message) => {
    const object = JSON.parse(message);
    const data = object.data;
    switch (object.type) {
      case 'missionRequest':
        console.log(`${thisClient.uid} requesting ${data.dbKey}`)
        thisClient.playMission(mysql_real_escape_string(data.dbKey));
        break;
      default:
        break;
    }
  })

  ws.on('close', () => {
    console.log(`${ws.id} has disconnected`);
    clients.delete(ws.id);
  });
});

const update = function() {
  // for each tick, update client 15 times as fast
  clients.forEach(client => { client.update(overallUpdateRate * config.client_update_update_ahead) });
  cachedMissions.update();
  setTimeout(update, overallUpdateRate);
}
update();
var express = require('express');
var router = express.Router();
const sqlite3 = require('sqlite3');

/* GET home page. */
router.get('/', function(req, res, next) {
  let db = new sqlite3.Database('missions.db', sqlite3.OPEN_READONLY, (err) => {
    if (err) {
      console.error(err.message);
      return;
    }
  });
  
  var entries = [];
  db.serialize(() => {
    this.missions = [];
    db.each('SELECT * FROM missions ORDER BY Date DESC;', (err, row) => {
      if (err) {
        console.error(err);
        return;
      }
      let date = new Date(row.Date * 1000);
      row.RealDate = row.Date;
      row.Date = date.toLocaleString();

      entries.push(row);
    }, (err, count) => {
      if (err) {
        console.error(err);
        return;
      }
      res.render('index', { missions: entries });
    });
  });
  
  db.close((err) => {
    if (err) {
      console.error(err.message);
    }
  });
});

module.exports = router;

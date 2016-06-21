var NRF51Node = require('./index.js');
var debug = require('debug')('teleinfo_ble');
var async = require('async');
var influx = require('influx');

/************************************************
 * teleinfo_ble_node.js
 *************************************************/
var teleinfoBleNode = null;

var TeleinfoTypes = Object.freeze({
  IINST : 0,
  APP_POWER : 1
});

if(process.env.DB){
  var db = process.env.DB;
}
else{
  var db = 'teleinfo_report';
}

if(process.env.DB_USER){
  var db_user = process.env.DB_USER;
}
else{
  console.log('ERROR : DB_USER variable must be set');
  process.exit(1);
}

if(process.env.DB_PASS){
  var db_pass = process.env.DB_PASS;
}
else{
  console.log('ERROR : DB_PASS variable must be set');
  process.exit(1);
}

dbClient = influx({

  //single-host configuration 
  host : 'localhost',
  port : 8086, // optional, default 8086 
  protocol : 'http', // optional, default 'http' 
  database : db,
  username : db_user,
  password : db_pass
});

/**************************************
 * Exit handlers
 ***************************************/
function cleanNRF51Node() {
  debug('clean test');
  if (teleinfoBleNode !== null) {
    teleinfoBleNode.disconnect();
    teleinfoBleNode = null;
  }
  debug('teleinfo_node: nrf51 node cleaned');
}

function exitHandler(options, err) {
  if (options.cleanup) {cleanNRF51Node();}
  if (err) {debug(err.stack);}
  if (options.exit) {process.exit();}
}

//do something when app is closing
process.on('exit', exitHandler.bind(null, {
  cleanup: true
}));

//catches ctrl+c event
process.on('SIGINT', exitHandler.bind(null, {
  exit: true
}));

//catches uncaught exceptions
process.on('uncaughtException', exitHandler.bind(null, {
  exit: true
}));

/*******************************************
 * teleinf_ble init 
 *******************************************/

debug('starting teleinfo_ble'); 

async.series([
  function (callback) {
    debug('discovering some teleinfoBleNode nodes');
    NRF51Node.discover(function (err, discoveredTeleinfoBleNode) {
      if(err){
        callback(err);
      }
      else{
        debug('teleinfoBleNode with uuid ' + discoveredTeleinfoBleNode._uuid + ' discovered');
        teleinfoBleNode = discoveredTeleinfoBleNode;
        teleinfoBleNode.on('connectionDrop', function(){
          debug('connection drop - reconnect');
          teleinfoBleNode.reconnect();
        });
        callback();
      }
    });
  },

  function (callback) {
    debug('connect to teleinfoBleNode');
    teleinfoBleNode.connect(function () {
      debug('connected to teleinfoBleNode');
      callback();
    });
  },

  function (callback) {
    debug('discover teleinfoBleNode services');
    teleinfoBleNode.on('reconnect', function() {
      debug('successfully reconnected during chars discovery!');
      callback();
    });
    teleinfoBleNode.discoverServicesAndCharacteristics(function () {
      debug('teleinfoBleNode services discovered');
      callback();
    });
  },

  function (callback) {
    teleinfoBleNode.removeAllListeners('reconnect');
    teleinfoBleNode.readDeviceName(function (deviceName) {
      debug('teleinfoBleNode name is ' + deviceName);
      callback();
    });
  },

  function (callback) {
    openDB(function(err){
      if(!err){
        debug('DB ' + db + ' successfully created');
      } 
      callback(err);
    });	
  },


  function (callback) {
    debug('notify for new data');
    /** PREREQUISITIES : Some data must written on discovered RFDuino using serial interface */
    teleinfoBleNode.on('dataReceived', function (data) {
      if(data.length === 0){
        debug('no data in received frame');
      }
      else{
        onDataReceived(data, function(err){
          if(err){
            debug('error ' + err + ' when handling data receieved');
          }
          else{
            /** nothing to do - data handled */
          }
        }); 			
      }
    });
    teleinfoBleNode.notifyDataReceive(function () {
      debug('you will be notified on new data');
      callback();
    });
  }
],

function (error, results) {
  if (error) {
    debug('teleinfo_ble : - error : ' + error + ' - exiting...');
    cleanNRF51Node();
  } else {
    debug('teleinfo_ble - init ok');
  }
});

function onDataReceived(data, callback){
  switch(data[0]){
    case TeleinfoTypes.IINST :
      var iinst = data.readUInt16BE(1);
      debug('IINST=' + iinst + 'A');
      toDB('teleinfo_iinst', iinst, callback);
      break;

    case TeleinfoTypes.APP_POWER:
      var appPower = data.readUInt32BE(1);
      debug('APPPOWER=' + appPower + 'W');
      toDB('teleinfo_app_power', appPower, callback);
      break;
      
    default:
      debug('teleinfo data ' + data[0] + ' not handled');
  }
}

function openDB(callback){
  var nbConnectTries = 10;

  /** check db connection & existence */
  dbClient.getDatabaseNames(function(err, dbNames)
      {
        if(err)
        {
          debug(err + ' unable to get db names - try ' + nbConnectTries + ' times');
          var timer = setInterval(function(){
            dbClient.getDatabaseNames(function(err, dbNames){
              nbConnectTries--;
              if(!err)
              {
                clearInterval(timer);
                createDb(dbNames, callback);
              }

              if(nbConnectTries === 0)
              {  
                clearInterval(timer);
                debug(err + ' - could not connect to db');
                callback(new Error(err + ' - could not check db connection or db existence'));
              }
              else
              {
                debug(err + ' unable to get db names - try ' + nbConnectTries + ' times');
              }
            });
          }, 2000);
        }
        else
        {
          createDb(dbNames, callback);
        }
      });
}

function createDb(dbNames, callback)
{
  if(dbNames.indexOf(db) == -1)
  {
    dbClient.createDatabase(db, function(err, result) {
      if(err)
      {
        debug(err + ' when creating db ' + db);
      }
      else
      {
        debug("db " + db + " successfully created");
      }
      callback(err); 
    });
  }
  else
  {
    callback();
  }
};


function toDB(field, fieldValue, callback)
{
  dbClient.writePoint(field, {time: new Date(), value: fieldValue}, null, function(err, response) { 
    if(err)
    {
      debug("Cannot write to db : " + err);
    }});
}


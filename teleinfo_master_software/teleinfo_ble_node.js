var NRF51Node = require('./index.js');
var debug = require('debug')('teleinfo_ble');
var async = require('async');

/************************************************
 * teleinfo_ble_node.js
 *************************************************/
var nrf51Node = null;

var TeleinfoTypes = Object.freeze({
	IINST: 0
});

/**************************************
 * Exit handlers
 ***************************************/
function cleanNRF51Node() {
	debug('clean test');
	if (nrf51Node !== null) {
		nrf51Node.disconnect();
		nrf51Node = null;
	}
	debug('nrf51Node_node_test : TEST END');
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
		debug('discovering some nrf51Node nodes');
		NRF51Node.discover(function (err, discoveredNRF51Node) {
			if(err){
        callback(err);
      }
      else{
        debug('nrf51Node_node with uuid ' + discoveredNRF51Node._uuid + ' discovered');
        nrf51Node = discoveredNRF51Node;
        nrf51Node.on('connectionDrop', function(){
          debug('connection drop - reconnect');
          nrf51Node.reconnect();
        });
        callback();
      }
		});
	},

	function (callback) {
		debug('connect to nrf51Node_node');
		nrf51Node.connect(function () {
			debug('connected to nrf51Node_node');
			callback();
		});
	},

	function (callback) {
		debug('discover nrf51Node_node services');
		nrf51Node.on('reconnect', function() {
		    debug('successfully reconnected during chars discovery!');
		    callback();
		});
		nrf51Node.discoverServicesAndCharacteristics(function () {
			debug('nrf51Node_node services discovered');
			callback();
		});
	},

	function (callback) {
		nrf51Node.removeAllListeners('reconnect');
		nrf51Node.readDeviceName(function (deviceName) {
			debug('nrf51Node_node name is ' + deviceName);
			callback();
		});
	},

	function (callback) {
		nrf51Node.readAppearance(function (appearance) {
			debug('nrf51Node_node apparance bytes are name is : ');
			for (var index = 0; index < appearance.length; index++) {
				debug('0x' + appearance[index].toString(16) + ' ');
			}
			callback();
		});
	},

	function (callback) {
		nrf51Node.readPreferredConnParams(function (preferredConnParams) {
			debug('nrf51Node_node preferred conn params are : ');
			for (var index = 0; index < preferredConnParams.length; index++) {
				debug('0x' + preferredConnParams[index].toString(16) + ' ');
			}
			callback();
		});
	},

	function (callback) {
		debug('notify for new data');
		/** PREREQUISITIES : Some data must written on discovered RFDuino using serial interface */
		nrf51Node.on('dataReceived', function (data) {
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
		nrf51Node.notifyDataReceive(function () {
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
	
	    default:
              debug('teleinfo data ' + data[0] + ' not handled');
	}
}

function toDB(field, value, callback)
{
  callback();
}

var NRF51Node = require('./index.js');
var debug = require('debug')('nrf51_template_app');
var async = require('async');

/************************************************
 * nrf51_template_app.js
 *************************************************/

//object to test
var nrf51Node = null;

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
 * Start nrf51Node_node_test scenario
 *******************************************/

debug('starting nrf51Node_node_test');

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
		debug('writing data to node');
		nrf51Node.on('reconnect', function() {
		debug('successfully reconnected when writing data');
			nrf51Node.writeData(new Buffer([0x48, 0x65, 0x6C, 0x6C, 0x6F]), function () {
			debug('data written to  nrf51Node');
			callback();
			});
		});
		/** PREREQUISITIES : Some data must read on discovered RFDuino using serial interface */
		nrf51Node.writeData(new Buffer([0x48, 0x65, 0x6C, 0x6C, 0x6F]), function () {
			debug('data written to  nrf51Node');
			callback();
		});
	},

	function (callback) {
		debug('notify for new data');
		nrf51Node.on('reconnect', function() {
		    debug('successfully reconnected during test!');
		});

		/** PREREQUISITIES : Some data must written on discovered RFDuino using serial interface */
		nrf51Node.on('dataReceived', function (data) {
			if(data.length === 0){
				debug('no data in received frame');
			}
			else{
				debug('data received : ');
				for (var index = 0; index < data.length; index++) {
					debug('0x' + data[index].toString(16) + ' ');
				}
				if(data.length === 1){
					var ackFrame = new Buffer(1);
					ackFrame[0] = 0x02;
					if(data[0] === 0x0){
						debug('led is off');
						//send an ack
						nrf51Node.writeData(ackFrame, function(){
							debug('ack sent');
						});
					}
					else if(data[0] === 0x1){
						debug('led is on');
						//send an ack
						nrf51Node.writeData(ackFrame, function(){
							debug('ack sent');
						});
					}
					else{
						debug('invalid byte 0x' + data[0].toString(16) + ' received');
					}
				}else{
					debug('too much data received');
				}
			}
		});
		nrf51Node.notifyDataReceive(function () {
			debug('you will be notified on new data');
			callback();
		});
	},

	function (callback) {
	    nrf51Node.removeAllListeners('reconnect');
		debug('application running...');
		// Insert other things to do...
}],

function (error, results) {
	if (error) {
		debug('nrf51Node_node template application : - error : ' + error + ' - exiting test...');
		cleanNRF51Node();
	} else {
		debug('nrf51Node_node template application - SUCCESS');
	}
});

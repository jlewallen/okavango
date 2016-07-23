var _ = require('lodash');
var Promise = require('promise');
var SerialPort = require('serialport');

var getPorts = Promise.denodeify(SerialPort.list);

function openAndClosePort(path) {
    return new Promise(function(resolve, reject) {
        var port = new SerialPort(path, {
            baudRate: 1200
        });

        port.on('open', function() {
            port.close();
            resolve();
        });
        port.on('error', function(err) {
            reject(err);
        });
    });
}

function guessPort(ports, pattern, portPath) {
    if (portPath) {
        return portPath;
    }
    if (pattern) {
        var matches = _(ports).filter(function(port) {
            return port.manufacturer.match(pattern);
        });

        if (matches.some()) {
            return matches.first().comName;
        }
    }

    throw "Unable to guess Port.";
}

function delay(ms) {
    return new Promise(function(resolve) {
        setTimeout(resolve, ms);
    });
}

var board = _(process.argv).slice(2).first();
var pattern = _(process.argv).slice(3).first();
var portPath = _(process.argv).slice(4).first();

getPorts()
    .then(function(portsBefore) {
        var port = guessPort(portsBefore, pattern, portPath);
        if (board == "adafruit:avr:feather32u4") {
            return openAndClosePort(port).then(function() {
                return delay(1000);
            }).then(function() {
                return getPorts();
            }).then(function(portsAfter) {
                var newPorts = _(portsAfter).map('comName').difference(_(portsBefore).map('comName').value());
                if (newPorts.some()) {
                    return newPorts.first();
                }
                else {
                    return guessPort(portsAfter, pattern, null);
                }
            });
        }

        return port;
    })
    .then(function(port) {
        console.log(port);
    })
    .done();

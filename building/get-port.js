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

function portNames(array) {
    return _(array).map('comName');
}

function checkForPortChange(portsBefore, tries, desiredPort) {
    return getPorts().then(function(portsAfter) {
        if (desiredPort) {
            if (portNames(portsAfter).includes(desiredPort)) {
                return desiredPort;
            }
        }

        console.warn(portNames(portsBefore).join(' '), '->', portNames(portsAfter).join(' '));

        var newPorts = portNames(portsAfter).difference(portNames(portsBefore).value());
        if (newPorts.some()) {
            return newPorts.first();
        }
        else {
            if (tries == 10) {
                console.error("Giving up, guessing...");
                return guessPort(portsAfter, pattern, portPath);
            }

            return delay(500).then(function() {
                return checkForPortChange(portsBefore, tries + 1);
            });
        }
    });
}

var args = _(process.argv);
var mode = args.slice(2).first();
var board = args.slice(3).first();
var pattern = args.slice(4).first();
var portPath = args.slice(5).first();

getPorts()
    .then(function(portsBefore) {
        var port = guessPort(portsBefore, pattern, portPath);
        console.warn("First guess", port);

        if (mode == 'monitor') {
            return port;
        }

        if (mode == 'watch') {
            return checkForPortChange(portsBefore, 0, portPath);
        }

        if (/feather/.test(board) && mode == 'upload') {
            console.warn("Using 1200bps trick...");

            return openAndClosePort(port).then(function() {
                return delay(500);
            }).then(function() {
                return checkForPortChange(portsBefore, 0, undefined);
            });
        }

        return port;
    })
    .then(function(port) {
        console.log(port);
    })
    .done();

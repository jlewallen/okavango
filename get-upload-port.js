var _ = require('lodash');
var SerialPort = require('serialport');

var board = _(process.argv).slice(2).first();
var portPath = _(process.argv).slice(3).first();
var regex = _(process.argv).slice(4).first();

if (board == "arduino:avr:uno") {
    console.log(portPath);
}
else {
    SerialPort.list(function(err, portsBefore) {
        if (_(portsBefore).map('comName').includes(portPath)) {
            var port = new SerialPort(portPath, {
                baudRate: 1200
            });

            port.on('open', function() {
                port.close();

                setTimeout(function() {
                    SerialPort.list(function(err, portsAfter) {
                        var additionalPorts = _(portsAfter).map('comName').difference(_(portsBefore).map('comName').value());
                        if (additionalPorts.some()) {
                            console.log(additionalPorts.first());
                        }
                        else {
                            console.log("Unable to find PORT");
                        }
                    });
                }, 1000);
            });
        }
        else {
            var matches = _(portsBefore).filter(function(port) {
                return port.manufacturer.match(regex);
            });

            if (matches.some()) {
                console.log(matches.map('comName').first());
            }
            else {
                console.log(_.map(portsBefore, 'comName'));
                console.log('Unable to find', portPath);
            }
        }
    });
}

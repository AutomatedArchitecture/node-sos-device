'use strict';

var path = require('path');
var sosNative = require(path.join(__dirname, 'build/Release/sos.node'));

exports.connect = function(callback) {
  return sosNative.findDevice(function(err, device) {
    if (err) {
      return callback(err);
    }
    device.readInfo(function(err, info) {
      if (err) {
        return callback(err);
      }

      device.readAllInfo = readAllInfo.bind(device);

      return callback(null, device);
    });
  });
};

function readAllInfo(callback) {
  var self = this;
  self.readInfo(function(err, deviceInfo) {
    if (err) {
      return callback(err);
    }

    return self.readLedPatterns(function(err, ledPatterns) {
      if (err) {
        return callback(err);
      }

      deviceInfo.ledPatterns = ledPatterns;
      return self.readAudioPatterns(function(err, audioPatterns) {
        if (err) {
          return callback(err);
        }

        deviceInfo.audioPatterns = audioPatterns;
        return callback(null, deviceInfo);
      });
    })
  });
}
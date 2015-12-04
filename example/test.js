var sos = require('../');

sos.connect(function(err, sosDevice) {
  if (err) {
    console.error(err);
    return;
  }

  return getDeviceInfo();

  function getDeviceInfo() {
    sosDevice.readAllInfo(function(err, deviceInfo) {
      if (err) {
        console.error(err);
        return;
      }

      console.log("Device Info");
      console.log(deviceInfo);

      return play(deviceInfo);
    });
  }

  function play(deviceInfo) {
    var controlPacket = {
      ledMode: deviceInfo.ledPatterns[0].id,
      ledPlayDuration: 500,
      audioMode: deviceInfo.audioPatterns[0].id,
      audioPlayDuration: 500
    };

    return sosDevice.sendControlPacket(controlPacket, function(err) {
      if (err) {
        console.error(err);
        return;
      }
      return;
    });
  }
});

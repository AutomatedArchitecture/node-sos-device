[![Build Status](https://travis-ci.org/AutomatedArchitecture/node-sos-device.svg)](https://travis-ci.org/AutomatedArchitecture/node-sos-device)

# node-sos

Node.js driver for a [Siren of Shame](http://sirenofshame.com/) device.

## Installation

```bash
$ npm install sos-device
```

NOTE: You must have libusb 0.x.x installed first.

## Quick Examples

```javascript
var sos = require("sos-device");

sos.connect(function(err, sosDevice) {
  sosDevice.readAllInfo(function(err, deviceInfo) {
    var controlPacket = {
      ledMode: deviceInfo.ledPatterns[0].id,
      ledPlayDuration: 500,
      audioMode: deviceInfo.audioPatterns[0].id,
      audioPlayDuration: 500
    };

    return sosDevice.sendControlPacket(controlPacket, function(err) {

    });
  })
});
```

# Index

## sos
 * [connect](#sosConnect)

## sosDevice
 * [readAllInfo](#sosDeviceReadAllInfo)
 * [sendControlPacket](#sosDeviceSendControlPacket)

# API Documentation

<a name="sos"/>
## sos

<a name="sosConnect" />
**sos.connect(callback)**

Connects to the Siren of Shame device.

__Arguments__

 * callback(err, sosDevice) - The callback called once the device is connected.

<a name="sosDevice"/>
## sosDevice

<a name="sosDeviceReadAllInfo" />
**sosDevice.readAllInfo(callback)**

Gets all information from the SoS device (LED patterns, Audio patterns, version, etc.).

__Arguments__

 * callback(err, deviceInfo) - The callback called once the device info is retrieved.

<a name="sosDeviceSendControlPacket" />
**sosDevice.sendControlPacket(packet, callback)**

Sends a control packet (ie controls the SoS device).

__Arguments__

 * packet - The packet to send. All the items below are optional.
 ** ledMode - An id from the list of LED patterns, 0 for off, or 1 for manual control.
 ** ledPlayDuration - The duration to play the LED pattern in milliseconds.
 ** audioMode - An id from the list of audio patterns, 0 for off, or 1 for manual control.
 ** audioPlayDuration - The duration to play the audio pattern in milliseconds.
 ** manualLeds0 - Control LED 0. 1 or 0.
 ** manualLeds1 - Control LED 1. 1 or 0.
 ** manualLeds2 - Control LED 2. 1 or 0.
 ** manualLeds3 - Control LED 3. 1 or 0.
 ** manualLeds4 - Control LED 4. 1 or 0.
 * callback(err) - Called once the control packet has been sent.

## License

(The MIT License)

Copyright (c) 2012 Near Infinity Corporation

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

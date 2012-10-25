
#include "nodeSos.h"
#include "usbPackets.h"

int sosVendorId = 5840;
int sosProductId = 1606;

// Values for bmRequestType in the Setup transaction's Data packet.
static const int CONTROL_REQUEST_TYPE_IN = USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
static const int CONTROL_REQUEST_TYPE_OUT = USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;

// From the HID spec:
static const int HID_REPORT_GET = 0x01;
static const int HID_REPORT_SET = 0x09;
static const int HID_REPORT_TYPE_INPUT = 0x01;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

static const int INTERFACE_NUMBER = 0;

class NodeSosException {
  char errorMessage[1000];

public:
  NodeSosException(const char* errorMessage) {
    strcpy(this->errorMessage, errorMessage);
  }

  v8::Handle<v8::Value> toV8() {
    return v8::Exception::Error(v8::String::New(errorMessage));
  }
};

void initControlPacket(UsbControlPacket *packet) {
  memset(packet, 0, sizeof(UsbControlPacket));
  packet->controlByte1 = 0;
  packet->audioMode = 0xff;
  packet->ledMode = 0xff;
  packet->audioPlayDuration = 0xffff;
  packet->ledPlayDuration = 0xffff;
  packet->readAudioIndex = 0xff;
  packet->readLedIndex = 0xff;
  packet->manualLeds0 = 0xff;
  packet->manualLeds1 = 0xff;
  packet->manualLeds2 = 0xff;
  packet->manualLeds3 = 0xff;
  packet->manualLeds4 = 0xff;
}

void SosDevice::getInputReport(int reportId, char* buf, int bufSize) {
  char errorBuffer[1000];
  int claimResult = usb_claim_interface(devHandle, INTERFACE_NUMBER);
  if(claimResult != 0) {
    sprintf(errorBuffer, "usb_claim_interface: %d %s\n", claimResult, usb_strerror());
    throw NodeSosException(errorBuffer);
  }

  int bytesSent = usb_control_msg(
    devHandle,
    CONTROL_REQUEST_TYPE_IN,
    HID_REPORT_GET,
    (HID_REPORT_TYPE_INPUT << 8) | reportId,
    INTERFACE_NUMBER,
    buf,
    bufSize,
    10000);
  if(bytesSent < 0) {
    sprintf(errorBuffer, "usb_control_msg: %d %s\n", bytesSent, usb_strerror());
    throw NodeSosException(errorBuffer);
  }

  int releaseResult = usb_release_interface(devHandle, INTERFACE_NUMBER);
  if(releaseResult != 0) {
    sprintf(errorBuffer, "usb_release_interface: %d %s\n", releaseResult, usb_strerror());
    throw NodeSosException(errorBuffer);
  }
}

void SosDevice::setOutputReport(int reportId, char* buf, int bufSize) {
  char errorBuffer[1000];
  int claimResult = usb_claim_interface(devHandle, INTERFACE_NUMBER);
  if(claimResult != 0) {
    sprintf(errorBuffer, "usb_claim_interface: %d %s\n", claimResult, usb_strerror());
    throw NodeSosException(errorBuffer);
  }

  int bytesSent = usb_control_msg(
    devHandle,
    CONTROL_REQUEST_TYPE_OUT,
    HID_REPORT_SET,
    (HID_REPORT_TYPE_INPUT << 8) | reportId,
    INTERFACE_NUMBER,
    buf,
    bufSize,
    10000);
  if(bytesSent < 0) {
    sprintf(errorBuffer, "usb_control_msg: %d %s\n", bytesSent, usb_strerror());
    throw NodeSosException(errorBuffer);
  }

  int releaseResult = usb_release_interface(devHandle, INTERFACE_NUMBER);
  if(releaseResult != 0) {
    sprintf(errorBuffer, "usb_release_interface: %d %s\n", releaseResult, usb_strerror());
    throw NodeSosException(errorBuffer);
  }
}

/*static*/ v8::Handle<v8::Value> SosDevice::readInfo(const v8::Arguments& args) {
  v8::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = node::ObjectWrap::Unwrap<SosDevice>(args.This());

  v8::Function *callback = v8::Function::Cast(*args[0]);

  UsbInfoPacket usbInfoPacket;
  try {
    sosDevice->getInputReport(USB_REPORTID_IN_INFO, (char*)&usbInfoPacket, sizeof(usbInfoPacket));
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = v8::Undefined();
    callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
    return scope.Close(v8::Undefined());
  }

  v8::Local<v8::Object> result = v8::Object::New();
  result->Set(v8::String::New("version"), v8::Integer::New(usbInfoPacket.version));
  result->Set(v8::String::New("hardwareType"), v8::Integer::New(usbInfoPacket.hardwareType));
  result->Set(v8::String::New("hardwareVersion"), v8::Integer::New(usbInfoPacket.hardwareVersion));
  result->Set(v8::String::New("externalMemorySize"), v8::Integer::New(usbInfoPacket.externalMemorySize));
  result->Set(v8::String::New("audioMode"), v8::Integer::New(usbInfoPacket.audioMode));
  result->Set(v8::String::New("audioPlayDuration"), v8::Integer::New(usbInfoPacket.audioPlayDuration));
  result->Set(v8::String::New("ledMode"), v8::Integer::New(usbInfoPacket.ledMode));
  result->Set(v8::String::New("ledPlayDuration"), v8::Integer::New(usbInfoPacket.ledPlayDuration));

  callbackArgs[0] = v8::Undefined();
  callbackArgs[1] = result;
  callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
  return scope.Close(v8::Undefined());
}

/*static*/ v8::Handle<v8::Value> SosDevice::readLedPatterns(const v8::Arguments& args) {
  v8::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = node::ObjectWrap::Unwrap<SosDevice>(args.This());

  v8::Function *callback = v8::Function::Cast(*args[0]);

  v8::Local<v8::Array> ledPatterns = v8::Array::New();
  UsbReadLedPacket usbReadLedPacket;
  try {
    UsbControlPacket usbControlPacket;
    initControlPacket(&usbControlPacket);
    usbControlPacket.readLedIndex = 0;
    sosDevice->setOutputReport(USB_REPORTID_OUT_CONTROL, (char*)&usbControlPacket, sizeof(usbControlPacket));

    for(int i=0; ; i++) {
      sosDevice->getInputReport(USB_REPORTID_IN_READ_LED, (char*)&usbReadLedPacket, sizeof(usbReadLedPacket));
      if(usbReadLedPacket.id == 0xff) {
        break;
      }
      v8::Local<v8::Object> ledPattern = v8::Object::New();
      ledPattern->Set(v8::String::New("id"), v8::Integer::New(usbReadLedPacket.id));
      ledPattern->Set(v8::String::New("name"), v8::String::New(usbReadLedPacket.name));
      ledPatterns->Set(i, ledPattern);
    }
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = v8::Undefined();
    callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
    return scope.Close(v8::Undefined());
  }

  callbackArgs[0] = v8::Undefined();
  callbackArgs[1] = ledPatterns;
  callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
  return scope.Close(v8::Undefined());
}

/*static*/ v8::Handle<v8::Value> SosDevice::readAudioPatterns(const v8::Arguments& args) {
  v8::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = node::ObjectWrap::Unwrap<SosDevice>(args.This());

  v8::Function *callback = v8::Function::Cast(*args[0]);

  v8::Local<v8::Array> audioPatterns = v8::Array::New();
  UsbReadLedPacket UsbReadAudioPacket;
  try {
    UsbControlPacket usbControlPacket;
    initControlPacket(&usbControlPacket);
    usbControlPacket.readAudioIndex = 0;
    sosDevice->setOutputReport(USB_REPORTID_OUT_CONTROL, (char*)&usbControlPacket, sizeof(usbControlPacket));

    for(int i=0; ; i++) {
      sosDevice->getInputReport(USB_REPORTID_IN_READ_AUDIO, (char*)&UsbReadAudioPacket, sizeof(UsbReadAudioPacket));
      if(UsbReadAudioPacket.id == 0xff) {
        break;
      }
      v8::Local<v8::Object> audioPattern = v8::Object::New();
      audioPattern->Set(v8::String::New("id"), v8::Integer::New(UsbReadAudioPacket.id));
      audioPattern->Set(v8::String::New("name"), v8::String::New(UsbReadAudioPacket.name));
      audioPatterns->Set(i, audioPattern);
    }
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = v8::Undefined();
    callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
    return scope.Close(v8::Undefined());
  }

  callbackArgs[0] = v8::Undefined();
  callbackArgs[1] = audioPatterns;
  callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
  return scope.Close(v8::Undefined());
}

/*static*/ v8::Handle<v8::Value> SosDevice::sendControlPacket(const v8::Arguments& args) {
  v8::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = node::ObjectWrap::Unwrap<SosDevice>(args.This());

  v8::Object *values = v8::Object::Cast(*args[0]);
  v8::Function *callback = v8::Function::Cast(*args[1]);

  UsbControlPacket usbControlPacket;
  initControlPacket(&usbControlPacket);

  if(values->Has(v8::String::New("ledMode"))) {
    usbControlPacket.ledMode = v8::Integer::Cast(*values->Get(v8::String::New("ledMode")))->Value();
  }
  if(values->Has(v8::String::New("ledPlayDuration"))) {
    usbControlPacket.ledPlayDuration = v8::Integer::Cast(*values->Get(v8::String::New("ledPlayDuration")))->Value() / 10;
  }

  if(values->Has(v8::String::New("audioMode"))) {
    usbControlPacket.audioMode = v8::Integer::Cast(*values->Get(v8::String::New("audioMode")))->Value();
  }
  if(values->Has(v8::String::New("audioPlayDuration"))) {
    usbControlPacket.audioPlayDuration = v8::Integer::Cast(*values->Get(v8::String::New("audioPlayDuration")))->Value() / 10;
  }

  if(values->Has(v8::String::New("manualLeds0"))) {
    usbControlPacket.manualLeds0 = v8::Integer::Cast(*values->Get(v8::String::New("manualLeds0")))->Value();
  }
  if(values->Has(v8::String::New("manualLeds1"))) {
    usbControlPacket.manualLeds1 = v8::Integer::Cast(*values->Get(v8::String::New("manualLeds1")))->Value();
  }
  if(values->Has(v8::String::New("manualLeds2"))) {
    usbControlPacket.manualLeds2 = v8::Integer::Cast(*values->Get(v8::String::New("manualLeds2")))->Value();
  }
  if(values->Has(v8::String::New("manualLeds3"))) {
    usbControlPacket.manualLeds3 = v8::Integer::Cast(*values->Get(v8::String::New("manualLeds3")))->Value();
  }
  if(values->Has(v8::String::New("manualLeds4"))) {
    usbControlPacket.manualLeds4 = v8::Integer::Cast(*values->Get(v8::String::New("manualLeds4")))->Value();
  }

  try {
    sosDevice->setOutputReport(USB_REPORTID_OUT_CONTROL, (char*)&usbControlPacket, sizeof(usbControlPacket));
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = v8::Undefined();
    callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
    return scope.Close(v8::Undefined());
  }

  callbackArgs[0] = v8::Undefined();
  callbackArgs[1] = v8::Undefined();
  callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
  return scope.Close(v8::Undefined());
}

v8::Persistent<v8::FunctionTemplate> SosDevice::s_ct;

/*static*/ void SosDevice::Init(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
  s_ct = v8::Persistent<v8::FunctionTemplate>::New(t);
  s_ct->InstanceTemplate()->SetInternalFieldCount(1);
  s_ct->SetClassName(v8::String::NewSymbol("SosDevice"));

  NODE_SET_PROTOTYPE_METHOD(t, "readInfo", readInfo);
  NODE_SET_PROTOTYPE_METHOD(t, "sendControlPacket", sendControlPacket);
  NODE_SET_PROTOTYPE_METHOD(t, "readLedPatterns", readLedPatterns);
  NODE_SET_PROTOTYPE_METHOD(t, "readAudioPatterns", readAudioPatterns);

  target->Set(v8::String::NewSymbol("SosDevice"), s_ct->GetFunction());
}

/*static*/ v8::Local<v8::Object> SosDevice::New(struct usb_device *dev, struct usb_dev_handle *devHandle) {
  v8::HandleScope scope;

  v8::Local<v8::Function> ctor = s_ct->GetFunction();
  v8::Local<v8::Object> obj = ctor->NewInstance();
  SosDevice *self = new SosDevice(dev, devHandle);
  self->Wrap(obj);

  return scope.Close(obj);
}

SosDevice::SosDevice(struct usb_device *dev, struct usb_dev_handle *devHandle) {
  this->dev = dev;
  this->devHandle = devHandle;
}

static struct usb_device *findSos() {
  struct usb_bus *bus;
  struct usb_device *dev;
  struct usb_bus *busses;

  usb_init();
  usb_find_busses();
  usb_find_devices();

  busses = usb_get_busses();

  for (bus = busses; bus; bus = bus->next){
    for (dev = bus->devices; dev; dev = dev->next) {
      if ((dev->descriptor.idVendor == sosVendorId) && (dev->descriptor.idProduct == sosProductId)) {
        return dev;
      }
    }
  }
  return NULL;
}

v8::Handle<v8::Value> findDevice(const v8::Arguments& args) {
  char errorBuffer[1000];
  v8::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];

  v8::Function *callback = v8::Function::Cast(*args[0]);

  struct usb_device *dev = findSos();
  if(dev == NULL) {
    callbackArgs[0] = v8::Exception::Error(v8::String::New("No Siren of Shame devices found"));
    callbackArgs[1] = v8::Undefined();
    callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
    return scope.Close(v8::Undefined());
  }

  usb_dev_handle *devHandle = usb_open(dev);
  if(devHandle == NULL) {
    callbackArgs[0] = v8::Exception::Error(v8::String::New("Could not open Siren of Shame"));
    callbackArgs[1] = v8::Undefined();
    callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
    return scope.Close(v8::Undefined());
  }

  int detachResult = usb_detach_kernel_driver_np(devHandle, INTERFACE_NUMBER);
  if(detachResult != 0 && detachResult != -61) {
    sprintf(errorBuffer, "usb_detach_kernel_driver_np: %d %s\n", detachResult, usb_strerror());
    callbackArgs[0] = v8::Exception::Error(v8::String::New(errorBuffer));
    callbackArgs[1] = v8::Undefined();
    callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
    return scope.Close(v8::Undefined());
  }

  v8::Local<v8::Object> sosDevice = SosDevice::New(dev, devHandle);
  callbackArgs[0] = v8::Undefined();
  callbackArgs[1] = sosDevice;
  callback->Call(v8::Context::GetCurrent()->Global(), 2, callbackArgs);
  return scope.Close(v8::Undefined());
}


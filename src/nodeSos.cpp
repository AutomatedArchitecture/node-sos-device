
#include "nodeSos.h"
#include "usbPackets.h"

#ifdef WIN32
  HidD_GetInputReportFn HidD_GetInputReport = NULL;
  HidD_SetOutputReportFn HidD_SetOutputReport = NULL;
  HidD_GetHidGuidFn HidD_GetHidGuid = NULL;
  SetupDiGetClassDevsFn sosSetupDiGetClassDevs = NULL;
  SetupDiDestroyDeviceInfoListFn sosSetupDiDestroyDeviceInfoList = NULL;
  SetupDiEnumDeviceInterfacesFn sosSetupDiEnumDeviceInterfaces = NULL;
  SetupDiGetDeviceInterfaceDetailFn sosSetupDiGetDeviceInterfaceDetail = NULL;

  void initFunctionPointers() {
    if(HidD_GetInputReport == NULL) {
      HMODULE hid = LoadLibrary("hid.dll");
      HidD_GetInputReport = (HidD_GetInputReportFn)GetProcAddress(hid, "HidD_GetInputReport");
      HidD_SetOutputReport = (HidD_SetOutputReportFn)GetProcAddress(hid, "HidD_SetOutputReport");
      HidD_GetHidGuid = (HidD_GetHidGuidFn)GetProcAddress(hid, "HidD_GetHidGuid");

      HMODULE setupapi = LoadLibrary("setupapi.dll");
      sosSetupDiGetClassDevs = (SetupDiGetClassDevsFn)GetProcAddress(setupapi, "SetupDiGetClassDevsA");
      sosSetupDiDestroyDeviceInfoList = (SetupDiDestroyDeviceInfoListFn)GetProcAddress(setupapi, "SetupDiDestroyDeviceInfoList");
      sosSetupDiEnumDeviceInterfaces = (SetupDiEnumDeviceInterfacesFn)GetProcAddress(setupapi, "SetupDiEnumDeviceInterfaces");
      sosSetupDiGetDeviceInterfaceDetail = (SetupDiGetDeviceInterfaceDetailFn)GetProcAddress(setupapi, "SetupDiGetDeviceInterfaceDetailA");
    }
  }
#endif

int sosVendorId = 5840;
int sosProductId = 1606;
int sosPacketSize = 1 + 37; // report id + packet length

#ifndef WIN32
// Values for bmRequestType in the Setup transaction's Data packet.
static const int CONTROL_REQUEST_TYPE_IN = USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
static const int CONTROL_REQUEST_TYPE_OUT = USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
#endif

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
    return Nan::Error(errorMessage);
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

  #ifdef WIN32
    buf[0] = reportId;
    if(!HidD_GetInputReport(devHandle, buf, sosPacketSize)) {
      sprintf(errorBuffer, "Could not get input report: 0x%08X", GetLastError());
      throw NodeSosException(errorBuffer);
    }
  #else
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
  #endif
}

void SosDevice::setOutputReport(int reportId, char* buf, int bufSize) {
  char errorBuffer[1000];

  #ifdef WIN32
    buf[0] = reportId;
    if(!HidD_SetOutputReport(devHandle, buf, sosPacketSize)) {
      sprintf(errorBuffer, "Could not set output report: 0x%08X", GetLastError());
      throw NodeSosException(errorBuffer);
    }
  #else
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
  #endif
}

NAN_METHOD(SosDevice::readInfo) {
  Nan::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = Nan::ObjectWrap::Unwrap<SosDevice>(info.This());

  Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());

  UsbInfoPacket usbInfoPacket;
  try {
    sosDevice->getInputReport(USB_REPORTID_IN_INFO, (char*)&usbInfoPacket, sizeof(usbInfoPacket));
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = Nan::Undefined();
    callback->Call(2, callbackArgs);
    return;
  }

  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  Nan::Set(result, Nan::New<v8::String>("version").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.version));
  Nan::Set(result, Nan::New<v8::String>("hardwareType").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.hardwareType));
  Nan::Set(result, Nan::New<v8::String>("hardwareVersion").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.hardwareVersion));
  Nan::Set(result, Nan::New<v8::String>("externalMemorySize").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.externalMemorySize));
  Nan::Set(result, Nan::New<v8::String>("audioMode").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.audioMode));
  Nan::Set(result, Nan::New<v8::String>("audioPlayDuration").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.audioPlayDuration));
  Nan::Set(result, Nan::New<v8::String>("ledMode").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.ledMode));
  Nan::Set(result, Nan::New<v8::String>("ledPlayDuration").ToLocalChecked(), Nan::New<v8::Integer>(usbInfoPacket.ledPlayDuration));

  callbackArgs[0] = Nan::Undefined();
  callbackArgs[1] = result;
  callback->Call(2, callbackArgs);
}

NAN_METHOD(SosDevice::readLedPatterns) {
  Nan::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = Nan::ObjectWrap::Unwrap<SosDevice>(info.This());

  Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());

  v8::Local<v8::Array> ledPatterns = Nan::New<v8::Array>();
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
      v8::Local<v8::Object> ledPattern = Nan::New<v8::Object>();
      Nan::Set(ledPattern, Nan::New<v8::String>("id").ToLocalChecked(), Nan::New<v8::Integer>(usbReadLedPacket.id));
      Nan::Set(ledPattern, Nan::New<v8::String>("name").ToLocalChecked(), Nan::New<v8::String>(usbReadLedPacket.name).ToLocalChecked());
      ledPatterns->Set(i, ledPattern);
    }
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = Nan::Undefined();
    callback->Call(2, callbackArgs);
    return;
  }

  callbackArgs[0] = Nan::Undefined();
  callbackArgs[1] = ledPatterns;
  callback->Call(2, callbackArgs);
  return;
}

NAN_METHOD(SosDevice::readAudioPatterns) {
  Nan::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = Nan::ObjectWrap::Unwrap<SosDevice>(info.This());

  Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());

  v8::Local<v8::Array> audioPatterns = Nan::New<v8::Array>();
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
      v8::Local<v8::Object> audioPattern = Nan::New<v8::Object>();
      Nan::Set(audioPattern, Nan::New<v8::String>("id").ToLocalChecked(), Nan::New<v8::Integer>(UsbReadAudioPacket.id));
      Nan::Set(audioPattern, Nan::New<v8::String>("name").ToLocalChecked(), Nan::New<v8::String>(UsbReadAudioPacket.name).ToLocalChecked());
      audioPatterns->Set(i, audioPattern);
    }
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = Nan::Undefined();
    callback->Call(2, callbackArgs);
    return;
  }

  callbackArgs[0] = Nan::Undefined();
  callbackArgs[1] = audioPatterns;
  callback->Call(2, callbackArgs);
  return;
}

NAN_METHOD(SosDevice::sendControlPacket) {
  Nan::HandleScope scope;
  v8::Handle<v8::Value> callbackArgs[2];
  SosDevice* sosDevice = Nan::ObjectWrap::Unwrap<SosDevice>(info.This());

  v8::Local<v8::Object> values = info[0].As<v8::Object>();
  Nan::Callback *callback = new Nan::Callback(info[1].As<v8::Function>());

  UsbControlPacket usbControlPacket;
  initControlPacket(&usbControlPacket);

  if(Nan::Has(values, Nan::New<v8::String>("ledMode").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.ledMode = Nan::Get(values, Nan::New<v8::String>("ledMode").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value();
  }
  if(Nan::Has(values, Nan::New<v8::String>("ledPlayDuration").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.ledPlayDuration = Nan::Get(values, Nan::New<v8::String>("ledPlayDuration").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value() / 10;
  }

  if(Nan::Has(values, Nan::New<v8::String>("audioMode").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.audioMode = Nan::Get(values, Nan::New<v8::String>("audioMode").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value();
  }
  if(Nan::Has(values, Nan::New<v8::String>("audioPlayDuration").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.audioPlayDuration = Nan::Get(values, Nan::New<v8::String>("audioPlayDuration").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value() / 10;
  }

  if(Nan::Has(values, Nan::New<v8::String>("manualLeds0").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.manualLeds0 = Nan::Get(values, Nan::New<v8::String>("manualLeds0").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value();
  }
  if(Nan::Has(values, Nan::New<v8::String>("manualLeds1").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.manualLeds1 = Nan::Get(values, Nan::New<v8::String>("manualLeds1").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value();
  }
  if(Nan::Has(values, Nan::New<v8::String>("manualLeds2").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.manualLeds2 = Nan::Get(values, Nan::New<v8::String>("manualLeds2").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value();
  }
  if(Nan::Has(values, Nan::New<v8::String>("manualLeds3").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.manualLeds3 = Nan::Get(values, Nan::New<v8::String>("manualLeds3").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value();
  }
  if(Nan::Has(values, Nan::New<v8::String>("manualLeds4").ToLocalChecked()).FromMaybe(false)) {
    usbControlPacket.manualLeds4 = Nan::Get(values, Nan::New<v8::String>("manualLeds4").ToLocalChecked()).ToLocalChecked().As<v8::Integer>()->Value();
  }

  try {
    sosDevice->setOutputReport(USB_REPORTID_OUT_CONTROL, (char*)&usbControlPacket, sizeof(usbControlPacket));
  } catch(NodeSosException &ex) {
    callbackArgs[0] = ex.toV8();
    callbackArgs[1] = Nan::Undefined();
    callback->Call(2, callbackArgs);
    return;
  }

  callbackArgs[0] = Nan::Undefined();
  callbackArgs[1] = Nan::Undefined();
  callback->Call(2, callbackArgs);
  return;
}

Nan::Persistent<v8::FunctionTemplate> SosDevice::s_ct;

/*static*/ void SosDevice::Init(v8::Handle<v8::Object> target) {
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = Nan::New<v8::FunctionTemplate>();
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(Nan::New("SosDevice").ToLocalChecked());
  s_ct.Reset(t);

  Nan::SetPrototypeMethod(t, "readInfo", SosDevice::readInfo);
  Nan::SetPrototypeMethod(t, "sendControlPacket", SosDevice::sendControlPacket);
  Nan::SetPrototypeMethod(t, "readLedPatterns", SosDevice::readLedPatterns);
  Nan::SetPrototypeMethod(t, "readAudioPatterns", SosDevice::readAudioPatterns);

  Nan::Set(target, Nan::New("SosDevice").ToLocalChecked(), Nan::New(s_ct)->GetFunction());
}

#ifdef WIN32
  /*static*/ v8::Local<v8::Object> SosDevice::New(HANDLE devHandle) {
    Nan::HandleScope scope;

    initFunctionPointers();

    v8::Local<v8::Function> ctor = s_ct->GetFunction();
    v8::Local<v8::Object> obj = ctor->NewInstance();
    SosDevice *self = new SosDevice(devHandle);
    self->Wrap(obj);

    return scope.Close(obj);
  }

  SosDevice::SosDevice(HANDLE devHandle) {
    initFunctionPointers();
    this->devHandle = devHandle;
  }
#else
  /*static*/ v8::Local<v8::Object> SosDevice::New(struct usb_device *dev, struct usb_dev_handle *devHandle) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> ctor = Nan::New(s_ct)->GetFunction();
    v8::Local<v8::Object> obj = ctor->NewInstance();
    SosDevice *self = new SosDevice(dev, devHandle);
    self->Wrap(obj);

    return scope.Escape(obj);
  }

  SosDevice::SosDevice(struct usb_device *dev, struct usb_dev_handle *devHandle) {
    this->dev = dev;
    this->devHandle = devHandle;
  }
#endif

#ifdef WIN32
  HANDLE findSos() {
    GUID hidGuid;
    HANDLE devHandle = NULL;
    char sosVendorIdStr[10];
    char sosProductIdStr[10];

    sprintf(sosVendorIdStr, "%x", sosVendorId);
    sprintf(sosProductIdStr, "%x", sosProductId);

    HidD_GetHidGuid(&hidGuid);

    HANDLE deviceInfoSet = sosSetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(deviceInfoSet == INVALID_HANDLE_VALUE) {
      return INVALID_HANDLE_VALUE;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA *deviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)malloc(sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 1000);
    for(int i=0; ; i++) {
      deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
      if(!sosSetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, i, &deviceInterfaceData)) {
        goto deviceNotFound;
      }
      DWORD requiredSize;

      deviceInterfaceDetailData->cbSize = 8;

      SP_DEVINFO_DATA deviceInfoData;
      deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

      sosSetupDiGetDeviceInterfaceDetail(
        deviceInfoSet,
        &deviceInterfaceData,
        deviceInterfaceDetailData,
        1000,
        &requiredSize,
        &deviceInfoData);

      if(strstr(deviceInterfaceDetailData->DevicePath, sosVendorIdStr)
        && strstr(deviceInterfaceDetailData->DevicePath, sosProductIdStr)) {
        break;
      }
    }

    devHandle = CreateFile(
      deviceInterfaceDetailData->DevicePath,
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_OVERLAPPED,
      NULL);

deviceNotFound:
    free(deviceInterfaceDetailData);
    sosSetupDiDestroyDeviceInfoList(deviceInfoSet);

    return devHandle;
  }

  NAN_METHOD(findDevice) {
    char errorBuffer[1000];
    Nan::HandleScope scope;
    v8::Handle<v8::Value> callbackArgs[2];

    initFunctionPointers();

    Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());

    HANDLE devHandle = findSos();
    if(devHandle == NULL || devHandle == INVALID_HANDLE_VALUE) {
      callbackArgs[0] = v8::Exception::Error(Nan::New<v8::String>("No Siren of Shame devices found"));
      callbackArgs[1] = Nan::Undefined();
      callback->Call(2, callbackArgs);
      return;
    }

    v8::Local<v8::Object> sosDevice = SosDevice::New(devHandle);
    callbackArgs[0] = Nan::Undefined();
    callbackArgs[1] = sosDevice;
    callback->Call(2, callbackArgs);
    return;
  }
#else
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

  NAN_METHOD(findDevice) {
    char errorBuffer[1000];
    Nan::HandleScope scope;
    v8::Local<v8::Value> callbackArgs[2];

    Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());

    struct usb_device *dev = findSos();
    if(dev == NULL) {
      callbackArgs[0] = Nan::Error("No Siren of Shame devices found");
      callbackArgs[1] = Nan::Undefined();
      callback->Call(2, callbackArgs);
      return;
    }

    usb_dev_handle *devHandle = usb_open(dev);
    if(devHandle == NULL) {
      callbackArgs[0] = Nan::Error("Could not open Siren of Shame");
      callbackArgs[1] = Nan::Undefined();
      callback->Call(2, callbackArgs);
      return;
    }

    int detachResult = usb_detach_kernel_driver_np(devHandle, INTERFACE_NUMBER);
    if(detachResult != 0 && detachResult != -61) {
      sprintf(errorBuffer, "usb_detach_kernel_driver_np: %d %s\n", detachResult, usb_strerror());
      callbackArgs[0] = Nan::Error(errorBuffer);
      callbackArgs[1] = Nan::Undefined();
      callback->Call(2, callbackArgs);
      return;
    }

    v8::Local<v8::Object> sosDevice = SosDevice::New(dev, devHandle);
    callbackArgs[0] = Nan::Undefined();
    callbackArgs[1] = sosDevice;
    callback->Call(2, callbackArgs);
    return;
  }
#endif

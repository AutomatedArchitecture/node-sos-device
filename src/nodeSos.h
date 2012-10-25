
#include <node.h>
#include <v8.h>
#include <node_buffer.h>
#include <usb.h>
#include <stdio.h>
#include <node.h>

v8::Handle<v8::Value> findDevice(const v8::Arguments& args);

class SosDevice : public node::ObjectWrap {
  struct usb_device *dev;
  struct usb_dev_handle *devHandle;
  static v8::Persistent<v8::FunctionTemplate> s_ct;
  static v8::Handle<v8::Value> readInfo(const v8::Arguments& args);
  static v8::Handle<v8::Value> readLedPatterns(const v8::Arguments& args);
  static v8::Handle<v8::Value> readAudioPatterns(const v8::Arguments& args);
  static v8::Handle<v8::Value> sendControlPacket(const v8::Arguments& args);

public:
  static void Init(v8::Handle<v8::Object> target);
  static v8::Local<v8::Object> New(struct usb_device *dev, struct usb_dev_handle *devHandle);

  SosDevice(struct usb_device *dev, struct usb_dev_handle *devHandle);

private:
  void getInputReport(int reportId, char* buf, int bufSize);
  void setOutputReport(int reportId, char* buf, int bufSize);
};



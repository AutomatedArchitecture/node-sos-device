
#include "nodeSos.h"

extern "C" {
  void init (v8::Handle<v8::Object> target)
  {
    v8::HandleScope scope;
    NODE_SET_METHOD(target, "findDevice", findDevice);
    SosDevice::Init(target);
  }
}

NODE_MODULE(sos, init);
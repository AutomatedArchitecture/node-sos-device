
#include "nodeSos.h"

extern "C" {
  void init (v8::Handle<v8::Object> target)
  {
    Nan::SetMethod(target, "findDevice", findDevice);
    SosDevice::Init(target);
  }
}

NODE_MODULE(sos, init);

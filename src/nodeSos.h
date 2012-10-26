
#include <node.h>
#include <v8.h>
#include <node_buffer.h>
#ifdef WIN32
  #include <Setupapi.h>
#else
  #include <usb.h>
#endif
#include <stdio.h>
#include <node.h>

v8::Handle<v8::Value> findDevice(const v8::Arguments& args);

class SosDevice : public node::ObjectWrap {
  #ifdef WIN32
    HANDLE devHandle;
  #else
    struct usb_device *dev;
    struct usb_dev_handle *devHandle;
  #endif
  static v8::Persistent<v8::FunctionTemplate> s_ct;
  static v8::Handle<v8::Value> readInfo(const v8::Arguments& args);
  static v8::Handle<v8::Value> readLedPatterns(const v8::Arguments& args);
  static v8::Handle<v8::Value> readAudioPatterns(const v8::Arguments& args);
  static v8::Handle<v8::Value> sendControlPacket(const v8::Arguments& args);

public:
  static void Init(v8::Handle<v8::Object> target);

  #ifdef WIN32
    static v8::Local<v8::Object> New(HANDLE devHandle);
    SosDevice(HANDLE devHandle);
  #else
    static v8::Local<v8::Object> New(struct usb_device *dev, struct usb_dev_handle *devHandle);
    SosDevice(struct usb_device *dev, struct usb_dev_handle *devHandle);
  #endif

private:
  void getInputReport(int reportId, char* buf, int bufSize);
  void setOutputReport(int reportId, char* buf, int bufSize);
};

#ifdef WIN32
  typedef BOOLEAN (__stdcall *HidD_GetInputReportFn)(
    _In_   HANDLE HidDeviceObject,
    _Out_  PVOID ReportBuffer,
    _In_   ULONG ReportBufferLength
  );

  typedef BOOLEAN (__stdcall *HidD_SetOutputReportFn)(
    _In_  HANDLE HidDeviceObject,
    _In_  PVOID ReportBuffer,
    _In_  ULONG ReportBufferLength
  );

  typedef void (__stdcall *HidD_GetHidGuidFn)(
    _Out_  LPGUID HidGuid
  );

  typedef HDEVINFO (*SetupDiGetClassDevsFn)(
    _In_opt_  const GUID *ClassGuid,
    _In_opt_  PCTSTR Enumerator,
    _In_opt_  HWND hwndParent,
    _In_      DWORD Flags
  );

  typedef BOOL (*SetupDiDestroyDeviceInfoListFn)(
    _In_  HDEVINFO DeviceInfoSet
  );

  typedef BOOL (*SetupDiEnumDeviceInterfacesFn)(
    _In_      HDEVINFO DeviceInfoSet,
    _In_opt_  PSP_DEVINFO_DATA DeviceInfoData,
    _In_      const GUID *InterfaceClassGuid,
    _In_      DWORD MemberIndex,
    _Out_     PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
  );

  typedef BOOL (*SetupDiGetDeviceInterfaceDetailFn)(
    _In_       HDEVINFO DeviceInfoSet,
    _In_       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _Out_opt_  PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData,
    _In_       DWORD DeviceInterfaceDetailDataSize,
    _Out_opt_  PDWORD RequiredSize,
    _Out_opt_  PSP_DEVINFO_DATA DeviceInfoData
  );
#endif
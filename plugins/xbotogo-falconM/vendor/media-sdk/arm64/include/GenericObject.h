#pragma once
#include <memory>

namespace blink {
namespace utils {

enum GenericObjectType {
  GENERIC_OBJECT_TYPE_UNKNOWN = 0,
  GENERIC_OBJECT_TYPE_JOBJECT,
  GENERIC_OBJECT_TYPE_NSOBJECT,
  GENERIC_OBJECT_TYPE_CFTYPE,
  GENERIC_OBJECT_TYPE_CPPOBJECT
};

class GenericObject {
public:
  static std::shared_ptr<GenericObject> create(void* obj, GenericObjectType type);

  virtual ~GenericObject() {}
  // 1. for java object, return jobject
  // 2. for NSObject, return void*, if object is intent to use in OC, 
  //  use __bridge_retained to convert it to NSObject*, then 
  //  object life is managed by Objective-C runtime. 
  //  example:
  //    NSObject* obj = (__bridge_retained NSObject*)genericObj.getObject();
  //    // do something with obj;
  // 3. for CFTypeRef, return CFTypeRef, if object is intent to be 
  //  used in OC, use CFRetain and CFRelease to manage the object life.
  //  example:
  //    CFTypeRef obj = genericObj.getObject();
  //    CFRetain(obj);
  //    // do something with obj;
  //    CFRelease(obj);
  virtual void *getObject() = 0;
  // 1. for jobject, setObject((void*)obj)
  // 2. for NSObject, setObject((__bridge void*)obj)
  // 3. for CFTypeRef, setObject((void*)obj)
  virtual void setObject(void *obj) = 0;
};

}
}
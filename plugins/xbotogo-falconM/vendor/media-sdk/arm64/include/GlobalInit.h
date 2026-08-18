#pragma once
#ifdef WEBRTC_ANDROID
#include "JavaNativeConverter.h"
#endif


namespace blink {
namespace utils {

struct GlobalConfig {
  void *jvm;
  GlobalConfig() : jvm(nullptr) {}
};

#ifdef WEBRTC_ANDROID
extern spotify::jni::ClassRegistry gJniClasses;
extern JavaVM *gJvm;
extern media::JavaNativeConverterRegistry gJavaNativeConverter;
#endif

class GlobalInit {
public:
    static GlobalInit& getInstance();

    int init(const GlobalConfig &cfg);
    int setLogCallback(void* jobj);
    int initSystemUtils(void* ctx);
private:
    GlobalInit();
    ~GlobalInit();

    bool m_initialized;
};

}
}
#pragma once
#include "DataTypes.h"

namespace blink {
namespace utils {

class MediaCodecUtils {
public:
    // for android: must be called once (e.g. during JNI_OnLoad) so the jclass/jmethodID
    // can be cached ahead of time. FindClass called later from a native thread that wasn't
    // attached via a Java call stack uses the bootstrap classloader and won't find app classes.
    static void init();

    // Queries whether the platform has a hardware codec for the given format. New formats
    // are supported by extending the format-to-codec mapping in each platform's implementation,
    // not by changing this signature.
    static bool isHardwareCodecSupported(media::MEDIA_DATA_FORMAT format, bool isEncoder);
};

}
}

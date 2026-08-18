#pragma once
#include <string>

namespace blink {
namespace utils {

class StorageUtils {
public:
    // for android
    static void init(void *ctx);
    static std::string getAppExternalFilesDir();
};

}
}

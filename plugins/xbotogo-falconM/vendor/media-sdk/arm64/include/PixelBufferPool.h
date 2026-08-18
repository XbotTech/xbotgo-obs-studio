#pragma once
#include "GenericObject.h"

namespace blink {
namespace utils {

class PixelBufferPool {
public:
    PixelBufferPool(int width, int height, int format);
    ~PixelBufferPool();
    std::shared_ptr<GenericObject> make_shared();
private:
    std::shared_ptr<GenericObject> m_pixelBufferPool;
};

std::shared_ptr<GenericObject> copyPixelBuffer(
    std::shared_ptr<GenericObject> srcPixelBuffer,
    std::shared_ptr<PixelBufferPool> pixelBufferPool);

}
}

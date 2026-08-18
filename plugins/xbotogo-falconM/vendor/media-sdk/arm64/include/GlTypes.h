#pragma once
#include <cstdint>
#include <memory>
#include "GenericObject.h"


namespace blink {
namespace media {

// OES texture together with its backing SurfaceTexture and Surface.
// oesTextureId    – GL_TEXTURE_EXTERNAL_OES texture name, valid on the GL thread that created it.
// surfaceTexture  – android.graphics.SurfaceTexture (wrapped as GENERIC_OBJECT_TYPE_JOBJECT).
// surface         – android.view.Surface built from surfaceTexture (wrapped as GENERIC_OBJECT_TYPE_JOBJECT).
struct OesSurfaceBundle {
    uint32_t oesTextureId = 0;
    std::shared_ptr<utils::GenericObject> surfaceTexture;
    std::shared_ptr<utils::GenericObject> surface;
};

// Platform-agnostic OpenGL ES context handle.
// On Android wraps an EGLDisplay + EGLContext pair (stored as void*).
// On iOS wraps an EAGLContext* (stored as void*).
struct GlContext {
#ifdef __ANDROID__
    void* eglDisplay = nullptr;  // EGLDisplay
    void* eglContext = nullptr;  // EGLContext
#elif defined(__APPLE__)
    void* eaglContext = nullptr; // EAGLContext*
#endif

    bool isValid() const {
#ifdef __ANDROID__
        return eglDisplay != nullptr && eglContext != nullptr;
#elif defined(__APPLE__)
        return eaglContext != nullptr;
#else
        return false;
#endif
    }
};

} // namespace media
} // namespace blink

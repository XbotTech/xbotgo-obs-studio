#pragma once
#include <stdint.h>

namespace blink {
namespace media {

class TextureBuffer {
public:
    // Default constructor, create an empty TextureBuffer with width and height set to 0, and textureId set to 0.
    TextureBuffer();
    // Constructor to create a TextureBuffer with specified width and height. The textureId will be generated automatically.
    TextureBuffer(int width, int height);
    // Wrap a pre-existing, externally-owned GL texture id (e.g. a camera/decoder OES
    // SurfaceTexture, or a texture whose lifetime another object already manages).
    // The destructor will NOT delete textureId — the original owner remains responsible for it.
    TextureBuffer(uint32_t textureId, int width, int height);
    // When copyTexture is false, the copy aliases `other`'s GL texture without owning it
    // (the alias will not delete it on destruction). When true, a new GL texture of the
    // same size is allocated and `other`'s pixel content is copied into it.
    TextureBuffer(const TextureBuffer& other, bool copyTexture = false);
    ~TextureBuffer();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    uint32_t getTextureId() const { return m_textureId; }

    // Attaches a GPU fence describing the most recent draw into this texture, so a
    // consumer on another thread/context (within the same EGL/GL share group) can wait on
    // it before sampling the texture. Deletes any previously set sync. Ownership of `sync`
    // transfers to this TextureBuffer: it is deleted on release()/destruction or when
    // replaced by a later setSync() call.
    // `sync` is the platform GLsync handle (GLsync on Android/iOS) passed as void* so this
    // public header doesn't need to pull in platform GL headers; callers cast it back to
    // GLsync before passing it to real GL calls.
    void setSync(void* sync);
    // Returns the sync set via setSync(), or nullptr if none is pending. Still owned by
    // this TextureBuffer — callers must not delete it themselves.
    void* getSync() const { return m_sync; }

    void setSize(int width, int height);
    void assign(const TextureBuffer& other, bool copyTexture = false);
private:
    void release();

    int m_width;
    int m_height;
    uint32_t m_textureId;
    void* m_sync;
    // Whether this instance created (and thus must delete) m_textureId.
    // False for constructor-wrapped external textures and for shallow (copyTexture=false)
    // aliases, which must never delete GL objects they don't own.
    bool m_owned;
};

}
}

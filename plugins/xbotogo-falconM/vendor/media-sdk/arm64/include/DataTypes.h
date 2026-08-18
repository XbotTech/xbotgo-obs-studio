#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <shared_mutex>
#include "GenericObject.h"
#include "worker_thread.h"
#include "TextureBuffer.h"

namespace blink {
namespace media {

class ByteArrayBuffer {
public:
  ByteArrayBuffer() : m_isReadOnly(false),
    m_data(NULL), m_size(0) {
    // m_type = DATA_BUFFER_BYTE_ARRAY;
  }
  // allocate buffer of size bytes
  ByteArrayBuffer(int32_t size) : m_isReadOnly(false), m_buffer(size) {
    m_data = (uint8_t *)m_buffer.data();
    m_size = size;
  }
  ByteArrayBuffer(uint8_t *buf, int32_t len, bool copyData) :
    m_isReadOnly(false), m_data(NULL), m_size(0) {
    // m_type = DATA_BUFFER_BYTE_ARRAY;
    setData(buf, len, copyData);
  }
  ByteArrayBuffer(std::vector<uint8_t> &buf, bool copyData) :
    m_isReadOnly(false), m_data(NULL), m_size(0) {
    // m_type = DATA_BUFFER_BYTE_ARRAY;
    setData(buf, copyData);
  }
  void setReadOnly(bool readonly) { m_isReadOnly = readonly; }
  bool isReadOnly() { return m_isReadOnly; }
  int setData(uint8_t *buf, int32_t len, bool copyData) {
    if (m_isReadOnly) {
      return -1;
    }
    if (copyData) {
      if (m_buffer.size() < len) {
        m_buffer.resize(len);
      }
      memcpy(m_buffer.data(), buf, len);
      m_data = (uint8_t *)m_buffer.data();
      m_size = len;
    } else {
      m_data = buf;
      m_size = len;
    }
    return 0;
  }
  // notice that if copyData is false, the input buffer will changed.
  int setData(std::vector<uint8_t> &buf, bool copyData) {
    if (m_isReadOnly) {
      return -1;
    }
    if (copyData) {
      m_buffer = buf;
    } else {
      m_buffer.swap(buf);
    }
    m_data = (uint8_t*)m_buffer.data();
    m_size = m_buffer.size();
    return 0;
  }
  int setSize(int32_t size) {
    if (m_isReadOnly) {
      return -1;
    }
    m_buffer.resize(size);
    m_data = (uint8_t *)m_buffer.data();
    m_size = size;
    return 0;
  }
  uint8_t *getData() { return m_data; }
  int32_t getSize() { return m_size; }
private:
  std::vector<uint8_t> m_buffer;
  uint8_t *m_data;
  int32_t m_size;
  bool m_isReadOnly;
};

enum MEDIA_DATA_TYPE {
  MEDIA_DATA_TYPE_UNKNOWN = -1,
  MEDIA_DATA_TYPE_VIDEO,
  MEDIA_DATA_TYPE_AUDIO,
  MEDIA_DATA_TYPE_DATA,
  MEDIA_DATA_TYPE_SUBTITLE,
  MEDIA_DATA_TYPE_ATTACHMENT,
  MEDIA_DATA_TYPE_NUM,
};

enum MEDIA_DATA_FORMAT {
  MEDIA_DATA_FORMAT_UNKNOWN = -1,
  MEDIA_DATA_FORMAT_YUV420P = 0,
  MEDIA_DATA_FORMAT_NV21 = 1,
  MEDIA_DATA_FORMAT_NV12 = 2,
  MEDIA_DATA_FORMAT_RGB24 = 3,
  MEDIA_DATA_FORMAT_RGBA = 4,  // corresponding to libyuv ABGR, for android rendering with Bitmap.Config.ARGB_8888
  MEDIA_DATA_FORMAT_SURFACE = 5,
  // ios use CVImageBufferRef as video input
  MEDIA_DATA_FORMAT_IMAGE_BUFFER = 6,
  MEDIA_DATA_FORMAT_UYVY = 7, // NDI recv UYVY format
  MEDIA_DATA_FORMAT_BGRA = 8, // corresponding to libyuv ARGB
  MEDIA_DATA_FORMAT_OES_TEXTURE = 70, // corresponding to OesSurfaceBundle
  MEDIA_DATA_FORMAT_TEXTURE = 71,
  MEDIA_DATA_FORMAT_PCM_S16 = 80,
  MEDIA_DATA_FORMAT_H264 = 100,
  MEDIA_DATA_FORMAT_H265 = 101,
  MEDIA_DATA_FORMAT_AAC = 110,
  MEDIA_DATA_FORMAT_PROTOBUF = 150,
  MEDIA_DATA_FORMAT_DETECT_RESULT = 151,
};

struct MediaData {
  MEDIA_DATA_TYPE type;
  MEDIA_DATA_FORMAT format;
  int32_t fromNodeId;
  int32_t toNodeId;
  // dts and pts in microsecond
  int64_t dts;
  int64_t pts;
  std::shared_ptr<ByteArrayBuffer> buffer;
  // when format is MEDIA_DATA_FORMAT_IMAGE_BUFFER,
  // dataObj is a CVImageBufferRef
  // when format is MEDIA_DATA_FORMAT_SURFACE,
  // dataObj is a SurfaceTexture
  std::shared_ptr<utils::GenericObject> bufferObj;
  // extra data, e.g., codec config, etc.
  std::shared_ptr<ByteArrayBuffer> extraData;
  // for video frame in OES_TEXTURE/TEXTURE format, used by render node. Lifetime of the
  // underlying GL texture is owned by whoever produced it (a camera/decoder's external
  // SurfaceTexture, or a texture pool) — see TextureBuffer's ownership semantics.
  std::shared_ptr<TextureBuffer> textureBuffer;
  // Clockwise degrees (0/90/180/270) the frame's content needs to be rotated to appear
  // upright, e.g. camera sensor mounting orientation. 0 if not applicable/unknown.
  int32_t rotation;

  // for video
  int width;
  int height;

  // for audio
  int sampleRate;
  int channels;
  int bitsPerSample;

  std::shared_ptr<std::shared_mutex> mutex;

  MediaData() :
    type(MEDIA_DATA_TYPE_UNKNOWN), format(MEDIA_DATA_FORMAT_UNKNOWN),
    fromNodeId(-1), toNodeId(-1), dts(-1),
    pts(-1), buffer(nullptr), bufferObj(nullptr), textureBuffer(nullptr), rotation(0), width(0), height(0),
    sampleRate(0), channels(0), bitsPerSample(0) {}

  MediaData(const MediaData &data) :
    type(data.type), format(data.format),
    fromNodeId(data.fromNodeId), toNodeId(data.toNodeId), dts(data.dts),
    pts(data.pts), buffer(data.buffer), bufferObj(data.bufferObj),
    textureBuffer(data.textureBuffer), rotation(data.rotation), width(data.width), height(data.height),
    sampleRate(data.sampleRate), channels(data.channels),
    bitsPerSample(data.bitsPerSample), mutex(data.mutex) {}
};

struct GenericData {
  virtual ~GenericData() {}
};

struct MediaNodeMessage {
  std::string fromNode;
  std::string toNode;
  int32_t msgId;
  std::string msgName;
  // message types see NodeMessages.h
  std::shared_ptr<GenericData> msgData;
  MediaNodeMessage() : msgId(-1) {}
};

class MessageListener {
public:
    virtual ~MessageListener() {}
    virtual int onMessage(std::shared_ptr<MediaNodeMessage> msg) = 0;
};

struct MediaNodeConfig {
  std::string nodeType;
  int32_t nodeId;
  std::string nodeName;
  MessageListener *pipeline;
  std::shared_ptr<GenericData> nodeConfig;
  MediaNodeConfig() : nodeId(-1), pipeline(NULL), nodeConfig(NULL) {}
};


}
}

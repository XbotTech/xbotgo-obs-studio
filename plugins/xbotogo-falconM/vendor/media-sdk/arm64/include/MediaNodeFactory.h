#pragma once

#include "MediaNodeBase.h"
#include "worker_thread.h"
#include <string>
#include <map>

namespace blink {
namespace media {

class MediaNodeCreater {
public:
  virtual ~MediaNodeCreater() {}
  virtual MediaNodeBase *create() = 0;
};

template <typename T>
class GenericMediaNodeCreater : public MediaNodeCreater {
public:
  virtual MediaNodeBase *create() {
    T *node = new T();
    return node;
  }
};

class MediaNodeFactory {
public:
  static MediaNodeFactory* getInstance();
  void registerMediaNodeType(const std::string &nodeType, MediaNodeCreater* creater);
  MediaNodeBase* createMediaNode(const std::string &nodeType);
  MediaNodeBase* createAsyncMediaNode(const std::string &nodeType, std::shared_ptr<worker_thread> worker);
private:
  std::map<std::string, MediaNodeCreater*> m_nodeCreaters;
};

}
}

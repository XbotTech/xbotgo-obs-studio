#pragma once
#include "GenericObject.h"
#include <memory>

namespace blink {
namespace utils {

template <typename T>
class GenericCppObject : public GenericObject {
public:
    GenericCppObject(T* obj) : m_obj(obj) {}
    virtual ~GenericCppObject() {
        m_obj.reset();
    }
    virtual void *getObject() override {
        return m_obj.get();
    }
    virtual void setObject(void *obj) override {
      m_obj.reset(static_cast<T*>(obj));
    }
private:
    std::unique_ptr<T> m_obj;
};

}
}


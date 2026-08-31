#pragma once

#include <cstdint>
#include <memory>
#include <deque>
#include <stack>
#include <mutex>
#include <atomic>
#include "DataTypes.h"

namespace blink {
namespace utils {

class ByteArrayBufferPoolImpl {
private:
    typedef media::ByteArrayBuffer StorageType;
    std::mutex mutex_;
    std::stack<StorageType*> free_objects_;
    //NOTICE: must not use vector, because vector resize may invalidate pointers
    std::deque<std::unique_ptr<StorageType[]>> allocated_blocks_;
    size_t block_size_;
    size_t current_index_;
    std::atomic<int> outstanding_buffers_;  // Track number of buffers in use

    void allocate_block();

public:
    explicit ByteArrayBufferPoolImpl(size_t block_size = 16);
    ~ByteArrayBufferPoolImpl();

    // 获取对象
    media::ByteArrayBuffer* acquire(int32_t bufferSize);

    // 回收对象
    void release(media::ByteArrayBuffer* obj);

    // 获取当前在外的buffer数量
    int get_outstanding_count() const;

    // 禁止拷贝
    ByteArrayBufferPoolImpl(const ByteArrayBufferPoolImpl&) = delete;
    ByteArrayBufferPoolImpl& operator=(const ByteArrayBufferPoolImpl&) = delete;
};

class ByteArrayBufferPool {
private:
    std::shared_ptr<ByteArrayBufferPoolImpl> pool_impl_;

public:
    explicit ByteArrayBufferPool(size_t block_size = 16);

    // 创建对象并返回shared_ptr
    std::shared_ptr<media::ByteArrayBuffer> make_shared(int32_t bufferSize);

    // 获取当前活跃的buffer数量（用于调试/监控）
    int get_outstanding_count() const;
};

}
}

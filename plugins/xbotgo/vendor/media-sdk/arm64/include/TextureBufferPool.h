#pragma once

#include <cstdint>
#include <memory>
#include <deque>
#include <stack>
#include <mutex>
#include <atomic>
#include "TextureBuffer.h"

namespace blink {
namespace media {

class TextureBufferPoolImpl {
private:
    typedef TextureBuffer StorageType;
    std::mutex mutex_;
    std::stack<StorageType*> free_objects_;
    //NOTICE: must not use vector, because vector resize may invalidate pointers
    std::deque<std::unique_ptr<StorageType[]>> allocated_blocks_;
    size_t block_size_;
    size_t current_index_;
    std::atomic<int> outstanding_buffers_;  // Track number of buffers in use

    void allocate_block();

public:
    explicit TextureBufferPoolImpl(size_t block_size = 16);
    ~TextureBufferPoolImpl();

    // 获取对象
    TextureBuffer* acquire(int width, int height);

    // 回收对象
    void release(TextureBuffer* obj);

    // 获取当前在外的buffer数量
    int get_outstanding_count() const;

    // 禁止拷贝
    TextureBufferPoolImpl(const TextureBufferPoolImpl&) = delete;
    TextureBufferPoolImpl& operator=(const TextureBufferPoolImpl&) = delete;
};

class TextureBufferPool {
private:
    std::shared_ptr<TextureBufferPoolImpl> pool_impl_;

public:
    explicit TextureBufferPool(size_t block_size = 4);

    // 创建对象并返回shared_ptr
    std::shared_ptr<TextureBuffer> make_shared(int width, int height);

    // 获取当前活跃的buffer数量（用于调试/监控）
    int get_outstanding_count() const;
};

}
}

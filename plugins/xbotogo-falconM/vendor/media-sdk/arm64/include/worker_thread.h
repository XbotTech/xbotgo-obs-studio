#pragma once
#include <functional>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include "utils.h"

namespace blink {
  namespace media {

    class worker_thread {
    public:
      typedef std::function<void(void)> task_type;
      typedef std::function<int(void)> sync_task_type;
      struct task_info {
        int32_t id;
        std::string name;
        task_type task;
        task_info() : id(-1), task(nullptr) {}
        task_info(int32_t tid, const std::string &n, task_type t) : id(tid), name(n), task(t) {}
      };

      worker_thread(const std::string &thrd_name, bool attach_to_java_thread = false, bool debug = false);
      ~worker_thread();

      // append task to the end of the queue
      int32_t post(task_type task, const std::string &name = "");
      // insert task to the front of the queue
      int32_t invoke(task_type task, const std::string &name = "");
      // append task to the end of the queue and do not wait for it to finish
      int32_t append(task_type task, const std::string &name = "");
      // append task to the end of the queue and wait for it to finish
      int32_t call(worker_thread::sync_task_type task, const std::string &name = "");
      // wait for all tasks to finish
      int32_t wait(int64_t timeout = -1, const std::string &name = "");
      // cancel all tasks in the queue
      int32_t cancel();

    private:
      // int32_t list_tasks();

      std::unique_ptr<std::thread> thread_;
      std::list<task_info> task_queue_;
      std::mutex mtx_;
      std::condition_variable cv_;
      bool stop_;
      std::string thread_name_;
      int32_t task_id_counter_;
      bool debug_;
    };

#define gen_task_name \
    (std::string(utils::basename(__FILE__)) + ":" + std::to_string(__LINE__))

  }
}


#pragma once

#include <string>
#include <vector>

namespace blink {
namespace utils {

  enum BLINK_FILE_TYPE {
    BLINK_FILE_TYPE_NONE,
    BLINK_FILE_TYPE_REGULAR,
    BLINK_FILE_TYPE_DIR
  };

  struct file_state_t {
    BLINK_FILE_TYPE file_type;
    std::string file_path;
    int32_t file_size;
    int64_t last_modify_time;

    file_state_t() : file_type(BLINK_FILE_TYPE_NONE), file_size(0), last_modify_time(0) {}
  };

const char* basename(const char* path);
int32_t write_file(const std::string& file_path, const std::string &data);
int32_t read_file(const std::string& file_path, std::string& data);
int32_t remove_file(const std::string& file_path);
bool dir_exists(const std::string& path);
bool file_exists(const std::string& path);
int32_t file_state(const std::string& path, file_state_t& state);
// eg: /path/to/dir
int32_t create_dir(const std::string& dir);
int32_t create_dir_tree(const std::string& full_path);
//NOTICE: path must be passed as value
int32_t read_dir(std::string path, std::vector<std::string>& files);

std::string gen_random_string(int length);

int64_t now_ms();
int64_t now_mono_ms();

}
}

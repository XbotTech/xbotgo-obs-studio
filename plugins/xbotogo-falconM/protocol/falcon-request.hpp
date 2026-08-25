#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace xbotgo {

enum class falconm_direction : uint8_t { up = 0, down = 1, left = 2, right = 3, center = 4 };
enum class falconm_operation : uint8_t { short_press = 0, long_press = 1, release = 2 };
enum class falconm_zoom_type : uint8_t { relative = 0, absolute = 1 };

class FalconRequest {
public:
	virtual ~FalconRequest() = default;
	virtual std::string_view topic() const = 0;
	virtual std::vector<uint8_t> encodePayload() const = 0;
};

} // namespace xbotgo

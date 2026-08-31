#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace xbotgo {

class FalconEvent {
public:
	virtual ~FalconEvent() = default;
	virtual std::string_view topic() const = 0;
	virtual bool parse(const uint8_t *payload, size_t size) = 0;
};

using falcon_event_factory = std::unique_ptr<FalconEvent> (*)();

} // namespace xbotgo

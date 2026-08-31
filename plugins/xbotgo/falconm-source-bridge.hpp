#pragma once

#include "falconm-stream.hpp"

#include <callback/signal.h>
#include <obs.hpp>

#include <string>

namespace xbotgo {

class FalconRequest;

class FalconMSourceBridge final {
public:
	explicit FalconMSourceBridge(obs_source_t *source);

	OBSSource lock() const;
	std::string uuid() const;
	bool valid() const;
	bool connected() const;
	bool send(const FalconRequest &request) const;
	falconm_device_state state() const;
	bool connectMotorAngleReport(signal_callback_t callback, void *context) const;
	void disconnectMotorAngleReport(signal_callback_t callback, void *context) const;

	static bool IsFalconM(obs_source_t *source);

private:
	OBSWeakSource source_;
	std::string uuid_;
};

} // namespace xbotgo

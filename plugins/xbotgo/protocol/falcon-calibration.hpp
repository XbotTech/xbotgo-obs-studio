#pragma once

#include "falcon-event.hpp"
#include "falcon-request.hpp"

namespace xbotgo {

enum class falconm_hall_calibration_status : uint8_t {
	uncalibrated = 0,
	calibrating = 1,
	succeeded = 2,
	failed = 3,
};

class QueryHallCalibrationRequest final : public FalconRequest {
public:
	std::string_view topic() const override { return "CUR"; }
	std::vector<uint8_t> encodePayload() const override { return {0}; }
};

class StartHallCalibrationRequest final : public FalconRequest {
public:
	std::string_view topic() const override { return "CVR"; }
	std::vector<uint8_t> encodePayload() const override { return {0}; }
};

class HallCalibrationStatusEvent final : public FalconEvent {
public:
	static constexpr std::string_view kTopic = "CWR";

	std::string_view topic() const override { return kTopic; }
	bool parse(const uint8_t *payload, size_t size) override
	{
		if (!payload || size != 1 ||
		    payload[0] > static_cast<uint8_t>(falconm_hall_calibration_status::failed)) {
			return false;
		}
		status_ = static_cast<falconm_hall_calibration_status>(payload[0]);
		return true;
	}

	falconm_hall_calibration_status status() const { return status_; }

private:
	falconm_hall_calibration_status status_ = falconm_hall_calibration_status::uncalibrated;
};

} // namespace xbotgo

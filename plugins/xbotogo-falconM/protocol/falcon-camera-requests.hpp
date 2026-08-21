#pragma once

#include "falcon-request.hpp"

namespace xbotgo {

class SendDirectionRequest final : public FalconRequest {
public:
	SendDirectionRequest(falconm_direction direction, falconm_operation operation)
		: direction_(direction),
		  operation_(operation)
	{
	}
	std::string_view topic() const override { return "AYR"; }
	std::vector<uint8_t> encodePayload() const override
	{
		return {static_cast<uint8_t>(direction_), static_cast<uint8_t>(operation_)};
	}

private:
	falconm_direction direction_;
	falconm_operation operation_;
};

class QueryMotorAngleRequest final : public FalconRequest {
public:
	std::string_view topic() const override { return "BXR"; }
	std::vector<uint8_t> encodePayload() const override { return {0}; }
};

class SetMotorAngleReportingRequest final : public FalconRequest {
public:
	explicit SetMotorAngleReportingRequest(bool enabled) : enabled_(enabled) {}
	std::string_view topic() const override { return "DGR"; }
	std::vector<uint8_t> encodePayload() const override { return {static_cast<uint8_t>(enabled_)}; }

private:
	bool enabled_;
};

class SetBuzzerModeRequest final : public FalconRequest {
public:
	explicit SetBuzzerModeRequest(uint8_t mode) : mode_(mode) {}
	std::string_view topic() const override { return "AIR"; }
	std::vector<uint8_t> encodePayload() const override { return {mode_}; }

private:
	uint8_t mode_;
};

} // namespace xbotgo

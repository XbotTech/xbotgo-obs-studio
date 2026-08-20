#pragma once

#include "falcon-request.hpp"

namespace xbotgo {

class QuerySupportedModesRequest final : public FalconRequest {
public:
	explicit QuerySupportedModesRequest(uint8_t version) : version_(version) {}
	std::string_view topic() const override { return "BPR"; }
	std::vector<uint8_t> encodePayload() const override { return {version_}; }

private:
	uint8_t version_;
};

class SetCaptureModeRequest final : public FalconRequest {
public:
	explicit SetCaptureModeRequest(uint16_t mode) : mode_(mode) {}
	std::string_view topic() const override { return "AVR"; }
	std::vector<uint8_t> encodePayload() const override
	{
		return {static_cast<uint8_t>(mode_ >> 8), static_cast<uint8_t>(mode_)};
	}

private:
	uint16_t mode_;
};

class QueryCaptureParametersRequest final : public FalconRequest {
public:
	std::string_view topic() const override { return "ANR"; }
	std::vector<uint8_t> encodePayload() const override { return {0}; }
};

class QueryDefaultCaptureParametersRequest final : public FalconRequest {
public:
	std::string_view topic() const override { return "AXR"; }
	std::vector<uint8_t> encodePayload() const override { return {0}; }
};

} // namespace xbotgo

#include "falconm-protocol.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

using namespace xbotgo;

static void test_bpr_builds_version_payload()
{
	for (const uint8_t version : {uint8_t(1), uint8_t(3), uint8_t(255)}) {
		const auto payload = falconm_build_supported_modes_request(version);
		assert(payload.size() == 1);
		assert(payload[0] == version);
	}
}

static void test_bpa_parses_big_endian_modes()
{
	const std::vector<uint8_t> payload = {
		0x01, 0x02, // current mode
		0x00, 0x07, 0x00, 0x12, 0x34, 0x01,
		0xff, // incomplete trailing entry
	};

	falconm_supported_modes result;
	assert(falconm_parse_supported_modes(payload.data(), payload.size(), result));
	assert(result.current_mode == 0x0102);
	assert(result.modes.size() == 2);
	assert(result.modes[0].mode == 7 && !result.modes[0].beta);
	assert(result.modes[1].mode == 0x1234 && result.modes[1].beta);
}

static void test_bpa_rejects_short_payload()
{
	const uint8_t payload[] = {0x00};
	falconm_supported_modes result;
	assert(!falconm_parse_supported_modes(payload, sizeof(payload), result));
}

static void test_avr_builds_big_endian_mode_payload()
{
	const auto payload = falconm_build_capture_mode_request(0x1234);
	assert(payload.size() == 2);
	assert(payload[0] == 0x12);
	assert(payload[1] == 0x34);
}

static void test_ava_parses_success_and_rejects_invalid_payloads()
{
	bool success = false;
	const uint8_t successful[] = {1};
	const uint8_t failed[] = {0};
	const uint8_t invalid[] = {2};

	assert(falconm_parse_capture_mode_result(successful, sizeof(successful), success));
	assert(success);
	assert(falconm_parse_capture_mode_result(failed, sizeof(failed), success));
	assert(!success);
	assert(!falconm_parse_capture_mode_result(invalid, sizeof(invalid), success));
	assert(!falconm_parse_capture_mode_result(nullptr, 0, success));
}

static void test_basketball_mode_filter()
{
	for (const uint16_t mode : {uint16_t(5), uint16_t(6), uint16_t(7), uint16_t(8), uint16_t(36), uint16_t(37),
				    uint16_t(38), uint16_t(39)}) {
		assert(falconm_is_basketball_mode(mode));
	}
	for (const uint16_t mode : {uint16_t(1), uint16_t(20), uint16_t(41), uint16_t(50), uint16_t(65535)}) {
		assert(!falconm_is_basketball_mode(mode));
	}
}

int main()
{
	test_bpr_builds_version_payload();
	test_bpa_parses_big_endian_modes();
	test_bpa_rejects_short_payload();
	test_avr_builds_big_endian_mode_payload();
	test_ava_parses_success_and_rejects_invalid_payloads();
	test_basketball_mode_filter();
	return 0;
}

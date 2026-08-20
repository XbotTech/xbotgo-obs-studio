#include "falconm-protocol.hpp"
#include "protocol/falcon-events.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

using namespace xbotgo;

static void test_bpa_parses_big_endian_modes()
{
	const std::vector<uint8_t> payload = {
		0x01, 0x02, // current mode
		0x00, 0x07, 0x00, 0x12, 0x34, 0x01,
		0xff, // incomplete trailing entry
	};

	SupportedModesEvent event;
	assert(event.parse(payload.data(), payload.size()));
	const auto &result = event.modes();
	assert(result.current_mode == 0x0102);
	assert(result.modes.size() == 2);
	assert(result.modes[0].mode == 7 && !result.modes[0].beta);
	assert(result.modes[1].mode == 0x1234 && result.modes[1].beta);
}

static void test_bpa_rejects_short_payload()
{
	const uint8_t payload[] = {0x00};
	SupportedModesEvent event;
	assert(!event.parse(payload, sizeof(payload)));
}

static void test_ava_parses_success_and_rejects_invalid_payloads()
{
	const uint8_t successful[] = {1};
	const uint8_t failed[] = {0};
	const uint8_t invalid[] = {2};

	CaptureModeResultEvent event;
	assert(event.parse(successful, sizeof(successful)));
	assert(event.success());
	assert(event.parse(failed, sizeof(failed)));
	assert(!event.success());
	assert(!event.parse(invalid, sizeof(invalid)));
	assert(!event.parse(nullptr, 0));
}

static void test_bxa_parses_signed_motor_angles()
{
	const std::vector<uint8_t> payload = {
		0x00, 0x01, 0xff, 0xff, 0xff, 0x9c, 0x00, 0x00, 0x01, 0xf4,
	};

	MotorAngleEvent event;
	assert(event.parse(payload.data(), payload.size()));
	const auto &result = event.angle();
	assert(result.result == 1);
	assert(result.horizontal == -100);
	assert(result.vertical == 500);
}

static void test_dfa_parses_limits_and_rejects_short_payloads()
{
	const std::vector<uint8_t> payload = {
		0x12, 0x34, 0x00, 0x00, 0x00, 0x64, 0x01, 0xff, 0xff, 0xff, 0x9c, 0x00, 0xaa,
	};

	MotorAngleReportEvent event;
	assert(event.parse(payload.data(), payload.size()));
	const auto &result = event.angle();
	assert(result.result == 0x1234);
	assert(result.horizontal == 100);
	assert(result.horizontal_limit == 1);
	assert(result.vertical == -100);
	assert(result.vertical_limit == 0);

	const uint8_t short_payload[11] = {};
	assert(!event.parse(short_payload, sizeof(short_payload)));
	MotorAngleEvent bxa_event;
	assert(!bxa_event.parse(short_payload, 9));
	assert(!bxa_event.parse(nullptr, 0));
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

static void test_request_encoding()
{
	const auto bpr = QuerySupportedModesRequest{3};
	assert(bpr.topic() == "BPR" && bpr.encodePayload() == std::vector<uint8_t>({3}));
	const auto avr = SetCaptureModeRequest{0x1234};
	assert(avr.topic() == "AVR" && avr.encodePayload() == std::vector<uint8_t>({0x12, 0x34}));
	const auto ayr = SendDirectionRequest{falconm_direction::left, falconm_operation::release};
	assert(ayr.topic() == "AYR" && ayr.encodePayload() == std::vector<uint8_t>({2, 2}));
	const auto bxr = QueryMotorAngleRequest{};
	assert(bxr.topic() == "BXR" && bxr.encodePayload() == std::vector<uint8_t>({0}));
	const auto dgr = SetMotorAngleReportingRequest{true};
	assert(dgr.topic() == "DGR" && dgr.encodePayload() == std::vector<uint8_t>({1}));
}

static void test_event_classes_parse_responses()
{
	const std::vector<uint8_t> bpa = {0x01, 0x02, 0x00, 0x07, 0x00};
	SupportedModesEvent modes;
	assert(modes.topic() == "BPA");
	assert(modes.parse(bpa.data(), bpa.size()));
	assert(modes.modes().current_mode == 0x0102);

	const std::vector<uint8_t> bxa = {0x00, 0x01, 0xff, 0xff, 0xff, 0x9c, 0x00, 0x00, 0x01, 0xf4};
	MotorAngleEvent angle;
	assert(angle.topic() == "BXA");
	assert(angle.parse(bxa.data(), bxa.size()));
	assert(angle.angle().horizontal == -100);

	const std::vector<uint8_t> dfa = {0x00, 0x01, 0x00, 0x00, 0x00, 0x64, 0x01, 0x00, 0x00, 0x00, 0xc8, 0x00};
	MotorAngleReportEvent report;
	assert(report.topic() == "DFA");
	assert(report.parse(dfa.data(), dfa.size()));
	assert(report.angle().vertical == 200);
	assert(report.angle().horizontal_limit == 1);

	const std::vector<uint8_t> ava = {1};
	CaptureModeResultEvent capture;
	assert(capture.topic() == "AVA");
	assert(capture.parse(ava.data(), ava.size()));
	assert(capture.success());
}

int main()
{
	test_bpa_parses_big_endian_modes();
	test_bpa_rejects_short_payload();
	test_ava_parses_success_and_rejects_invalid_payloads();
	test_bxa_parses_signed_motor_angles();
	test_dfa_parses_limits_and_rejects_short_payloads();
	test_basketball_mode_filter();
	test_request_encoding();
	test_event_classes_parse_responses();
	return 0;
}

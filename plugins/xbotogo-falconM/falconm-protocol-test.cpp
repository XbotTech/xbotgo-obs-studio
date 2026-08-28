#include "falconm-protocol.hpp"
#include "falconm-angle-state.hpp"
#include "falconm-time.hpp"
#include "protocol/falcon-events.hpp"

#include <cassert>
#include <cstdint>
#include <ctime>
#include <utility>
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

static void test_cwr_parses_hall_calibration_status()
{
	HallCalibrationStatusEvent event;
	for (uint8_t value = 0; value <= 3; ++value) {
		assert(event.parse(&value, 1));
		assert(event.status() == static_cast<falconm_hall_calibration_status>(value));
	}
	const uint8_t invalid[] = {4};
	const uint8_t too_long[] = {1, 2};
	assert(!event.parse(invalid, sizeof(invalid)));
	assert(!event.parse(too_long, sizeof(too_long)));
	assert(!event.parse(nullptr, 0));
}

static void test_dca_parses_current_zoom()
{
	CurrentZoomEvent event;
	assert(event.topic() == "DCA");
	for (const uint8_t value : {uint8_t(10), uint8_t(20), uint8_t(30)}) {
		assert(event.parse(&value, 1));
		assert(event.value() == value);
	}
	const uint8_t too_small[] = {9};
	const uint8_t too_large[] = {31};
	const uint8_t too_long[] = {10, 20};
	assert(!event.parse(too_small, sizeof(too_small)));
	assert(!event.parse(too_large, sizeof(too_large)));
	assert(!event.parse(too_long, sizeof(too_long)));
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

static void test_motor_angle_state_only_notifies_for_reports()
{
	FalconMAngleState state;
	int report_count = 0;
	falconm_motor_angle reported_angle;
	state.setReportCallback([&](const falconm_motor_angle &angle) {
		++report_count;
		reported_angle = angle;
	});

	falconm_motor_angle queried;
	queried.horizontal = -3000;
	state.updateQuery(queried);
	assert(report_count == 0);
	assert(state.snapshot().horizontal == -3000);

	falconm_motor_angle report;
	report.result = 7;
	report.horizontal = 3001;
	report.vertical = -500;
	state.updateReport(report);
	assert(report_count == 1);
	assert(reported_angle.result == 7);
	assert(reported_angle.horizontal == 3001);
	assert(reported_angle.vertical == -500);
	assert(state.snapshot().horizontal == 3001);
}

static void test_basketball_mode_filter()
{
	for (const ModeType mode : {
		     ModeType::BasketballWholeOver14,
		     ModeType::BasketballWholeUnder14,
		     ModeType::BasketballHalfOver14,
		     ModeType::BasketballHalfUnder14,
		     ModeType::BasketballWholeOver14High,
		     ModeType::BasketballWholeUnder14High,
		     ModeType::BasketballHalfOver14High,
		     ModeType::BasketballHalfUnder14High,
	     }) {
		assert(falconm_is_basketball_mode(mode));
	}
	for (const ModeType mode : {
		     ModeType::Soccer5v5Over14,
		     ModeType::FollowMe,
		     ModeType::KeyPlayerHalf,
		     ModeType::Baseball,
	     }) {
		assert(!falconm_is_basketball_mode(mode));
	}
}

static constexpr bool init_mode_type_values_are_stable()
{
	constexpr std::pair<ModeType, uint16_t> values[] = {
		     std::pair{ModeType::Team, uint16_t(0)},
		     std::pair{ModeType::Soccer5v5Over14, uint16_t(1)},
		     std::pair{ModeType::Soccer5v5Under14, uint16_t(2)},
		     std::pair{ModeType::Soccer7v7Over14, uint16_t(3)},
		     std::pair{ModeType::Soccer7v7Under14, uint16_t(4)},
		     std::pair{ModeType::BasketballWholeOver14, uint16_t(5)},
		     std::pair{ModeType::BasketballWholeUnder14, uint16_t(6)},
		     std::pair{ModeType::BasketballHalfOver14, uint16_t(7)},
		     std::pair{ModeType::BasketballHalfUnder14, uint16_t(8)},
		     std::pair{ModeType::Soccer11v11Over14, uint16_t(11)},
		     std::pair{ModeType::Soccer11v11Under14, uint16_t(12)},
		     std::pair{ModeType::RugbyWholeOver14, uint16_t(13)},
		     std::pair{ModeType::RugbyWholeUnder14, uint16_t(14)},
		     std::pair{ModeType::LacrosseWholeOver14, uint16_t(15)},
		     std::pair{ModeType::LacrosseWholeUnder14, uint16_t(16)},
		     std::pair{ModeType::IceHockeyWholeOver14, uint16_t(17)},
		     std::pair{ModeType::IceHockeyWholeUnder14, uint16_t(18)},
		     std::pair{ModeType::WheelchairSoccer, uint16_t(19)},
		     std::pair{ModeType::FollowMe, uint16_t(20)},
		     std::pair{ModeType::TennisDouble, uint16_t(23)},
		     std::pair{ModeType::TennisSingle, uint16_t(24)},
		     std::pair{ModeType::HandballWholeOver14, uint16_t(25)},
		     std::pair{ModeType::HandballWholeUnder14, uint16_t(26)},
		     std::pair{ModeType::HandballHalfOver14, uint16_t(27)},
		     std::pair{ModeType::HandballHalfUnder14, uint16_t(28)},
		     std::pair{ModeType::BroomballWholeOver14, uint16_t(29)},
		     std::pair{ModeType::BroomballWholeUnder14, uint16_t(30)},
		     std::pair{ModeType::PickleballDouble, uint16_t(31)},
		     std::pair{ModeType::PickleballSingle, uint16_t(32)},
		     std::pair{ModeType::BadmintonDouble, uint16_t(33)},
		     std::pair{ModeType::BadmintonSingle, uint16_t(34)},
		     std::pair{ModeType::BasketballWholeOver14High, uint16_t(36)},
		     std::pair{ModeType::BasketballWholeUnder14High, uint16_t(37)},
		     std::pair{ModeType::BasketballHalfOver14High, uint16_t(38)},
		     std::pair{ModeType::BasketballHalfUnder14High, uint16_t(39)},
		     std::pair{ModeType::Volleyball, uint16_t(40)},
		     std::pair{ModeType::KeyPlayerHalf, uint16_t(41)},
		     std::pair{ModeType::KeyPlayerFull, uint16_t(42)},
		     std::pair{ModeType::KeyPlayerHalfHigh, uint16_t(43)},
		     std::pair{ModeType::KeyPlayerFullHigh, uint16_t(44)},
		     std::pair{ModeType::AmericanFootballCloseHigh, uint16_t(45)},
		     std::pair{ModeType::AmericanFootballMediumHigh, uint16_t(46)},
		     std::pair{ModeType::AmericanFootballFarHigh, uint16_t(47)},
		     std::pair{ModeType::RugbyHigh, uint16_t(48)},
		     std::pair{ModeType::FlagFootballHigh, uint16_t(49)},
		     std::pair{ModeType::Baseball, uint16_t(50)},
	};
	for (const auto [strategy, expected] : values) {
		if (static_cast<uint16_t>(strategy) != expected) {
			return false;
		}
	}
	return true;
}

static_assert(init_mode_type_values_are_stable());

static void test_request_encoding()
{
	const auto bpr = QuerySupportedModesRequest{3};
	assert(bpr.topic() == "BPR" && bpr.encodePayload() == std::vector<uint8_t>({3}));
	const auto avr = SetCaptureModeRequest{ModeType::BasketballWholeOver14High};
	assert(avr.topic() == "AVR" && avr.encodePayload() == std::vector<uint8_t>({0x00, 0x24}));
	const auto ayr = SendDirectionRequest{falconm_direction::left, falconm_operation::release};
	assert(ayr.topic() == "AYR" && ayr.encodePayload() == std::vector<uint8_t>({2, 2}));
	const auto bxr = QueryMotorAngleRequest{};
	assert(bxr.topic() == "BXR" && bxr.encodePayload() == std::vector<uint8_t>({0}));
	const auto dgr = SetMotorAngleReportingRequest{true};
	assert(dgr.topic() == "DGR" && dgr.encodePayload() == std::vector<uint8_t>({1}));
	for (const auto [mode, expected] : {
		     std::pair{BuzzerMode::Off, uint8_t(0)},
		     std::pair{BuzzerMode::Beep200Ms, uint8_t(1)},
		     std::pair{BuzzerMode::BeepTwice, uint8_t(2)},
		     std::pair{BuzzerMode::Beep1000Ms, uint8_t(3)},
		     std::pair{BuzzerMode::BeepTwiceLoop, uint8_t(4)},
		     std::pair{BuzzerMode::Beep3000Ms, uint8_t(5)},
	     }) {
		const auto air = SetBuzzerModeRequest{mode};
		assert(air.topic() == "AIR" && air.encodePayload() == std::vector<uint8_t>({expected}));
		assert(is_valid_buzzer_mode(expected));
	}
	assert(!is_valid_buzzer_mode(-1));
	assert(!is_valid_buzzer_mode(6));
	const auto anr = QueryCaptureParametersRequest{};
	assert(anr.topic() == "ANR" && anr.encodePayload() == std::vector<uint8_t>({0}));
	const auto axr = QueryDefaultCaptureParametersRequest{};
	assert(axr.topic() == "AXR" && axr.encodePayload() == std::vector<uint8_t>({0}));
	const auto cur = QueryHallCalibrationRequest{};
	assert(cur.topic() == "CUR" && cur.encodePayload() == std::vector<uint8_t>({0}));
	const auto cvr = StartHallCalibrationRequest{};
	assert(cvr.topic() == "CVR" && cvr.encodePayload() == std::vector<uint8_t>({0}));
	const auto dbr_zoom_in = ManualZoomRequest{falconm_zoom_type::relative, 1};
	assert(dbr_zoom_in.topic() == "DBR" && dbr_zoom_in.encodePayload() == std::vector<uint8_t>({0, 1}));
	const auto dbr_zoom_out = ManualZoomRequest{falconm_zoom_type::relative, -1};
	assert(dbr_zoom_out.topic() == "DBR" &&
	       dbr_zoom_out.encodePayload() == std::vector<uint8_t>({0, 0xff}));
	const auto dbr_absolute = ManualZoomRequest{falconm_zoom_type::absolute, 3};
	assert(dbr_absolute.topic() == "DBR" && dbr_absolute.encodePayload() == std::vector<uint8_t>({1, 3}));
	const auto dcr = QueryCurrentZoomRequest{};
	assert(dcr.topic() == "DCR" && dcr.encodePayload() == std::vector<uint8_t>({0}));

	falconm_capture_parameters settings;
	settings.watermark = true;
	settings.resolution_id = 4;
	settings.resolution = "1080p/30fps";
	settings.auto_zoom = true;
	settings.angle_range = 300;
	settings.accel_speed = 100;
	settings.has_countdown_time = true;
	settings.countdown_time = 5;
	settings.has_flicker_set = true;
	settings.flicker_set = 2;
	settings.has_supported_resolutions = true;
	settings.supported_resolutions = {{4, "1080p/30fps"}};
	const auto aor = SetCaptureParametersRequest{settings};
	const auto aor_payload = aor.encodePayload();
	assert(aor.topic() == "AOR");
	assert(aor_payload.size() == 145);
	assert(aor_payload[0] == 0xff && aor_payload[1] == 0xff);
	assert(aor_payload[2] == 1 && aor_payload[3] == 0);
	assert(aor_payload[4] == 4 && aor_payload[5] == '1');
	assert(aor_payload[69] == 1 && aor_payload[70] == 0);
	assert(aor_payload[71] == 0x01 && aor_payload[72] == 0x2c);
	assert(aor_payload[73] == 0x00 && aor_payload[74] == 0x64);
	assert(aor_payload[75] == 0x00 && aor_payload[76] == 0x05);
	assert(aor_payload[77] == 2 && aor_payload[78] == 1);
	assert(aor_payload[79] == 4 && aor_payload[80] == '1');

	settings.has_supported_resolutions = false;
	settings.supported_resolutions.clear();
	const auto aor_without_table = SetCaptureParametersRequest{settings}.encodePayload();
	assert(aor_without_table.size() == 80);
	assert(aor_without_table[78] == 0);
}

static void test_atr_encodes_clock_fields_and_timezone_id()
{
	const auto atr = RtcClockRequest{0x0102030405060708, -18000, "Asia/Shanghai"};
	const auto payload = atr.encodePayload();

	assert(atr.topic() == "ATR");
	assert(payload.size() == 76);
	const std::vector<uint8_t> expected_prefix = {
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0xb0, 0xb9, 0xff, 0xff,
	};
	assert(std::equal(expected_prefix.begin(), expected_prefix.end(), payload.begin()));
	assert(std::string(reinterpret_cast<const char *>(payload.data() + 12)) == "Asia/Shanghai");
	assert(std::all_of(payload.begin() + 26, payload.end(), [](uint8_t value) { return value == 0; }));
}

static void test_atr_truncates_timezone_id_and_keeps_null_terminator()
{
	const auto atr = RtcClockRequest{0, 28800, std::string(80, 'x')};
	const auto payload = atr.encodePayload();

	const std::vector<uint8_t> expected_offset = {0x80, 0x70, 0x00, 0x00};
	assert(std::equal(expected_offset.begin(), expected_offset.end(), payload.begin() + 8));
	assert(std::all_of(payload.begin() + 12, payload.begin() + 75, [](uint8_t value) { return value == 'x'; }));
	assert(payload[75] == 0);
}

static void test_system_rtc_clock_contains_current_timezone_data()
{
	const auto before = static_cast<uint64_t>(std::time(nullptr));
	const auto clock = falconm_read_system_rtc_clock();
	const auto after = static_cast<uint64_t>(std::time(nullptr));

	assert(clock.timestamp >= before && clock.timestamp <= after);
	assert(clock.timezone >= -24 * 60 * 60 && clock.timezone <= 24 * 60 * 60);
	assert(!clock.timezone_id.empty());
}

static void test_ana_parses_capture_parameters()
{
	std::vector<uint8_t> payload = {
		0x00, 0x07, 0x01, 0x00, 0x03,
	};
	payload.resize(75, 0);
	payload[5] = '1';
	payload[6] = '0';
	payload[7] = '8';
	payload[8] = '0';
	payload[9] = 'p';
	payload[69] = 0x01;
	payload[71] = 0x01;
	payload[72] = 0x2c;
	payload[73] = 0x00;
	payload[74] = 0x64;
	payload.insert(payload.end(), {0x00, 0x0a, 0x02, 0x01});
	payload.insert(payload.end(), {0x04});
	payload.insert(payload.end(), {'7', '2', '0', 'p'});
	payload.resize(payload.size() + 60, 0);

	CaptureParametersEvent event;
	assert(event.topic() == "ANA");
	assert(event.parse(payload.data(), payload.size()));
	const auto &parameters = event.parameters();
	assert(parameters.mode == 7);
	assert(parameters.watermark);
	assert(!parameters.mute);
	assert(parameters.resolution_id == 3);
	assert(parameters.resolution == "1080p");
	assert(parameters.auto_zoom);
	assert(!parameters.auto_tracking);
	assert(parameters.angle_range == 300);
	assert(parameters.accel_speed == 100);
	assert(parameters.has_countdown_time && parameters.countdown_time == 10);
	assert(parameters.has_flicker_set && parameters.flicker_set == 2);
	assert(parameters.has_supported_resolutions);
	assert(parameters.supported_resolutions.size() == 1);
	assert(parameters.supported_resolutions[0].id == 4);
	assert(parameters.supported_resolutions[0].value == "720p");

	// AXA devices may return only part of the optional tail.
	const std::vector<uint8_t> partial_payload(payload.begin(), payload.begin() + 76);
	CaptureParametersEvent partial_event;
	assert(partial_event.parse(partial_payload.data(), partial_payload.size()));
	assert(!partial_event.parameters().has_countdown_time);

	payload.pop_back();
	assert(!event.parse(payload.data(), payload.size()));
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

	std::vector<uint8_t> axa(75, 0);
	axa[0] = 0x00;
	axa[1] = 0x09;
	axa[4] = 0x02;
	axa[5] = '4';
	axa[6] = 'K';
	axa[69] = 1;
	axa[70] = 1;
	axa[71] = 0x00;
	axa[72] = 0x64;
	axa[73] = 0x00;
	axa[74] = 0x32;
	axa.insert(axa.end(), {0x00, 0x05, 0x01, 0x01});
	axa.resize(axa.size() + 65, 0);
	axa[79] = 1;
	DefaultCaptureParametersEvent defaults;
	assert(defaults.topic() == "AXA");
	assert(defaults.parse(axa.data(), axa.size()));
	const auto &default_parameters = defaults.parameters();
	assert(default_parameters.mode == 9);
	assert(default_parameters.resolution == "4K");
	assert(default_parameters.auto_zoom && default_parameters.auto_tracking);
	assert(default_parameters.countdown_time == 5);
	assert(default_parameters.flicker_set == 1);
	assert(default_parameters.supported_resolutions.size() == 1);
	assert(default_parameters.supported_resolutions[0].id == 1);
}

int main()
{
	test_bpa_parses_big_endian_modes();
	test_bpa_rejects_short_payload();
	test_ava_parses_success_and_rejects_invalid_payloads();
	test_cwr_parses_hall_calibration_status();
	test_dca_parses_current_zoom();
	test_bxa_parses_signed_motor_angles();
	test_dfa_parses_limits_and_rejects_short_payloads();
	test_motor_angle_state_only_notifies_for_reports();
	test_basketball_mode_filter();
	test_request_encoding();
	test_atr_encodes_clock_fields_and_timezone_id();
	test_atr_truncates_timezone_id_and_keeps_null_terminator();
	test_system_rtc_clock_contains_current_timezone_data();
	test_ana_parses_capture_parameters();
	test_event_classes_parse_responses();
	return 0;
}

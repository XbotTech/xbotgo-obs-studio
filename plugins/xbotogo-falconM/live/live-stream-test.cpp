#include "live-stream-parser.hpp"
#include "live-stream-session.hpp"

#include <QByteArray>
#include <QString>

#include <cassert>

using namespace xbotgo;

static QByteArray valid_response()
{
	return R"({
		"code": 200,
		"data": {
			"livePushUrl": "rtmps://push.example.com/live/stream-key?token=abc",
			"livePlayUrl": "{\"rtmpPlayUrl\":\"rtmp://pull.example.com/live/play-key\"}",
			"taskId": "task-1"
		}
	})";
}

static void test_valid_response_is_split_into_service_fields()
{
	QString error;
	const auto config = ParseLiveStreamResponse(valid_response(), error);

	assert(config);
	assert(error.isEmpty());
	assert(config->pushServer == QStringLiteral("rtmps://push.example.com/live"));
	assert(config->pushStreamKey == QStringLiteral("stream-key?token=abc"));
	assert(config->pullServer == QStringLiteral("rtmp://pull.example.com/live"));
	assert(config->pullStreamKey == QStringLiteral("play-key"));
	assert(config->taskId == QStringLiteral("task-1"));
	assert(config->pullUrl() == QStringLiteral("rtmp://pull.example.com/live/play-key"));
}

static void test_invalid_json_is_rejected()
{
	QString error;
	assert(!ParseLiveStreamResponse(QByteArrayLiteral("not-json"), error));
	assert(!error.isEmpty());
}

static void test_unsuccessful_business_code_is_rejected()
{
	QString error;
	assert(!ParseLiveStreamResponse(QByteArrayLiteral(R"({"code":500,"msg":"failed"})"), error));
	assert(error == QStringLiteral("failed"));
}

static void test_missing_task_id_is_rejected()
{
	QByteArray response = valid_response();
	response.replace(R"("taskId": "task-1")", R"("taskId": "")");

	QString error;
	assert(!ParseLiveStreamResponse(response, error));
	assert(!error.isEmpty());
}

static void test_non_rtmp_urls_are_rejected()
{
	QString error;
	assert(!ParseLiveStreamUrls(QStringLiteral("https://push.example.com/live/key"),
				    QStringLiteral("rtmp://pull.example.com/live/key"), error));
	assert(!error.isEmpty());
}

static void test_session_accepts_only_legal_transitions()
{
	LiveStreamSession session;
	assert(session.phase() == LiveStreamPhase::Idle);
	assert(!session.beginStarting());
	assert(session.beginFetch());
	assert(!session.beginFetch());
	assert(!session.observeStreamingStarted());
	assert(session.beginConfirming(QStringLiteral("task-1")));
	assert(session.beginStarting());
	assert(session.observeStreamingStarting());
	assert(session.startingEventObserved());
	assert(session.observeStreamingStarted());
	assert(session.observeStreamingStopping());
	assert(session.phase() == LiveStreamPhase::Stopping);
}

static void test_session_releases_task_id_once()
{
	LiveStreamSession session;
	assert(session.beginFetch());
	assert(session.beginConfirming(QStringLiteral("task-1")));

	const auto task_id = session.finish();
	assert(task_id == QStringLiteral("task-1"));
	assert(session.phase() == LiveStreamPhase::Idle);
	assert(!session.finish());
}

int main()
{
	test_valid_response_is_split_into_service_fields();
	test_invalid_json_is_rejected();
	test_unsuccessful_business_code_is_rejected();
	test_missing_task_id_is_rejected();
	test_non_rtmp_urls_are_rejected();
	test_session_accepts_only_legal_transitions();
	test_session_releases_task_id_once();
	return 0;
}

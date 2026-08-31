#include "curl-request-worker.hpp"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

#include <curl/curl.h>

#include <cassert>
#include <chrono>
#include <mutex>
#include <optional>

using namespace std::chrono_literals;

namespace {

class HttpServer final : public QTcpServer {
public:
	explicit HttpServer(bool respond = true, int status = 200) : respond_(respond), status_(status)
	{
		connect(this, &QTcpServer::newConnection, this, [this] {
			QTcpSocket *socket = nextPendingConnection();
			connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
				request_ += socket->readAll();
				const qsizetype headerEnd = request_.indexOf("\r\n\r\n");
				if (!respond_ || headerEnd < 0) {
					return;
				}
				const QByteArray response = QByteArrayLiteral("HTTP/1.1 ") +
							    QByteArray::number(status_) +
							    (status_ == 200 ? QByteArrayLiteral(" OK\r\n")
									    : QByteArrayLiteral(" Error\r\n")) +
							    "Content-Type: application/json\r\nContent-Length: 11\r\n"
							    "Connection: close\r\n\r\n{\"ok\":true}";
				socket->write(response);
				socket->disconnectFromHost();
			});
		});
		assert(listen(QHostAddress::LocalHost));
	}

	QByteArray url() const
	{
		return QByteArrayLiteral("http://127.0.0.1:") + QByteArray::number(serverPort()) +
		       QByteArrayLiteral("/task");
	}

	const QByteArray &request() const { return request_; }

private:
	bool respond_ = true;
	int status_ = 200;
	QByteArray request_;
};

class ResultSlot {
public:
	void set(xbotgo::CurlPostResult value)
	{
		std::lock_guard lock(mutex_);
		result_ = std::move(value);
	}

	std::optional<xbotgo::CurlPostResult> get() const
	{
		std::lock_guard lock(mutex_);
		return result_;
	}

private:
	mutable std::mutex mutex_;
	std::optional<xbotgo::CurlPostResult> result_;
};

bool WaitFor(const std::function<bool()> &predicate, std::chrono::milliseconds timeout = 2s)
{
	QElapsedTimer timer;
	timer.start();
	while (!predicate() && timer.elapsed() < timeout.count()) {
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	return predicate();
}

void TestReportsSystemHttpsSupport()
{
	assert(xbotgo::CurlRequestWorker::SupportsHttps());
}

void TestPostsJsonOffTheCallingThread()
{
	HttpServer server;
	ResultSlot slot;
	QThread *callingThread = QThread::currentThread();
	QThread *completionThread = nullptr;
	xbotgo::CurlRequestWorker worker([&](xbotgo::CurlPostResult result) {
		completionThread = QThread::currentThread();
		slot.set(std::move(result));
	});

	const QByteArray body = R"({"taskId":"task-1"})";
	const uint64_t requestId = worker.post(server.url(), body, 1'000);
	assert(WaitFor([&] { return slot.get().has_value(); }));

	const auto result = slot.get();
	assert(result->requestId == requestId);
	assert(result->transportCode == 0);
	assert(result->httpStatus == 200);
	assert(result->body == QByteArrayLiteral("{\"ok\":true}"));
	assert(result->error.isEmpty());
	assert(!result->cancelled);
	assert(completionThread != callingThread);
	assert(server.request().startsWith("POST /task HTTP/1.1\r\n"));
	assert(server.request().contains("Content-Type: application/json\r\n"));
	assert(server.request().contains("DATA-REGION: CN\r\n"));
	assert(server.request().contains("BLINK-APP-LANG: en_US\r\n"));
	assert(server.request().endsWith(body));
}

void TestTimesOut()
{
	HttpServer server(false);
	ResultSlot slot;
	xbotgo::CurlRequestWorker worker([&](xbotgo::CurlPostResult result) { slot.set(std::move(result)); });

	worker.post(server.url(), QByteArrayLiteral("{}"), 50);
	assert(WaitFor([&] { return slot.get().has_value(); }));
	const auto result = slot.get();
	assert(result->transportCode != 0);
	assert(!result->error.isEmpty());
	assert(!result->cancelled);
}

void TestReportsHttpFailure()
{
	HttpServer server(true, 500);
	ResultSlot slot;
	xbotgo::CurlRequestWorker worker([&](xbotgo::CurlPostResult result) { slot.set(std::move(result)); });

	worker.post(server.url(), QByteArrayLiteral("{}"), 1'000);
	assert(WaitFor([&] { return slot.get().has_value(); }));
	const auto result = slot.get();
	assert(result->transportCode != 0);
	assert(result->httpStatus == 500);
	assert(!result->error.isEmpty());
	assert(!result->cancelled);
}

void TestCancellationWakesTheWorker()
{
	HttpServer server(false);
	ResultSlot slot;
	xbotgo::CurlRequestWorker worker([&](xbotgo::CurlPostResult result) { slot.set(std::move(result)); });

	const uint64_t requestId = worker.post(server.url(), QByteArrayLiteral("{}"), 10'000);
	QElapsedTimer timer;
	timer.start();
	worker.cancelThrough(requestId);
	assert(WaitFor([&] { return slot.get().has_value(); }));
	assert(slot.get()->cancelled);
	assert(timer.elapsed() < 1'000);
}

void TestRequestAfterCancellationIsNotCancelled()
{
	HttpServer stalledServer(false);
	HttpServer respondingServer;
	ResultSlot cancelledSlot;
	ResultSlot completedSlot;
	uint64_t cancelledRequestId = 0;
	xbotgo::CurlRequestWorker worker([&](xbotgo::CurlPostResult result) {
		if (result.requestId == cancelledRequestId) {
			cancelledSlot.set(std::move(result));
		} else {
			completedSlot.set(std::move(result));
		}
	});

	cancelledRequestId = worker.post(stalledServer.url(), QByteArrayLiteral("{}"), 10'000);
	worker.cancelThrough(cancelledRequestId);
	assert(WaitFor([&] { return cancelledSlot.get().has_value(); }));
	assert(cancelledSlot.get()->cancelled);

	const uint64_t completedRequestId = worker.post(respondingServer.url(), QByteArrayLiteral("{}"), 1'000);
	assert(WaitFor([&] { return completedSlot.get().has_value(); }));
	const auto completed = completedSlot.get();
	assert(completed->requestId == completedRequestId);
	assert(completed->transportCode == 0);
	assert(completed->httpStatus == 200);
	assert(!completed->cancelled);
}

} // namespace

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	assert(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
	TestReportsSystemHttpsSupport();
	TestPostsJsonOffTheCallingThread();
	TestTimesOut();
	TestReportsHttpFailure();
	TestCancellationWakesTheWorker();
	TestRequestAfterCancellationIsNotCancelled();
	curl_global_cleanup();
	return 0;
}

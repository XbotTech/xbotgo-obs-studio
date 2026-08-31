#pragma once

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <functional>
#include <memory>

namespace xbotgo {

struct CurlPostResult {
	uint64_t requestId = 0;
	int transportCode = 0;
	long httpStatus = 0;
	QByteArray body;
	QString error;
	bool cancelled = false;
};

class CurlRequestWorker final {
public:
	using Completion = std::function<void(CurlPostResult)>;

	explicit CurlRequestWorker(Completion completion);
	~CurlRequestWorker();

	CurlRequestWorker(const CurlRequestWorker &) = delete;
	CurlRequestWorker &operator=(const CurlRequestWorker &) = delete;

	uint64_t post(QByteArray url, QByteArray body, int timeoutMs);
	void cancelThrough(uint64_t requestId);
	void shutdown();

	static bool SupportsHttps();

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace xbotgo

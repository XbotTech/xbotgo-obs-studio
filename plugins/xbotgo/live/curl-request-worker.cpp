#include "curl-request-worker.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

namespace xbotgo {
namespace {

struct PendingRequest {
	uint64_t id = 0;
	QByteArray url;
	QByteArray body;
	int timeoutMs = 0;
};

struct ActiveRequest {
	PendingRequest request;
	CURL *easy = nullptr;
	curl_slist *headers = nullptr;
	QByteArray response;
	char error[CURL_ERROR_SIZE]{};
};

size_t WriteResponse(char *data, size_t size, size_t count, void *context)
{
	auto *response = static_cast<QByteArray *>(context);
	const size_t length = size * count;
	response->append(data, static_cast<qsizetype>(length));
	return length;
}

QString CurlError(const ActiveRequest &request, CURLcode code)
{
	if (request.error[0] != '\0') {
		return QString::fromUtf8(request.error);
	}
	return QString::fromUtf8(curl_easy_strerror(code));
}

} // namespace

struct CurlRequestWorker::Impl {
	explicit Impl(Completion completion_) : completion(std::move(completion_)), thread([this] { run(); }) {}

	~Impl() { shutdown(); }

	uint64_t post(QByteArray url, QByteArray body, int timeoutMs)
	{
		std::lock_guard lock(mutex);
		if (stopping) {
			return 0;
		}
		const uint64_t id = nextRequestId++;
		queued.push_back({id, std::move(url), std::move(body), timeoutMs});
		condition.notify_one();
		wake();
		return id;
	}

	void cancelThrough(uint64_t requestId)
	{
		{
			std::lock_guard lock(mutex);
			cancelledThrough = std::max(cancelledThrough, requestId);
		}
		condition.notify_one();
		wake();
	}

	void shutdown()
	{
		{
			std::lock_guard lock(mutex);
			if (stopping) {
				return;
			}
			stopping = true;
			cancelledThrough = std::numeric_limits<uint64_t>::max();
		}
		condition.notify_one();
		wake();
		if (thread.joinable()) {
			thread.join();
		}
	}

	void wake()
	{
		if (CURLM *handle = multi.load(std::memory_order_acquire)) {
			curl_multi_wakeup(handle);
		}
	}

	void completeCancelled(PendingRequest request)
	{
		CurlPostResult result;
		result.requestId = request.id;
		result.transportCode = static_cast<int>(CURLE_ABORTED_BY_CALLBACK);
		result.error = QString::fromUtf8(curl_easy_strerror(CURLE_ABORTED_BY_CALLBACK));
		result.cancelled = true;
		completion(std::move(result));
	}

	void finish(CURLM *multiHandle, CURL *easy, CURLcode code, bool cancelled)
	{
		auto it = active.find(easy);
		if (it == active.end()) {
			return;
		}
		std::unique_ptr<ActiveRequest> request = std::move(it->second);
		active.erase(it);
		curl_multi_remove_handle(multiHandle, easy);

		CurlPostResult result;
		result.requestId = request->request.id;
		result.transportCode = static_cast<int>(code);
		result.body = std::move(request->response);
		result.cancelled = cancelled;
		curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &result.httpStatus);
		if (code != CURLE_OK) {
			result.error = CurlError(*request, code);
		}

		curl_easy_cleanup(easy);
		curl_slist_free_all(request->headers);
		completion(std::move(result));
	}

	void add(CURLM *multiHandle, PendingRequest request)
	{
		auto activeRequest = std::make_unique<ActiveRequest>();
		activeRequest->request = std::move(request);
		activeRequest->easy = curl_easy_init();
		if (!activeRequest->easy) {
			CurlPostResult result;
			result.requestId = activeRequest->request.id;
			result.transportCode = static_cast<int>(CURLE_FAILED_INIT);
			result.error = QString::fromUtf8(curl_easy_strerror(CURLE_FAILED_INIT));
			completion(std::move(result));
			return;
		}

		activeRequest->headers = curl_slist_append(activeRequest->headers, "Content-Type: application/json");
		activeRequest->headers = curl_slist_append(activeRequest->headers, "DATA-REGION: CN");
		activeRequest->headers = curl_slist_append(activeRequest->headers, "BLINK-APP-LANG: en_US");
		activeRequest->headers = curl_slist_append(activeRequest->headers, "Accept: */*");

		CURL *easy = activeRequest->easy;
		curl_easy_setopt(easy, CURLOPT_URL, activeRequest->request.url.constData());
		curl_easy_setopt(easy, CURLOPT_POST, 1L);
		curl_easy_setopt(easy, CURLOPT_POSTFIELDS, activeRequest->request.body.constData());
		curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE,
				 static_cast<curl_off_t>(activeRequest->request.body.size()));
		curl_easy_setopt(easy, CURLOPT_HTTPHEADER, activeRequest->headers);
		curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, WriteResponse);
		curl_easy_setopt(easy, CURLOPT_WRITEDATA, &activeRequest->response);
		curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, activeRequest->error);
		curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, static_cast<long>(activeRequest->request.timeoutMs));
		curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(activeRequest->request.timeoutMs));
		curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 3L);
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https");
		curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 2L);

		active.emplace(easy, std::move(activeRequest));
		const CURLMcode addResult = curl_multi_add_handle(multiHandle, easy);
		if (addResult != CURLM_OK) {
			finish(multiHandle, easy, CURLE_FAILED_INIT, false);
		}
	}

	bool processCommands(CURLM *multiHandle)
	{
		std::deque<PendingRequest> requests;
		uint64_t cancelCutoff = 0;
		bool shouldStop = false;
		{
			std::lock_guard lock(mutex);
			requests.swap(queued);
			cancelCutoff = cancelledThrough;
			shouldStop = stopping;
		}

		for (auto it = active.begin(); it != active.end();) {
			CURL *easy = it->first;
			const uint64_t id = it->second->request.id;
			++it;
			if (id <= cancelCutoff) {
				finish(multiHandle, easy, CURLE_ABORTED_BY_CALLBACK, true);
			}
		}

		for (PendingRequest &request : requests) {
			if (request.id <= cancelCutoff || shouldStop) {
				completeCancelled(std::move(request));
			} else {
				add(multiHandle, std::move(request));
			}
		}
		return shouldStop;
	}

	void run()
	{
		CURLM *multiHandle = curl_multi_init();
		if (!multiHandle) {
			return;
		}
		multi.store(multiHandle, std::memory_order_release);

		for (;;) {
			if (processCommands(multiHandle)) {
				break;
			}

			int running = 0;
			curl_multi_perform(multiHandle, &running);
			int messages = 0;
			while (CURLMsg *message = curl_multi_info_read(multiHandle, &messages)) {
				if (message->msg == CURLMSG_DONE) {
					finish(multiHandle, message->easy_handle, message->data.result, false);
				}
			}

			if (active.empty()) {
				std::unique_lock lock(mutex);
				condition.wait(lock, [this] { return stopping || !queued.empty(); });
			} else {
				int descriptors = 0;
				curl_multi_poll(multiHandle, nullptr, 0, 250, &descriptors);
			}
		}

		for (auto it = active.begin(); it != active.end();) {
			CURL *easy = it->first;
			++it;
			finish(multiHandle, easy, CURLE_ABORTED_BY_CALLBACK, true);
		}
		multi.store(nullptr, std::memory_order_release);
		curl_multi_cleanup(multiHandle);
	}

	Completion completion;
	std::mutex mutex;
	std::condition_variable condition;
	std::deque<PendingRequest> queued;
	std::unordered_map<CURL *, std::unique_ptr<ActiveRequest>> active;
	std::atomic<CURLM *> multi{nullptr};
	uint64_t nextRequestId = 1;
	uint64_t cancelledThrough = 0;
	bool stopping = false;
	std::thread thread;
};

CurlRequestWorker::CurlRequestWorker(Completion completion) : impl_(std::make_unique<Impl>(std::move(completion))) {}

CurlRequestWorker::~CurlRequestWorker() = default;

uint64_t CurlRequestWorker::post(QByteArray url, QByteArray body, int timeoutMs)
{
	return impl_->post(std::move(url), std::move(body), timeoutMs);
}

void CurlRequestWorker::cancelThrough(uint64_t requestId)
{
	impl_->cancelThrough(requestId);
}

void CurlRequestWorker::shutdown()
{
	impl_->shutdown();
}

bool CurlRequestWorker::SupportsHttps()
{
	const curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);
	if (!info || !(info->features & CURL_VERSION_SSL) || !info->protocols) {
		return false;
	}
	for (const char *const *protocol = info->protocols; *protocol; ++protocol) {
		if (std::strcmp(*protocol, "https") == 0) {
			return true;
		}
	}
	return false;
}

} // namespace xbotgo

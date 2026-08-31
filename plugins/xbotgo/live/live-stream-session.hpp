#pragma once

#include <QString>

#include <optional>

namespace xbotgo {

enum class LiveStreamPhase { Idle, Fetching, Confirming, Starting, Streaming, Stopping };

class LiveStreamSession {
public:
	LiveStreamPhase phase() const noexcept;
	bool beginFetch() noexcept;
	bool beginConfirming(QString taskId);
	bool beginStarting() noexcept;
	bool observeStreamingStarting() noexcept;
	bool observeStreamingStarted() noexcept;
	bool observeStreamingStopping() noexcept;
	bool startingEventObserved() const noexcept;
	std::optional<QString> finish();

private:
	LiveStreamPhase phase_ = LiveStreamPhase::Idle;
	QString taskId_;
	bool startingEventObserved_ = false;
};

} // namespace xbotgo

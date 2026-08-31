#include "live-stream-session.hpp"

#include <utility>

namespace xbotgo {

LiveStreamPhase LiveStreamSession::phase() const noexcept
{
	return phase_;
}

bool LiveStreamSession::beginFetch() noexcept
{
	if (phase_ != LiveStreamPhase::Idle) {
		return false;
	}
	phase_ = LiveStreamPhase::Fetching;
	return true;
}

bool LiveStreamSession::beginConfirming(QString taskId)
{
	if (phase_ != LiveStreamPhase::Fetching || taskId.isEmpty()) {
		return false;
	}
	taskId_ = std::move(taskId);
	phase_ = LiveStreamPhase::Confirming;
	return true;
}

bool LiveStreamSession::beginStarting() noexcept
{
	if (phase_ != LiveStreamPhase::Confirming) {
		return false;
	}
	startingEventObserved_ = false;
	phase_ = LiveStreamPhase::Starting;
	return true;
}

bool LiveStreamSession::observeStreamingStarting() noexcept
{
	if (phase_ != LiveStreamPhase::Starting) {
		return false;
	}
	startingEventObserved_ = true;
	return true;
}

bool LiveStreamSession::observeStreamingStarted() noexcept
{
	if (phase_ != LiveStreamPhase::Starting) {
		return false;
	}
	phase_ = LiveStreamPhase::Streaming;
	return true;
}

bool LiveStreamSession::observeStreamingStopping() noexcept
{
	if (phase_ != LiveStreamPhase::Streaming) {
		return false;
	}
	phase_ = LiveStreamPhase::Stopping;
	return true;
}

bool LiveStreamSession::startingEventObserved() const noexcept
{
	return startingEventObserved_;
}

std::optional<QString> LiveStreamSession::finish()
{
	std::optional<QString> taskId;
	if (!taskId_.isEmpty()) {
		taskId = std::exchange(taskId_, QString{});
	}
	phase_ = LiveStreamPhase::Idle;
	startingEventObserved_ = false;
	return taskId;
}

} // namespace xbotgo

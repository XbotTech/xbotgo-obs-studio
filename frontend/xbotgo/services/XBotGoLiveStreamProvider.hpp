#pragma once

#include "../models/XBotGoLiveStreamConfig.hpp"

#include <functional>
#include <optional>

class QObject;

namespace XBotGo {

using LiveStreamConfigCallback =
	std::function<void(std::optional<LiveStreamConfig> config, const QString &error)>;

LiveStreamConfig generateLiveStreamConfig(const QString &pushDomain, const QString &pushSecret,
					  const QString &pullDomain, const QString &pullSecret,
					  const QString &streamName);

class LiveStreamProvider {
public:
	virtual ~LiveStreamProvider() = default;

	virtual void requestLiveStreamConfig(QObject *context, LiveStreamConfigCallback callback) = 0;
};

class HardcodedLiveStreamProvider final : public LiveStreamProvider {
public:
	void requestLiveStreamConfig(QObject *context, LiveStreamConfigCallback callback) override;
};

} // namespace XBotGo

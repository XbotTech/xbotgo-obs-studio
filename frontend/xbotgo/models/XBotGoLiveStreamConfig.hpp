#pragma once

#include <QString>

namespace XBotGo {

struct LiveStreamConfig {
	QString pushServer;
	QString pushStreamKey;
	QString pullServer;
	QString pullStreamKey;

	QString pushUrl() const
	{
		return pushServer.endsWith(QLatin1Char('/')) ? pushServer + pushStreamKey
							    : pushServer + QLatin1Char('/') + pushStreamKey;
	}

	QString pullUrl() const
	{
		return pullServer.endsWith(QLatin1Char('/')) ? pullServer + pullStreamKey
							    : pullServer + QLatin1Char('/') + pullStreamKey;
	}
};

} // namespace XBotGo

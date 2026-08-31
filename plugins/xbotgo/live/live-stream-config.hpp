#pragma once

#include <QString>

namespace xbotgo {

struct LiveStreamConfig {
	QString pushServer;
	QString pushStreamKey;
	QString pullServer;
	QString pullStreamKey;
	QString taskId;

	QString pushUrl() const;
	QString pullUrl() const;
};

} // namespace xbotgo

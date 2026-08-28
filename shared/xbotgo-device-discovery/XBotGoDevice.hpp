#pragma once

#include <QString>

#include <optional>

namespace xbotgo {

struct Device {
	QString id;
	QString serialNumber;
	QString ip;
	QString mask;
	QString mac;
	QString version;
	quint16 mqttPort;
	QString role;
	std::optional<bool> timeSynchronized;
};

} // namespace xbotgo

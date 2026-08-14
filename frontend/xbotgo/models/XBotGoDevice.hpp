#pragma once

#include <QHostAddress>
#include <QString>

namespace XBotGo {

struct Device {
	QString id;
	QString ip;
	quint16 mqttPort;
	quint16 protocolVersion;
};

} // namespace XBotGo

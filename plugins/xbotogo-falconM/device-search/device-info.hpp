#pragma once

#include "QHash"
#include "QHostAddress"
#include "QRegularExpression"

#include <QString>

#include <optional>

#define XBOTGO_SSDP_DEVICE_ID "x-device-id"
#define XBOTGO_SSDP_DEVICE_SN "x-device-sn"
#define XBOTGO_SSDP_DEVICE_IP "x-device-ip"
#define XBOTGO_SSDP_DEVICE_MASK "x-device-mask"
#define XBOTGO_SSDP_DEVICE_MAC "x-device-mac"
#define XBOTGO_SSDP_DEVICE_VERSION "x-device-version"
#define XBOTGO_SSDP_DEVICE_ROLE "x-device-role"
#define XBOTGO_SSDP_DEVICE_TIME "x-device-time"
#define XBOTGO_SSDP_MQTT_PORT "x-mqtt-port"

namespace xbotgo {

class DeviceInfo {
public:
	const QString id;
	const QString serialNumber;
	const QString ip;
	const QString mask;
	const QString mac;
	const QString version;
	const quint16 mqttPort;
	const QString role;
	const std::optional<bool> timeSynchronized;

	bool isValid() const { return !id.isEmpty() && !ip.isEmpty() && mqttPort > 0 && mqttPort <= 65535; }

	static std::optional<DeviceInfo> parseSsdpDevice(const QByteArray &payload)
	{
		QHash<QString, QString> headers;
		for (const QString &line : QString::fromUtf8(payload).split(QLatin1Char('\n'), Qt::KeepEmptyParts)) {
			const qsizetype separator = line.indexOf(QLatin1Char(':'));
			if (separator > 0) {
				headers.insert(line.left(separator).trimmed().toLower(),
					       line.mid(separator + 1).trimmed());
			}
		}
		return DeviceInfo{
			.id = headers.value(QStringLiteral(XBOTGO_SSDP_DEVICE_ID)),
			.serialNumber = headers.value(QStringLiteral(XBOTGO_SSDP_DEVICE_SN)),
			.ip = headers.value(QStringLiteral(XBOTGO_SSDP_DEVICE_IP)),
			.mask = headers.value(QStringLiteral(XBOTGO_SSDP_DEVICE_MASK)),
			.mac = headers.value(QStringLiteral(XBOTGO_SSDP_DEVICE_MAC)),
			.version = headers.value(QStringLiteral(XBOTGO_SSDP_DEVICE_VERSION)),
			.mqttPort = headers.value(QStringLiteral(XBOTGO_SSDP_MQTT_PORT)).toUShort(),
			.role = headers.value(QStringLiteral(XBOTGO_SSDP_DEVICE_ROLE)),
			.timeSynchronized = std::nullopt,
		};
	}
};

} // namespace xbotgo

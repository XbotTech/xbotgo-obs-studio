#include "XBotGoSsdpParser.hpp"

#include <QAbstractSocket>
#include <QHash>
#include <QRegularExpression>
#include <QStringList>

namespace xbotgo {

std::optional<Device> parseSsdpDevice(const QByteArray &payload, const QHostAddress &senderAddress)
{
	QString message = QString::fromUtf8(payload);
	message.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

	const QStringList lines = message.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
	if (lines.isEmpty()) {
		return std::nullopt;
	}

	const QString startLine = lines.front().trimmed();
	const bool isNotify = startLine.compare(QStringLiteral("NOTIFY * HTTP/1.1"), Qt::CaseInsensitive) == 0;
	const bool isSearchResponse = startLine.compare(QStringLiteral("HTTP/1.1 200 OK"), Qt::CaseInsensitive) == 0;
	if (!isNotify && !isSearchResponse) {
		return std::nullopt;
	}

	QHash<QString, QString> headers;
	for (qsizetype index = 1; index < lines.size(); ++index) {
		const QString line = lines.at(index).trimmed();
		if (line.isEmpty()) {
			continue;
		}

		const qsizetype separator = line.indexOf(QLatin1Char(':'));
		if (separator <= 0) {
			continue;
		}

		const QString name = line.left(separator).trimmed().toLower();
		const QString value = line.mid(separator + 1).trimmed();
		headers.insert(name, value);
	}

	const QString id = headers.value(QStringLiteral("x-device-id"));
	const QString ipText = headers.value(QStringLiteral("x-device-ip"));
	const QString mqttPortText = headers.value(QStringLiteral("x-mqtt-port"));
	if (id.isEmpty() || ipText.isEmpty() || mqttPortText.isEmpty()) {
		return std::nullopt;
	}

	QHostAddress deviceAddress;
	if (!deviceAddress.setAddress(ipText) || deviceAddress.protocol() != QAbstractSocket::IPv4Protocol) {
		return std::nullopt;
	}

	if (senderAddress.protocol() != QAbstractSocket::IPv4Protocol || deviceAddress != senderAddress) {
		return std::nullopt;
	}

	bool portValid = false;
	const uint mqttPort = mqttPortText.toUInt(&portValid);
	if (!portValid || mqttPort == 0 || mqttPort > 65535) {
		return std::nullopt;
	}

	Device device;
	device.id = id;
	device.serialNumber = headers.value(QStringLiteral("x-device-sn"));
	device.ip = deviceAddress.toString();
	device.mqttPort = static_cast<quint16>(mqttPort);

	const QString maskText = headers.value(QStringLiteral("x-device-mask"));
	QHostAddress maskAddress;
	if (maskAddress.setAddress(maskText) && maskAddress.protocol() == QAbstractSocket::IPv4Protocol) {
		device.mask = maskAddress.toString();
	}

	const QString macText = headers.value(QStringLiteral("x-device-mac"));
	static const QRegularExpression macPattern(
		QStringLiteral("^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$"));
	if (macPattern.match(macText).hasMatch()) {
		device.mac = macText.toUpper();
	}

	device.version = headers.value(QStringLiteral("x-device-version"));

	const QString roleText = headers.value(QStringLiteral("x-device-role")).toLower();
	if (roleText == QStringLiteral("master") || roleText == QStringLiteral("slave")) {
		device.role = roleText;
	}

	const QString timeText = headers.value(QStringLiteral("x-device-time"));
	if (timeText == QStringLiteral("0") || timeText == QStringLiteral("1")) {
		device.timeSynchronized = timeText == QStringLiteral("1");
	}

	return device;
}

} // namespace xbotgo

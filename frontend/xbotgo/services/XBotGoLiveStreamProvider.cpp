#include "XBotGoLiveStreamProvider.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QTimer>

#include <utility>

namespace XBotGo {

static QString calculateSignature(const QString &secret, const QString &path, const QString &timestamp)
{
	const QByteArray source = (secret + path + timestamp).toUtf8();
	return QString::fromLatin1(QCryptographicHash::hash(source, QCryptographicHash::Md5).toHex());
}

LiveStreamConfig generateLiveStreamConfig(const QString &pushDomain, const QString &pushSecret,
					  const QString &pullDomain, const QString &pullSecret,
					  const QString &streamName)
{
	constexpr qint64 urlLifetimeSeconds = 10 * 60;
	const QString timestamp = QString::number(QDateTime::currentSecsSinceEpoch() + urlLifetimeSeconds);
	const QString path = QStringLiteral("/live/%1").arg(streamName);
	const QString pushSignature = calculateSignature(pushSecret, path, timestamp);
	const QString pullSignature = calculateSignature(pullSecret, path, timestamp);

	LiveStreamConfig config;
	config.pushServer = QStringLiteral("rtmp://%1/live").arg(pushDomain);
	config.pushStreamKey =
		QStringLiteral("%1?ts=%2&sign=%3").arg(streamName, timestamp, pushSignature);
	config.pullServer = QStringLiteral("rtmp://%1/live").arg(pullDomain);
	config.pullStreamKey =
		QStringLiteral("%1?ts=%2&sign=%3").arg(streamName, timestamp, pullSignature);
	return config;
}

void HardcodedLiveStreamProvider::requestLiveStreamConfig(QObject *context, LiveStreamConfigCallback callback)
{
	QTimer::singleShot(0, context, [callback = std::move(callback)] {
		const QString pushDomain = QStringLiteral("test-live-push.xbotgo.cn");
		const QString pushSecret = QStringLiteral("FU1kXTADjUHBCCGZh3jf");
		const QString pullDomain = QStringLiteral("test-live-pull.xbotgo.cn");
		const QString pullSecret = QStringLiteral("DW1kXTskDUHBCCGZdsed");
		const QString streamName = QStringLiteral("xbotgo-test");
		LiveStreamConfig config =
			generateLiveStreamConfig(pushDomain, pushSecret, pullDomain, pullSecret, streamName);
		callback(std::move(config), {});
	});
}

} // namespace XBotGo

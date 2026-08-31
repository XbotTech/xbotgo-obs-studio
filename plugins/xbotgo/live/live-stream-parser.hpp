#pragma once

#include "live-stream-config.hpp"

#include <QByteArray>
#include <QString>

#include <optional>

namespace xbotgo {

std::optional<LiveStreamConfig> ParseLiveStreamUrls(const QString &pushUrl, const QString &pullUrl, QString &error);
std::optional<LiveStreamConfig> ParseLiveStreamResponse(const QByteArray &response, QString &error);

} // namespace xbotgo

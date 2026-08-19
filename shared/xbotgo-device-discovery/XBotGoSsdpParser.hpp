#pragma once

#include "XBotGoDevice.hpp"

#include <QByteArray>
#include <QHostAddress>

#include <optional>

namespace XBotGo {

std::optional<Device> parseSsdpDevice(const QByteArray &payload, const QHostAddress &senderAddress);

} // namespace XBotGo

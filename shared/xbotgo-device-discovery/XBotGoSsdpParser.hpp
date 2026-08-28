#pragma once

#include "XBotGoDevice.hpp"

#include <QByteArray>
#include <QHostAddress>

#include <optional>

namespace xbotgo {

std::optional<Device> parseSsdpDevice(const QByteArray &payload, const QHostAddress &senderAddress);

} // namespace xbotgo

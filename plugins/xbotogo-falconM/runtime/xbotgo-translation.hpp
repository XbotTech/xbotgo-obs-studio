#pragma once

#include <QString>

#include <obs-module.h>

namespace xbotgo {

inline QString Tr(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

} // namespace xbotgo

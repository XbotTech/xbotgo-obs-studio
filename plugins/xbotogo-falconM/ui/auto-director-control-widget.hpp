#pragma once

#include <QWidget>

class QString;

namespace xbotgo {

class AutoDirector;

class AutoDirectorControlWidget final : public QWidget {
public:
	explicit AutoDirectorControlWidget(AutoDirector &director, const QString &title, const QString &cooldownLabel,
					   const QString &secondsSuffix, QWidget *parent = nullptr);
};

} // namespace xbotgo

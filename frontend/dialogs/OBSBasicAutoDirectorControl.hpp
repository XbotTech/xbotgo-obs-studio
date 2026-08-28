#pragma once

#include <QWidget>

class QString;

namespace xbotgo {
class AutoDirector;
}

class OBSBasicAutoDirectorControl final : public QWidget {
public:
	explicit OBSBasicAutoDirectorControl(xbotgo::AutoDirector &director, const QString &title,
					     const QString &cooldownLabel, const QString &secondsSuffix,
					     QWidget *parent = nullptr);
};

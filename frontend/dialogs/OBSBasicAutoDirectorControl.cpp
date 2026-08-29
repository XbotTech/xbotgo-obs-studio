#include "OBSBasicAutoDirectorControl.hpp"

#include <Idian/ToggleSwitch.hpp>
#include <xbotgo/director/XBotGoAutoDirector.hpp>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

OBSBasicAutoDirectorControl::OBSBasicAutoDirectorControl(xbotgo::AutoDirector &director, const QString &title,
							 const QString &cooldownLabel, const QString &secondsSuffix,
							 QWidget *parent)
	: QWidget(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	auto *panel = new QFrame(this);
	panel->setFrameShape(QFrame::StyledPanel);
	panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	auto *titleLabel = new QLabel(title, panel);
	titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	auto titleFont = titleLabel->font();
	titleFont.setBold(true);
	titleLabel->setFont(titleFont);

	auto *enabled = new idian::ToggleSwitch(director.isStarted(), panel);
	enabled->setObjectName(QStringLiteral("autoDirectorEnabled"));
	enabled->setAccessibleName(title);

	auto *headerLayout = new QHBoxLayout;
	headerLayout->addWidget(titleLabel, 1);
	headerLayout->addWidget(enabled);

	auto *settings = new QWidget(panel);
	settings->setObjectName(QStringLiteral("autoDirectorSettings"));
	auto *cooldownTitle = new QLabel(cooldownLabel, settings);
	auto *cooldown = new QSpinBox(settings);
	cooldown->setObjectName(QStringLiteral("autoDirectorCooldown"));
	cooldown->setRange(xbotgo::AutoDirector::MinimumSwitchCooldownSeconds,
			   xbotgo::AutoDirector::MaximumSwitchCooldownSeconds);
	cooldown->setSingleStep(1);
	cooldown->setValue(director.switchCooldownSeconds());
	cooldown->setSuffix(secondsSuffix);
	cooldown->setAccessibleName(cooldownLabel);
	settings->setEnabled(director.isStarted());

	auto *settingsLayout = new QHBoxLayout(settings);
	settingsLayout->setContentsMargins(0, 0, 0, 0);
	settingsLayout->addWidget(cooldownTitle);
	settingsLayout->addWidget(cooldown);

	auto *panelLayout = new QVBoxLayout(panel);
	panelLayout->setContentsMargins(10, 8, 10, 8);
	panelLayout->setSpacing(8);
	panelLayout->addLayout(headerLayout);
	panelLayout->addWidget(settings);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 0, 4, 4);
	layout->addWidget(panel);

	connect(enabled, &idian::ToggleSwitch::toggled, settings, &QWidget::setEnabled);
	connect(enabled, &idian::ToggleSwitch::toggled, &director, [&director](bool checked) {
		if (checked) {
			director.start();
		} else {
			director.stop();
		}
	});
	connect(cooldown, &QSpinBox::valueChanged, &director,
		[&director](int seconds) { director.setSwitchCooldownSeconds(seconds); });
}

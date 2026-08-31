#include "falconm-devices-widget.hpp"
#include "auto-director-control-widget.hpp"
#include "falconm-control-widget.hpp"

#include "moc_falconm-devices-widget.cpp"

#include <obs.h>
#include "../falconm-source-bridge.hpp"
#include "../runtime/xbotgo-translation.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

namespace xbotgo {

class FalconMDeviceCard : public QFrame {
	FalconMSourceBridge sourceBridge;
	QLabel *connectionIndicator = nullptr;
	QLabel *title = nullptr;
	QToolButton *toggle = nullptr;
	FalconMControlWidget *control = nullptr;

public:
	explicit FalconMDeviceCard(obs_source_t *source_, QWidget *parent = nullptr)
		: QFrame(parent),
		  sourceBridge(source_)
	{
		setFrameShape(QFrame::StyledPanel);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

		connectionIndicator = new QLabel(this);
		connectionIndicator->setFixedSize(10, 10);
		title = new QLabel(QString::fromUtf8(obs_source_get_name(source_)), this);
		title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		toggle = new QToolButton(this);
		toggle->setAutoRaise(true);
		toggle->setCheckable(true);
		toggle->setChecked(true);
		toggle->setArrowType(Qt::DownArrow);

		auto *header = new QHBoxLayout;
		header->addWidget(connectionIndicator);
		header->addWidget(title, 1);
		header->addWidget(toggle);

		control = new FalconMControlWidget(source_, this);
		auto *layout = new QVBoxLayout(this);
		layout->addLayout(header);
		layout->addWidget(control);

		connect(toggle, &QToolButton::toggled, this, [this](bool expanded) {
			control->setVisible(expanded);
			toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
		});
		connect(control, &FalconMControlWidget::connectionStateChanged, this,
			[this](bool connected) { SetConnected(connected); });
		SetConnected(FalconMSourceBridge(source_).connected());
	}

	void SetSourceName(const QString &name) { title->setText(name); }
	QString SourceId() const { return QString::fromStdString(sourceBridge.uuid()); }

private:
	void SetConnected(bool connected)
	{
		connectionIndicator->setStyleSheet(
			QStringLiteral("background-color: %1; border-radius: 5px;")
				.arg(connected ? QStringLiteral("#34c759") : QStringLiteral("#777777")));
	}
};

FalconMDevicesWidget::FalconMDevicesWidget(AutoDirector &director, QWidget *parent) : QWidget(parent)
{
	auto *contents = new QWidget(this);
	cardsLayout = new QVBoxLayout(contents);
	cardsLayout->setContentsMargins(8, 8, 8, 8);
	cardsLayout->setSpacing(8);
	cardsLayout->setAlignment(Qt::AlignTop);
	empty = new QLabel(Tr("Basic.MainMenu.XBotGo.DeviceManagement.Empty"), contents);
	empty->setAlignment(Qt::AlignCenter);
	cardsLayout->addWidget(empty);

	auto *scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setWidget(contents);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(scrollArea, 1);
	layout->addWidget(new AutoDirectorControlWidget(
		director, Tr("Basic.MainMenu.XBotGo.DeviceManagement.AutoDirector"),
		Tr("Basic.MainMenu.XBotGo.DeviceManagement.AutoDirector.SwitchCooldown"),
		Tr("Basic.MainMenu.XBotGo.DeviceManagement.AutoDirector.SecondsSuffix"), this));

	signal_handler_t *handler = obs_get_signal_handler();
	sourceSignals.emplace_back(handler, "source_create", SourceCreated, this);
	sourceSignals.emplace_back(handler, "source_remove", SourceRemoved, this);
	sourceSignals.emplace_back(handler, "source_destroy", SourceDestroyed, this);
	sourceSignals.emplace_back(handler, "source_rename", SourceRenamed, this);
	obs_enum_sources(EnumSource, this);
	UpdateEmptyState();
}

FalconMDevicesWidget::~FalconMDevicesWidget() = default;

bool FalconMDevicesWidget::EnumSource(void *data, obs_source_t *source)
{
	static_cast<FalconMDevicesWidget *>(data)->AddSource(OBSSource(source));
	return true;
}

void FalconMDevicesWidget::SourceCreated(void *data, calldata_t *calldata)
{
	auto *dialog = static_cast<FalconMDevicesWidget *>(data);
	OBSSource source(static_cast<obs_source_t *>(calldata_ptr(calldata, "source")));
	QMetaObject::invokeMethod(dialog, [dialog, source] { dialog->AddSource(source); });
}

void FalconMDevicesWidget::SourceRemoved(void *data, calldata_t *calldata)
{
	auto *dialog = static_cast<FalconMDevicesWidget *>(data);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(calldata, "source"));
	const char *sourceId = source ? obs_source_get_uuid(source) : nullptr;
	const QString id = QString::fromUtf8(sourceId ? sourceId : "");
	QMetaObject::invokeMethod(dialog, [dialog, id] { dialog->RemoveSource(id); });
}

void FalconMDevicesWidget::SourceDestroyed(void *data, calldata_t *calldata)
{
	SourceRemoved(data, calldata);
}

void FalconMDevicesWidget::SourceRenamed(void *data, calldata_t *calldata)
{
	auto *dialog = static_cast<FalconMDevicesWidget *>(data);
	OBSSource source(static_cast<obs_source_t *>(calldata_ptr(calldata, "source")));
	const QString name = QString::fromUtf8(calldata_string(calldata, "new_name"));
	QMetaObject::invokeMethod(dialog, [dialog, source, name] { dialog->RenameSource(source, name); });
}

void FalconMDevicesWidget::AddSource(OBSSource source)
{
	obs_source_t *rawSource = source;
	if (!rawSource || obs_source_removed(rawSource)) {
		return;
	}
	const char *uuid = obs_source_get_uuid(rawSource);
	const QString sourceId = QString::fromUtf8(uuid ? uuid : "");
	if (!FalconMSourceBridge::IsFalconM(rawSource) || sourceId.isEmpty() || cards.contains(sourceId)) {
		return;
	}

	auto *card = new FalconMDeviceCard(rawSource, this);
	cards.insert(card->SourceId(), card);
	cardsLayout->addWidget(card);
	UpdateEmptyState();
}

void FalconMDevicesWidget::RemoveSource(const QString &sourceId)
{
	if (auto *card = cards.take(sourceId)) {
		cardsLayout->removeWidget(card);
		delete card;
		UpdateEmptyState();
	}
}

void FalconMDevicesWidget::RenameSource(OBSSource source, const QString &name)
{
	const char *uuid = source ? obs_source_get_uuid(source) : nullptr;
	if (auto *card = cards.value(QString::fromUtf8(uuid ? uuid : ""))) {
		card->SetSourceName(name);
	}
}

void FalconMDevicesWidget::UpdateEmptyState()
{
	empty->setVisible(cards.isEmpty());
}

} // namespace xbotgo

#include "OBSBasicFalconMDevices.hpp"
#include "OBSBasicAutoDirectorControl.hpp"
#include "OBSBasicFalconMControl.hpp"

#include <OBSApp.hpp>

#include "moc_OBSBasicFalconMDevices.cpp"

#include <obs.h>
#include <qt-wrappers.hpp>
#include <xbotgo/sources/XBotGoFalconMSource.hpp>
#include <xbotgo/sources/XBotGoSourceObserver.hpp>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <cstring>

namespace {
constexpr const char *FalconMSourceId = "xbotogo_falconm";
} // namespace

class FalconMDeviceCard : public QFrame {
	xbotgo::SourceObserver sourceObserver;
	QLabel *connectionIndicator = nullptr;
	QLabel *title = nullptr;
	QToolButton *toggle = nullptr;
	OBSBasicFalconMControl *control = nullptr;

public:
	explicit FalconMDeviceCard(obs_source_t *source_, QWidget *parent = nullptr)
		: QFrame(parent),
		  sourceObserver(source_)
	{
		setFrameShape(QFrame::StyledPanel);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

		connectionIndicator = new QLabel(this);
		connectionIndicator->setFixedSize(10, 10);
		title = new QLabel(QT_UTF8(obs_source_get_name(source_)), this);
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

		control = new OBSBasicFalconMControl(source_, this);
		auto *layout = new QVBoxLayout(this);
		layout->addLayout(header);
		layout->addWidget(control);

		connect(toggle, &QToolButton::toggled, this, [this](bool expanded) {
			control->setVisible(expanded);
			toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
		});
		connect(control, &OBSBasicFalconMControl::connectionStateChanged, this,
			[this](bool connected) { SetConnected(connected); });
		SetConnected(xbotgo::IsFalconMSourceConnected(source_));
	}

	void SetSourceName(const QString &name) { title->setText(name); }
	QString SourceId() const { return QString::fromStdString(sourceObserver.Id()); }

private:
	void SetConnected(bool connected)
	{
		connectionIndicator->setStyleSheet(
			QStringLiteral("background-color: %1; border-radius: 5px;")
				.arg(connected ? QStringLiteral("#34c759") : QStringLiteral("#777777")));
	}
};

OBSBasicFalconMDevices::OBSBasicFalconMDevices(xbotgo::AutoDirector &director, QWidget *parent) : QWidget(parent)
{
	auto *contents = new QWidget(this);
	cardsLayout = new QVBoxLayout(contents);
	cardsLayout->setContentsMargins(8, 8, 8, 8);
	cardsLayout->setSpacing(8);
	cardsLayout->setAlignment(Qt::AlignTop);
	empty = new QLabel(QTStr("Basic.MainMenu.XBotGo.DeviceManagement.Empty"), contents);
	empty->setAlignment(Qt::AlignCenter);
	cardsLayout->addWidget(empty);

	auto *scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setWidget(contents);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(scrollArea, 1);
	layout->addWidget(new OBSBasicAutoDirectorControl(
		director, QTStr("Basic.MainMenu.XBotGo.DeviceManagement.AutoDirector"),
		QTStr("Basic.MainMenu.XBotGo.DeviceManagement.AutoDirector.SwitchCooldown"),
		QTStr("Basic.MainMenu.XBotGo.DeviceManagement.AutoDirector.SecondsSuffix"), this));

	signal_handler_t *handler = obs_get_signal_handler();
	sourceSignals.emplace_back(handler, "source_create", SourceCreated, this);
	sourceSignals.emplace_back(handler, "source_remove", SourceRemoved, this);
	sourceSignals.emplace_back(handler, "source_destroy", SourceDestroyed, this);
	sourceSignals.emplace_back(handler, "source_rename", SourceRenamed, this);
	obs_enum_sources(EnumSource, this);
	UpdateEmptyState();
}

OBSBasicFalconMDevices::~OBSBasicFalconMDevices() = default;

bool OBSBasicFalconMDevices::EnumSource(void *data, obs_source_t *source)
{
	static_cast<OBSBasicFalconMDevices *>(data)->AddSource(OBSSource(source));
	return true;
}

void OBSBasicFalconMDevices::SourceCreated(void *data, calldata_t *calldata)
{
	auto *dialog = static_cast<OBSBasicFalconMDevices *>(data);
	OBSSource source(static_cast<obs_source_t *>(calldata_ptr(calldata, "source")));
	QMetaObject::invokeMethod(dialog, [dialog, source] { dialog->AddSource(source); });
}

void OBSBasicFalconMDevices::SourceRemoved(void *data, calldata_t *calldata)
{
	auto *dialog = static_cast<OBSBasicFalconMDevices *>(data);
	auto *source = static_cast<obs_source_t *>(calldata_ptr(calldata, "source"));
	const char *sourceId = source ? obs_source_get_uuid(source) : nullptr;
	const QString id = QT_UTF8(sourceId ? sourceId : "");
	QMetaObject::invokeMethod(dialog, [dialog, id] { dialog->RemoveSource(id); });
}

void OBSBasicFalconMDevices::SourceDestroyed(void *data, calldata_t *calldata)
{
	SourceRemoved(data, calldata);
}

void OBSBasicFalconMDevices::SourceRenamed(void *data, calldata_t *calldata)
{
	auto *dialog = static_cast<OBSBasicFalconMDevices *>(data);
	OBSSource source(static_cast<obs_source_t *>(calldata_ptr(calldata, "source")));
	const QString name = QT_UTF8(calldata_string(calldata, "new_name"));
	QMetaObject::invokeMethod(dialog, [dialog, source, name] { dialog->RenameSource(source, name); });
}

void OBSBasicFalconMDevices::AddSource(OBSSource source)
{
	obs_source_t *rawSource = source;
	if (!rawSource || obs_source_removed(rawSource)) {
		return;
	}
	const char *id = obs_source_get_id(rawSource);
	const char *uuid = obs_source_get_uuid(rawSource);
	const QString sourceId = QT_UTF8(uuid ? uuid : "");
	if (!id || strcmp(id, FalconMSourceId) != 0 || sourceId.isEmpty() || cards.contains(sourceId)) {
		return;
	}

	auto *card = new FalconMDeviceCard(rawSource, this);
	cards.insert(card->SourceId(), card);
	cardsLayout->addWidget(card);
	UpdateEmptyState();
}

void OBSBasicFalconMDevices::RemoveSource(const QString &sourceId)
{
	if (auto *card = cards.take(sourceId)) {
		cardsLayout->removeWidget(card);
		delete card;
		UpdateEmptyState();
	}
}

void OBSBasicFalconMDevices::RenameSource(OBSSource source, const QString &name)
{
	const char *uuid = source ? obs_source_get_uuid(source) : nullptr;
	if (auto *card = cards.value(QT_UTF8(uuid ? uuid : ""))) {
		card->SetSourceName(name);
	}
}

void OBSBasicFalconMDevices::UpdateEmptyState()
{
	empty->setVisible(cards.isEmpty());
}

#include "live-stream-config-dialog.hpp"

#include "../runtime/xbotgo-translation.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QUrl>
#include <QVBoxLayout>

namespace xbotgo {

namespace {

bool IsValidRtmpUrl(const QString &value)
{
	const QUrl url(value, QUrl::StrictMode);
	const QString scheme = url.scheme().toLower();
	return url.isValid() && !url.host().isEmpty() &&
	       (scheme == QStringLiteral("rtmp") || scheme == QStringLiteral("rtmps"));
}

} // namespace

LiveStreamConfigDialog::LiveStreamConfigDialog(const LiveStreamConfig &config, QWidget *parent) : QDialog(parent)
{
	setWindowTitle(Tr("Basic.MainMenu.XBotGo.LiveConfig.Title"));
	setModal(true);

	pushServerEdit_ = new QLineEdit(config.pushServer, this);
	pushStreamKeyEdit_ = new QLineEdit(config.pushStreamKey, this);
	pullUrlEdit_ = new QLineEdit(config.pullUrl(), this);
	pullUrlEdit_->setReadOnly(true);
	pullUrlEdit_->setCursorPosition(0);
	pullServer_ = config.pullServer;
	pullStreamKey_ = config.pullStreamKey;
	taskId_ = config.taskId;

	auto *formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setHorizontalSpacing(16);
	formLayout->setVerticalSpacing(16);
	formLayout->addRow(Tr("Basic.MainMenu.XBotGo.LiveConfig.PushServer"), pushServerEdit_);
	formLayout->addRow(Tr("Basic.MainMenu.XBotGo.LiveConfig.PushStreamKey"), pushStreamKeyEdit_);
	formLayout->addRow(Tr("Basic.MainMenu.XBotGo.LiveConfig.PullUrl"), pullUrlEdit_);

	auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &LiveStreamConfigDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &LiveStreamConfigDialog::reject);

	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(24, 24, 24, 24);
	mainLayout->setSpacing(24);
	mainLayout->addLayout(formLayout);
	mainLayout->addStretch();
	mainLayout->addWidget(buttonBox);

	setMinimumSize(560, 280);
	resize(640, 320);
}

LiveStreamConfig LiveStreamConfigDialog::liveStreamConfig() const
{
	return {pushServerEdit_->text().trimmed(), pushStreamKeyEdit_->text().trimmed(), pullServer_, pullStreamKey_,
		taskId_};
}

void LiveStreamConfigDialog::accept()
{
	const LiveStreamConfig config = liveStreamConfig();
	if (!IsValidRtmpUrl(config.pushServer) || config.pushStreamKey.isEmpty() ||
	    !IsValidRtmpUrl(config.pullServer) || config.pullStreamKey.isEmpty()) {
		QMessageBox::warning(this, Tr("Basic.MainMenu.XBotGo.LiveConfig.Invalid.Title"),
				     Tr("Basic.MainMenu.XBotGo.LiveConfig.Invalid.Text"));
		return;
	}
	QDialog::accept();
}

} // namespace xbotgo

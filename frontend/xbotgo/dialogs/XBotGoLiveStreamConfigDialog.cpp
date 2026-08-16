#include "XBotGoLiveStreamConfigDialog.hpp"

#include <OBSApp.hpp>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace XBotGo {

static bool isValidRtmpUrl(const QString &value)
{
	const QUrl url(value, QUrl::StrictMode);
	const QString scheme = url.scheme().toLower();
	return url.isValid() && !url.host().isEmpty() &&
	       (scheme == QStringLiteral("rtmp") || scheme == QStringLiteral("rtmps"));
}

LiveStreamConfigDialog::LiveStreamConfigDialog(const LiveStreamConfig &config, QWidget *parent) : QDialog(parent)
{
	setWindowTitle(QTStr("Basic.MainMenu.XBotGo.LiveConfig.Title"));
	setModal(true);

	pushServerEdit = new QLineEdit(config.pushServer, this);
	pushStreamKeyEdit = new QLineEdit(config.pushStreamKey, this);
	pullUrlEdit = new QLineEdit(config.pullUrl(), this);
	pullUrlEdit->setReadOnly(true);
	pullUrlEdit->setCursorPosition(0);
	pullServer = config.pullServer;
	pullStreamKey = config.pullStreamKey;

	auto formLayout = new QFormLayout;
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setHorizontalSpacing(16);
	formLayout->setVerticalSpacing(16);
	formLayout->addRow(QTStr("Basic.MainMenu.XBotGo.LiveConfig.PushServer"), pushServerEdit);
	formLayout->addRow(QTStr("Basic.MainMenu.XBotGo.LiveConfig.PushStreamKey"), pushStreamKeyEdit);
	formLayout->addRow(QTStr("Basic.MainMenu.XBotGo.LiveConfig.PullUrl"), pullUrlEdit);

	auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &LiveStreamConfigDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &LiveStreamConfigDialog::reject);

	auto mainLayout = new QVBoxLayout(this);
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
	return {pushServerEdit->text().trimmed(), pushStreamKeyEdit->text().trimmed(),
		pullServer, pullStreamKey};
}

void LiveStreamConfigDialog::accept()
{
	const LiveStreamConfig config = liveStreamConfig();
	if (!isValidRtmpUrl(config.pushServer) || config.pushStreamKey.isEmpty() ||
	    !isValidRtmpUrl(config.pullServer) || config.pullStreamKey.isEmpty()) {
		QMessageBox::warning(this, QTStr("Basic.MainMenu.XBotGo.LiveConfig.Invalid.Title"),
				     QTStr("Basic.MainMenu.XBotGo.LiveConfig.Invalid.Text"));
		return;
	}

	QDialog::accept();
}

} // namespace XBotGo

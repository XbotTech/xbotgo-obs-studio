#include "live-stream-runtime.hpp"

#include "live-stream-config-dialog.hpp"
#include "../runtime/xbotgo-translation.hpp"

#include <QAction>
#include <QMessageBox>
#include <QTimer>

#include <obs-frontend-api.h>
#include <obs.hpp>

#include <utility>

namespace xbotgo {
namespace {

constexpr int HeartbeatIntervalMs = 10'000;

} // namespace

LiveStreamRuntime::LiveStreamRuntime(QAction &startAction, QWidget &dialogParent, QObject *parent)
	: QObject(parent),
	  startAction_(&startAction),
	  dialogParent_(&dialogParent),
	  client_(this)
{
	heartbeatTimer_.setInterval(HeartbeatIntervalMs);
	connect(&heartbeatTimer_, &QTimer::timeout, this, &LiveStreamRuntime::sendHeartbeat);
	connect(&startAction, &QAction::triggered, this, &LiveStreamRuntime::start);
	setActionForPhase();
}

void LiveStreamRuntime::start()
{
	if (shutdown_ || obs_frontend_streaming_active() || !session_.beginFetch()) {
		return;
	}
	setActionForPhase();
	client_.requestStart(this, [this](std::optional<LiveStreamConfig> config, QString error) {
		if (shutdown_ || session_.phase() != LiveStreamPhase::Fetching) {
			return;
		}
		if (!config) {
			session_.finish();
			setActionForPhase();
			if (dialogParent_) {
				QMessageBox::critical(dialogParent_, Tr("Basic.MainMenu.XBotGo.LiveConfig.Error.Title"),
						      Tr("Basic.MainMenu.XBotGo.LiveConfig.Error.Text").arg(error));
			}
			return;
		}

		heartbeatTaskId_ = config->taskId;
		if (!session_.beginConfirming(config->taskId)) {
			stopTaskAndReset();
			return;
		}
		setActionForPhase();
		LiveStreamConfigDialog dialog(*config, dialogParent_);
		if (dialog.exec() != QDialog::Accepted) {
			stopTaskAndReset();
			return;
		}
		const LiveStreamConfig selected = dialog.liveStreamConfig();
		if (!installService(selected) || !session_.beginStarting()) {
			if (dialogParent_) {
				QMessageBox::critical(dialogParent_, Tr("Basic.MainMenu.XBotGo.LiveConfig.Error.Title"),
						      Tr("Basic.MainMenu.XBotGo.LiveConfig.ServiceError"));
			}
			stopTaskAndReset();
			return;
		}
		setActionForPhase();
		obs_frontend_streaming_start();
		QTimer::singleShot(0, this, [this] {
			if (session_.phase() == LiveStreamPhase::Starting && !session_.startingEventObserved() &&
			    !obs_frontend_streaming_active()) {
				stopTaskAndReset();
			}
		});
	});
}

void LiveStreamRuntime::handleFrontendEvent(obs_frontend_event event)
{
	if (shutdown_) {
		return;
	}
	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTING:
		if (session_.observeStreamingStarting()) {
			heartbeatTimer_.start();
			sendHeartbeat();
		}
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		session_.observeStreamingStarted();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		session_.observeStreamingStopping();
		heartbeatTimer_.stop();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		heartbeatTimer_.stop();
		stopTaskAndReset();
		break;
	default:
		break;
	}
	setActionForPhase();
}

void LiveStreamRuntime::shutdown()
{
	if (shutdown_) {
		return;
	}
	shutdown_ = true;
	heartbeatTimer_.stop();
	heartbeatInFlight_ = false;
	client_.abortAll();
	if (auto taskId = session_.finish()) {
		client_.requestStop(this, std::move(*taskId));
	}
	heartbeatTaskId_.clear();
	if (startAction_) {
		startAction_->setEnabled(false);
	}
}

void LiveStreamRuntime::setActionForPhase()
{
	if (!startAction_) {
		return;
	}
	startAction_->setText(session_.phase() == LiveStreamPhase::Fetching
				      ? Tr("Basic.MainMenu.XBotGo.StartStreaming.Fetching")
				      : Tr("Basic.MainMenu.XBotGo.StartStreaming"));
	startAction_->setEnabled(!shutdown_ && session_.phase() == LiveStreamPhase::Idle &&
				 !obs_frontend_streaming_active());
}

void LiveStreamRuntime::sendHeartbeat()
{
	if (shutdown_ || heartbeatInFlight_ || heartbeatTaskId_.isEmpty()) {
		return;
	}
	heartbeatInFlight_ = true;
	client_.requestHeartbeat(this, heartbeatTaskId_, [this](QString) { heartbeatInFlight_ = false; });
}

void LiveStreamRuntime::stopTaskAndReset()
{
	heartbeatTimer_.stop();
	heartbeatInFlight_ = false;
	if (auto taskId = session_.finish()) {
		client_.requestStop(this, std::move(*taskId));
	}
	heartbeatTaskId_.clear();
	setActionForPhase();
}

bool LiveStreamRuntime::installService(const LiveStreamConfig &config)
{
	OBSDataAutoRelease settings = obs_data_create();
	const QByteArray server = config.pushServer.toUtf8();
	const QByteArray key = config.pushStreamKey.toUtf8();
	obs_data_set_string(settings, "server", server.constData());
	obs_data_set_string(settings, "key", key.constData());
	OBSServiceAutoRelease service = obs_service_create("rtmp_custom", "default_service", settings, nullptr);
	if (!service) {
		return false;
	}
	obs_frontend_set_streaming_service(service);
	obs_frontend_save_streaming_service();
	return true;
}

} // namespace xbotgo

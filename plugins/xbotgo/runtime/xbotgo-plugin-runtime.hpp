#pragma once

#include <QObject>
#include <QPointer>

#include <memory>

#include <obs-frontend-api.h>

class QDockWidget;
class QMenu;

namespace xbotgo {

class AutoDirector;
class LiveStreamRuntime;

class XBotGoPluginRuntime final : public QObject {
	Q_OBJECT

public:
	explicit XBotGoPluginRuntime(QObject *parent = nullptr);
	~XBotGoPluginRuntime() override;
	void requestInitialize();
	void shutdown();

private:
	static void FrontendEvent(obs_frontend_event event, void *data);
	void initializeFrontend();
	void handleFrontendEvent(obs_frontend_event event);

	bool initialized_ = false;
	bool callbackRegistered_ = false;
	QPointer<QMenu> menu_;
	QPointer<QDockWidget> dock_;
	std::unique_ptr<AutoDirector> director_;
	std::unique_ptr<LiveStreamRuntime> live_;
};

} // namespace xbotgo

#include "xbotgo-plugin-runtime.hpp"

#include "../director/auto-director.hpp"
#include "../live/live-stream-runtime.hpp"
#include "../ui/falconm-devices-widget.hpp"
#include "xbotgo-translation.hpp"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>

#include <obs-frontend-api.h>

namespace xbotgo {

XBotGoPluginRuntime::XBotGoPluginRuntime(QObject *parent) : QObject(parent) {}

XBotGoPluginRuntime::~XBotGoPluginRuntime()
{
	shutdown();
}

void XBotGoPluginRuntime::requestInitialize()
{
	if (!callbackRegistered_) {
		obs_frontend_add_event_callback(FrontendEvent, this);
		callbackRegistered_ = true;
	}
	QMetaObject::invokeMethod(this, &XBotGoPluginRuntime::initializeFrontend, Qt::QueuedConnection);
}

void XBotGoPluginRuntime::shutdown()
{
	if (callbackRegistered_) {
		obs_frontend_remove_event_callback(FrontendEvent, this);
		callbackRegistered_ = false;
	}
	if (live_) {
		live_->shutdown();
		live_.reset();
	}
	if (director_) {
		director_->stop();
		director_.reset();
	}
	if (dock_) {
		delete dock_;
		dock_.clear();
	}
	if (menu_) {
		delete menu_;
		menu_.clear();
	}
	initialized_ = false;
}

void XBotGoPluginRuntime::FrontendEvent(obs_frontend_event event, void *data)
{
	static_cast<XBotGoPluginRuntime *>(data)->handleFrontendEvent(event);
}

void XBotGoPluginRuntime::initializeFrontend()
{
	if (initialized_) {
		return;
	}
	auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!mainWindow || !mainWindow->menuBar()) {
		return;
	}

	menu_ = new QMenu(Tr("Basic.MainMenu.XBotGo"), mainWindow->menuBar());
	menu_->setObjectName(QStringLiteral("menuXBotGo"));
	auto *deviceAction = menu_->addAction(Tr("Basic.MainMenu.XBotGo.DeviceManagement"));
	auto *startAction = menu_->addAction(Tr("Basic.MainMenu.XBotGo.StartStreaming"));
	QMenu *helpMenu = mainWindow->findChild<QMenu *>(QStringLiteral("menuBasic_MainMenu_Help"));
	if (!helpMenu) {
		helpMenu = mainWindow->findChild<QMenu *>(QStringLiteral("menuHelp"));
	}
	if (helpMenu) {
		mainWindow->menuBar()->insertMenu(helpMenu->menuAction(), menu_);
	} else {
		mainWindow->menuBar()->addMenu(menu_);
	}

	director_ = std::make_unique<AutoDirector>(this);
	director_->start();
	dock_ = new QDockWidget(Tr("Basic.MainMenu.XBotGo.DeviceManagement"), mainWindow);
	dock_->setObjectName(QStringLiteral("xbotgoDeviceManagementDock"));
	dock_->setAllowedAreas(Qt::RightDockWidgetArea);
	dock_->setFeatures(QDockWidget::DockWidgetClosable);
	dock_->setMinimumWidth(500);
	dock_->setWidget(new FalconMDevicesWidget(*director_, dock_));
	mainWindow->addDockWidget(Qt::RightDockWidgetArea, dock_);

	connect(deviceAction, &QAction::triggered, dock_, [this] {
		if (dock_) {
			dock_->show();
			dock_->raise();
			dock_->setFocus(Qt::ShortcutFocusReason);
		}
	});
	live_ = std::make_unique<LiveStreamRuntime>(*startAction, *mainWindow, this);
	initialized_ = true;
}

void XBotGoPluginRuntime::handleFrontendEvent(obs_frontend_event event)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		initializeFrontend();
	}
	if (event == OBS_FRONTEND_EVENT_EXIT) {
		if (live_) {
			live_->shutdown();
		}
		if (director_) {
			director_->stop();
		}
		return;
	}
	if (live_) {
		live_->handleFrontendEvent(event);
	}
}

} // namespace xbotgo

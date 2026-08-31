#pragma once

#include <QHash>
#include <QString>
#include <QWidget>
#include <obs.hpp>

#include <vector>

class QLabel;
class QVBoxLayout;

namespace xbotgo {
class AutoDirector;
class FalconMDeviceCard;

class FalconMDevicesWidget : public QWidget {
	Q_OBJECT

	QVBoxLayout *cardsLayout = nullptr;
	QLabel *empty = nullptr;
	QHash<QString, FalconMDeviceCard *> cards;
	std::vector<OBSSignal> sourceSignals;

public:
	explicit FalconMDevicesWidget(AutoDirector &director, QWidget *parent = nullptr);
	~FalconMDevicesWidget() override;

private:
	static bool EnumSource(void *data, obs_source_t *source);
	static void SourceCreated(void *data, calldata_t *calldata);
	static void SourceRemoved(void *data, calldata_t *calldata);
	static void SourceDestroyed(void *data, calldata_t *calldata);
	static void SourceRenamed(void *data, calldata_t *calldata);

	void AddSource(OBSSource source);
	void RemoveSource(const QString &sourceId);
	void RenameSource(OBSSource source, const QString &name);
	void UpdateEmptyState();
};

} // namespace xbotgo

#pragma once

#include <QDialog>
#include <QHash>
#include <obs.hpp>

#include <vector>

class QLabel;
class QVBoxLayout;
class FalconMDeviceCard;

class OBSBasicFalconMDevices : public QDialog {
	Q_OBJECT

	QVBoxLayout *cardsLayout = nullptr;
	QLabel *empty = nullptr;
	QHash<obs_source_t *, FalconMDeviceCard *> cards;
	std::vector<OBSSignal> sourceSignals;

public:
	explicit OBSBasicFalconMDevices(QWidget *parent = nullptr);
	~OBSBasicFalconMDevices() override;

private:
	static bool EnumSource(void *data, obs_source_t *source);
	static void SourceCreated(void *data, calldata_t *calldata);
	static void SourceRemoved(void *data, calldata_t *calldata);
	static void SourceRenamed(void *data, calldata_t *calldata);

	void AddSource(OBSSource source);
	void RemoveSource(OBSSource source);
	void RenameSource(OBSSource source, const QString &name);
	void UpdateEmptyState();
};

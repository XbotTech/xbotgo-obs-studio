#pragma once

#include <QDialog>

namespace XBotGo {

class SliderControlDemoDialog final : public QDialog {
	Q_OBJECT

public:
	explicit SliderControlDemoDialog(QWidget *parent = nullptr);
};

} // namespace XBotGo

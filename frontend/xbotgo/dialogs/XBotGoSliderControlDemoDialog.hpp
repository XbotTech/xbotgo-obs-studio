#pragma once

#include <QDialog>

namespace xbotgo {

class SliderControlDemoDialog final : public QDialog {
	Q_OBJECT

public:
	explicit SliderControlDemoDialog(QWidget *parent = nullptr);
};

} // namespace xbotgo

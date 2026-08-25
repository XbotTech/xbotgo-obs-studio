#include "XBotGoSliderControlDemoDialog.hpp"

#include "../components/XBotGoSliderControl.hpp"

#include <QHBoxLayout>

#include "moc_XBotGoSliderControlDemoDialog.cpp"

namespace XBotGo {

SliderControlDemoDialog::SliderControlDemoDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(QStringLiteral("Slider control demo"));

	auto angleControl = new SliderControl(this);
	angleControl->setTitle(QStringLiteral("强制角度限制"));
	angleControl->setRange(60, 150);
	angleControl->setSingleStep(1);
	angleControl->setValueFormatter([](int value) { return QStringLiteral("%1°").arg(value); });
	angleControl->setValue(120);

	auto zoomControl = new SliderControl(this);
	zoomControl->setTitle(QStringLiteral("手动变焦"));
	zoomControl->setRange(10, 20);
	zoomControl->setSingleStep(1);
	zoomControl->setValueFormatter(
		[](int value) { return QStringLiteral("%1X").arg(static_cast<double>(value) / 10.0, 0, 'f', 1); });
	zoomControl->setValue(13);

	auto mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(20, 20, 20, 20);
	mainLayout->setSpacing(20);
	mainLayout->addWidget(angleControl);
	mainLayout->addWidget(zoomControl);

	resize(520, sizeHint().height());
}

} // namespace XBotGo

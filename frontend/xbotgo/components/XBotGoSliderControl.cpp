#include "XBotGoSliderControl.hpp"

#include <components/AbsoluteSlider.hpp>

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <utility>

#include "moc_XBotGoSliderControl.cpp"

namespace xbotgo {

SliderControl::SliderControl(QWidget *parent) : QWidget(parent)
{
	titleLabel = new QLabel(this);
	valueLabel = new QLabel(this);
	minimumLabel = new QLabel(this);
	maximumLabel = new QLabel(this);
	slider = new AbsoluteSlider(Qt::Horizontal, this);

	titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	minimumLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	maximumLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	titleLabel->setBuddy(slider);
	slider->setFocusPolicy(Qt::StrongFocus);
	slider->setMinimumHeight(18);
	slider->setStyleSheet(QStringLiteral(R"(
		QSlider::groove:horizontal {
			background: palette(mid);
			border: none;
			border-radius: 2px;
			height: 4px;
		}
		QSlider::sub-page:horizontal {
			background: palette(highlight);
			border: none;
			border-radius: 2px;
		}
		QSlider::add-page:horizontal:disabled,
		QSlider::sub-page:horizontal:disabled {
			background: palette(midlight);
		}
		QSlider::handle:horizontal {
			background: palette(text);
			border: none;
			border-radius: 5px;
			height: 10px;
			margin: -3px 0;
			width: 10px;
		}
		QSlider::handle:horizontal:hover {
			background: palette(highlighted-text);
		}
		QSlider::handle:horizontal:disabled {
			background: palette(mid);
		}
	)"));

	auto headerLayout = new QHBoxLayout;
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(8);
	headerLayout->addWidget(titleLabel);
	headerLayout->addWidget(valueLabel);

	auto rangeLayout = new QHBoxLayout;
	rangeLayout->setContentsMargins(0, 0, 0, 0);
	rangeLayout->setSpacing(8);
	rangeLayout->addWidget(minimumLabel);
	rangeLayout->addStretch();
	rangeLayout->addWidget(maximumLabel);

	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(2);
	mainLayout->addLayout(headerLayout);
	mainLayout->addWidget(slider);
	mainLayout->addLayout(rangeLayout);

	connect(slider, &AbsoluteSlider::valueChanged, this, [this](int value) {
		updateValueLabels();
		emit valueChanged(value);
	});
	connect(slider, &AbsoluteSlider::sliderPressed, this, &SliderControl::sliderPressed);
	connect(slider, &AbsoluteSlider::sliderReleased, this, &SliderControl::sliderReleased);

	updateValueLabels();
}

void SliderControl::setTitle(const QString &title)
{
	titleLabel->setText(title);
	slider->setAccessibleName(title);
}

void SliderControl::setRange(int minimum, int maximum)
{
	slider->setRange(minimum, maximum);
	updateValueLabels();
}

void SliderControl::setMinimum(int minimum)
{
	slider->setMinimum(minimum);
	updateValueLabels();
}

void SliderControl::setMaximum(int maximum)
{
	slider->setMaximum(maximum);
	updateValueLabels();
}

void SliderControl::setSingleStep(int step)
{
	slider->setSingleStep(step);
}

void SliderControl::setValue(int value)
{
	slider->setValue(value);
}

int SliderControl::value() const
{
	return slider->value();
}

void SliderControl::setValueFormatter(ValueFormatter formatter)
{
	valueFormatter = std::move(formatter);
	updateValueLabels();
}

QString SliderControl::formatValue(int value) const
{
	return valueFormatter ? valueFormatter(value) : QString::number(value);
}

void SliderControl::updateValueLabels()
{
	valueLabel->setText(formatValue(slider->value()));
	minimumLabel->setText(formatValue(slider->minimum()));
	maximumLabel->setText(formatValue(slider->maximum()));
}

} // namespace xbotgo

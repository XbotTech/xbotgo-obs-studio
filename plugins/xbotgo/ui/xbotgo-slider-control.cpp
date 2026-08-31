#include "xbotgo-slider-control.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <utility>

#include "moc_xbotgo-slider-control.cpp"

namespace xbotgo {
namespace {

class ControlSlider final : public QSlider {
public:
	using QSlider::QSlider;

protected:
	void mousePressEvent(QMouseEvent *event) override
	{
		if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
			setSliderDown(true);
			setValue(valueAt(event->position().toPoint()));
			event->accept();
			return;
		}
		QSlider::mousePressEvent(event);
	}

	void mouseMoveEvent(QMouseEvent *event) override
	{
		if (isSliderDown()) {
			setValue(valueAt(event->position().toPoint()));
		}
		QSlider::mouseMoveEvent(event);
	}

	void mouseReleaseEvent(QMouseEvent *event) override
	{
		if (isSliderDown()) {
			setSliderDown(false);
			event->accept();
			return;
		}
		QSlider::mouseReleaseEvent(event);
	}

	void wheelEvent(QWheelEvent *event) override { event->ignore(); }

private:
	int valueAt(const QPoint &position) const
	{
		QStyleOptionSlider option;
		initStyleOption(&option);
		const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
		const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
		const int handleLength = orientation() == Qt::Horizontal ? handle.width() : handle.height();
		const int minimumPosition =
			(orientation() == Qt::Horizontal ? groove.left() : groove.top()) + handleLength / 2;
		const int maximumPosition =
			(orientation() == Qt::Horizontal ? groove.right() : groove.bottom()) - handleLength / 2 + 1;
		const int positionValue = orientation() == Qt::Horizontal ? position.x() : position.y();
		return QStyle::sliderValueFromPosition(minimum(), maximum(), positionValue - minimumPosition,
						       maximumPosition - minimumPosition, option.upsideDown);
	}
};

} // namespace

SliderControl::SliderControl(QWidget *parent) : QWidget(parent)
{
	titleLabel = new QLabel(this);
	valueLabel = new QLabel(this);
	minimumLabel = new QLabel(this);
	maximumLabel = new QLabel(this);
	slider = new ControlSlider(Qt::Horizontal, this);

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

	connect(slider, &QSlider::valueChanged, this, [this](int value) {
		updateValueLabels();
		emit valueChanged(value);
	});
	connect(slider, &QSlider::sliderPressed, this, &SliderControl::sliderPressed);
	connect(slider, &QSlider::sliderReleased, this, &SliderControl::sliderReleased);

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

#pragma once

#include <QWidget>

#include <functional>

class AbsoluteSlider;
class QLabel;

namespace XBotGo {

class SliderControl final : public QWidget {
	Q_OBJECT

public:
	using ValueFormatter = std::function<QString(int)>;

	explicit SliderControl(QWidget *parent = nullptr);

	void setTitle(const QString &title);
	void setRange(int minimum, int maximum);
	void setMinimum(int minimum);
	void setMaximum(int maximum);
	void setSingleStep(int step);
	void setValue(int value);
	int value() const;
	void setValueFormatter(ValueFormatter formatter);

signals:
	void valueChanged(int value);
	void sliderPressed();
	void sliderReleased();

private:
	QString formatValue(int value) const;
	void updateValueLabels();

	QLabel *titleLabel = nullptr;
	QLabel *valueLabel = nullptr;
	QLabel *minimumLabel = nullptr;
	QLabel *maximumLabel = nullptr;
	AbsoluteSlider *slider = nullptr;
	ValueFormatter valueFormatter;
};

} // namespace XBotGo

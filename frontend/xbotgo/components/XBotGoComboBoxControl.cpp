#include "XBotGoComboBoxControl.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>

#include "moc_XBotGoComboBoxControl.cpp"

namespace xbotgo {

ComboBoxControl::ComboBoxControl(QWidget *parent) : QWidget(parent)
{
	titleLabel = new QLabel(this);
	comboBox = new QComboBox(this);

	titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	titleLabel->setBuddy(comboBox);

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);
	layout->addWidget(titleLabel);
	layout->addWidget(comboBox, 1);

	connect(comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
		&ComboBoxControl::currentIndexChanged);
}

void ComboBoxControl::setTitle(const QString &title)
{
	titleLabel->setText(title);
	comboBox->setAccessibleName(title);
}

void ComboBoxControl::addItem(const QString &text, const QVariant &userData)
{
	comboBox->addItem(text, userData);
}

void ComboBoxControl::addItems(const QStringList &texts)
{
	comboBox->addItems(texts);
}

void ComboBoxControl::clear()
{
	comboBox->clear();
}

int ComboBoxControl::currentIndex() const
{
	return comboBox->currentIndex();
}

QVariant ComboBoxControl::currentData() const
{
	return comboBox->currentData();
}

void ComboBoxControl::setCurrentIndex(int index)
{
	comboBox->setCurrentIndex(index);
}

} // namespace xbotgo

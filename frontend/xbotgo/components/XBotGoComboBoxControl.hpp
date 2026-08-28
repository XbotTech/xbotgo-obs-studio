#pragma once

#include <QStringList>
#include <QVariant>
#include <QWidget>

class QComboBox;
class QLabel;
class QString;

namespace xbotgo {

class ComboBoxControl final : public QWidget {
	Q_OBJECT

public:
	explicit ComboBoxControl(QWidget *parent = nullptr);

	void setTitle(const QString &title);
	void addItem(const QString &text, const QVariant &userData = QVariant());
	void addItems(const QStringList &texts);
	void clear();
	int currentIndex() const;
	QVariant currentData() const;
	void setCurrentIndex(int index);

signals:
	void currentIndexChanged(int index);

private:
	QLabel *titleLabel = nullptr;
	QComboBox *comboBox = nullptr;
};

} // namespace xbotgo

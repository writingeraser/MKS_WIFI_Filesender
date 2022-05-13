#ifndef WIDGET_HPP
#define WIDGET_HPP

#include <QWidget>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
	Q_OBJECT

public:
	Widget(QWidget *parent = nullptr);
	~Widget();

protected:
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void updateFileName();

	Ui::Widget *ui;

public slots:
	void selectButtonClick(bool);
	void fileNameAutoGenChanged(int);
	void updatePrinterFileName(const QString& str);
	void postFinished(QNetworkReply *reply);
	void clearButtonHandler(bool);
	void sendButtonHandler(bool);
};
#endif // WIDGET_HPP

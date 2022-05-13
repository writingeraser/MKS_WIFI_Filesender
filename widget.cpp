#include "widget.hpp"
#include "./ui_widget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <utility>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

void Widget::clearButtonHandler(bool) {
	ui->fileEdit->clear();
	ui->printerNameEdit->clear();
	ui->ipEdit->clear();
	ui->printerFileNameEdit->clear();
}

void Widget::closeEvent(QCloseEvent* event) {
	QFile cfg("MKS_WIFI_Filesender.cfg");
	if(cfg.open(QFile::OpenModeFlag::WriteOnly | QFile::OpenModeFlag::Truncate)) {
		cfg.write(("ip=" + ui->ipEdit->text() + "\n").toUtf8());
		cfg.write(("printer=" + ui->printerNameEdit->text() + "\n").toUtf8());
	}
	cfg.close();
}

void Widget::sendButtonHandler(bool) {
	QFileInfo finfo = QFileInfo(ui->fileEdit->text());
	if(!finfo.exists()) {
		QMessageBox::warning(this, "", "Не найден файл");
		return;
	}

	QFile file = QFile(finfo.absoluteFilePath());
	if(!file.open(QFile::OpenModeFlag::ReadOnly)) {
		QMessageBox::warning(this, "", "Не удалось открыть файл");
		return;
	}

	auto data = file.readAll();
	if(data.size() == 0) {
		QMessageBox::warning(this, "", "Файл пуст!");
		return;
	}

	QNetworkRequest req (QString("http://") + ui->ipEdit->text() + "/upload?X-Filename=" + ui->printerFileNameEdit->text());
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

	QNetworkAccessManager* manager = new QNetworkAccessManager(this);
	manager->setTransferTimeout(15000);
	QObject::connect(manager, &QNetworkAccessManager::finished, this, &Widget::postFinished);
	manager->post(req, data);
	this->setDisabled(true);
}

void Widget::postFinished(QNetworkReply *reply) {
	this->setDisabled(false);
	auto err = reply->error();
	if(err != QNetworkReply::NoError) {
		QMessageBox::warning(this, "", reply->errorString());
	} else {
		auto content = reply->readAll();
		QMessageBox::information(this, "", content == "{\"err\":0}" ? QString("Success").toUtf8() : content);
	}
	reply->deleteLater();
}

Widget::Widget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::Widget)
{
	ui->setupUi(this);

	resize(width(), minimumHeight());
	setMaximumHeight(minimumHeight());
	setMinimumWidth(width());
	setWindowTitle("MKS WiFi Filesender");

	QFile cfg("MKS_WIFI_Filesender.cfg");
	if(cfg.open(QFile::OpenModeFlag::ReadOnly)) {

		for(;;) {
			auto data = QString(cfg.readLine()).trimmed();
			auto splits = data.split('=');
			if(splits.length() != 2) {
				cfg.close();
				cfg.remove();
				break;
			}

			if(splits[0] == "ip") {
				ui->ipEdit->setText(splits[1]);
			} else if(splits[0] == "printer") {
				ui->printerNameEdit->setText(splits[1]);
			}
		}
	}

	QObject::connect(ui->selectFileButton, SIGNAL(clicked(bool)), this, SLOT(selectButtonClick(bool)));
	QObject::connect(ui->fileNameAutoGenCheckBox, SIGNAL(stateChanged(int)), this, SLOT(fileNameAutoGenChanged(int)));
	QObject::connect(ui->fileEdit, &QLineEdit::textChanged, this, &Widget::updatePrinterFileName);
	QObject::connect(ui->printerNameEdit, &QLineEdit::textChanged, this, &Widget::updatePrinterFileName);
	QObject::connect(ui->sendButton, &QAbstractButton::clicked, this, &Widget::sendButtonHandler);
	QObject::connect(ui->clearButton, &QAbstractButton::clicked, this, &Widget::clearButtonHandler);
}

void Widget::selectButtonClick(bool checked) {
	QFileInfo info (QFileDialog::getOpenFileName(this, "", "", tr("GCode (*.gcode)")));
	if(!info.exists())	return;
	auto x = info.fileName().split(".");
	auto y = x.last();
	if(y != "gcode") return;


	ui->fileEdit->setText(info.absoluteFilePath());
}

void Widget::updatePrinterFileName(const QString& str) {
	if(ui->fileNameAutoGenCheckBox->checkState() == Qt::CheckState::Checked)
		updateFileName();
}

void Widget::updateFileName() {
	ui->printerFileNameEdit->clear();

	QFileInfo info (ui->fileEdit->text());
	if(!info.isFile()) {
		return;
	}

	auto formatDate = [](QDate const& date) {
		return QString::asprintf("%04d%02d%02d", date.year(), date.month(), date.day());
	};
	auto pname = ui->printerNameEdit->text();
	auto date = formatDate(QDate::currentDate());
	auto fname = info.baseName();
	fname.truncate(std::max(28 - pname.size() - date.size(), 0ll));
	auto name = pname + "_" + fname + "_" + date;
	name.truncate(30);

	ui->printerFileNameEdit->setText(name + ".gcode");
}

void Widget::fileNameAutoGenChanged(int state) {
	ui->printerFileNameEdit->setReadOnly(state == Qt::CheckState::Checked);
	switch(state) {
		case Qt::CheckState::Unchecked:
			break;
		case Qt::CheckState::Checked:
			updateFileName();
			break;
		default:
			QMessageBox::warning(this, "", QString::fromStdString("Неопознанное состояние"));
	}
}

Widget::~Widget()
{
	delete ui;
}



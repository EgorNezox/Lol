/**
 ******************************************************************************
 * @file    uartconsolewidget.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    16.11.2015
 *
 ******************************************************************************
 */

#include <qtexttable.h>

#include "uartconsolewidget.h"
#include "ui_uartconsolewidget.h"
#include "hardware_resources.h"

enum {
	rxeditAscii,
	rxeditHex
};

UartConsoleWidget::UartConsoleWidget(QWidget *parent, int hw_resource) :
	QFrame(parent),
	ui(new Ui::UartConsoleWidget),
	uart_interface(0), hex_edit_validator(QRegExp("([0-9A-Fa-f]{2}\\s)*([0-9A-Fa-f]{2})"))
{
	ui->setupUi(this);
	ui->groupRxEditFormat->setId(ui->rbtnRxEditAscii, rxeditAscii);
	ui->groupRxEditFormat->setId(ui->rbtnRxEditHex, rxeditHex);
	setRxEditFormat(ui->groupRxEditFormat->checkedId());
	QObject::connect(ui->groupRxEditFormat, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
			this, &UartConsoleWidget::setRxEditFormat);
	QObject::connect(ui->editRx, &QLineEdit::returnPressed, this, &UartConsoleWidget::execRxDataTransfer);
	QObject::connect(ui->btnRxError, &QToolButton::clicked, this, &UartConsoleWidget::execRxDataError);
	if (hw_resource != -1)
		assignHwResource(hw_resource);
}

UartConsoleWidget::~UartConsoleWidget()
{
	if (uart_interface)
		UartInterface::destroyInstance(uart_interface);
	delete ui;
}

QString UartConsoleWidget::getHwResource() {
	return QString();
}

void UartConsoleWidget::assignHwResource(const QString& value) {
	assignHwResource(QtHwEmu::convertToPlatformHwResource(value));
}

void UartConsoleWidget::assignHwResource(int value) {
	Q_ASSERT(uart_interface == 0);
	uart_interface = UartInterface::createInstance(value);
	QObject::connect(uart_interface, &UartInterface::txTransferred, this, &UartConsoleWidget::logTxTransfer);
}

void UartConsoleWidget::setRxEditFormat(int format_id) {
	ui->editRx->clear();
	switch (format_id) {
	case rxeditAscii:
		ui->editRx->setValidator(0);
		ui->cboxSendOnEnter->setEnabled(true);
		break;
	case rxeditHex:
		ui->editRx->setValidator(&hex_edit_validator);
		ui->cboxSendOnEnter->setEnabled(false);
		break;
	}
}

void UartConsoleWidget::appendDataActivity(const QString& type, const QByteArray& data) {
	QString hex_data;
	for (int i = 0; i < data.size(); i++)
		hex_data.append(QString("%1 ").arg((quint8)data.at(i), 2, 16, QChar('0')));
	QString ascii_data = QString::fromLatin1(data);
	for (int i = 0; i < ascii_data.length(); i++)
		if (!ascii_data[i].isPrint())
			ascii_data[i] = QChar('.');
	QTextCursor cursor(ui->textActivity->document());
	cursor.movePosition(QTextCursor::End);
	QTextTableFormat record_format;
	record_format.setBorder(0);
	QVector<QTextLength> record_columns_constraints;
	record_columns_constraints << QTextLength(QTextLength::FixedLength, 20);
	record_columns_constraints << QTextLength(QTextLength::PercentageLength, 50);
	record_columns_constraints << QTextLength(QTextLength::PercentageLength, 50);
	record_format.setColumnWidthConstraints(record_columns_constraints);
	QTextTable *record_table = cursor.insertTable(1, 3, record_format);
	record_table->cellAt(0, 0).firstCursorPosition().insertText(type+":");
	record_table->cellAt(0, 1).firstCursorPosition().insertText(hex_data);
	record_table->cellAt(0, 2).firstCursorPosition().insertText(ascii_data);
	ui->textActivity->moveCursor(QTextCursor::End);
}

void UartConsoleWidget::logTxTransfer(const QByteArray& data) {
	appendDataActivity("Tx", data);
}

void UartConsoleWidget::execRxDataTransfer() {
	if (!uart_interface)
		return;
	QByteArray data;
	switch (ui->groupRxEditFormat->checkedId()) {
	case rxeditAscii:
		data = ui->editRx->text().toLatin1();
		switch (ui->cboxSendOnEnter->currentIndex()) {
		case 1: data.append("\r"); break;
		case 2: data.append("\n"); break;
		case 3: data.append("\r\n"); break;
		}
		break;
	case rxeditHex:
		data = QByteArray::fromHex(ui->editRx->text().toLatin1());
		break;
	}
	uart_interface->transferRx(data);
	appendDataActivity("Rx", data);
	ui->editRx->clear();
}

void UartConsoleWidget::execRxDataError() {
	if (!uart_interface)
		return;
	uart_interface->injectRxDataError();
	QTextCursor cursor(ui->textActivity->document());
	cursor.movePosition(QTextCursor::End);
	QTextBlockFormat text_format;
	text_format.setLeftMargin(3);
	cursor.insertBlock(text_format);
	cursor.insertText("Rx data error");
	ui->textActivity->moveCursor(QTextCursor::End);
}

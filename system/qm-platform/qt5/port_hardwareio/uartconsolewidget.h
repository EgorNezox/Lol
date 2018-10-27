/**
 ******************************************************************************
 * @file    uartconsolewidget.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    16.11.2015
 *
 ******************************************************************************
 */

#ifndef UARTCONSOLEWIDGET_H
#define UARTCONSOLEWIDGET_H

#include <qframe.h>
#include <qvalidator.h>
#include "uartinterface.h"

namespace Ui {
class UartConsoleWidget;
}

class UartConsoleWidget : public QFrame
{
	Q_OBJECT

	Q_PROPERTY(QString hw_resource READ getHwResource WRITE assignHwResource)

public:
	UartConsoleWidget(QWidget *parent = 0, int hw_resource = -1);
	~UartConsoleWidget();
	QString getHwResource();
	void assignHwResource(const QString &value);
	void assignHwResource(int value);

private Q_SLOTS:
	void setRxEditFormat(int format_id);
	void appendDataActivity(const QString &type, const QByteArray &data);
	void logTxTransfer(const QByteArray &data);
	void execRxDataTransfer();
	void execRxDataError();

    void on_editRx_textChanged(const QString &arg1);

private:
	Ui::UartConsoleWidget *ui;
	UartInterface *uart_interface;
	const QRegExpValidator hex_edit_validator;
};

#endif // UARTCONSOLEWIDGET_H

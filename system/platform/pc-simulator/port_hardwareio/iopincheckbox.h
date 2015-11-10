/**
 ******************************************************************************
 * @file    iopincheckbox.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#ifndef IOPINCHECKBOX_H_
#define IOPINCHECKBOX_H_

#include <QCheckBox>
#include "iopininterface.h"

class IopinCheckBox: public QCheckBox {
	Q_OBJECT

	Q_PROPERTY(QString hw_resource READ getHwResource WRITE assignHwResource)
	Q_PROPERTY(QString direction READ getDirection WRITE setDirection)
	Q_ENUMS(Direction)

public:
	enum Direction {
		Direction_Input,
		Direction_Output
	};

	IopinCheckBox(QWidget *parent = 0, int hw_resource = -1);
	~IopinCheckBox();
	QString getHwResource();
	void assignHwResource(const QString &value);
	void assignHwResource(int hw_resource);
	QString getDirection();
	void setDirection(const QString &value);
	void setDirection(Direction value);
	void synchronizeInterface();

private Q_SLOTS:
	void processPinOutputLevelChanged(IopinInterface::Level level);
	void processCBStateChanged(int state);

private:
	IopinInterface *pin_interface;
};

#endif /* IOPINCHECKBOX_H_ */

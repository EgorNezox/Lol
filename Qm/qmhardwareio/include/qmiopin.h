/**
  ******************************************************************************
  * @file    qmiopin.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMIOPIN_H_
#define QMIOPIN_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmIopin)

/*! The QmIopin class provides functions to access general-purpose I/O pins.
 */
class QmIopin: public QmObject {
public:
	enum LevelTriggerMode {
		InputTrigger_Disabled,
		InputTrigger_Rising,
		InputTrigger_Falling,
		InputTrigger_Both
	};
	enum Level {
		Level_Low,
		Level_High
	};

	/*! Constructs an iopin with the given \a parent.
	 *
	 * Parameter \a hw_resource specifies platform-identified instance of pin.
	 */
	QmIopin(int hw_resource, QmObject *parent = 0);

	/*! Destroys the iopin. */
	virtual ~QmIopin();

	bool setInputTriggerMode(LevelTriggerMode mode);

	Level readInput();

	void writeOutput(Level level);

	void init();

	void deinit();

	sigc::signal<void> inputTrigger;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmIopin)
	QM_DISABLE_COPY(QmIopin)
	friend class QmIopinPrivateAdapter;
};

#endif /* QMIOPIN_H_ */

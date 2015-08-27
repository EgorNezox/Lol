/**
  ******************************************************************************
  * @file    qm_core_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <QEvent>

#include "qm.h"
#include "qmevent.h"
#include "qm_core.h"
#include "qmthread.h"

qmcore_global_environment_t qmcore_global_environment;

int main(int argc, char *argv[]) {
	new QmThread("qmmain");
	qmcore_global_environment.argc = argc;
	qmcore_global_environment.argv = argv;
	qmcore_global_environment.exit_code = 0;
	qmMain();
	return qmcore_global_environment.exit_code;
}

QmCoreEvent::QmCoreEvent(QmEvent *qmevent, bool auto_destroy) :
	QEvent(toQtEventType(static_cast<int>(qmevent->type()))),
	qmevent(qmevent), auto_destroy(auto_destroy)
{
}

QmCoreEvent::~QmCoreEvent() {
	if (auto_destroy)
		delete qmevent;
}

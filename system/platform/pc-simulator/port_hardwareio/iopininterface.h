/**
 ******************************************************************************
 * @file    iopininterface.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#ifndef IOPININTERFACE_H_
#define IOPININTERFACE_H_

#include <QObject>

class IopinInterface : public QObject
{
	Q_OBJECT

public:
	enum Level {
		Level_Low,
		Level_HiZ,
		Level_High
	};

public Q_SLOTS:
	Level getOutputLevel();
	void setInputLevel(Level level);

Q_SIGNALS:
	void outputLevelChanged(Level level);

private:
	friend class IopinsFactory;
	friend class QmIopinPrivateAdapter;

	IopinInterface();
	virtual ~IopinInterface();
	void assignOutputLevel(Level level);

	Level output_level;

#ifndef Q_MOC_RUN
private:
#else
Q_SIGNALS:
#endif
	void inputLevelAssigned(Level level);
};

#endif /* IOPININTERFACE_H_ */

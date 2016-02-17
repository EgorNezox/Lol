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

#include <qobject.h>
#include <qmetatype.h>

class IopinInterface : public QObject
{
	Q_OBJECT

public:
	enum Level {
		Level_Low,
		Level_HiZ,
		Level_High
	};

	static IopinInterface* getInstance(int hw_resource);
	static IopinInterface* createInstance(int hw_resource);
	static void destroyInstance(IopinInterface *instance);

public Q_SLOTS:
	Level getOutputLevel();
	void setInputLevel(Level level);

Q_SIGNALS:
	void outputLevelChanged(Level level);

private:
	friend class QmIopinPrivateAdapter;

	static void __attribute__((constructor)) init();
	IopinInterface();
	virtual ~IopinInterface();

	Level input_level, output_level;

#ifndef Q_MOC_RUN
private:
#else
Q_SIGNALS:
#endif
	void inputLevelAssigned(Level level);
private Q_SLOTS:
	Level getInputLevel();
	void assignOutputLevel(Level level);
};

Q_DECLARE_METATYPE(IopinInterface::Level)

#endif /* IOPININTERFACE_H_ */

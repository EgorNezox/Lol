/**
 ******************************************************************************
 * @file    iopinsfactory.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    06.11.2015
 *
 ******************************************************************************
 */

#ifndef IOPINSFACTORY_H_
#define IOPINSFACTORY_H_

class IopinInterface;

class IopinsFactory {
public:
	static IopinInterface* getInstance(int hw_resource);
	static IopinInterface* createInstance(int hw_resource);
	static void destroyInstance(IopinInterface *instance);
};

#endif /* IOPINSFACTORY_H_ */

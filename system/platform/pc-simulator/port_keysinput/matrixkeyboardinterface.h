/**
  ******************************************************************************
  * @file    matrixkeyboardinterface.h
  * @author  Petr Dmitriev
  * @date    18.12.2015
  *
  ******************************************************************************
  */

#ifndef MATRIXKEYBOARDINTERFACE_H
#define MATRIXKEYBOARDINTERFACE_H

#include <QObject>

class MatrixKeyboardInterface : public QObject
{
    Q_OBJECT

public:
    static MatrixKeyboardInterface* getInstance(int hw_resource);
    static MatrixKeyboardInterface* createInstance(int hw_resource);
    static void destroyInstance(MatrixKeyboardInterface *instance);

public Q_SLOTS:
    void setKeyStateChanged(int id, bool state);

#ifndef Q_MOC_RUN
private:
#else
Q_SIGNALS:
#endif
    void keyStateChanged(int id, bool state);

private:
    friend class QmMatrixKeyboardPrivateAdapter;

    MatrixKeyboardInterface();
    virtual ~MatrixKeyboardInterface();
};

#endif // MATRIXKEYBOARDINTERFACE_H

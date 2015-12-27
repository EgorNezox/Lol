/**
  ******************************************************************************
  * @file    matrixkeyboardwidget.h
  * @author  Petr Dmitriev
  * @date    18.12.2015
  *
  ******************************************************************************
  */

#ifndef MATRIXKEYBOARDWIDGET_H
#define MATRIXKEYBOARDWIDGET_H

#include <QFrame>
#include <QButtonGroup>
#include "matrixkeyboardinterface.h"

namespace Ui {
class MatrixKeyboardWidget;
}

class MatrixKeyboardWidget : public QFrame
{
    Q_OBJECT

public:
    explicit MatrixKeyboardWidget(QWidget *parent = 0, int hw_resource = -1);
    ~MatrixKeyboardWidget();

private Q_SLOTS:
    void keyPressed(int id);
    void keyReleased(int id);
    void keyToggled(int id, bool checked);
    void on_chb_Checkable_stateChanged(int arg1);

private:
    Ui::MatrixKeyboardWidget *ui;
    QButtonGroup* buttonsGroup;
    MatrixKeyboardInterface* matrixkb_interface;

    static const int buttons_count = 16;
};

#endif // MATRIXKEYBOARDWIDGET_H

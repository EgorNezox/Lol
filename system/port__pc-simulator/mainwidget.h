/**
  ******************************************************************************
  * @file    mainwidget.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.10.2015
  * @brief   Интерфейс класса виджета Qt, реализующего главное окно симулятора HOST
  *
  ******************************************************************************
  */

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <qwidget.h>

namespace Ui {
class MainWidget;
}
namespace QtHwEmu {
void init();
void deinit();
}

class MainWidget : public QWidget
{
    Q_OBJECT

private:
    friend void QtHwEmu::init();
    friend void QtHwEmu::deinit();

    MainWidget(QWidget *parent = 0);
    ~MainWidget();

    Ui::MainWidget *ui;
};

#endif // MAINWIDGET_H

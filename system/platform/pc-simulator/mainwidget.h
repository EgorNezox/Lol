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

#include <QWidget>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    static void initDisplay(QWidget *display_widget);

private:
    MainWidget();
    ~MainWidget();
    static void createIfNotExists();
    static void destroyIfExists();
    Ui::MainWidget *ui;
    static MainWidget *self;
};

#endif // MAINWIDGET_H

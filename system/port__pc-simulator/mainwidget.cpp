/**
  ******************************************************************************
  * @file    mainwidget.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.10.2015
  * @brief   Реализация класса виджета Qt, реализующего главное окно симулятора HOST
  *
  ******************************************************************************
  */

#include "mainwidget.h"
#include "ui_mainwidget.h"
#include "ramtexdisplaywidget.h"

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    RamtexDisplayWidget *display_widget = new RamtexDisplayWidget(ui->placeholderDisplay);
    display_widget->show();
}

MainWidget::~MainWidget()
{
    delete ui;
}

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

MainWidget *MainWidget::self = 0;

void MainWidget::initDisplay(QWidget *display_widget)
{
    createIfNotExists();
    display_widget->setParent(self->ui->placeholderDisplay);
    display_widget->show();
}

MainWidget::MainWidget() :
    QWidget(0),
    ui(new Ui::MainWidget)
{
    self = this;
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}

MainWidget::~MainWidget()
{
    delete ui;
    self = 0;
}

void MainWidget::createIfNotExists()
{
    if (self != 0)
        return;
    new MainWidget();
    self->show();
}

void MainWidget::destroyIfExists()
{
    if (self == 0)
        return;
    delete self;
}

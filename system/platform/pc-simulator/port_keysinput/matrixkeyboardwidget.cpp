/**
  ******************************************************************************
  * @file    matrixkeyboardwidget.cpp
  * @author  Petr Dmitriev
  * @date    18.12.2015
  *
  ******************************************************************************
  */

#include <qdebug.h>
#include "matrixkeyboardwidget.h"
#include "ui_matrixkeyboardwidget.h"

MatrixKeyboardWidget::MatrixKeyboardWidget(QWidget *parent, int hw_resource) :
    QFrame(parent),
    ui(new Ui::MatrixKeyboardWidget)
{
    ui->setupUi(this);
    buttonsGroup = new QButtonGroup(this);
    buttonsGroup->addButton(ui->pushButton_1, 0);
    buttonsGroup->addButton(ui->pushButton_2, 1);
    buttonsGroup->addButton(ui->pushButton_3, 2);
    buttonsGroup->addButton(ui->pushButton_4, 3);
    buttonsGroup->addButton(ui->pushButton_5, 4);
    buttonsGroup->addButton(ui->pushButton_6, 5);
    buttonsGroup->addButton(ui->pushButton_7, 6);
    buttonsGroup->addButton(ui->pushButton_8, 7);
    buttonsGroup->addButton(ui->pushButton_9, 8);
    buttonsGroup->addButton(ui->pushButton_10, 9);
    buttonsGroup->addButton(ui->pushButton_11, 10);
    buttonsGroup->addButton(ui->pushButton_12, 11);
    buttonsGroup->addButton(ui->pushButton_13, 12);
    buttonsGroup->addButton(ui->pushButton_14, 13);
    buttonsGroup->addButton(ui->pushButton_15, 14);
    buttonsGroup->addButton(ui->pushButton_16, 15);
    buttonsGroup->setExclusive(false);

    matrixkb_interface = MatrixKeyboardInterface::createInstance(hw_resource, 16);
    QObject::connect(buttonsGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonPressed),
                     this, &MatrixKeyboardWidget::keyPressed);
    QObject::connect(buttonsGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonReleased),
                     this, &MatrixKeyboardWidget::keyReleased);
    QObject::connect(buttonsGroup, static_cast<void (QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled),
                     this, &MatrixKeyboardWidget::keyToggled);
}

MatrixKeyboardWidget::~MatrixKeyboardWidget()
{
    delete ui;
}

void MatrixKeyboardWidget::keyPressed(int id)
{
    Q_ASSERT(id > -1 && id < buttons_count);
    if (buttonsGroup->buttons().at(id)->isCheckable())
        return;
    matrixkb_interface->setKeyStateChanged(id, true);
}

void MatrixKeyboardWidget::keyReleased(int id)
{
    Q_ASSERT(id > -1 && id < buttons_count);
    if (buttonsGroup->buttons().at(id)->isCheckable())
        return;
    matrixkb_interface->setKeyStateChanged(id, false);
}

void MatrixKeyboardWidget::keyToggled(int id, bool checked)
{
    if (checked) {
        matrixkb_interface->setKeyStateChanged(id, true);
    } else {
        matrixkb_interface->setKeyStateChanged(id, false);
    }
}

void MatrixKeyboardWidget::on_chb_Checkable_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked) {
        for (int i = 0; i < buttonsGroup->buttons().size(); ++i)
            buttonsGroup->buttons().at(i)->setCheckable(true);
    } else if (arg1 == Qt::Unchecked) {
        for (int i = 0; i < buttonsGroup->buttons().size(); ++i) {
            if (buttonsGroup->buttons().at(i)->isChecked())
                buttonsGroup->buttons().at(i)->setChecked(false);
            buttonsGroup->buttons().at(i)->setCheckable(false);
        }
    } else {
        Q_ASSERT(0);
    }
}

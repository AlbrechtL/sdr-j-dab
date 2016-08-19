/********************************************************************************
** Form generated from reading UI file 'scopewidget.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SCOPEWIDGET_H
#define UI_SCOPEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSlider>
#include <QtWidgets/QWidget>
#include "qwt_plot.h"

QT_BEGIN_NAMESPACE

class Ui_scopeWidget
{
public:
    QwtPlot *dabScope;
    QSlider *scopeAmplification;

    void setupUi(QWidget *scopeWidget)
    {
        if (scopeWidget->objectName().isEmpty())
            scopeWidget->setObjectName(QStringLiteral("scopeWidget"));
        scopeWidget->resize(866, 262);
        dabScope = new QwtPlot(scopeWidget);
        dabScope->setObjectName(QStringLiteral("dabScope"));
        dabScope->setGeometry(QRect(50, 10, 791, 231));
        scopeAmplification = new QSlider(scopeWidget);
        scopeAmplification->setObjectName(QStringLiteral("scopeAmplification"));
        scopeAmplification->setGeometry(QRect(10, 10, 18, 261));
        scopeAmplification->setValue(50);
        scopeAmplification->setOrientation(Qt::Vertical);

        retranslateUi(scopeWidget);

        QMetaObject::connectSlotsByName(scopeWidget);
    } // setupUi

    void retranslateUi(QWidget *scopeWidget)
    {
        scopeWidget->setWindowTitle(QApplication::translate("scopeWidget", "dab spectrum", 0));
    } // retranslateUi

};

namespace Ui {
    class scopeWidget: public Ui_scopeWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SCOPEWIDGET_H

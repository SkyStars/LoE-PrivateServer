/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 5.1.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QGridLayout *gridLayout;
    QLabel *status;
    QPlainTextEdit *log;
    QLineEdit *cmdLine;
    QPushButton *sendButton;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName(QStringLiteral("Widget"));
        Widget->resize(400, 300);
        gridLayout = new QGridLayout(Widget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        status = new QLabel(Widget);
        status->setObjectName(QStringLiteral("status"));
        status->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(status, 0, 0, 1, 3);

        log = new QPlainTextEdit(Widget);
        log->setObjectName(QStringLiteral("log"));
        log->setReadOnly(true);

        gridLayout->addWidget(log, 1, 0, 1, 3);

        cmdLine = new QLineEdit(Widget);
        cmdLine->setObjectName(QStringLiteral("cmdLine"));

        gridLayout->addWidget(cmdLine, 2, 0, 1, 2);

        sendButton = new QPushButton(Widget);
        sendButton->setObjectName(QStringLiteral("sendButton"));

        gridLayout->addWidget(sendButton, 2, 2, 1, 1);


        retranslateUi(Widget);
        QObject::connect(cmdLine, SIGNAL(returnPressed()), sendButton, SLOT(click()));

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QApplication::translate("Widget", "mlkj's Private Server", 0));
        status->setText(QApplication::translate("Widget", "Server status", 0));
        sendButton->setText(QApplication::translate("Widget", "Send", 0));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H

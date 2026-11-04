/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QVBoxLayout *rootLayout;
    QSplitter *mainSplitter;
    QWidget *upperArea;
    QHBoxLayout *upperLayout;
    QVBoxLayout *controlPanelLayout;
    QVBoxLayout *simulationLayout;
    QWidget *timelineArea;
    QVBoxLayout *timelineAreaLayout;
    QLabel *labelStatusBar;

    void setupUi(QWidget *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1280, 800);
        MainWindow->setStyleSheet(QString::fromUtf8("\n"
"QWidget#MainWindow {\n"
"    background-color: #0A0D12;\n"
"}\n"
"\n"
"QWidget {\n"
"    color: #E8E9ED;\n"
"    font-family: \"Segoe UI\", \"Microsoft YaHei\", sans-serif;\n"
"    font-size: 10pt;\n"
"}\n"
"\n"
"/* \347\212\266\346\200\201\346\240\217\357\274\232\344\270\215\345\206\215\347\224\250 QStatusBar\357\274\214\346\215\242\346\210\220\346\231\256\351\200\232 QLabel \345\217\202\344\270\216\345\220\214\344\270\200\345\270\203\345\261\200\347\232\204\345\260\272\345\257\270\345\210\206\351\205\215 */\n"
"QLabel#labelStatusBar {\n"
"    background-color: #0F131C;\n"
"    border-top: 1px solid #1E2636;\n"
"    color: #C8D2E0;\n"
"    font-size: 9pt;\n"
"    padding: 4px 8px;\n"
"}\n"
"\n"
"/* \345\210\206\346\240\217\346\213\226\345\212\250\346\211\213\346\237\204\357\274\232\351\273\230\350\256\244\345\244\252\347\262\227\344\270\224\351\242\234\350\211\262\347\252\201\345\205\200 */\n"
"QSplitter::handle {\n"
"    background-color: #1E2636;\n"
"}\n"
"QSplitter::handle:hover {\n"
"    background-c"
                        "olor: #38BDF8;\n"
"}\n"
"   "));
        rootLayout = new QVBoxLayout(MainWindow);
        rootLayout->setSpacing(0);
        rootLayout->setObjectName("rootLayout");
        rootLayout->setContentsMargins(0, 0, 0, 0);
        mainSplitter = new QSplitter(MainWindow);
        mainSplitter->setObjectName("mainSplitter");
        mainSplitter->setOrientation(Qt::Orientation::Vertical);
        mainSplitter->setChildrenCollapsible(false);
        mainSplitter->setHandleWidth(6);
        upperArea = new QWidget(mainSplitter);
        upperArea->setObjectName("upperArea");
        upperLayout = new QHBoxLayout(upperArea);
        upperLayout->setSpacing(8);
        upperLayout->setObjectName("upperLayout");
        upperLayout->setContentsMargins(8, 8, 8, 0);
        controlPanelLayout = new QVBoxLayout();
        controlPanelLayout->setObjectName("controlPanelLayout");
        controlPanelLayout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

        upperLayout->addLayout(controlPanelLayout);

        simulationLayout = new QVBoxLayout();
        simulationLayout->setObjectName("simulationLayout");
        simulationLayout->setSizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);

        upperLayout->addLayout(simulationLayout);

        mainSplitter->addWidget(upperArea);
        timelineArea = new QWidget(mainSplitter);
        timelineArea->setObjectName("timelineArea");
        timelineAreaLayout = new QVBoxLayout(timelineArea);
        timelineAreaLayout->setSpacing(0);
        timelineAreaLayout->setObjectName("timelineAreaLayout");
        timelineAreaLayout->setContentsMargins(8, 0, 8, 0);
        mainSplitter->addWidget(timelineArea);

        rootLayout->addWidget(mainSplitter);

        labelStatusBar = new QLabel(MainWindow);
        labelStatusBar->setObjectName("labelStatusBar");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(labelStatusBar->sizePolicy().hasHeightForWidth());
        labelStatusBar->setSizePolicy(sizePolicy);

        rootLayout->addWidget(labelStatusBar);


        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QWidget *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\345\217\214\350\207\202\346\234\272\345\231\250\344\272\272\345\212\250\344\275\234\350\247\204\345\210\222\345\231\250 - ActionPlanner", nullptr));
        labelStatusBar->setText(QCoreApplication::translate("MainWindow", "\345\260\261\347\273\252", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H

/********************************************************************************
** Form generated from reading UI file 'SimulationWidget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SIMULATIONWIDGET_H
#define UI_SIMULATIONWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SimulationWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *topBarLayout;
    QLabel *labelTitle;
    QSpacerItem *horizontalSpacer;
    QLabel *labelURDF;
    QComboBox *comboBoxURDF;
    QPushButton *btnLoadURDF;
    QWidget *renderContainer;
    QHBoxLayout *infoBarLayout;
    QLabel *labelInfo;
    QSpacerItem *horizontalSpacer_2;
    QLabel *labelFPS;

    void setupUi(QWidget *SimulationWidget)
    {
        if (SimulationWidget->objectName().isEmpty())
            SimulationWidget->setObjectName("SimulationWidget");
        SimulationWidget->resize(800, 600);
        SimulationWidget->setStyleSheet(QString::fromUtf8("\n"
"QWidget#SimulationWidget {\n"
"    background-color: #0A0D12;\n"
"}\n"
"\n"
"QLabel {\n"
"    color: #C8D2E0;\n"
"    font-size: 10pt;\n"
"}\n"
"\n"
"QComboBox {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
"    border-radius: 6px;\n"
"    padding: 6px 12px;\n"
"    color: #E8E9ED;\n"
"    min-width: 200px;\n"
"}\n"
"\n"
"QComboBox:hover {\n"
"    border: 1px solid #38BDF8;\n"
"}\n"
"\n"
"QComboBox::drop-down {\n"
"    border: none;\n"
"    width: 20px;\n"
"}\n"
"\n"
"QComboBox::down-arrow {\n"
"    image: none;\n"
"    border-left: 5px solid transparent;\n"
"    border-right: 5px solid transparent;\n"
"    border-top: 5px solid #C8D2E0;\n"
"    margin-right: 8px;\n"
"}\n"
"\n"
"QComboBox QAbstractItemView {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #38BDF8;\n"
"    selection-background-color: #38BDF8;\n"
"    selection-color: #0A0D12;\n"
"    outline: none;\n"
"}\n"
"\n"
"QPushButton {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
""
                        "    border-radius: 6px;\n"
"    padding: 8px 16px;\n"
"    color: #E8E9ED;\n"
"    font-size: 10pt;\n"
"    font-weight: 500;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #242D3D;\n"
"    border: 1px solid #38BDF8;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #2A3444;\n"
"}\n"
"\n"
"QPushButton#btnLoadURDF {\n"
"    background-color: #38BDF8;\n"
"    border: none;\n"
"    color: #0A0D12;\n"
"    font-weight: 600;\n"
"}\n"
"\n"
"QPushButton#btnLoadURDF:hover {\n"
"    background-color: #4AC5FF;\n"
"}\n"
"   "));
        verticalLayout = new QVBoxLayout(SimulationWidget);
        verticalLayout->setSpacing(12);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(12, 12, 12, 12);
        topBarLayout = new QHBoxLayout();
        topBarLayout->setSpacing(12);
        topBarLayout->setObjectName("topBarLayout");
        labelTitle = new QLabel(SimulationWidget);
        labelTitle->setObjectName("labelTitle");
        labelTitle->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    font-size: 12pt;\n"
"    font-weight: bold;\n"
"    color: #38BDF8;\n"
"}\n"
"        "));

        topBarLayout->addWidget(labelTitle);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        topBarLayout->addItem(horizontalSpacer);

        labelURDF = new QLabel(SimulationWidget);
        labelURDF->setObjectName("labelURDF");

        topBarLayout->addWidget(labelURDF);

        comboBoxURDF = new QComboBox(SimulationWidget);
        comboBoxURDF->setObjectName("comboBoxURDF");

        topBarLayout->addWidget(comboBoxURDF);

        btnLoadURDF = new QPushButton(SimulationWidget);
        btnLoadURDF->setObjectName("btnLoadURDF");

        topBarLayout->addWidget(btnLoadURDF);


        verticalLayout->addLayout(topBarLayout);

        renderContainer = new QWidget(SimulationWidget);
        renderContainer->setObjectName("renderContainer");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(renderContainer->sizePolicy().hasHeightForWidth());
        renderContainer->setSizePolicy(sizePolicy);
        renderContainer->setStyleSheet(QString::fromUtf8("\n"
"QWidget#renderContainer {\n"
"    background-color: #161D2B;\n"
"    border: 1px solid #1E2636;\n"
"    border-radius: 8px;\n"
"}\n"
"      "));

        verticalLayout->addWidget(renderContainer);

        infoBarLayout = new QHBoxLayout();
        infoBarLayout->setSpacing(16);
        infoBarLayout->setObjectName("infoBarLayout");
        labelInfo = new QLabel(SimulationWidget);
        labelInfo->setObjectName("labelInfo");
        labelInfo->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    color: #6EE7B7;\n"
"    font-size: 9pt;\n"
"    padding: 4px 8px;\n"
"    background-color: #1E2636;\n"
"    border-radius: 4px;\n"
"}\n"
"        "));

        infoBarLayout->addWidget(labelInfo);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        infoBarLayout->addItem(horizontalSpacer_2);

        labelFPS = new QLabel(SimulationWidget);
        labelFPS->setObjectName("labelFPS");
        labelFPS->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    color: #C8D2E0;\n"
"    font-size: 9pt;\n"
"    font-family: monospace;\n"
"}\n"
"        "));

        infoBarLayout->addWidget(labelFPS);


        verticalLayout->addLayout(infoBarLayout);


        retranslateUi(SimulationWidget);

        QMetaObject::connectSlotsByName(SimulationWidget);
    } // setupUi

    void retranslateUi(QWidget *SimulationWidget)
    {
        SimulationWidget->setWindowTitle(QCoreApplication::translate("SimulationWidget", "3D\344\273\277\347\234\237", nullptr));
        labelTitle->setText(QCoreApplication::translate("SimulationWidget", "3D \346\234\272\345\231\250\344\272\272\344\273\277\347\234\237", nullptr));
        labelURDF->setText(QCoreApplication::translate("SimulationWidget", "URDF\346\250\241\345\236\213:", nullptr));
#if QT_CONFIG(tooltip)
        comboBoxURDF->setToolTip(QCoreApplication::translate("SimulationWidget", "\351\200\211\346\213\251\346\234\272\345\231\250\344\272\272URDF\346\250\241\345\236\213\346\226\207\344\273\266", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        btnLoadURDF->setToolTip(QCoreApplication::translate("SimulationWidget", "\345\212\240\350\275\275\351\200\211\344\270\255\347\232\204URDF\346\250\241\345\236\213", nullptr));
#endif // QT_CONFIG(tooltip)
        btnLoadURDF->setText(QCoreApplication::translate("SimulationWidget", "\345\212\240\350\275\275\346\250\241\345\236\213", nullptr));
        labelInfo->setText(QCoreApplication::translate("SimulationWidget", "\360\237\222\241 \346\217\220\347\244\272: \351\274\240\346\240\207\345\267\246\351\224\256\346\213\226\345\212\250\346\227\213\350\275\254\357\274\214\346\273\232\350\275\256\347\274\251\346\224\276", nullptr));
        labelFPS->setText(QCoreApplication::translate("SimulationWidget", "FPS: --", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SimulationWidget: public Ui_SimulationWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SIMULATIONWIDGET_H

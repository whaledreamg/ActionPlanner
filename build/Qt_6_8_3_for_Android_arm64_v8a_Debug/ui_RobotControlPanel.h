/********************************************************************************
** Form generated from reading UI file 'RobotControlPanel.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ROBOTCONTROLPANEL_H
#define UI_ROBOTCONTROLPANEL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_RobotControlPanel
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *connectionGroup;
    QVBoxLayout *verticalLayout_conn;
    QFormLayout *formLayout;
    QLabel *labelHost;
    QLineEdit *lineEditHost;
    QLabel *labelPort;
    QSpinBox *spinBoxPort;
    QPushButton *btnConnect;
    QLabel *labelStatus;
    QTabWidget *tabWidget;
    QWidget *tabLeftArm;
    QVBoxLayout *verticalLayout_left;
    QHBoxLayout *horizontalLayout_left;
    QPushButton *btnToggleLeft;
    QPushButton *btnGetCurrentAnglesLeft;
    QScrollArea *scrollAreaLeft;
    QWidget *scrollAreaWidgetContents_left;
    QVBoxLayout *verticalLayout_3;
    QWidget *tabRightArm;
    QVBoxLayout *verticalLayout_right;
    QHBoxLayout *horizontalLayout_right;
    QPushButton *btnToggleRight;
    QPushButton *btnGetCurrentAnglesRight;
    QScrollArea *scrollAreaRight;
    QWidget *scrollAreaWidgetContents_right;
    QVBoxLayout *verticalLayout_5;
    QWidget *tabHand;
    QVBoxLayout *verticalLayout_hand;
    QLabel *labelHandHint;
    QPushButton *btnConnectHand;
    QLabel *labelHandStatus;
    QPushButton *btnRefreshHand;
    QComboBox *comboHandPreset;
    QPushButton *btnApplyHandPreset;
    QSpacerItem *verticalSpacerHand;

    void setupUi(QWidget *RobotControlPanel)
    {
        if (RobotControlPanel->objectName().isEmpty())
            RobotControlPanel->setObjectName("RobotControlPanel");
        RobotControlPanel->resize(363, 669);
        RobotControlPanel->setMinimumSize(QSize(300, 0));
        RobotControlPanel->setMaximumSize(QSize(400, 16777215));
        RobotControlPanel->setStyleSheet(QString::fromUtf8("\n"
"/* \344\270\273\351\235\242\346\235\277\346\240\267\345\274\217 */\n"
"QWidget#RobotControlPanel {\n"
"    background-color: #0F131C;\n"
"    border-radius: 8px;\n"
"}\n"
"\n"
"/* \345\210\207\346\215\242\346\214\211\351\222\256 - \346\234\252\351\200\211\344\270\255\347\212\266\346\200\201\357\274\210\345\244\261\350\203\275\357\274\211 */\n"
"QPushButton#btnToggleLeft,\n"
"QPushButton#btnToggleRight {\n"
"    background-color: #1E2636;\n"
"    border: 2px solid #38BDF8;\n"
"    color: #38BDF8;\n"
"    border-radius: 5px;\n"
"    padding: 4px 12px;\n"
"    font-weight: 600;\n"
"    font-size: 9pt;\n"
"    min-height: 22px;\n"
"    min-width: 84px;\n"
"}\n"
"\n"
"QPushButton#btnToggleLeft:hover,\n"
"QPushButton#btnToggleRight:hover {\n"
"    background-color: #263445;\n"
"    border-color: #4AC5FF;\n"
"    color: #4AC5FF;\n"
"}\n"
"\n"
"QPushButton#btnToggleLeft:pressed,\n"
"QPushButton#btnToggleRight:pressed {\n"
"    background-color: #2A3A50;\n"
"}\n"
"\n"
"/* \345\210\207\346\215\242\346\214\211\351\222"
                        "\256 - \351\200\211\344\270\255\347\212\266\346\200\201\357\274\210\344\275\277\350\203\275\357\274\211 */\n"
"QPushButton#btnToggleLeft:checked,\n"
"QPushButton#btnToggleRight:checked {\n"
"    background-color: #6EE7B7;\n"
"    border-color: #6EE7B7;\n"
"    color: #0A0D12;\n"
"}\n"
"\n"
"QPushButton#btnToggleLeft:checked:hover,\n"
"QPushButton#btnToggleRight:checked:hover {\n"
"    background-color: #7EEFC3;\n"
"    border-color: #7EEFC3;\n"
"}\n"
"\n"
"/* \345\210\207\346\215\242\346\214\211\351\222\256 - \347\246\201\347\224\250\347\212\266\346\200\201 */\n"
"QPushButton#btnToggleLeft:disabled,\n"
"QPushButton#btnToggleRight:disabled {\n"
"    background-color: #161D2B;\n"
"    border-color: #2A3444;\n"
"    color: #8A94A6;\n"
"}\n"
"\n"
"/* \345\210\206\347\273\204\346\241\206\346\240\267\345\274\217 */\n"
"QGroupBox {\n"
"    background-color: #161D2B;\n"
"    border: 1px solid #1E2636;\n"
"    border-radius: 8px;\n"
"    margin-top: 12px;\n"
"    padding: 16px 8px 8px 8px;\n"
"    font-weight: bold;\n"
""
                        "    color: #E8E9ED;\n"
"}\n"
"\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    subcontrol-position: top left;\n"
"    left: 12px;\n"
"    padding: 0 6px;\n"
"    color: #38BDF8;\n"
"}\n"
"\n"
"/* \346\240\207\347\255\276\346\240\267\345\274\217 */\n"
"QLabel {\n"
"    color: #C8D2E0;\n"
"    font-size: 9pt;\n"
"}\n"
"\n"
"/* \350\276\223\345\205\245\346\241\206\346\240\267\345\274\217 */\n"
"QLineEdit {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
"    border-radius: 6px;\n"
"    padding: 8px 12px;\n"
"    color: #E8E9ED;\n"
"    font-size: 10pt;\n"
"}\n"
"\n"
"QLineEdit:focus {\n"
"    border: 1px solid #38BDF8;\n"
"    background-color: #242D3D;\n"
"}\n"
"\n"
"QSpinBox {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
"    border-radius: 6px;\n"
"    padding: 6px 8px;\n"
"    color: #E8E9ED;\n"
"}\n"
"\n"
"/* \344\270\213\346\213\211\346\241\206\357\274\210\347\201\265\345\267\247\346\211\213\351\242\204\350\256\276\346\211\213\345\212\277"
                        "\347\255\211\357\274\211\357\274\232\344\270\216\350\276\223\345\205\245\346\241\206\345\220\214\350\211\262 */\n"
"QComboBox {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
"    border-radius: 6px;\n"
"    padding: 6px 10px;\n"
"    color: #E8E9ED;\n"
"    font-size: 10pt;\n"
"    min-height: 20px;\n"
"}\n"
"\n"
"QComboBox:hover, QComboBox:focus {\n"
"    border: 1px solid #38BDF8;\n"
"    background-color: #242D3D;\n"
"}\n"
"\n"
"QComboBox:disabled {\n"
"    background-color: #161D2B;\n"
"    color: #8A94A6;\n"
"    border: 1px solid #1E2636;\n"
"}\n"
"\n"
"/* \345\261\225\345\274\200\347\232\204\346\235\241\347\233\256\345\210\227\350\241\250\346\230\257\347\213\254\347\253\213\351\241\266\345\261\202\347\252\227\345\217\243\357\274\214\344\270\215\347\273\247\346\211\277\344\270\212\351\235\242\347\232\204 QComboBox \350\247\204\345\210\231\357\274\214\n"
"   \345\277\205\351\241\273\345\215\225\347\213\254\346\214\207\345\256\232\350\203\214\346\231\257/\346\226\207\345\255\227/"
                        "\351\200\211\344\270\255\346\200\201\357\274\214\345\220\246\345\210\231\346\235\241\347\233\256\346\226\207\345\255\227\344\274\232\345\233\236\351\200\200\346\210\220\347\263\273\347\273\237\346\267\261\350\211\262\357\274\214\n"
"   \345\216\213\345\234\250\346\267\261\350\211\262\345\272\225\344\270\212\345\207\240\344\271\216\347\234\213\344\270\215\350\247\201 */\n"
"QComboBox QAbstractItemView {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
"    border-radius: 6px;\n"
"    padding: 4px;\n"
"    color: #E8E9ED;\n"
"    outline: none;\n"
"    selection-background-color: #38BDF8;\n"
"    selection-color: #0A0D12;\n"
"}\n"
"\n"
"QComboBox QAbstractItemView::item {\n"
"    color: #E8E9ED;\n"
"    background-color: transparent;\n"
"    min-height: 26px;\n"
"    padding: 2px 6px;\n"
"}\n"
"\n"
"QComboBox QAbstractItemView::item:selected {\n"
"    background-color: #38BDF8;\n"
"    color: #0A0D12;\n"
"}\n"
"\n"
"QComboBox QAbstractItemView::item:hover {\n"
"    background-color: #2A34"
                        "44;\n"
"    color: #E8E9ED;\n"
"}\n"
"\n"
"/* \346\214\211\351\222\256\346\240\267\345\274\217 */\n"
"QPushButton {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
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
"QPushButton:disabled {\n"
"    background-color: #161D2B;\n"
"    color: #8A94A6;\n"
"    border: 1px solid #1E2636;\n"
"}\n"
"\n"
"/* \344\270\273\350\246\201\345\212\250\344\275\234\346\214\211\351\222\256 */\n"
"QPushButton#btnConnect,\n"
"QPushButton#btnEnableLeft,\n"
"QPushButton#btnEnableRight {\n"
"    background-color: #38BDF8;\n"
"    border: none;\n"
"    color: #0A0D12;\n"
"    font-weight: 600;\n"
"}\n"
"\n"
"QPushButton#btnConnect:hover,\n"
"QPushButton#btnEnableLeft:hover,\n"
"QPushButton#btnEnable"
                        "Right:hover {\n"
"    background-color: #4AC5FF;\n"
"}\n"
"\n"
"QPushButton#btnConnect:pressed,\n"
"QPushButton#btnEnableLeft:pressed,\n"
"QPushButton#btnEnableRight:pressed {\n"
"    background-color: #2AA9DD;\n"
"}\n"
"\n"
"/* \345\215\261\351\231\251\345\212\250\344\275\234\346\214\211\351\222\256 */\n"
"QPushButton#btnDisableLeft,\n"
"QPushButton#btnDisableRight {\n"
"    background-color: #E94560;\n"
"    border: none;\n"
"    color: #FFFFFF;\n"
"    font-weight: 600;\n"
"}\n"
"\n"
"QPushButton#btnDisableLeft:hover,\n"
"QPushButton#btnDisableRight:hover {\n"
"    background-color: #F05575;\n"
"}\n"
"\n"
"/* Tab\346\240\207\347\255\276\351\241\265\346\240\267\345\274\217 */\n"
"QTabWidget::pane {\n"
"    background-color: #161D2B;\n"
"    border: 1px solid #1E2636;\n"
"    border-radius: 8px;\n"
"    top: -1px;\n"
"}\n"
"\n"
"QTabBar::tab {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
"    border-bottom: none;\n"
"    border-top-left-radius: 6px;\n"
"    border-top-right-radius:"
                        " 6px;\n"
"    padding: 8px 20px;\n"
"    margin-right: 4px;\n"
"    color: #C8D2E0;\n"
"    font-weight: 500;\n"
"}\n"
"\n"
"QTabBar::tab:selected {\n"
"    background-color: #161D2B;\n"
"    color: #38BDF8;\n"
"    border-color: #38BDF8;\n"
"    border-bottom: 1px solid #161D2B;\n"
"}\n"
"\n"
"QTabBar::tab:hover:!selected {\n"
"    background-color: #242D3D;\n"
"    color: #E8E9ED;\n"
"}\n"
"\n"
"/* \346\273\232\345\212\250\345\214\272\345\237\237\346\240\267\345\274\217 */\n"
"QScrollArea {\n"
"    background-color: transparent;\n"
"    border: none;\n"
"}\n"
"\n"
"QScrollArea > QWidget > QWidget {\n"
"    background-color: transparent;\n"
"}\n"
"\n"
"QScrollBar:vertical {\n"
"    background-color: #1E2636;\n"
"    width: 10px;\n"
"    border-radius: 5px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical {\n"
"    background-color: #38BDF8;\n"
"    border-radius: 5px;\n"
"    min-height: 20px;\n"
"}\n"
"\n"
"QScrollBar::add-line:vertical,\n"
"QScrollBar::sub-line:vertical {\n"
"    height: 0px;\n"
"}\n"
"   "));
        verticalLayout = new QVBoxLayout(RobotControlPanel);
        verticalLayout->setSpacing(12);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(12, 12, 12, 12);
        connectionGroup = new QGroupBox(RobotControlPanel);
        connectionGroup->setObjectName("connectionGroup");
        verticalLayout_conn = new QVBoxLayout(connectionGroup);
        verticalLayout_conn->setSpacing(8);
        verticalLayout_conn->setObjectName("verticalLayout_conn");
        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        formLayout->setHorizontalSpacing(12);
        formLayout->setVerticalSpacing(8);
        labelHost = new QLabel(connectionGroup);
        labelHost->setObjectName("labelHost");

        formLayout->setWidget(0, QFormLayout::LabelRole, labelHost);

        lineEditHost = new QLineEdit(connectionGroup);
        lineEditHost->setObjectName("lineEditHost");

        formLayout->setWidget(0, QFormLayout::FieldRole, lineEditHost);

        labelPort = new QLabel(connectionGroup);
        labelPort->setObjectName("labelPort");

        formLayout->setWidget(1, QFormLayout::LabelRole, labelPort);

        spinBoxPort = new QSpinBox(connectionGroup);
        spinBoxPort->setObjectName("spinBoxPort");
        spinBoxPort->setMaximum(65535);
        spinBoxPort->setValue(9000);

        formLayout->setWidget(1, QFormLayout::FieldRole, spinBoxPort);


        verticalLayout_conn->addLayout(formLayout);

        btnConnect = new QPushButton(connectionGroup);
        btnConnect->setObjectName("btnConnect");

        verticalLayout_conn->addWidget(btnConnect);

        labelStatus = new QLabel(connectionGroup);
        labelStatus->setObjectName("labelStatus");
        labelStatus->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    color: #C8D2E0;\n"
"    background-color: #1E2636;\n"
"    border-radius: 4px;\n"
"    padding: 6px;\n"
"    font-size: 9pt;\n"
"}\n"
"         "));
        labelStatus->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_conn->addWidget(labelStatus);


        verticalLayout->addWidget(connectionGroup);

        tabWidget = new QTabWidget(RobotControlPanel);
        tabWidget->setObjectName("tabWidget");
        tabLeftArm = new QWidget();
        tabLeftArm->setObjectName("tabLeftArm");
        verticalLayout_left = new QVBoxLayout(tabLeftArm);
        verticalLayout_left->setSpacing(8);
        verticalLayout_left->setObjectName("verticalLayout_left");
        horizontalLayout_left = new QHBoxLayout();
        horizontalLayout_left->setSpacing(8);
        horizontalLayout_left->setObjectName("horizontalLayout_left");
        btnToggleLeft = new QPushButton(tabLeftArm);
        btnToggleLeft->setObjectName("btnToggleLeft");
        btnToggleLeft->setEnabled(false);
        btnToggleLeft->setCheckable(true);

        horizontalLayout_left->addWidget(btnToggleLeft);


        verticalLayout_left->addLayout(horizontalLayout_left);

        btnGetCurrentAnglesLeft = new QPushButton(tabLeftArm);
        btnGetCurrentAnglesLeft->setObjectName("btnGetCurrentAnglesLeft");
        btnGetCurrentAnglesLeft->setEnabled(false);

        verticalLayout_left->addWidget(btnGetCurrentAnglesLeft);

        scrollAreaLeft = new QScrollArea(tabLeftArm);
        scrollAreaLeft->setObjectName("scrollAreaLeft");
        scrollAreaLeft->setWidgetResizable(true);
        scrollAreaWidgetContents_left = new QWidget();
        scrollAreaWidgetContents_left->setObjectName("scrollAreaWidgetContents_left");
        scrollAreaWidgetContents_left->setGeometry(QRect(0, 0, 319, 242));
        verticalLayout_3 = new QVBoxLayout(scrollAreaWidgetContents_left);
        verticalLayout_3->setObjectName("verticalLayout_3");
        scrollAreaLeft->setWidget(scrollAreaWidgetContents_left);

        verticalLayout_left->addWidget(scrollAreaLeft);

        tabWidget->addTab(tabLeftArm, QString());
        tabRightArm = new QWidget();
        tabRightArm->setObjectName("tabRightArm");
        verticalLayout_right = new QVBoxLayout(tabRightArm);
        verticalLayout_right->setSpacing(8);
        verticalLayout_right->setObjectName("verticalLayout_right");
        horizontalLayout_right = new QHBoxLayout();
        horizontalLayout_right->setSpacing(8);
        horizontalLayout_right->setObjectName("horizontalLayout_right");
        btnToggleRight = new QPushButton(tabRightArm);
        btnToggleRight->setObjectName("btnToggleRight");
        btnToggleRight->setEnabled(false);
        btnToggleRight->setCheckable(true);

        horizontalLayout_right->addWidget(btnToggleRight);


        verticalLayout_right->addLayout(horizontalLayout_right);

        btnGetCurrentAnglesRight = new QPushButton(tabRightArm);
        btnGetCurrentAnglesRight->setObjectName("btnGetCurrentAnglesRight");
        btnGetCurrentAnglesRight->setEnabled(false);

        verticalLayout_right->addWidget(btnGetCurrentAnglesRight);

        scrollAreaRight = new QScrollArea(tabRightArm);
        scrollAreaRight->setObjectName("scrollAreaRight");
        scrollAreaRight->setWidgetResizable(true);
        scrollAreaWidgetContents_right = new QWidget();
        scrollAreaWidgetContents_right->setObjectName("scrollAreaWidgetContents_right");
        scrollAreaWidgetContents_right->setGeometry(QRect(0, 0, 319, 242));
        verticalLayout_5 = new QVBoxLayout(scrollAreaWidgetContents_right);
        verticalLayout_5->setObjectName("verticalLayout_5");
        scrollAreaRight->setWidget(scrollAreaWidgetContents_right);

        verticalLayout_right->addWidget(scrollAreaRight);

        tabWidget->addTab(tabRightArm, QString());
        tabHand = new QWidget();
        tabHand->setObjectName("tabHand");
        verticalLayout_hand = new QVBoxLayout(tabHand);
        verticalLayout_hand->setSpacing(8);
        verticalLayout_hand->setObjectName("verticalLayout_hand");
        labelHandHint = new QLabel(tabHand);
        labelHandHint->setObjectName("labelHandHint");
        labelHandHint->setStyleSheet(QString::fromUtf8("color: #8A94A6; font-size: 9pt;"));
        labelHandHint->setWordWrap(true);

        verticalLayout_hand->addWidget(labelHandHint);

        btnConnectHand = new QPushButton(tabHand);
        btnConnectHand->setObjectName("btnConnectHand");

        verticalLayout_hand->addWidget(btnConnectHand);

        labelHandStatus = new QLabel(tabHand);
        labelHandStatus->setObjectName("labelHandStatus");
        labelHandStatus->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    color: #C8D2E0;\n"
"    background-color: #1E2636;\n"
"    border-radius: 4px;\n"
"    padding: 6px;\n"
"    font-size: 9pt;\n"
"}\n"
"         "));
        labelHandStatus->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_hand->addWidget(labelHandStatus);

        btnRefreshHand = new QPushButton(tabHand);
        btnRefreshHand->setObjectName("btnRefreshHand");
        btnRefreshHand->setEnabled(false);

        verticalLayout_hand->addWidget(btnRefreshHand);

        comboHandPreset = new QComboBox(tabHand);
        comboHandPreset->setObjectName("comboHandPreset");

        verticalLayout_hand->addWidget(comboHandPreset);

        btnApplyHandPreset = new QPushButton(tabHand);
        btnApplyHandPreset->setObjectName("btnApplyHandPreset");
        btnApplyHandPreset->setEnabled(false);

        verticalLayout_hand->addWidget(btnApplyHandPreset);

        verticalSpacerHand = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_hand->addItem(verticalSpacerHand);

        tabWidget->addTab(tabHand, QString());

        verticalLayout->addWidget(tabWidget);


        retranslateUi(RobotControlPanel);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(RobotControlPanel);
    } // setupUi

    void retranslateUi(QWidget *RobotControlPanel)
    {
        RobotControlPanel->setWindowTitle(QCoreApplication::translate("RobotControlPanel", "\346\234\272\345\231\250\344\272\272\346\216\247\345\210\266", nullptr));
        connectionGroup->setTitle(QCoreApplication::translate("RobotControlPanel", "\346\234\272\345\231\250\344\272\272\350\277\236\346\216\245", nullptr));
        labelHost->setText(QCoreApplication::translate("RobotControlPanel", "IP\345\234\260\345\235\200:", nullptr));
        lineEditHost->setText(QCoreApplication::translate("RobotControlPanel", "192.168.100.16", nullptr));
        lineEditHost->setPlaceholderText(QCoreApplication::translate("RobotControlPanel", "\350\276\223\345\205\245\346\234\272\345\231\250\344\272\272\346\216\247\345\210\266\345\231\250IP", nullptr));
        labelPort->setText(QCoreApplication::translate("RobotControlPanel", "\347\253\257\345\217\243:", nullptr));
        btnConnect->setText(QCoreApplication::translate("RobotControlPanel", "\350\277\236\346\216\245\346\234\272\345\231\250\344\272\272", nullptr));
        labelStatus->setText(QCoreApplication::translate("RobotControlPanel", "\342\227\217 \346\234\252\350\277\236\346\216\245", nullptr));
        btnToggleLeft->setText(QCoreApplication::translate("RobotControlPanel", "\344\275\277\350\203\275", nullptr));
        btnGetCurrentAnglesLeft->setText(QCoreApplication::translate("RobotControlPanel", "\360\237\223\213 \350\216\267\345\217\226\345\275\223\345\211\215\350\247\222\345\272\246", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabLeftArm), QCoreApplication::translate("RobotControlPanel", "\345\267\246\350\207\202\346\216\247\345\210\266", nullptr));
        btnToggleRight->setText(QCoreApplication::translate("RobotControlPanel", "\344\275\277\350\203\275", nullptr));
        btnGetCurrentAnglesRight->setText(QCoreApplication::translate("RobotControlPanel", "\360\237\223\213 \350\216\267\345\217\226\345\275\223\345\211\215\350\247\222\345\272\246", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabRightArm), QCoreApplication::translate("RobotControlPanel", "\345\217\263\350\207\202\346\216\247\345\210\266", nullptr));
        labelHandHint->setText(QCoreApplication::translate("RobotControlPanel", "\347\201\265\345\267\247\346\211\213\347\273\217\346\216\247\345\210\266\345\231\250 rs485 \350\275\254\345\217\221\357\274\214\351\234\200\345\215\225\347\213\254\350\277\236\346\216\245\343\200\202\350\277\236\346\216\245\346\210\220\345\212\237\345\220\216\357\274\214\346\222\255\346\224\276\345\212\250\344\275\234\345\272\217\345\210\227\346\227\266\344\274\232\350\207\252\345\212\250\346\214\211\346\211\213\347\211\207\346\256\265\344\270\213\345\217\221\346\211\213\344\275\215\357\274\233\346\234\252\350\277\236\346\216\245\346\227\266\346\211\213\344\275\215\344\270\215\344\270\213\345\217\221\343\200\202", nullptr));
        btnConnectHand->setText(QCoreApplication::translate("RobotControlPanel", "\350\277\236\346\216\245\347\201\265\345\267\247\346\211\213", nullptr));
        labelHandStatus->setText(QCoreApplication::translate("RobotControlPanel", "\342\227\217 \346\234\252\350\277\236\346\216\245", nullptr));
        btnRefreshHand->setText(QCoreApplication::translate("RobotControlPanel", "\360\237\224\204 \350\257\273\345\217\226\345\275\223\345\211\215\346\211\213\344\275\215", nullptr));
        btnApplyHandPreset->setText(QCoreApplication::translate("RobotControlPanel", "\342\234\213 \346\211\247\350\241\214\351\242\204\350\256\276\346\211\213\345\212\277", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabHand), QCoreApplication::translate("RobotControlPanel", "\347\201\265\345\267\247\346\211\213", nullptr));
    } // retranslateUi

};

namespace Ui {
    class RobotControlPanel: public Ui_RobotControlPanel {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ROBOTCONTROLPANEL_H

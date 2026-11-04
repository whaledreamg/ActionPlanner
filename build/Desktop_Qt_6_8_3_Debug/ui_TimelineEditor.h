/********************************************************************************
** Form generated from reading UI file 'TimelineEditor.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TIMELINEEDITOR_H
#define UI_TIMELINEEDITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TimelineEditor
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *toolbarContainer;
    QLabel *labelTitle;
    QPushButton *btnPlay;
    QPushButton *btnPause;
    QPushButton *btnStop;
    QCheckBox *checkLoop;
    QLabel *labelProgress;
    QPushButton *btnAddSegment;
    QPushButton *btnAddHandSegment;
    QPushButton *btnDeleteSegment;
    QPushButton *btnDuplicateSegment;
    QLabel *labelSeparator;
    QPushButton *btnSave;
    QPushButton *btnLoad;
    QPushButton *btnSettings;
    QWidget *timelineCanvas;

    void setupUi(QWidget *TimelineEditor)
    {
        if (TimelineEditor->objectName().isEmpty())
            TimelineEditor->setObjectName("TimelineEditor");
        TimelineEditor->resize(1200, 280);
        TimelineEditor->setMinimumSize(QSize(0, 0));
        TimelineEditor->setMaximumSize(QSize(16777215, 16777215));
        TimelineEditor->setStyleSheet(QString::fromUtf8("\n"
"QWidget#TimelineEditor {\n"
"    background-color: #0F131C;\n"
"    border-top: 1px solid #1E2636;\n"
"}\n"
"/* \345\267\245\345\205\267\346\240\217\346\214\211\351\222\256\346\240\267\345\274\217 */\n"
"QPushButton {\n"
"    background-color: #1E2636;\n"
"    border: 1px solid #2A3444;\n"
"    border-radius: 6px;\n"
"    padding: 8px 16px;\n"
"    color: #E8E9ED;\n"
"    font-size: 10pt;\n"
"    font-weight: 500;\n"
"}\n"
"QPushButton:hover {\n"
"    background-color: #242D3D;\n"
"    border: 1px solid #38BDF8;\n"
"}\n"
"QPushButton:pressed {\n"
"    background-color: #2A3444;\n"
"}\n"
"QPushButton:disabled {\n"
"    background-color: #161D2B;\n"
"    color: #8A94A6;\n"
"    border: 1px solid #1E2636;\n"
"}\n"
"/* \344\270\273\350\246\201\346\223\215\344\275\234\346\214\211\351\222\256 */\n"
"QPushButton#btnPlay {\n"
"    background-color: #6EE7B7;\n"
"    border: none;\n"
"    color: #0A0D12;\n"
"    font-weight: 600;\n"
"    padding: 10px 24px;\n"
"}\n"
"QPushButton#btnPlay:hover {\n"
"    background-c"
                        "olor: #7FEFC5;\n"
"}\n"
"QPushButton#btnAddSegment {\n"
"    background-color: #38BDF8;\n"
"    border: none;\n"
"    color: #0A0D12;\n"
"    font-weight: 600;\n"
"}\n"
"QPushButton#btnAddSegment:hover {\n"
"    background-color: #4AC5FF;\n"
"}\n"
"QPushButton#btnDeleteSegment {\n"
"    background-color: #E94560;\n"
"    border: none;\n"
"    color: #FFFFFF;\n"
"    font-weight: 600;\n"
"}\n"
"QPushButton#btnDeleteSegment:hover {\n"
"    background-color: #F05575;\n"
"}\n"
"/* \346\240\207\347\255\276\346\240\267\345\274\217 */\n"
"QLabel {\n"
"    color: #C8D2E0;\n"
"    font-size: 10pt;\n"
"}\n"
"/* \345\276\252\347\216\257\345\213\276\351\200\211 */\n"
"QCheckBox {\n"
"    color: #C8D2E0;\n"
"    font-size: 10pt;\n"
"    spacing: 6px;\n"
"}\n"
"QCheckBox:hover {\n"
"    color: #38BDF8;\n"
"}\n"
"QCheckBox::indicator {\n"
"    width: 15px;\n"
"    height: 15px;\n"
"    border: 1px solid #2A3444;\n"
"    border-radius: 4px;\n"
"    background-color: #161D2B;\n"
"}\n"
"QCheckBox::indicator:hover {\n"
"    bord"
                        "er: 1px solid #38BDF8;\n"
"}\n"
"QCheckBox::indicator:checked {\n"
"    background-color: #38BDF8;\n"
"    border: 1px solid #38BDF8;\n"
"}\n"
"   "));
        verticalLayout = new QVBoxLayout(TimelineEditor);
        verticalLayout->setSpacing(8);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(12, 12, 12, 12);
        toolbarContainer = new QWidget(TimelineEditor);
        toolbarContainer->setObjectName("toolbarContainer");
        labelTitle = new QLabel(toolbarContainer);
        labelTitle->setObjectName("labelTitle");
        labelTitle->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    font-size: 12pt;\n"
"    font-weight: bold;\n"
"    color: #38BDF8;\n"
"}\n"
"        "));
        btnPlay = new QPushButton(toolbarContainer);
        btnPlay->setObjectName("btnPlay");
        btnPause = new QPushButton(toolbarContainer);
        btnPause->setObjectName("btnPause");
        btnPause->setEnabled(false);
        btnStop = new QPushButton(toolbarContainer);
        btnStop->setObjectName("btnStop");
        btnStop->setEnabled(false);
        checkLoop = new QCheckBox(toolbarContainer);
        checkLoop->setObjectName("checkLoop");
        labelProgress = new QLabel(toolbarContainer);
        labelProgress->setObjectName("labelProgress");
        labelProgress->setMinimumSize(QSize(110, 0));
        labelProgress->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    color: #C8D2E0;\n"
"    font-family: \"Consolas\", \"DejaVu Sans Mono\", monospace;\n"
"}\n"
"        "));
        btnAddSegment = new QPushButton(toolbarContainer);
        btnAddSegment->setObjectName("btnAddSegment");
        btnAddHandSegment = new QPushButton(toolbarContainer);
        btnAddHandSegment->setObjectName("btnAddHandSegment");
        btnDeleteSegment = new QPushButton(toolbarContainer);
        btnDeleteSegment->setObjectName("btnDeleteSegment");
        btnDuplicateSegment = new QPushButton(toolbarContainer);
        btnDuplicateSegment->setObjectName("btnDuplicateSegment");
        btnDuplicateSegment->setEnabled(false);
        labelSeparator = new QLabel(toolbarContainer);
        labelSeparator->setObjectName("labelSeparator");
        labelSeparator->setStyleSheet(QString::fromUtf8("\n"
"QLabel {\n"
"    color: #2A3444;\n"
"    font-size: 14pt;\n"
"}\n"
"        "));
        btnSave = new QPushButton(toolbarContainer);
        btnSave->setObjectName("btnSave");
        btnLoad = new QPushButton(toolbarContainer);
        btnLoad->setObjectName("btnLoad");
        btnSettings = new QPushButton(toolbarContainer);
        btnSettings->setObjectName("btnSettings");

        verticalLayout->addWidget(toolbarContainer);

        timelineCanvas = new QWidget(TimelineEditor);
        timelineCanvas->setObjectName("timelineCanvas");
        timelineCanvas->setMinimumSize(QSize(0, 180));

        verticalLayout->addWidget(timelineCanvas);


        retranslateUi(TimelineEditor);

        QMetaObject::connectSlotsByName(TimelineEditor);
    } // setupUi

    void retranslateUi(QWidget *TimelineEditor)
    {
        TimelineEditor->setWindowTitle(QCoreApplication::translate("TimelineEditor", "\346\227\266\351\227\264\350\275\264\347\274\226\350\276\221\345\231\250", nullptr));
        labelTitle->setText(QCoreApplication::translate("TimelineEditor", "\345\212\250\344\275\234\346\227\266\351\227\264\350\275\264", nullptr));
        btnPlay->setText(QCoreApplication::translate("TimelineEditor", "\342\226\266 \346\222\255\346\224\276", nullptr));
        btnPause->setText(QCoreApplication::translate("TimelineEditor", "\342\217\270 \346\232\202\345\201\234", nullptr));
        btnStop->setText(QCoreApplication::translate("TimelineEditor", "\342\217\271 \345\201\234\346\255\242", nullptr));
        checkLoop->setText(QCoreApplication::translate("TimelineEditor", "\360\237\224\201 \345\276\252\347\216\257", nullptr));
#if QT_CONFIG(tooltip)
        checkLoop->setToolTip(QCoreApplication::translate("TimelineEditor", "\346\222\255\345\210\260\346\234\253\345\260\276\345\220\216\350\207\252\345\212\250\344\273\216\345\244\264\347\273\247\347\273\255\357\274\214\347\233\264\345\210\260\346\211\213\345\212\250\345\201\234\346\255\242", nullptr));
#endif // QT_CONFIG(tooltip)
        labelProgress->setText(QCoreApplication::translate("TimelineEditor", "0.00 / 0.00 s", nullptr));
#if QT_CONFIG(tooltip)
        btnAddSegment->setToolTip(QCoreApplication::translate("TimelineEditor", "\346\267\273\345\212\240\344\270\200\346\256\265\346\234\272\346\242\260\350\207\202\345\205\263\350\212\202\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        btnAddSegment->setText(QCoreApplication::translate("TimelineEditor", "\342\236\225 \346\234\272\346\242\260\350\207\202\347\211\207\346\256\265", nullptr));
#if QT_CONFIG(tooltip)
        btnAddHandSegment->setToolTip(QCoreApplication::translate("TimelineEditor", "\346\267\273\345\212\240\344\270\200\346\256\265\347\201\265\345\267\247\346\211\213\345\212\250\344\275\234\357\274\210\346\234\272\346\242\260\350\207\202\344\277\235\346\214\201\344\270\215\345\212\250\357\274\211", nullptr));
#endif // QT_CONFIG(tooltip)
        btnAddHandSegment->setText(QCoreApplication::translate("TimelineEditor", "\342\234\213 \345\244\271\347\210\252\347\211\207\346\256\265", nullptr));
        btnDeleteSegment->setText(QCoreApplication::translate("TimelineEditor", "\360\237\227\221 \345\210\240\351\231\244\347\211\207\346\256\265", nullptr));
#if QT_CONFIG(tooltip)
        btnDuplicateSegment->setToolTip(QCoreApplication::translate("TimelineEditor", "\345\244\215\345\210\266\345\275\223\345\211\215\351\200\211\344\270\255\347\232\204\347\211\207\346\256\265\357\274\214\350\277\275\345\212\240\345\210\260\346\227\266\351\227\264\350\275\264\346\234\253\345\260\276", nullptr));
#endif // QT_CONFIG(tooltip)
        btnDuplicateSegment->setText(QCoreApplication::translate("TimelineEditor", "\342\247\211 \345\244\215\345\210\266\347\211\207\346\256\265", nullptr));
        labelSeparator->setText(QCoreApplication::translate("TimelineEditor", "|", nullptr));
        btnSave->setText(QCoreApplication::translate("TimelineEditor", "\360\237\222\276 \344\277\235\345\255\230", nullptr));
        btnLoad->setText(QCoreApplication::translate("TimelineEditor", "\360\237\223\201 \345\212\240\350\275\275", nullptr));
#if QT_CONFIG(tooltip)
        btnSettings->setToolTip(QCoreApplication::translate("TimelineEditor", "\350\256\276\347\275\256\346\227\266\351\227\264\350\275\264\351\225\277\345\272\246\343\200\201\351\273\230\350\256\244\347\211\207\346\256\265\346\227\266\351\225\277\343\200\201\344\270\213\345\217\221\351\242\221\347\216\207\347\255\211", nullptr));
#endif // QT_CONFIG(tooltip)
        btnSettings->setText(QCoreApplication::translate("TimelineEditor", "\342\232\231 \350\256\276\347\275\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TimelineEditor: public Ui_TimelineEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TIMELINEEDITOR_H

#include "ui/SegmentEditDialog.h"
#include "ui/DialogUtil.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

SegmentEditDialog::SegmentEditDialog(const Segment& segment,
                                     const JointState& derivedStart,
                                     QWidget *parent)
    : QDialog(parent)
    , m_result(segment)
    , m_derivedStart(derivedStart)
    , m_limits(JointLimitsLoader::load())
{
    setModal(true);
    setStyleSheet(DialogUtil::dialogStyleSheet());
    resize(760, 640);
}

void SegmentEditDialog::finishSetup()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    // 手机屏幕高度有限：属性区 + 关节/手位表很容易超出屏幕，
    // 底部「确定/取消」会被挤出可视区（表现为点不到）。
    // 因此把整个主体放进滚动区，按钮行固定在底部——移动端对话框的标准布局。
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_body = new QWidget(m_scroll);
    m_bodyLayout = new QVBoxLayout(m_body);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(10);

    // ---------------- 基本属性（所有片段共有） ----------------
    m_basicGroup = new QGroupBox(QStringLiteral("片段属性"), m_body);
    auto *form = new QGridLayout(m_basicGroup);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(8);

    form->addWidget(new QLabel(QStringLiteral("标签")), 0, 0);
    m_labelEdit = new QLineEdit(m_basicGroup);
    form->addWidget(m_labelEdit, 0, 1, 1, 3);

    form->addWidget(new QLabel(QStringLiteral("起始时间 (s)")), 1, 0);
    m_startSpin = new QDoubleSpinBox(m_basicGroup);
    m_startSpin->setRange(0.0, 36000.0);
    m_startSpin->setDecimals(2);
    m_startSpin->setSingleStep(0.1);
    form->addWidget(m_startSpin, 1, 1);

    form->addWidget(new QLabel(QStringLiteral("时长 (s)")), 1, 2);
    m_durationSpin = new QDoubleSpinBox(m_basicGroup);
    m_durationSpin->setRange(0.1, 3600.0);
    m_durationSpin->setDecimals(2);
    m_durationSpin->setSingleStep(0.1);
    form->addWidget(m_durationSpin, 1, 3);

    form->setColumnStretch(1, 1);
    form->setColumnStretch(3, 1);

    m_bodyLayout->addWidget(m_basicGroup);

    // ---------------- 子类的编辑区 ----------------
    buildEditor(m_bodyLayout);
    m_bodyLayout->addStretch();

    m_scroll->setWidget(m_body);
    root->addWidget(m_scroll, 1);

    // ---------------- 底部按钮（固定在窗口底部，不随内容滚动） ----------------
    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    m_buttons->button(QDialogButtonBox::Ok)->setDefault(true);
    m_buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));

    // 触屏上把按钮加高，手指更容易点中
    const int btnHeight = DialogUtil::touchButtonHeight();
    if (btnHeight > 0) {
        m_buttons->button(QDialogButtonBox::Ok)->setMinimumHeight(btnHeight);
        m_buttons->button(QDialogButtonBox::Cancel)->setMinimumHeight(btnHeight);
    }
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted,
            this, &SegmentEditDialog::validateAndAccept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 公共字段的变更统一走 notifyChanged()
    connect(m_labelEdit, &QLineEdit::textChanged,
            this, [this](const QString&) { notifyChanged(); });
    connect(m_startSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { notifyChanged(); });
    connect(m_durationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { notifyChanged(); });

    // 用模型值填充公共字段
    m_updating = true;
    m_labelEdit->setText(m_result.label);
    m_startSpin->setValue(m_result.startTime);
    m_durationSpin->setValue(m_result.duration);
    m_updating = false;

    // 布局建立之后再适配：设计尺寸在手机上会超出屏幕，
    // 底部的「确定/取消」被挤到可视区之外，点了没反应。
    DialogUtil::adaptForMobile(this);
}

void SegmentEditDialog::notifyChanged()
{
    if (m_updating) {
        return;
    }
    if (m_labelEdit) {
        m_result.label = m_labelEdit->text().trimmed();
    }
    if (m_startSpin) {
        m_result.startTime = qMax(0.0, m_startSpin->value());
    }
    if (m_durationSpin) {
        m_result.duration = qMax(0.1, m_durationSpin->value());
    }

    syncResultFromUi();
    emitPreview();
}

void SegmentEditDialog::setCurrentAngles(const QVector<double>& leftArm,
                                         const QVector<double>& rightArm)
{
    m_currentLeft = leftArm;
    m_currentRight = rightArm;
    while (m_currentLeft.size() < 7) {
        m_currentLeft.append(0.0);
    }
    while (m_currentRight.size() < 7) {
        m_currentRight.append(0.0);
    }
}

void SegmentEditDialog::setCurrentHand(const QVector<int>& leftHand,
                                       const QVector<int>& rightHand,
                                       bool handAvailable)
{
    m_currentHandLeft = leftHand;
    m_currentHandRight = rightHand;
    m_handAvailable = handAvailable;
}

void SegmentEditDialog::validateAndAccept()
{
    notifyChanged();

    if (m_result.label.isEmpty()) {
        m_result.label = QStringLiteral("片段");
    }
    if (m_result.duration < 0.1) {
        m_result.duration = 0.1;
    }

    // 限位检查只提示，不阻止（调试时常需要越界观察）
    const QStringList warnings = collectWarnings();
    if (!warnings.isEmpty()) {
        // 不用 QMessageBox：Android 上模态 QMessageBox 的按钮收不到触摸事件
        const bool proceed = DialogUtil::confirm(
            this, QStringLiteral("角度超出限位"),
            QStringLiteral("以下关节角超出 URDF 限位：\n\n%1\n\n仍然保存吗？")
                .arg(warnings.mid(0, 10).join(QLatin1Char('\n'))),
            QStringLiteral("仍然保存"), QStringLiteral("返回修改"));
        if (!proceed) {
            return;
        }
    }

    accept();
}

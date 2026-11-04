#include "ui/HandSegmentEditDialog.h"
#include "ui/DialogUtil.h"
#include "robot/HandController.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

HandSegmentEditDialog::HandSegmentEditDialog(const Segment& segment,
                                             const JointState& derivedStart,
                                             QWidget *parent)
    : SegmentEditDialog(segment, derivedStart, parent)
{
    setWindowTitle(QStringLiteral("编辑夹爪片段 — %1").arg(segment.label));
    finishSetup();
}

void HandSegmentEditDialog::buildEditor(QVBoxLayout *body)
{
    body->addWidget(createHead());
    body->addWidget(createHandGroup(true));
    body->addWidget(createHandGroup(false));

    auto *hint = new QLabel(
        QStringLiteral("手位为归一化 0~1000，右侧显示对应物理角度。"
                       "夹爪片段在进入时下发一次目标，整段不再重复发；"
                       "机械臂在整段期间保持不动。"),
        m_body);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color:#8A94A6; font-size:9pt;"));
    body->addWidget(hint);

    // 用模型值填充滑条
    m_updating = true;
    for (int side = 0; side < 2; ++side) {
        for (int d = 0; d < HandPose::kDofCount; ++d) {
            if (m_slider[side][d]) {
                m_slider[side][d]->setValue(handValueOf(side == 0, d));
            }
        }
    }
    m_updating = false;

    syncValueLabels();
}

QWidget* HandSegmentEditDialog::createHead()
{
    auto *head = new QWidget(m_body);
    auto *layout = new QVBoxLayout(head);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *hint = new QLabel(
        QStringLiteral("夹爪片段只驱动灵巧手，机械臂整段保持不动。"), head);
    hint->setStyleSheet(QStringLiteral("color:#5EEAD4; font-size:9pt;"));
    layout->addWidget(hint);

    auto *row = new QHBoxLayout();
    row->setSpacing(8);

    row->addWidget(new QLabel(QStringLiteral("预设"), head));
    m_presetCombo = new QComboBox(head);
    m_presetCombo->addItem(QStringLiteral("—"), QString());
    for (const QString &name : HandProtocol::presetNames()) {
        m_presetCombo->addItem(name, name);
    }
    row->addWidget(m_presetCombo);

    m_btnFromCurrent = new QPushButton(QStringLiteral("取当前手位"), head);
    m_btnFromCurrent->setToolTip(
        QStringLiteral("读回灵巧手当前手位填入本段（需先连接灵巧手）"));
    m_btnFromCurrent->setEnabled(m_handAvailable);
    row->addWidget(m_btnFromCurrent);

    row->addStretch();
    layout->addLayout(row);

    // ---- 联动 ----
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                const QString name = m_presetCombo->itemData(index).toString();
                if (name.isEmpty()) {
                    return;
                }
                const QVector<int> pos = HandProtocol::preset(name);
                if (pos.size() != HandProtocol::kDofCount) {
                    return;
                }
                m_updating = true;
                for (int d = 0; d < HandProtocol::kDofCount; ++d) {
                    setHandValue(true, d, pos.at(d));
                    setHandValue(false, d, pos.at(d));
                    if (m_slider[0][d]) m_slider[0][d]->setValue(pos.at(d));
                    if (m_slider[1][d]) m_slider[1][d]->setValue(pos.at(d));
                }
                m_updating = false;

                syncValueLabels();
                notifyChanged();
            });

    connect(m_btnFromCurrent, &QPushButton::clicked, this, [this]() {
        m_updating = true;
        for (int d = 0; d < HandProtocol::kDofCount; ++d) {
            const int lv = m_currentHandLeft.value(d, 0);
            const int rv = m_currentHandRight.value(d, 0);
            setHandValue(true, d, lv);
            setHandValue(false, d, rv);
            if (m_slider[0][d]) m_slider[0][d]->setValue(lv);
            if (m_slider[1][d]) m_slider[1][d]->setValue(rv);
        }
        m_updating = false;

        syncValueLabels();
        notifyChanged();
    });

    return head;
}

QWidget* HandSegmentEditDialog::createHandGroup(bool leftHand)
{
    using namespace HandProtocol;
    const int side = leftHand ? 0 : 1;

    auto *group = new QGroupBox(leftHand ? QStringLiteral("左手") : QStringLiteral("右手"), m_body);
    auto *gl = new QGridLayout(group);
    gl->setHorizontalSpacing(8);
    gl->setVerticalSpacing(DialogUtil::touchSliderRowSpacing());

    for (int d = 0; d < kDofCount; ++d) {
        auto *name = new QLabel(dofNamesCn().value(d), group);
        name->setStyleSheet(QStringLiteral("color:#E8E9ED; font-weight:600;"));
        name->setMinimumWidth(70);
        gl->addWidget(name, d, 0);

        auto *range = new QLabel(QStringLiteral("0~%1°").arg(kMaxDeg[d]), group);
        range->setStyleSheet(QStringLiteral("color:#8A94A6; font-size:9pt;"));
        range->setMinimumWidth(50);
        gl->addWidget(range, d, 1);

        auto *slider = new QSlider(Qt::Horizontal, group);
        slider->setRange(0, 1000);
        slider->setMinimumWidth(200);
        // 触屏上给滑条足够的高度，滑块（见 DialogUtil::sliderStyleSheet）
        // 才有地方画大，手指也更容易压中而不误触相邻行
        const int sliderHeight = DialogUtil::touchSliderHeight();
        if (sliderHeight > 0) {
            slider->setMinimumHeight(sliderHeight);
        }
        m_slider[side][d] = slider;
        gl->addWidget(slider, d, 2);

        auto *val = new QLabel(group);
        val->setStyleSheet(
            QStringLiteral("color:#B8C4D6; font-family:Consolas,monospace;"));
        val->setMinimumWidth(110);
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_valueLabel[side][d] = val;
        gl->addWidget(val, d, 3);

        connect(slider, &QSlider::valueChanged, this, [this, side, d](int v) {
            if (m_updating) {
                return;
            }
            setHandValue(side == 0, d, v);
            syncValueLabels();
            notifyChanged();
        });
    }

    gl->setColumnStretch(2, 1);
    return group;
}

// ---------------------------------------------------------------- 手位读写

int HandSegmentEditDialog::handValueOf(bool leftHand, int dof) const
{
    if (dof < 0 || dof >= HandPose::kDofCount) {
        return 0;
    }
    return leftHand ? m_result.leftHand.dof[dof] : m_result.rightHand.dof[dof];
}

void HandSegmentEditDialog::setHandValue(bool leftHand, int dof, int value)
{
    if (dof < 0 || dof >= HandPose::kDofCount) {
        return;
    }
    const int v = qBound(0, value, 1000);
    if (leftHand) {
        m_result.leftHand.dof[dof] = v;
    } else {
        m_result.rightHand.dof[dof] = v;
    }
}

void HandSegmentEditDialog::syncValueLabels()
{
    // 标签显示归一化值 + 物理角度
    for (int side = 0; side < 2; ++side) {
        for (int d = 0; d < HandPose::kDofCount; ++d) {
            QLabel *label = m_valueLabel[side][d];
            if (!label) {
                continue;
            }
            const int v = handValueOf(side == 0, d);
            label->setText(QStringLiteral("%1  (%2°)")
                               .arg(v, 3)
                               .arg(HandProtocol::rawToDeg(v, d), 0, 'f', 1));
        }
    }
}

void HandSegmentEditDialog::refreshEditor()
{
    m_updating = true;
    for (int side = 0; side < 2; ++side) {
        for (int d = 0; d < HandPose::kDofCount; ++d) {
            if (m_slider[side][d]) {
                m_slider[side][d]->setValue(handValueOf(side == 0, d));
            }
        }
    }
    m_updating = false;

    syncValueLabels();
}

void HandSegmentEditDialog::syncResultFromUi()
{
    for (int side = 0; side < 2; ++side) {
        for (int d = 0; d < HandPose::kDofCount; ++d) {
            if (m_slider[side][d]) {
                setHandValue(side == 0, d, m_slider[side][d]->value());
            }
        }
    }
}

void HandSegmentEditDialog::setCurrentHand(const QVector<int>& leftHand,
                                           const QVector<int>& rightHand,
                                           bool handAvailable)
{
    SegmentEditDialog::setCurrentHand(leftHand, rightHand, handAvailable);
    if (m_btnFromCurrent) {
        m_btnFromCurrent->setEnabled(handAvailable);
    }
}

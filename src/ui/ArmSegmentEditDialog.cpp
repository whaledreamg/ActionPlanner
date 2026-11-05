#include "ui/ArmSegmentEditDialog.h"
#include "ui/DialogUtil.h"
#include "ui/TouchComboBox.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QtMath>

namespace {
constexpr int kNumJoints = 7;
const double kRadToDeg = 180.0 / M_PI;

QString jointName(int joint) { return QStringLiteral("J%1").arg(joint + 1); }
} // namespace

ArmSegmentEditDialog::ArmSegmentEditDialog(const Segment& segment,
                                           const JointState& derivedStart,
                                           QWidget *parent)
    : SegmentEditDialog(segment, derivedStart, parent)
{
    setWindowTitle(QStringLiteral("编辑机械臂片段 — %1").arg(segment.label));
    finishSetup();
}

void ArmSegmentEditDialog::buildEditor(QVBoxLayout *body)
{
    body->addWidget(createToolbar());

    // 左右臂上下排布：窄屏（手机竖屏）时是两段纵向列表，
    // 宽屏时也不会被并排挤成两列细窄的表格。
    body->addWidget(createArmGroup(true));
    body->addWidget(createArmGroup(false));

    refreshEditor();
}

QWidget* ArmSegmentEditDialog::createToolbar()
{
    auto *bar = new QWidget(m_body);
    auto *layout = new QVBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // ---- 第一行：批量操作 ----
    auto *tools = new QHBoxLayout();
    tools->setSpacing(6);

    auto *btnCurrentToEnd = new QPushButton(QStringLiteral("当前实测角 → 结束角"), bar);
    btnCurrentToEnd->setToolTip(QStringLiteral("把机器人当前实测角度填入双臂结束角"));
    tools->addWidget(btnCurrentToEnd);

    auto *btnEndToStart = new QPushButton(QStringLiteral("结束角 = 起点"), bar);
    btnEndToStart->setToolTip(QStringLiteral("结束角取本段起点，做原地保持（不动）"));
    tools->addWidget(btnEndToStart);

    auto *btnZero = new QPushButton(QStringLiteral("清零"), bar);
    btnZero->setToolTip(QStringLiteral("左右臂全部关节角置零"));
    tools->addWidget(btnZero);

    auto *btnCopyLR = new QPushButton(QStringLiteral("左臂→右臂"), bar);
    btnCopyLR->setToolTip(QStringLiteral("把左臂的结束角复制到右臂"));
    tools->addWidget(btnCopyLR);

    tools->addStretch();
    layout->addLayout(tools);

    // ---- 第二行：插值 / 单位 / 预览 ----
    auto *opts = new QGridLayout();
    opts->setHorizontalSpacing(10);
    opts->setVerticalSpacing(8);

    opts->addWidget(new QLabel(QStringLiteral("插值"), bar), 0, 0);
    m_interpCombo = new TouchComboBox(bar);
    m_interpCombo->addItem(QStringLiteral("线性 linear"), QStringLiteral("linear"));
    m_interpCombo->addItem(QStringLiteral("三次 cubic"), QStringLiteral("cubic"));
    m_interpCombo->addItem(QStringLiteral("样条 spline"), QStringLiteral("spline"));
    opts->addWidget(m_interpCombo, 0, 1);

    opts->addWidget(new QLabel(QStringLiteral("角度单位"), bar), 0, 2);
    m_unitCombo = new TouchComboBox(bar);
    m_unitCombo->addItem(QStringLiteral("度 (°)"), true);
    m_unitCombo->addItem(QStringLiteral("弧度 (rad)"), false);
    opts->addWidget(m_unitCombo, 0, 3);

    opts->addWidget(new QLabel(QStringLiteral("预览"), bar), 1, 0);
    m_previewCombo = new TouchComboBox(bar);
    m_previewCombo->addItem(QStringLiteral("结束角"), true);
    m_previewCombo->addItem(QStringLiteral("起点（上一段终点）"), false);
    m_previewCombo->setToolTip(QStringLiteral("编辑时 3D 视图跟随预览哪个姿态"));
    opts->addWidget(m_previewCombo, 1, 1, 1, 3);

    opts->setColumnStretch(1, 1);
    opts->setColumnStretch(3, 1);
    layout->addLayout(opts);

    // ---- 联动 ----
    connect(btnCurrentToEnd, &QPushButton::clicked, this, &ArmSegmentEditDialog::onCurrentToEnd);
    connect(btnEndToStart, &QPushButton::clicked, this, &ArmSegmentEditDialog::onEndFromStart);
    connect(btnZero, &QPushButton::clicked, this, &ArmSegmentEditDialog::onResetBoth);
    connect(btnCopyLR, &QPushButton::clicked, this, &ArmSegmentEditDialog::onCopyLeftToRight);

    connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onUnitChanged(); });

    connect(m_interpCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                m_result.interpolationType = m_interpCombo->currentData().toString();
                notifyChanged();
            });

    connect(m_previewCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                m_previewEnd = m_previewCombo->currentData().toBool();
                emitPreview();
            });

    // 用模型值填充
    m_updating = true;
    int interpIndex = m_interpCombo->findData(m_result.interpolationType);
    if (interpIndex < 0) {
        interpIndex = 1;    // 默认 cubic
    }
    m_interpCombo->setCurrentIndex(interpIndex);
    m_updating = false;

    return bar;
}

QWidget* ArmSegmentEditDialog::createArmGroup(bool leftArm)
{
    const int arm = leftArm ? 0 : 1;

    auto *group = new QGroupBox(leftArm ? QStringLiteral("左臂") : QStringLiteral("右臂"), m_body);
    auto *grid = new QGridLayout(group);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(DialogUtil::touchSliderRowSpacing());

    const QStringList headers = {
        QStringLiteral("关节"),
        QStringLiteral("限位"),
        QStringLiteral("起点（只读）"),
        QStringLiteral("结束角"),
        QString()
    };
    for (int c = 0; c < headers.size(); ++c) {
        auto *h = new QLabel(headers[c], group);
        QFont f = h->font();
        f.setBold(true);
        h->setFont(f);
        h->setStyleSheet(QStringLiteral("color:#38BDF8;"));
        grid->addWidget(h, 0, c);
    }

    for (int j = 0; j < kNumJoints; ++j) {
        const int r = j + 1;

        auto *nameLabel = new QLabel(jointName(j), group);
        nameLabel->setStyleSheet(QStringLiteral("color:#E8E9ED; font-weight:600;"));
        nameLabel->setMinimumWidth(34);
        grid->addWidget(nameLabel, r, 0);

        auto *limLabel = new QLabel(group);
        limLabel->setStyleSheet(QStringLiteral("color:#8A94A6; font-size:9pt;"));
        limLabel->setMinimumWidth(110);
        m_limitLabel[arm][j] = limLabel;
        grid->addWidget(limLabel, r, 1);

        // 起点：只读，来自上一段终点
        auto *startLabel = new QLabel(group);
        startLabel->setStyleSheet(
            QStringLiteral("color:#7E8CA0; font-family:Consolas,monospace;"));
        startLabel->setMinimumWidth(88);
        startLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_startLabel[arm][j] = startLabel;
        grid->addWidget(startLabel, r, 2);

        auto *spin = new QDoubleSpinBox(group);
        spin->setDecimals(3);
        spin->setMinimumWidth(96);
        spin->setKeyboardTracking(false);
        m_spin[arm][j] = spin;
        grid->addWidget(spin, r, 3);

        auto *slider = new QSlider(Qt::Horizontal, group);
        slider->setMinimumWidth(150);
        // 触屏上给滑条足够的高度，滑块（见 DialogUtil::sliderStyleSheet）
        // 才有地方画大，手指也更容易压中而不误触相邻关节
        const int sliderHeight = DialogUtil::touchSliderHeight();
        if (sliderHeight > 0) {
            slider->setMinimumHeight(sliderHeight);
        }
        m_slider[arm][j] = slider;
        grid->addWidget(slider, r, 4);

        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, arm, j](double v) {
                    if (m_updating) {
                        return;
                    }
                    // spin 显示的是界面单位，写回模型时统一转成弧度
                    const double rad = m_degrees ? qDegreesToRadians(v) : v;
                    if (arm == 0) {
                        m_result.endJoints.j[j] = rad;
                    } else {
                        m_result.endJoints.jr[j] = rad;
                    }

                    if (m_slider[arm][j]) {
                        m_updating = true;
                        m_slider[arm][j]->setValue(qRound(rad * 1000.0));
                        m_updating = false;
                    }
                    notifyChanged();
                });

        connect(slider, &QSlider::valueChanged, this, [this, arm, j](int v) {
            if (m_updating) {
                return;
            }
            const double rad = v / 1000.0;
            if (arm == 0) {
                m_result.endJoints.j[j] = rad;
            } else {
                m_result.endJoints.jr[j] = rad;
            }

            if (m_spin[arm][j]) {
                m_updating = true;
                m_spin[arm][j]->setValue(rad * (m_degrees ? kRadToDeg : 1.0));
                m_updating = false;
            }
            notifyChanged();
        });
    }

    grid->setColumnStretch(4, 1);
    return group;
}

// ---------------------------------------------------------------- 取值

double ArmSegmentEditDialog::endValueOf(bool leftArm, int joint) const
{
    if (joint < 0 || joint >= kNumJoints) {
        return 0.0;
    }
    return leftArm ? m_result.endJoints.j[joint] : m_result.endJoints.jr[joint];
}

double ArmSegmentEditDialog::startValueOf(bool leftArm, int joint) const
{
    if (joint < 0 || joint >= kNumJoints) {
        return 0.0;
    }
    return leftArm ? m_derivedStart.j[joint] : m_derivedStart.jr[joint];
}

void ArmSegmentEditDialog::setEndValue(bool leftArm, int joint, double rad)
{
    if (joint < 0 || joint >= kNumJoints) {
        return;
    }
    if (leftArm) {
        m_result.endJoints.j[joint] = rad;
    } else {
        m_result.endJoints.jr[joint] = rad;
    }

    const int arm = leftArm ? 0 : 1;
    const double scale = m_degrees ? kRadToDeg : 1.0;

    if (QDoubleSpinBox *spin = m_spin[arm][joint]) {
        m_updating = true;
        spin->setValue(rad * scale);
        m_updating = false;
    }
    if (QSlider *slider = m_slider[arm][joint]) {
        m_updating = true;
        slider->setValue(qRound(rad * 1000.0));
        m_updating = false;
    }
}

// ---------------------------------------------------------------- 刷新

void ArmSegmentEditDialog::refreshEditor()
{
    m_updating = true;

    const double scale = m_degrees ? kRadToDeg : 1.0;
    const QString suffix = m_degrees ? QStringLiteral(" °") : QStringLiteral(" rad");
    const int decimals = m_degrees ? 2 : 4;

    for (int arm = 0; arm < 2; ++arm) {
        const JointLimits& lim = (arm == 0) ? m_limits.left : m_limits.right;

        for (int j = 0; j < kNumJoints; ++j) {
            if (QLabel *l = m_limitLabel[arm][j]) {
                l->setText(QStringLiteral("[%1, %2]")
                               .arg(lim.lower[j] * scale, 0, 'f', m_degrees ? 0 : 2)
                               .arg(lim.upper[j] * scale, 0, 'f', m_degrees ? 0 : 2));
            }

            // 起点只读参考
            if (QLabel *l = m_startLabel[arm][j]) {
                const double s = startValueOf(arm == 0, j);
                l->setText(QStringLiteral("%1%2")
                               .arg(s * scale, 0, 'f', m_degrees ? 2 : 4)
                               .arg(m_degrees ? QStringLiteral("°") : QString()));
            }

            QDoubleSpinBox *spin = m_spin[arm][j];
            if (!spin) {
                continue;
            }

            const double rad = endValueOf(arm == 0, j);

            // 允许略微超出 URDF 限位，方便调试，但把范围卡在限位外 5%
            const double pad = 0.05 * (lim.upper[j] - lim.lower[j]);
            spin->setRange((lim.lower[j] - pad) * scale, (lim.upper[j] + pad) * scale);
            spin->setDecimals(decimals);
            spin->setSuffix(suffix);
            spin->setValue(rad * scale);

            if (QSlider *slider = m_slider[arm][j]) {
                slider->setRange(qRound((lim.lower[j] - pad) * 1000.0),
                                 qRound((lim.upper[j] + pad) * 1000.0));
                slider->setValue(qRound(rad * 1000.0));
            }
        }
    }

    m_updating = false;
}

void ArmSegmentEditDialog::syncResultFromUi()
{
    // 插值方式已在下拉框信号里写入；关节角在 spin/slider 信号里实时写入。
    // 这里兜底同步一次，防止有控件绕过了信号（例如程序化赋值）。
    for (int arm = 0; arm < 2; ++arm) {
        for (int j = 0; j < kNumJoints; ++j) {
            QDoubleSpinBox *spin = m_spin[arm][j];
            if (!spin || m_updating) {
                continue;
            }
            const double rad = m_degrees ? qDegreesToRadians(spin->value()) : spin->value();
            if (arm == 0) {
                m_result.endJoints.j[j] = rad;
            } else {
                m_result.endJoints.jr[j] = rad;
            }
        }
    }
}

QStringList ArmSegmentEditDialog::collectWarnings() const
{
    QStringList warnings;
    for (int arm = 0; arm < 2; ++arm) {
        const JointLimits& lim = (arm == 0) ? m_limits.left : m_limits.right;
        for (int j = 0; j < kNumJoints; ++j) {
            const double v = endValueOf(arm == 0, j);
            if (v < lim.lower[j] - 1e-6 || v > lim.upper[j] + 1e-6) {
                warnings << QStringLiteral("%1臂 %2 结束角 %3° 超出限位")
                                .arg(arm == 0 ? QStringLiteral("左") : QStringLiteral("右"))
                                .arg(jointName(j))
                                .arg(v * kRadToDeg, 0, 'f', 1);
            }
        }
    }
    return warnings;
}

void ArmSegmentEditDialog::emitPreview()
{
    QVector<double> left(kNumJoints), right(kNumJoints);
    for (int j = 0; j < kNumJoints; ++j) {
        left[j]  = m_previewEnd ? endValueOf(true, j)  : startValueOf(true, j);
        right[j] = m_previewEnd ? endValueOf(false, j) : startValueOf(false, j);
    }
    emit previewChanged(m_previewEnd, left, right);
}

// ---------------------------------------------------------------- 槽

void ArmSegmentEditDialog::onUnitChanged()
{
    m_degrees = m_unitCombo->currentData().toBool();
    refreshEditor();
}

void ArmSegmentEditDialog::onCurrentToEnd()
{
    for (int arm = 0; arm < 2; ++arm) {
        for (int j = 0; j < kNumJoints; ++j) {
            const double v = (arm == 0) ? m_currentLeft.value(j, 0.0)
                                        : m_currentRight.value(j, 0.0);
            setEndValue(arm == 0, j, v);
        }
    }
    notifyChanged();
}

void ArmSegmentEditDialog::onEndFromStart()
{
    // 结束角 = 起点 → 本段原地保持
    for (int arm = 0; arm < 2; ++arm) {
        for (int j = 0; j < kNumJoints; ++j) {
            setEndValue(arm == 0, j, startValueOf(arm == 0, j));
        }
    }
    notifyChanged();
}

void ArmSegmentEditDialog::onResetBoth()
{
    for (int arm = 0; arm < 2; ++arm) {
        for (int j = 0; j < kNumJoints; ++j) {
            setEndValue(arm == 0, j, 0.0);
        }
    }
    notifyChanged();
}

void ArmSegmentEditDialog::onCopyLeftToRight()
{
    for (int j = 0; j < kNumJoints; ++j) {
        setEndValue(false, j, endValueOf(true, j));
    }
    notifyChanged();
}

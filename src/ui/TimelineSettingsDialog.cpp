#include "ui/TimelineSettingsDialog.h"
#include "ui/DialogUtil.h"
#include "ui/TouchComboBox.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QVBoxLayout>

TimelineSettingsDialog::TimelineSettingsDialog(const Settings &current, QWidget *parent)
    : QDialog(parent)
    , m_result(current)
{
    setWindowTitle(QStringLiteral("设置"));
    setModal(true);
    // 与主窗口、片段编辑对话框共用同一套主题，避免同一个应用里出现两种深色
    setStyleSheet(DialogUtil::dialogStyleSheet());
    setMinimumWidth(460);

    setupUi();
    fillFrom(current);

    // 布局建立后再适配：setMinimumWidth(460) 在竖屏手机上可能超出屏幕宽度，
    // 适配函数会按可用屏幕裁剪并居中显示
    DialogUtil::adaptForMobile(this);
}

void TimelineSettingsDialog::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // 手机屏幕高度有限：三四个分组很容易超出屏幕，底部「确定/取消」会被
    // 挤出可视区（表现为点不到）。因此把主体放进滚动区，按钮行固定底部。
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *body = new QWidget(scroll);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(12);
    scroll->setWidget(body);
    root->addWidget(scroll, 1);

    // ---------------- 长度 ----------------
    auto *lenGroup = new QGroupBox(QStringLiteral("时间轴长度"), body);
    auto *lenGrid = new QGridLayout(lenGroup);
    lenGrid->setHorizontalSpacing(10);
    lenGrid->setVerticalSpacing(8);

    lenGrid->addWidget(new QLabel(QStringLiteral("总长度 (秒)"), lenGroup), 0, 0);

    m_durationSpin = new QDoubleSpinBox(lenGroup);
    m_durationSpin->setRange(5.0, 86400.0);     // 5 秒 ~ 24 小时
    m_durationSpin->setDecimals(0);
    m_durationSpin->setSingleStep(30.0);
    m_durationSpin->setSuffix(QStringLiteral(" s"));
    lenGrid->addWidget(m_durationSpin, 0, 1);

    m_durationHint = new QLabel(lenGroup);
    m_durationHint->setStyleSheet(QStringLiteral("color:#5EEAD4;"));
    lenGrid->addWidget(m_durationHint, 0, 2);

    // 常用长度的快捷按钮，省得手敲
    auto *presets = new QHBoxLayout();
    presets->setSpacing(6);
    presets->addWidget(new QLabel(QStringLiteral("快捷"), lenGroup));
    const struct { const char *label; double seconds; } quick[] = {
        {"1 分钟", 60.0}, {"3 分钟", 180.0}, {"5 分钟", 300.0}, {"10 分钟", 600.0}
    };
    for (const auto &q : quick) {
        auto *btn = new QPushButton(QString::fromUtf8(q.label), lenGroup);
        const double sec = q.seconds;
        connect(btn, &QPushButton::clicked, this, [this, sec]() {
            m_durationSpin->setValue(sec);
        });
        presets->addWidget(btn);
    }
    presets->addStretch();
    lenGrid->addLayout(presets, 1, 0, 1, 3);

    auto *lenHint = new QLabel(
        QStringLiteral("这只是编辑空间的大小，不影响动作内容。\n"
                       "片段超出这个长度时，时间轴会自动延长，不会截断。"),
        lenGroup);
    lenHint->setWordWrap(true);
    lenHint->setStyleSheet(QStringLiteral("color:#8A94A6; font-size:9pt;"));
    lenGrid->addWidget(lenHint, 2, 0, 1, 3);

    bodyLayout->addWidget(lenGroup);

    // ---------------- 默认片段时长 ----------------
    auto *segGroup = new QGroupBox(QStringLiteral("新增片段默认时长"), body);
    auto *segGrid = new QGridLayout(segGroup);
    segGrid->setHorizontalSpacing(10);
    segGrid->setVerticalSpacing(8);

    segGrid->addWidget(new QLabel(QStringLiteral("机械臂片段 (秒)"), segGroup), 0, 0);
    m_armDurationSpin = new QDoubleSpinBox(segGroup);
    m_armDurationSpin->setRange(0.1, 3600.0);
    m_armDurationSpin->setDecimals(2);
    m_armDurationSpin->setSingleStep(0.5);
    segGrid->addWidget(m_armDurationSpin, 0, 1);

    segGrid->addWidget(new QLabel(QStringLiteral("夹爪片段 (秒)"), segGroup), 1, 0);
    m_handDurationSpin = new QDoubleSpinBox(segGroup);
    m_handDurationSpin->setRange(0.1, 3600.0);
    m_handDurationSpin->setDecimals(2);
    m_handDurationSpin->setSingleStep(0.5);
    segGrid->addWidget(m_handDurationSpin, 1, 1);

    segGrid->setColumnStretch(2, 1);
    bodyLayout->addWidget(segGroup);

    // ---------------- 下发 ----------------
    auto *streamGroup = new QGroupBox(QStringLiteral("实时下发"), body);
    auto *streamGrid = new QGridLayout(streamGroup);
    streamGrid->setHorizontalSpacing(10);
    streamGrid->setVerticalSpacing(8);

    streamGrid->addWidget(new QLabel(QStringLiteral("下发频率"), streamGroup), 0, 0);

    m_frequencyCombo = new TouchComboBox(streamGroup);
    m_frequencyCombo->setToolTip(
        QStringLiteral("UDP 关节流发送频率。降低可减少控制器负载；\n"
                       "配合发送端插值，100Hz 已足够平滑。"));
    // 只列常用档位。协议允许 10~1000Hz，但再高对平滑度没有帮助
    // （上游插值是 62.5Hz），只会增加控制器负载。
    const struct { const char *label; int hz; } kFreqOptions[] = {
        {"50 Hz",  50},
        {"100 Hz", 100},
        {"150 Hz", 150},
        {"200 Hz", 200},
        {"300 Hz", 300},
    };
    for (const auto &o : kFreqOptions) {
        m_frequencyCombo->addItem(QString::fromUtf8(o.label), o.hz);
    }
    streamGrid->addWidget(m_frequencyCombo, 0, 1);

    auto *freqHint = new QLabel(
        QStringLiteral("100 Hz 与 3D 预览的 62.5 Hz 插值搭配时重叠度低，\n"
                       "不会出现同一目标连发多包造成的速度脉动。"),
        streamGroup);
    freqHint->setWordWrap(true);
    freqHint->setStyleSheet(QStringLiteral("color:#8A94A6; font-size:9pt;"));
    streamGrid->addWidget(freqHint, 1, 0, 1, 2);

    streamGrid->setColumnStretch(2, 1);
    bodyLayout->addWidget(streamGroup);

    // ---------------- 文件 ----------------
    auto *fileGroup = new QGroupBox(QStringLiteral("文件"), body);
    auto *fileGrid = new QGridLayout(fileGroup);
    fileGrid->setHorizontalSpacing(10);
    fileGrid->setVerticalSpacing(8);

    fileGrid->addWidget(new QLabel(QStringLiteral("加载默认目录"), fileGroup), 0, 0);

    m_loadFolderEdit = new QLineEdit(fileGroup);
    m_loadFolderEdit->setPlaceholderText(
        QStringLiteral("未指定 — 由系统对话框决定"));
    m_loadFolderEdit->setToolTip(
        QStringLiteral("打开「加载动作序列」时，文件对话框从这个目录开始。\n"
                       "留空则由系统记住上次打开的位置。"));
    fileGrid->addWidget(m_loadFolderEdit, 0, 1);

    auto *browseBtn = new QPushButton(QStringLiteral("浏览…"), fileGroup);
    browseBtn->setToolTip(QStringLiteral("选择一个目录"));
    fileGrid->addWidget(browseBtn, 0, 2);

    auto *clearBtn = new QPushButton(QStringLiteral("清除"), fileGroup);
    clearBtn->setToolTip(QStringLiteral("恢复为「由系统对话框决定」"));
    fileGrid->addWidget(clearBtn, 0, 3);

    auto *folderHint = new QLabel(
        QStringLiteral("只影响加载时的起始位置，不影响保存。\n"
                       "目录不存在时会自动退回系统默认位置。"),
        fileGroup);
    folderHint->setWordWrap(true);
    folderHint->setStyleSheet(QStringLiteral("color:#8A94A6; font-size:9pt;"));
    fileGrid->addWidget(folderHint, 1, 0, 1, 4);

    fileGrid->setColumnStretch(1, 1);
    bodyLayout->addWidget(fileGroup);

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        // 已经填了有效目录就从那里开始，否则从「文档」目录起步，
        // 比从进程的工作目录开始更符合用户的直觉
        QString start = normalizeFolder(m_loadFolderEdit->text());
        if (start.isEmpty() || !QDir(start).exists()) {
            start = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        const QString dir = QFileDialog::getExistingDirectory(
            this, QStringLiteral("选择加载动作序列的默认目录"), start);
        if (!dir.isEmpty()) {
            m_loadFolderEdit->setText(QDir::cleanPath(dir));
        }
    });
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        m_loadFolderEdit->clear();
    });

    // ---------------- 播放 ----------------
    auto *playGroup = new QGroupBox(QStringLiteral("播放"), body);
    auto *playLayout = new QVBoxLayout(playGroup);
    playLayout->setContentsMargins(10, 10, 10, 10);

    m_autoScrollCheck = new QCheckBox(
        QStringLiteral("播放时自动跟随游标横向滚动"), playGroup);
    m_autoScrollCheck->setToolTip(
        QStringLiteral("关掉后时间轴视野固定，游标会走出视野——\n"
                       "适合盯着某一段看细节"));
    playLayout->addWidget(m_autoScrollCheck);

    bodyLayout->addWidget(playGroup);
    bodyLayout->addStretch();

    // ---------------- 底部按钮 ----------------
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Ok)->setDefault(true);
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        collectToResult();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_durationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) {
                if (!m_updating) {
                    updateDurationHint();
                }
            });
}

void TimelineSettingsDialog::fillFrom(const Settings &s)
{
    m_updating = true;
    m_durationSpin->setValue(s.timelineDuration);
    m_armDurationSpin->setValue(s.defaultArmDuration);
    m_handDurationSpin->setValue(s.defaultHandDuration);
    m_autoScrollCheck->setChecked(s.autoScrollToPlayhead);
    m_loadFolderEdit->setText(s.loadFolder);

    // 档位里没有保存的值（例如配置文件被手改过）就退回默认 100Hz，
    // 而不是把下拉框停在第一项造成「显示 50 实际 300」。
    int idx = m_frequencyCombo->findData(s.streamFrequency);
    if (idx < 0) {
        idx = m_frequencyCombo->findData(100);
    }
    m_frequencyCombo->setCurrentIndex(qMax(0, idx));

    m_updating = false;

    updateDurationHint();
}

void TimelineSettingsDialog::collectToResult()
{
    m_result.timelineDuration = m_durationSpin->value();
    m_result.defaultArmDuration = m_armDurationSpin->value();
    m_result.defaultHandDuration = m_handDurationSpin->value();
    m_result.autoScrollToPlayhead = m_autoScrollCheck->isChecked();

    const QVariant freq = m_frequencyCombo->currentData();
    m_result.streamFrequency = freq.isValid() ? freq.toInt() : 100;

    // 目录统一成干净的绝对路径：用户可能手敲了 "~/动作/" 或带末尾斜杠。
    // 空输入保持为空，表示不指定。不在这里做存在性校验——
    // 目录可能暂时不可用（U 盘、网络盘），退回默认比拒绝保存设置更友好。
    m_result.loadFolder = normalizeFolder(m_loadFolderEdit->text());
}

QString TimelineSettingsDialog::normalizeFolder(const QString &input)
{
    QString folder = input.trimmed();
    if (folder.isEmpty()) {
        return QString();
    }

    // QDir 不认 "~"，而用户在 Linux 上最顺手的写法就是 "~/动作"。
    // 不展开的话会变成一个相对路径下的字面量目录名，看起来「设置无效」。
    if (folder == QLatin1String("~")) {
        folder = QDir::homePath();
    } else if (folder.startsWith(QLatin1String("~/"))) {
        folder = QDir::homePath() + folder.mid(1);
    }

    // absolutePath() 会以当前工作目录为基准补全相对路径，
    // cleanPath() 去掉 "//" 和末尾分隔符，保证同一个目录只有一种写法
    return QDir::cleanPath(QDir(folder).absolutePath());
}

void TimelineSettingsDialog::updateDurationHint()
{
    const double sec = m_durationSpin->value();
    if (!m_durationHint) {
        return;
    }
    // 180 → "3 分 0 秒"
    const int total = qRound(sec);
    const int minutes = total / 60;
    const int rest = total % 60;
    if (minutes > 0) {
        m_durationHint->setText(
            QStringLiteral("= %1 分 %2 秒").arg(minutes).arg(rest));
    } else {
        m_durationHint->setText(QStringLiteral("= %1 秒").arg(rest));
    }
}

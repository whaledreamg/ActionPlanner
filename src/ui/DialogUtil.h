#ifndef UI_DIALOGUTIL_H
#define UI_DIALOGUTIL_H

#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

// Android 子窗口适配（对齐 RobotControl/dialogs/dialogutil.h 的做法）
//
// 手机上和桌面最大的差异是「窗口尺寸」：
//   - 桌面上 resize(720, 600) 是合理尺寸，手机横屏可用高度可能只有 400dp，
//     竖屏大约 640dp 且还要扣掉状态栏/导航栏；
//   - Qt 在 Android 上不会自动把 QDialog 限制在屏幕内，超出屏幕的部分
//     既看不见也点不到，表现就是「弹出来了但按钮点不动」。
//
// 因此移动端的统一策略是：
//   1. 解除最小尺寸与布局约束，让窗口可以被压到屏幕范围内；
//   2. 直接最大化显示——这同时也是触摸能正常工作的前提，原因见函数内注释；
//   3. 接管系统返回键（QDialog 默认只认 Esc），否则手机上关不掉。
// 桌面平台完全不受影响。
namespace DialogUtil {

class BackKeyFilter : public QObject
{
public:
    explicit BackKeyFilter(QDialog *dlg) : QObject(dlg), m_dlg(dlg) {}

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj == m_dlg && ev->type() == QEvent::KeyPress
            && static_cast<QKeyEvent *>(ev)->key() == Qt::Key_Back) {
            m_dlg->close();   // QDialog::closeEvent -> reject()
            return true;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QDialog *m_dlg;
};

/**
 * 让子窗口在手机上可用。
 * 必须在窗口的布局建立之后调用——布局的最小尺寸提示会把窗口撑大，
 * 先解除约束，窗口才能被限制在屏幕范围内。
 */
inline void adaptForMobile(QDialog *dlg)
{
#ifdef Q_OS_ANDROID
    if (!dlg) {
        return;
    }
    if (dlg->property("dialogAdaptedForMobile").toBool()) {
        return;
    }
    dlg->setProperty("dialogAdaptedForMobile", true);

    dlg->setMinimumSize(0, 0);
    if (dlg->layout()) {
        dlg->layout()->setSizeConstraint(QLayout::SetNoConstraint);
    }

    // 必须最大化，不要改成「保持设计尺寸 + 居中」。
    //
    // Android 上 QDialog 是独立的原生视图（QtView）。未最大化的窗口，其屏幕
    // 实际位置由 Android 窗口系统决定，Qt 侧的 move()/居中计算不一定被采纳，
    // 于是 Qt 认为的 geometry 与屏幕真实位置出现偏差，触摸坐标按 geometry
    // 映射后整体错位——点哪儿都落不到按钮上，表现就是「窗口完全点不动」。
    // 最大化时平台层会把窗口 geometry 对齐到 availableGeometry
    // （见 QAndroidPlatformWindow::setVisible），坐标系一致，触摸才正常。
    //
    // 实测差异：「添加片段」的输入框窗口尺寸小，错位被放大到完全点不动；
    // 编辑/设置窗口内容多、接近满屏，偏差小到不易察觉。
    dlg->setWindowState(dlg->windowState() | Qt::WindowMaximized);

    dlg->installEventFilter(new BackKeyFilter(dlg));
#else
    Q_UNUSED(dlg);
#endif
}

/** 按钮最小高度：手指点击区域比鼠标要求更大 */
inline int touchButtonHeight()
{
#ifdef Q_OS_ANDROID
    return 48;
#else
    return 0;   // 0 表示沿用样式表的默认高度
#endif
}

/**
 * 关节/手位滑条的行间距。
 *
 * 滑条本身只有 4px 轨道 + 12px 滑块，桌面上靠鼠标精确定位没问题，但触屏上
 * 两行滑条挨在一起时，手指落点很难只压中其中一条——按在两条之间会同时
 * 接近两个滑块，容易拖错关节。移动端把行距拉开，并把滑块放大
 * （见 sliderStyleSheet）。
 */
inline int touchSliderRowSpacing()
{
#ifdef Q_OS_ANDROID
    return 16;
#else
    return 6;
#endif
}

/** 滑条最小高度：保证滑块四周有足够的空白，手指不会误触相邻行 */
inline int touchSliderHeight()
{
#ifdef Q_OS_ANDROID
    return 34;
#else
    return 0;
#endif
}

/**
 * 滑条样式。
 *
 * 桌面用细轨道小滑块；移动端必须放大：滑块的触摸判定区就是滑块本身
 * （外加轨道那一圈），4px 轨道配 12px 滑块在手指下几乎点不中，
 * 而且上下相邻两行只隔 6px，按下去很容易带到隔壁关节。
 */
inline QString sliderStyleSheet()
{
#ifdef Q_OS_ANDROID
    return QStringLiteral(R"(
QSlider::groove:horizontal { height: 8px; background: #1E2636; border-radius: 4px; }
QSlider::handle:horizontal {
    background: #38BDF8; width: 26px; margin: -9px 0; border-radius: 13px;
}
)");
#else
    return QStringLiteral(R"(
QSlider::groove:horizontal { height: 4px; background: #1E2636; border-radius: 2px; }
QSlider::handle:horizontal {
    background: #38BDF8; width: 12px; margin: -5px 0; border-radius: 6px;
}
)");
#endif
}

/**
 * 对话框统一样式。
 *
 * 配色必须和主窗口同一套——主界面是三层：窗口 #0A0D12、面板 #0F131C、
 * 卡片 #161D2B，输入控件比卡片再亮一档用 #1E2636。这里照搬：
 * 对话框底 = 面板色，分组框 = 卡片色（带填充，不是只有描边），
 * 输入框/下拉框 = #1E2636。
 *
 * 下拉框的弹出列表要单独写。弹出列表是独立的顶层窗口，
 * 不继承父控件的 QComboBox 规则；只设 color 不够，条目文字会回退到
 * 系统调色板的深色，压在深色底色上几乎看不见。所以背景、文字、
 * 选中态三项都要显式指定。
 */
inline QString dialogStyleSheet()
{
    return QStringLiteral(R"(
QDialog { background-color: #0F131C; }

QLabel { color: #C8D2E0; font-size: 10pt; }

QScrollArea { background: transparent; border: none; }
QScrollArea > QWidget > QWidget { background: transparent; }

/* 分组框：与主界面卡片同色，用填充区分层次而不只靠描边 */
QGroupBox {
    background-color: #161D2B;
    border: 1px solid #1E2636;
    border-radius: 8px;
    margin-top: 12px;
    padding: 16px 8px 8px 8px;
    font-weight: bold;
    color: #E8E9ED;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 0 6px;
    color: #38BDF8;
}

QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background-color: #1E2636;
    border: 1px solid #2A3444;
    border-radius: 6px;
    padding: 6px 10px;
    color: #E8E9ED;
    font-size: 10pt;
    selection-background-color: #38BDF8;
    selection-color: #0A0D12;
}
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
    border: 1px solid #38BDF8;
    background-color: #242D3D;
}
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
    background-color: #161D2B;
    color: #8A94A6;
    border: 1px solid #1E2636;
}

/* 下拉框：展开的条目列表（独立顶层窗口，必须单独指定） */
QComboBox QAbstractItemView {
    background-color: #1E2636;
    border: 1px solid #2A3444;
    border-radius: 6px;
    padding: 4px;
    color: #E8E9ED;
    outline: none;
    selection-background-color: #38BDF8;
    selection-color: #0A0D12;
}
QComboBox QAbstractItemView::item {
    color: #E8E9ED;
    background-color: transparent;
    min-height: 26px;
    padding: 2px 6px;
}
QComboBox QAbstractItemView::item:selected {
    background-color: #38BDF8;
    color: #0A0D12;
}
QComboBox QAbstractItemView::item:hover {
    background-color: #2A3444;
    color: #E8E9ED;
}

QPushButton {
    background-color: #1E2636;
    border: 1px solid #2A3444;
    border-radius: 6px;
    padding: 8px 16px;
    color: #E8E9ED;
    font-size: 10pt;
    font-weight: 500;
    min-width: 72px;
}
QPushButton:hover { background-color: #242D3D; border: 1px solid #38BDF8; }
QPushButton:pressed { background-color: #2A3444; }
QPushButton:disabled { background-color: #161D2B; color: #8A94A6; border: 1px solid #1E2636; }
QPushButton:default { border: 1px solid #38BDF8; color: #38BDF8; }

QCheckBox { color: #C8D2E0; font-size: 10pt; spacing: 6px; }
QCheckBox::indicator {
    width: 15px; height: 15px;
    border: 1px solid #2A3444; border-radius: 4px;
    background-color: #161D2B;
}
QCheckBox::indicator:hover { border: 1px solid #38BDF8; }
QCheckBox::indicator:checked { background-color: #38BDF8; border: 1px solid #38BDF8; }

QScrollBar:vertical { background-color: #1E2636; width: 10px; border-radius: 5px; }
QScrollBar::handle:vertical { background-color: #38BDF8; border-radius: 5px; min-height: 20px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar:horizontal { background-color: #1E2636; height: 10px; border-radius: 5px; }
QScrollBar::handle:horizontal { background-color: #38BDF8; border-radius: 5px; min-width: 20px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
)") + sliderStyleSheet();
}

/**
 * 替代 QMessageBox::question / QMessageBox::Yes|No 的确认框。
 *
 * 不用 QMessageBox 的原因：Android 上模态 QMessageBox 的按钮收不到触摸事件，
 * 弹出来就点不掉，直接把界面卡死。这里用普通 QDialog 自己搭，
 * 尺寸和按钮高度都按移动端调过。
 */
inline bool confirm(QWidget *parent, const QString &title, const QString &text,
                    const QString &okText = QStringLiteral("确定"),
                    const QString &cancelText = QStringLiteral("取消"))
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setModal(true);
    dlg.setStyleSheet(dialogStyleSheet());

    auto *root = new QVBoxLayout(&dlg);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // 文案可能很长（例如限位告警最多十几行），放进滚动区避免撑破小屏
    auto *scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *label = new QLabel(text, scroll);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    scroll->setWidget(label);
    root->addWidget(scroll, 1);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(10);
    buttonRow->addStretch(1);

    auto *cancelBtn = new QPushButton(cancelText, &dlg);
    auto *okBtn = new QPushButton(okText, &dlg);
    okBtn->setDefault(true);

    const int btnHeight = touchButtonHeight();
    if (btnHeight > 0) {
        cancelBtn->setMinimumHeight(btnHeight);
        okBtn->setMinimumHeight(btnHeight);
    }

    buttonRow->addWidget(cancelBtn);
    buttonRow->addWidget(okBtn);
    root->addLayout(buttonRow);

    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    // 桌面端给一个舒服的固定宽度；移动端直接最大化，用不上这个尺寸
#ifndef Q_OS_ANDROID
    dlg.resize(460, 0);
#endif
    adaptForMobile(&dlg);

    return dlg.exec() == QDialog::Accepted;
}

} // namespace DialogUtil

#endif // UI_DIALOGUTIL_H

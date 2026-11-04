#include "ui/MessageCenter.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace {

constexpr int kToastMargin     = 10;    // 提示条之间的间距
constexpr int kEdgeMargin      = 16;    // 提示条与窗口边缘的距离
constexpr int kDefaultDuration = 2500;
constexpr int kMinLabelWidth   = 160;
constexpr int kMaxLabelWidth   = 640;
constexpr int kLabelSlack      = 6;     // 文本宽度之外的固定余量，避免字体取整导致末尾换行

struct ToastStyle {
    const char *border;
    QColor      bg;     // 必须用 QColor(r,g,b,a)：QColor(QString) 不识别 CSS 的 rgba(...)
    const char *color;
    const char *icon;
};

ToastStyle styleFor(MessageLevel t)
{
    switch (t) {
    case MessageLevel::Success:
        return {"#67C23A", QColor(240, 249, 235, 235), "rgb(60,160,40)",  "✓"};
    case MessageLevel::Error:
        return {"#F56C6C", QColor(254, 240, 240, 235), "rgb(200,80,80)",  "✕"};
    case MessageLevel::Warning:
        return {"#E6A23C", QColor(253, 246, 236, 235), "rgb(190,130,40)", "!"};
    case MessageLevel::Info:
    default:
        return {"#409EFF", QColor(240, 247, 255, 235), "rgb(50,120,210)", "i"};
    }
}

} // namespace

// ============================================================================
// MessageToast：单条提示
// ============================================================================
class MessageToast : public QWidget
{
    Q_OBJECT
public:
    MessageToast(QWidget *parent, MessageLevel level, const QString &content, int durationMs)
        : QWidget(parent)
        , m_level(level)
        , m_content(content)
        , m_durationMs(durationMs > 0 ? durationMs : kDefaultDuration)
    {
        setObjectName(QStringLiteral("messageToast"));
        // 作为子控件显示，不能加 FramelessWindowHint，否则会脱离父窗口变成独立顶层窗口
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        buildUi();

        auto *effect = new QGraphicsDropShadowEffect(this);
        effect->setBlurRadius(18);
        effect->setOffset(0, 3);
        effect->setColor(QColor(0, 0, 0, 70));
        setGraphicsEffect(effect);
    }

    MessageLevel level() const { return m_level; }
    QString content() const { return m_content; }

    /** 启动自动消失计时（错误类型由外部决定是否调用） */
    void startTimer()
    {
        QTimer::singleShot(m_durationMs, this, [this]() { emit dismissed(this); });
    }

signals:
    void dismissed(MessageToast *toast);

protected:
    void paintEvent(QPaintEvent *) override
    {
        const ToastStyle sc = styleFor(m_level);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath path;
        path.addRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
        p.fillPath(path, sc.bg);
        p.setPen(QPen(QColor(QString::fromUtf8(sc.border)), 1.2));
        p.drawPath(path);
    }

private:
    void buildUi()
    {
        const ToastStyle sc = styleFor(m_level);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(14, 10, 10, 10);
        layout->setSpacing(10);

        auto *icon = new QLabel(QString::fromUtf8(sc.icon), this);
        icon->setFixedWidth(20);
        icon->setAlignment(Qt::AlignCenter);
        icon->setStyleSheet(QStringLiteral("color: %1; font-size: 15px; font-weight: bold;")
                                .arg(QString::fromUtf8(sc.color)));
        icon->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        layout->addWidget(icon);

        auto *label = new QLabel(m_content, this);
        label->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;")
                                 .arg(QString::fromUtf8(sc.color)));
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->ensurePolished();

        // 单行放得下就不换行；超过上限才自动换行
        const int hostW = parentWidget() ? parentWidget()->width() : 800;
        const int maxLabelW = qBound(kMinLabelWidth, hostW - 160, kMaxLabelWidth);
        const QRect textRect = label->fontMetrics().boundingRect(
            QRect(0, 0, maxLabelW, 100000), Qt::TextWordWrap, m_content);
        label->setWordWrap(true);
        label->setFixedWidth(qMax(kMinLabelWidth, textRect.width() + kLabelSlack));
        layout->addWidget(label);

        // 错误类型带手动关闭按钮，否则用户没有别的方式让它消失
        if (m_level == MessageLevel::Error) {
            auto *closeBtn = new QPushButton(QStringLiteral("✕"), this);
            closeBtn->setFixedSize(24, 24);       // 比桌面端略大，方便手指点
            closeBtn->setFlat(true);
            closeBtn->setFocusPolicy(Qt::NoFocus);
            closeBtn->setStyleSheet(QStringLiteral(
                "QPushButton { border: none; color: %1; font-size: 13px; }"
                "QPushButton:pressed { color: #000; }").arg(QString::fromUtf8(sc.color)));
            connect(closeBtn, &QPushButton::clicked, this,
                    [this]() { emit dismissed(this); });
            layout->addWidget(closeBtn);
        }
    }

    MessageLevel m_level;
    QString m_content;
    int m_durationMs;
};

// ============================================================================
// MessageCenter
// ============================================================================
MessageCenter::MessageCenter(QObject *parent)
    : QObject(parent)
{
}

MessageCenter *MessageCenter::instance()
{
    static MessageCenter *center = new MessageCenter(qApp);
    return center;
}

void MessageCenter::info(const QWidget *context, const QString &text)
{
    instance()->show(context, MessageLevel::Info, text);
}

void MessageCenter::success(const QWidget *context, const QString &text)
{
    instance()->show(context, MessageLevel::Success, text);
}

void MessageCenter::warning(const QWidget *context, const QString &text)
{
    instance()->show(context, MessageLevel::Warning, text);
}

void MessageCenter::error(const QWidget *context, const QString &text)
{
    instance()->show(context, MessageLevel::Error, text);
}

QWidget *MessageCenter::ensureHost(const QWidget *context)
{
    QWidget *host = context ? context->window() : nullptr;
    if (!host) {
        host = QApplication::activeWindow();
    }

    if (m_host == host && m_host) {
        return m_host;
    }

    // 换宿主（或首次）：清掉旧宿主上的提示，避免残留在已关闭的窗口里
    if (m_host) {
        m_host->removeEventFilter(this);
    }
    for (MessageToast *toast : std::as_const(m_toasts)) {
        if (toast) {
            toast->deleteLater();
        }
    }
    m_toasts.clear();

    m_host = host;
    if (!m_host) {
        return nullptr;
    }

    // 宿主窗口尺寸变化时重新居中，否则横竖屏切换后提示条会停在旧位置
    m_host->installEventFilter(this);
    return m_host;
}

bool MessageCenter::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_host && event->type() == QEvent::Resize) {
        relayout();
    }
    return QObject::eventFilter(obj, event);
}

void MessageCenter::show(const QWidget *context, MessageLevel level, const QString &text,
                         int durationMs)
{
    if (text.isEmpty()) {
        return;
    }

    QWidget *host = ensureHost(context);
    if (!host) {
        return;
    }

    // 错误类型去重：相同内容不重复弹
    for (MessageToast *toast : std::as_const(m_toasts)) {
        if (toast && toast->level() == level && toast->content() == text) {
            toast->raise();
            return;
        }
    }

    const int finalDuration = durationMs > 0 ? durationMs : kDefaultDuration;
    auto *toast = new MessageToast(host, level, text, finalDuration);
    connect(toast, &MessageToast::dismissed, this, &MessageCenter::onToastDismissed);
    toast->show();

    m_toasts.append(toast);

    // 错误不自动消失，交给用户点关闭，保证一定要看到
    if (level != MessageLevel::Error) {
        toast->startTimer();
    }

    relayout();
}

void MessageCenter::clear()
{
    const QList<MessageToast *> toasts = m_toasts;
    m_toasts.clear();
    for (MessageToast *toast : toasts) {
        if (toast) {
            toast->deleteLater();
        }
    }
    m_host = nullptr;
}

void MessageCenter::onToastDismissed(MessageToast *toast)
{
    if (!m_toasts.contains(toast)) {
        return;
    }
    m_toasts.removeAll(toast);
    toast->deleteLater();
    relayout();
}

void MessageCenter::removeOldest()
{
    if (m_toasts.isEmpty()) {
        return;
    }
    MessageToast *toast = m_toasts.first();
    m_toasts.removeFirst();
    if (toast) {
        toast->deleteLater();
    }
}

void MessageCenter::relayout()
{
    // 最多同时显示 5 条，超出就挤掉最早的
    while (m_toasts.size() > 5) {
        removeOldest();
    }

    if (!m_host) {
        return;
    }

    // 顶部居中纵向堆叠
    int y = kEdgeMargin;
    for (MessageToast *toast : std::as_const(m_toasts)) {
        if (!toast) {
            continue;
        }
        toast->adjustSize();
        const int x = qMax(kEdgeMargin, (m_host->width() - toast->width()) / 2);
        toast->move(x, y);
        toast->raise();
        y += toast->height() + kToastMargin;
    }
}

#include "MessageCenter.moc"

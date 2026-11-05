#include "ui/TouchComboBox.h"

#include <QApplication>
#include <QEvent>
#include <QListWidget>
#include <QMouseEvent>
#include <QScrollBar>
#include <QVBoxLayout>

namespace {
/** Android 上单个条目的高度：手指要能稳定压中 */
#ifdef Q_OS_ANDROID
constexpr int kTouchItemHeight = 52;
#else
constexpr int kTouchItemHeight = 28;
#endif
} // namespace

TouchComboBox::TouchComboBox(QWidget *parent)
    : QComboBox(parent)
{
#ifdef Q_OS_ANDROID
    // 收起状态的本体的触摸尺寸
    setMinimumHeight(48);
    setStyleSheet(QStringLiteral("font-size: 12pt; padding: 8px 10px;"));
#endif
}

void TouchComboBox::showPopup()
{
#ifndef Q_OS_ANDROID
    QComboBox::showPopup();
#else
    // 不用原生 popup：它是独立于主窗口的 native 子窗口，
    // Android 上收不到触摸事件，点任何 item 都没反应。
    showTouchPopup();
#endif
}

void TouchComboBox::hidePopup()
{
#ifndef Q_OS_ANDROID
    QComboBox::hidePopup();
#else
    hideTouchPopup();
    // 原生视图虽然没显示过，还是调一次基类保证内部状态干净
    QComboBox::hidePopup();
#endif
}

bool TouchComboBox::eventFilter(QObject *obj, QEvent *event)
{
#ifdef Q_OS_ANDROID
    if (m_overlay && m_overlay->isVisible()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            // 点在列表之外 -> 收起，并吞掉这次点击，
            // 免得穿透到底下的控件误触发别的东西
            const QPoint globalPos = me->globalPosition().toPoint();
            const QPoint localPos = m_list->mapFromGlobal(globalPos);
            if (!m_list->rect().contains(localPos)) {
                hideTouchPopup();
                return true;
            }
        } else if (event->type() == QEvent::KeyPress) {
            hideTouchPopup();
        }
    }
#endif
    return QComboBox::eventFilter(obj, event);
}

#ifdef Q_OS_ANDROID

void TouchComboBox::showTouchPopup()
{
    if (!isEnabled() || !isVisible()) {
        return;
    }

    if (!m_overlay) {
        // overlay 挂在顶层窗口上，和主窗口同属一个原生窗口，
        // 触摸坐标才是对的
        m_overlay = new QWidget(window());
        m_overlay->setObjectName(QStringLiteral("touchComboPopup"));
        m_overlay->setStyleSheet(QStringLiteral(R"(
QWidget#touchComboPopup {
    background-color: #1E2636;
    border: 1px solid #2A3444;
    border-radius: 6px;
}
QListWidget {
    background-color: transparent;
    border: none;
    outline: none;
    color: #E8E9ED;
}
QListWidget::item {
    color: #E8E9ED;
    background-color: transparent;
    min-height: 52px;
    padding: 10px 12px;
    font-size: 12pt;
}
QListWidget::item:selected {
    background-color: #38BDF8;
    color: #0A0D12;
}
QListWidget::item:hover {
    background-color: #2A3444;
    color: #E8E9ED;
}
QScrollBar:vertical { background-color: #1E2636; width: 10px; border-radius: 5px; }
QScrollBar::handle:vertical { background-color: #38BDF8; border-radius: 5px; min-height: 20px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
)"));

        m_list = new QListWidget(m_overlay);
        m_list->setFrameShape(QFrame::NoFrame);
        m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        auto *lay = new QVBoxLayout(m_overlay);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->addWidget(m_list);

        connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
            const int row = m_list->row(item);
            hideTouchPopup();
            // 先收起再改索引：某些槽函数会重建界面，先收起来更稳
            if (row >= 0 && row < count()) {
                setCurrentIndex(row);
                emit activated(row);
            }
        });
    }

    // 用当前内容重建列表项
    m_list->clear();
    for (int i = 0; i < count(); ++i) {
        m_list->addItem(itemText(i));
    }
    const int cur = currentIndex();
    m_list->setCurrentRow(cur);
    if (cur >= 0 && m_list->item(cur)) {
        m_list->scrollToItem(m_list->item(cur), QAbstractItemView::PositionAtCenter);
    }

    // ---- 定位：默认在下边，空间不够就翻到上面 ----
    QWidget *win = window();
    const QPoint bottomGlobal = mapToGlobal(QPoint(0, height()));
    const QPoint below = win->mapFromGlobal(bottomGlobal);

    const int rowH = m_list->sizeHintForRow(0) > 0 ? m_list->sizeHintForRow(0)
                                                   : kTouchItemHeight;
    const int needed = rowH * count() + 8;

    // 下边放不下而上边够，就往上翻
    const int spaceBelow = win->height() - below.y();
    const int spaceAbove = below.y() - height();
    int popupH = qMin(needed, qMax(spaceBelow, spaceAbove));
    popupH = qMax(popupH, rowH);

    QPoint pos = below;
    if (spaceBelow < popupH && spaceAbove > spaceBelow) {
        pos = win->mapFromGlobal(mapToGlobal(QPoint(0, 0)));
        pos.setY(pos.y() - popupH);
    }
    // 贴边时别顶出窗口
    pos.setX(qBound(0, pos.x(), qMax(0, win->width() - width())));
    pos.setY(qBound(0, pos.y(), qMax(0, win->height() - popupH)));

    m_overlay->setGeometry(pos.x(), pos.y(), width(), popupH);
    m_overlay->raise();
    m_overlay->show();

    // 全局过滤器：点到列表以外的地方就收起
    qApp->installEventFilter(this);
}

void TouchComboBox::hideTouchPopup()
{
    if (m_overlay) {
        m_overlay->hide();
    }
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}

#endif // Q_OS_ANDROID

#include "ui/FlowLayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpace(hSpacing)
    , m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem *item = takeAt(0)) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}

int FlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    }
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    }
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < m_items.size()) {
        return m_items.takeAt(index);
    }
    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    // 取「全部排成一行」的尺寸：宽度够时布局就按一行高度分配，
    // 宽屏下外观和原来的 QHBoxLayout 一致。
    QSize size;
    int x = 0;
    int lineHeight = 0;
    for (const QLayoutItem *item : m_items) {
        const QSize s = item->sizeHint();
        x += s.width() + horizontalSpacing();
        lineHeight = qMax(lineHeight, s.height());
    }
    size = QSize(x, lineHeight);

    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

QSize FlowLayout::minimumSize() const
{
    // 关键：返回「最宽的那个控件」，而不是所有控件宽度累加。
    // 累加值是 QHBoxLayout 撑爆窗口的原因（12 个按钮 = 1280px）。
    QSize size;
    for (const QLayoutItem *item : m_items) {
        size = size.expandedTo(item->minimumSize());
    }

    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    getContentsMargins(&left, &top, &right, &bottom);

    const QRect effectiveRect = rect.adjusted(left, top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    for (QLayoutItem *item : m_items) {
        QWidget *wid = item->widget();

        int spaceX = horizontalSpacing();
        if (spaceX == -1 && wid) {
            spaceX = wid->style()->layoutSpacing(
                QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
        }
        int spaceY = verticalSpacing();
        if (spaceY == -1 && wid) {
            spaceY = wid->style()->layoutSpacing(
                QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
        }

        const QSize hint = item->sizeHint();
        int nextX = x + hint.width() + spaceX;
        // 放不下这一行就换行（lineHeight > 0 保证每行至少摆一个，
        // 否则单个超宽控件会陷入死循环）
        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + hint.width() + spaceX;
            lineHeight = 0;
        }

        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), hint));
        }

        x = nextX;
        lineHeight = qMax(lineHeight, hint.height());
    }

    return y + lineHeight - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent) {
        return -1;
    }
    if (parent->isWidgetType()) {
        auto *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    }
    return static_cast<QLayout *>(parent)->spacing();
}

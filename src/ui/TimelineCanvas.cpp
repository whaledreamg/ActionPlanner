#include "ui/TimelineCanvas.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QtMath>

namespace {

constexpr int kRulerHeight = 34;
constexpr int kTrackTop = kRulerHeight + 8;
constexpr int kTrackHeight = 86;
constexpr int kEdgeGrab = 7;      // 边缘拖动热区宽度（像素）
constexpr double kMinDuration = 0.2;

QColor bgColor()          { return QColor(22, 29, 43); }
QColor gridMinor()        { return QColor(48, 60, 82); }
QColor gridMajor()        { return QColor(66, 82, 110); }
QColor rulerText()        { return QColor(200, 212, 228); }
QColor hintText()         { return QColor(176, 190, 210); }

/** 手位摘要：6 个自由度里取前三个够看了，完整值在编辑对话框里 */
QString handSummary(const HandPose &pose)
{
    return QStringLiteral("%1/%2/%3")
        .arg(pose.dof[0]).arg(pose.dof[1]).arg(pose.dof[2]);
}

} // namespace

TimelineCanvas::TimelineCanvas(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(kTrackTop + kTrackHeight + 16);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::ArrowCursor);
}

void TimelineCanvas::setModel(TimelineModel *model)
{
    m_model = model;
    update();
}

void TimelineCanvas::setSelectedIndex(int index)
{
    if (m_selectedIndex == index) {
        return;
    }
    m_selectedIndex = index;
    emit selectionChanged(index);
    update();
}

void TimelineCanvas::setPixelsPerSecond(double pps)
{
    const double clamped = qBound(2.0, pps, 400.0);
    if (qFuzzyCompare(m_pps, clamped)) {
        return;
    }
    m_pps = clamped;
    // 缩放后 viewStart 可能超出新的可滚范围，夹回来并通知外层更新滚动条
    setViewStart(m_viewStart);
    emit viewRangeChanged();
    update();
}

void TimelineCanvas::setTimelineDuration(double seconds)
{
    const double v = qBound(1.0, seconds, 24.0 * 3600.0);
    if (qFuzzyCompare(m_timelineDuration, v)) {
        return;
    }
    m_timelineDuration = v;
    setViewStart(m_viewStart);      // 变短时可滚范围会缩小，需要夹回
    emit viewRangeChanged();
    update();
}

double TimelineCanvas::effectiveDuration() const
{
    // 时间轴至少铺到配置长度；内容超出就继续往后延，
    // 免得片段被拖到「看不见的墙」以外没法操作。
    double content = 0.0;
    if (m_model) {
        content = m_model->totalDuration();
    }
    return qMax(m_timelineDuration, content + 5.0);
}

double TimelineCanvas::visibleDuration() const
{
    return width() / m_pps;
}

double TimelineCanvas::maxViewStart() const
{
    return qMax(0.0, effectiveDuration() - visibleDuration());
}

void TimelineCanvas::setViewStart(double time)
{
    const double v = qBound(0.0, time, maxViewStart());
    if (qFuzzyCompare(m_viewStart + 1.0, v + 1.0)) {
        return;
    }
    m_viewStart = v;
    emit viewRangeChanged();
    update();
}

// ---------------------------------------------------------------- 布局

QRectF TimelineCanvas::segmentRect(int index) const
{
    if (!m_model || index < 0 || index >= m_model->segmentCount()) {
        return QRectF();
    }
    const Segment seg = m_model->getSegment(index);
    const double x = xForTime(seg.startTime);
    double w = seg.duration * m_pps;
    if (w < 12.0) {
        w = 12.0;   // 太短的片段也给个可点到的宽度
    }
    return QRectF(x, kTrackTop, w, kTrackHeight);
}

int TimelineCanvas::hitTest(const QPointF &pos) const
{
    if (!m_model) {
        return -1;
    }
    // 从后往前找，后加的片段压在先加的上面
    for (int i = m_model->segmentCount() - 1; i >= 0; --i) {
        if (segmentRect(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

TimelineCanvas::DragMode TimelineCanvas::modeAt(const QPointF &pos, int index) const
{
    const QRectF r = segmentRect(index);
    if (!r.isValid()) {
        return DragNone;
    }
    const double grab = qMin<double>(kEdgeGrab, r.width() / 3.0);
    if (pos.x() <= r.left() + grab) {
        return DragResizeLeft;
    }
    if (pos.x() >= r.right() - grab) {
        return DragResizeRight;
    }
    return DragMove;
}

// ---------------------------------------------------------------- 绘制

void TimelineCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.fillRect(rect(), bgColor());

    // 控件尺寸变化会让可视跨度变化，可滚范围随之改变。
    // 这里在绘制前夹一次，不需要额外处理 resizeEvent。
    const double clampedStart = qBound(0.0, m_viewStart, maxViewStart());
    if (!qFuzzyCompare(clampedStart + 1.0, m_viewStart + 1.0)) {
        m_viewStart = clampedStart;
        emit viewRangeChanged();
    }

    drawRuler(p);

    if (!m_model || m_model->segmentCount() == 0) {
        drawEmptyHint(p);
        return;
    }

    // 视口内才画，片段很多时不至于卡
    for (int i = 0; i < m_model->segmentCount(); ++i) {
        const QRectF r = segmentRect(i);
        if (r.right() < 0 || r.left() > width()) {
            continue;
        }
        drawSegment(p, m_model->getSegment(i), i);
    }

    drawPlayhead(p);

    // 底部操作提示
    QFont hintFont = p.font();
    hintFont.setPointSizeF(8.0);
    p.setFont(hintFont);
    p.setPen(hintText());
    p.drawText(QRectF(8, height() - 21, width() - 16, 18),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("拖动片段平移 · 拖两端改时长 · 双击或右键编辑 · Ctrl+滚轮缩放"));
}

void TimelineCanvas::drawRuler(QPainter &painter)
{
    painter.fillRect(QRect(0, 0, width(), kRulerHeight), QColor(18, 24, 36));

    // 先决定刻度间隔，避免很密的时候标签糊成一片
    static const double steps[] = {0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0,
                                   30.0, 60.0, 120.0, 300.0, 600.0};
    double step = 1.0;
    for (double s : steps) {
        if (s * m_pps >= 56.0) {
            step = s;
            break;
        }
        step = s;
    }

    const double endTime = qMin(timeAtX(width()), effectiveDuration());
    // 刻度必须落在 step 的整数倍上。若从 m_viewStart 起步递推，滚动后
    // 零点会跟着视口漂移，刻度标签就和片段位置对不上了——片段用的是
    // 绝对时间（xForTime），两边基准必须一致。
    //
    // 用整数下标乘以 step，而不是 t += step 累加：0.1 这类步长累加十几次
    // 就会出现 12.300000000000001，标签会显示怪值，刻度也会慢慢错位。
    const int firstTick = qMax(0, qFloor(m_viewStart / step));
    const int lastTick = qCeil(endTime / step);

    painter.setPen(QPen(gridMinor(), 1));
    for (int k = firstTick; k <= lastTick; ++k) {
        const double x = xForTime(k * step);
        painter.drawLine(QPointF(x, kRulerHeight - 6), QPointF(x, height()));
    }

    painter.setPen(QPen(rulerText(), 1));
    QFont f = painter.font();
    f.setPointSizeF(8.5);
    f.setBold(false);
    painter.setFont(f);

    for (int k = firstTick; k <= lastTick; ++k) {
        const double t = k * step;
        const double x = xForTime(t);
        painter.drawLine(QPointF(x, kRulerHeight - 10), QPointF(x, kRulerHeight));
        painter.drawText(QPointF(x + 4, kRulerHeight - 13), formatTime(t, step));
    }

    painter.setPen(QPen(gridMajor(), 1));
    painter.drawLine(QPointF(0, kRulerHeight), QPointF(width(), kRulerHeight));
}

QString TimelineCanvas::formatTime(double t, double step) const
{
    // 短刻度用「12.5s」，长刻度用「1:30」——3 分钟的轴上一堆 s 后缀太吵
    if (step < 1.0) {
        return QString("%1s").arg(t, 0, 'f', 1);
    }
    if (t < 60.0 && step < 60.0) {
        return QString("%1s").arg(t, 0, 'f', 0);
    }
    const int totalSec = qRound(t);
    return QString("%1:%2")
        .arg(totalSec / 60)
        .arg(totalSec % 60, 2, 10, QLatin1Char('0'));
}

void TimelineCanvas::drawSegment(QPainter &painter, const Segment &seg, int index)
{
    const QRectF r = segmentRect(index);
    const bool selected = (index == m_selectedIndex);
    const bool isHand = seg.isHand();

    // 主体。夹爪片段用一套明显不同的配色，
    // 一眼就能看出这条轨道上的片段不会驱动机械臂。
    QLinearGradient grad(r.topLeft(), r.bottomLeft());
    if (selected) {
        grad.setColorAt(0.0, isHand ? QColor(90, 200, 170) : QColor(64, 148, 214));
        grad.setColorAt(1.0, isHand ? QColor(46, 150, 126) : QColor(38, 104, 168));
    } else {
        grad.setColorAt(0.0, isHand ? QColor(52, 118, 104) : QColor(46, 84, 122));
        grad.setColorAt(1.0, isHand ? QColor(36, 88, 78)  : QColor(32, 62, 94));
    }

    QPainterPath path;
    path.addRoundedRect(r, 6, 6);
    painter.fillPath(path, grad);

    painter.setPen(QPen(selected ? (isHand ? QColor(140, 245, 210) : QColor(120, 196, 255))
                                 : (isHand ? QColor(66, 140, 122) : QColor(58, 92, 128)),
                        selected ? 2.0 : 1.0));
    painter.drawPath(path);

    // 两端把手
    if (selected && r.width() > 28) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(200, 240, 230, 200));
        painter.drawRoundedRect(QRectF(r.left() + 3, r.center().y() - 12, 3, 24), 1.5, 1.5);
        painter.drawRoundedRect(QRectF(r.right() - 6, r.center().y() - 12, 3, 24), 1.5, 1.5);
        painter.setBrush(Qt::NoBrush);
    }

    if (r.width() < 34) {
        return;     // 太窄就不塞文字了
    }

    painter.save();
    painter.setClipRect(r.adjusted(2, 2, -2, -2));

    // 标签
    QFont f = painter.font();
    f.setPointSizeF(9.5);
    f.setBold(true);
    painter.setFont(f);
    painter.setPen(QColor(238, 244, 255));
    painter.drawText(QRectF(r.left() + 10, r.top() + 4, r.width() - 20, 18),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("%1 %2").arg(isHand ? QStringLiteral("✋")
                                                       : QStringLiteral("🦾"),
                                                 seg.label));

    // 时间范围
    f.setBold(false);
    f.setPointSizeF(8.5);
    painter.setFont(f);
    painter.setPen(QColor(232, 238, 248));
    painter.drawText(QRectF(r.left() + 10, r.top() + 22, r.width() - 20, 15),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString("%1 → %2 s")
                         .arg(seg.startTime, 0, 'f', 2)
                         .arg(seg.startTime + seg.duration, 0, 'f', 2));

    if (isHand) {
        // 夹爪片段：显示手位摘要，不显示插值与关节角——
        // 它不插值、也不碰机械臂。
        painter.setPen(QColor(210, 240, 232));
        painter.drawText(QRectF(r.left() + 10, r.top() + 38, r.width() - 20, 15),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("手位 左%1 右%2")
                             .arg(handSummary(seg.leftHand))
                             .arg(handSummary(seg.rightHand)));
        painter.restore();
        return;
    }

    // 机械臂片段：插值方式
    painter.setPen(QColor(210, 222, 238));
    painter.drawText(QRectF(r.left() + 10, r.top() + 40, r.width() - 20, 15),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString("插值 %1").arg(seg.interpolationType));

    // 结束关节角：本段的唯一目标姿态
    painter.setPen(QColor(196, 210, 228));
    painter.drawText(QRectF(r.left() + 10, r.top() + 58, r.width() - 20, 15),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString("终点 J1 %1   J2 %2")
                         .arg(seg.endJoints.j[0], 0, 'f', 2)
                         .arg(seg.endJoints.j[1], 0, 'f', 2));

    painter.restore();
}

void TimelineCanvas::drawEmptyHint(QPainter &painter)
{
    painter.setPen(QPen(gridMajor(), 1, Qt::DashLine));
    painter.drawRect(QRectF(0.5, kTrackTop + 0.5, width() - 1.0, kTrackHeight - 1.0));

    painter.setPen(hintText());
    QFont f = painter.font();
    f.setPointSizeF(9.5);
    painter.setFont(f);
    painter.drawText(QRectF(0, kTrackTop, width(), kTrackHeight),
                     Qt::AlignCenter,
                     QStringLiteral("还没有片段 — 点击「添加片段」开始编排动作序列"));
}

// ---------------------------------------------------------------- 交互

double TimelineCanvas::snap(double t)
{
    // 吸附到 0.05 s 网格
    return qMax(0.0, qRound(t / 0.05) * 0.05);
}

void TimelineCanvas::setPlayheadTime(double time)
{
    if (qFuzzyCompare(m_playheadTime + 2.0, time + 2.0)) {
        return;
    }
    m_playheadTime = time;

    // 播放时自动横向跟随，让游标留在视野内
    if (time >= 0.0 && m_autoScroll) {
        const double x = xForTime(time);
        if (x > width() - 40.0) {
            // 把游标挪到靠右侧的位置，留出一点提前量；
            // setViewStart 会自己夹到可滚范围内，不会滚出时间轴末端
            setViewStart(time - (width() - 60.0) / m_pps);
        } else if (x < 0.0) {
            setViewStart(qMax(0.0, time - 20.0 / m_pps));
        }
    }
    update();
}

void TimelineCanvas::drawPlayhead(QPainter &painter)
{
    if (m_playheadTime < 0.0) {
        return;
    }
    const double x = xForTime(m_playheadTime);

    painter.setPen(QPen(QColor(255, 138, 96), 2));
    painter.drawLine(QPointF(x, kRulerHeight - 14), QPointF(x, height() - 4));

    // 顶部三角手柄
    QPolygonF head;
    head << QPointF(x - 6, kRulerHeight - 14)
         << QPointF(x + 6, kRulerHeight - 14)
         << QPointF(x, kRulerHeight - 4);
    painter.setBrush(QColor(255, 138, 96));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(head);
    painter.setBrush(Qt::NoBrush);
}

void TimelineCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_model) {
        QWidget::mousePressEvent(event);
        return;
    }

    const QPointF pos = QPointF(event->pos());

    // 标尺区域（或游标附近）：拖动播放游标
    const double phX = (m_playheadTime >= 0.0) ? xForTime(m_playheadTime) : -100.0;
    const bool nearPlayhead = (m_playheadTime >= 0.0) && qAbs(pos.x() - phX) <= 6.0;
    if (pos.y() <= kRulerHeight || nearPlayhead) {
        m_dragMode = DragPlayhead;
        m_dragIndex = -1;
        const double t = snap(qMax(0.0, timeAtX(pos.x())));
        m_playheadTime = t;
        setCursor(Qt::SplitHCursor);
        emit playheadSeekRequested(t);
        update();
        return;
    }

    const int idx = hitTest(pos);
    setSelectedIndex(idx);

    if (idx < 0) {
        m_dragMode = DragNone;
        m_dragIndex = -1;
        return;
    }

    m_dragMode = modeAt(pos, idx);
    m_dragIndex = idx;
    m_dragPressTime = timeAtX(pos.x());
    m_dragOriginal = m_model->getSegment(idx);

    setCursor(m_dragMode == DragMove ? Qt::ClosedHandCursor : Qt::SizeHorCursor);
    update();
}

void TimelineCanvas::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF pos = QPointF(event->pos());

    if (m_dragMode == DragPlayhead) {
        const double t = snap(qMax(0.0, timeAtX(pos.x())));
        m_playheadTime = t;
        emit playheadSeekRequested(t);
        update();
        return;
    }

    if (m_dragMode == DragNone || m_dragIndex < 0 || !m_model) {
        // 仅更新光标提示
        const double phX = (m_playheadTime >= 0.0) ? xForTime(m_playheadTime) : -100.0;
        if (pos.y() <= kRulerHeight || (m_playheadTime >= 0.0 && qAbs(pos.x() - phX) <= 6.0)) {
            setCursor(Qt::SplitHCursor);
            return;
        }
        const int idx = hitTest(pos);
        if (idx < 0) {
            setCursor(Qt::ArrowCursor);
        } else {
            const DragMode m = modeAt(pos, idx);
            setCursor(m == DragMove ? Qt::OpenHandCursor : Qt::SizeHorCursor);
        }
        return;
    }

    const double nowTime = timeAtX(pos.x());
    const double delta = nowTime - m_dragPressTime;

    Segment seg = m_dragOriginal;

    switch (m_dragMode) {
    case DragMove: {
        seg.startTime = snap(qMax(0.0, m_dragOriginal.startTime + delta));
        break;
    }
    case DragResizeRight: {
        const double end = m_dragOriginal.startTime + m_dragOriginal.duration + delta;
        seg.duration = qMax(kMinDuration, snap(end) - m_dragOriginal.startTime);
        break;
    }
    case DragResizeLeft: {
        const double originalEnd = m_dragOriginal.startTime + m_dragOriginal.duration;
        double newStart = snap(m_dragOriginal.startTime + delta);
        newStart = qBound(0.0, newStart, originalEnd - kMinDuration);
        seg.startTime = newStart;
        seg.duration = originalEnd - newStart;
        break;
    }
    default:
        break;
    }

    m_model->updateSegment(m_dragIndex, seg);
    update();
}

void TimelineCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragMode = DragNone;
    m_dragIndex = -1;
    setCursor(Qt::ArrowCursor);
}

void TimelineCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const int idx = hitTest(QPointF(event->pos()));
    if (idx >= 0) {
        setSelectedIndex(idx);
        emit segmentEditRequested(idx);
    }
}

void TimelineCanvas::contextMenuEvent(QContextMenuEvent *event)
{
    if (!m_model) {
        return;
    }

    const int idx = hitTest(QPointF(event->pos()));
    setSelectedIndex(idx);

    QMenu menu(this);
    QAction *editAction = menu.addAction(QStringLiteral("编辑片段..."));
    QAction *deleteAction = menu.addAction(QStringLiteral("删除片段"));
    menu.addSeparator();
    QAction *addArmAction = menu.addAction(QStringLiteral("插入机械臂片段"));
    QAction *addHandAction = menu.addAction(QStringLiteral("插入夹爪片段"));
    menu.addSeparator();
    QAction *zoomIn  = menu.addAction(QStringLiteral("放大时间轴"));
    QAction *zoomOut = menu.addAction(QStringLiteral("缩小时间轴"));

    editAction->setEnabled(idx >= 0);
    deleteAction->setEnabled(idx >= 0);

    QAction *chosen = menu.exec(event->globalPos());
    if (!chosen) {
        return;
    }

    if (chosen == editAction) {
        emit segmentEditRequested(idx);
    } else if (chosen == deleteAction) {
        emit segmentDeleteRequested(idx);
    } else if (chosen == addArmAction) {
        emit segmentAddRequested(int(SegmentType::Arm));
    } else if (chosen == addHandAction) {
        emit segmentAddRequested(int(SegmentType::Hand));
    } else if (chosen == zoomIn) {
        setPixelsPerSecond(m_pps * 1.25);
    } else if (chosen == zoomOut) {
        setPixelsPerSecond(m_pps / 1.25);
    }
}

void TimelineCanvas::wheelEvent(QWheelEvent *event)
{
    const int dy = event->angleDelta().y();

    // Ctrl + 滚轮：缩放（以鼠标位置为锚点，缩放后指针下的时刻不动）
    if (event->modifiers() & Qt::ControlModifier) {
        const double anchorTime = timeAtX(event->position().x());
        const double factor = dy > 0 ? 1.15 : (1.0 / 1.15);
        const double newPps = qBound(2.0, m_pps * factor, 400.0);
        if (!qFuzzyCompare(newPps, m_pps)) {
            m_pps = newPps;
            // 让锚点时刻仍然停在同一像素位置
            setViewStart(anchorTime - event->position().x() / m_pps);
            emit viewRangeChanged();
            update();
        }
        event->accept();
        return;
    }

    // 普通滚轮 / Shift + 滚轮：横向滚动时间轴
    if (dy != 0 || event->angleDelta().x() != 0) {
        const int delta = (dy != 0) ? dy : event->angleDelta().x();
        const double seconds = -(delta / 120.0) * (visibleDuration() * 0.15);
        setViewStart(m_viewStart + seconds);
        event->accept();
        return;
    }

    QWidget::wheelEvent(event);
}

void TimelineCanvas::leaveEvent(QEvent *event)
{
    if (m_dragMode == DragNone) {
        setCursor(Qt::ArrowCursor);
    }
    QWidget::leaveEvent(event);
}

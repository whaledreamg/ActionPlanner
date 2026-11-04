#ifndef TIMELINECANVAS_H
#define TIMELINECANVAS_H

#include <QWidget>
#include <QPointF>
#include <QRectF>

#include "timeline/TimelineModel.h"

/**
 * 时间轴画布。
 *
 * 所有绘制与鼠标交互都发生在这个子控件内部，避免"父控件画内容、子控件盖在上面"
 * 导致的黑块遮挡，也保证鼠标事件能真正落到处理逻辑上。
 */
class TimelineCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineCanvas(QWidget *parent = nullptr);

    void setModel(TimelineModel *model);
    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);

    double pixelsPerSecond() const { return m_pps; }
    void setPixelsPerSecond(double pps);

    /**
     * 时间轴总长度（秒）。
     *
     * 这是「时间轴有多长」这个编辑空间的大小，不是内容长度：
     * 片段可以只有几秒，但时间轴仍铺 3 分钟，方便往后继续编排。
     * 实际可滚动的范围取它与内容长度（最后一个片段的结束时刻）的较大者。
     */
    void setTimelineDuration(double seconds);
    double timelineDuration() const { return m_timelineDuration; }

    /** 画布实际覆盖的总长度：配置长度与内容长度取大 */
    double effectiveDuration() const;

    /** 视图左边界对应的时间（秒） */
    double viewStart() const { return m_viewStart; }
    void setViewStart(double time);

    /** 当前视口能显示多长的时间跨度 */
    double visibleDuration() const;

    /** 可滚动最大值（秒），0 表示不需要滚动 */
    double maxViewStart() const;

    /**
     * 播放时是否自动横向跟随游标。
     *
     * 关掉后视野固定，游标会走出视野——适合盯着某一段看细节。
     */
    void setAutoScrollToPlayhead(bool on) { m_autoScroll = on; }
    bool autoScrollToPlayhead() const { return m_autoScroll; }

    /** 播放游标位置（秒）。负值表示不显示游标 */
    double playheadTime() const { return m_playheadTime; }
    void setPlayheadTime(double time);

signals:
    void selectionChanged(int index);
    void segmentEditRequested(int index);
    void segmentDeleteRequested(int index);
    /** 右键菜单请求新建片段。参数是 SegmentType 的 int 值。 */
    void segmentAddRequested(int type);
    /** 用户点击标尺或拖动游标请求跳转 */
    void playheadSeekRequested(double time);

    /** 视口滚动位置或时间轴长度变化，供外层同步滚动条 */
    void viewRangeChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    enum DragMode { DragNone, DragMove, DragResizeLeft, DragResizeRight, DragPlayhead };

    QRectF segmentRect(int index) const;
    int hitTest(const QPointF &pos) const;
    DragMode modeAt(const QPointF &pos, int index) const;

    void drawRuler(QPainter &painter);
    void drawSegment(QPainter &painter, const Segment &seg, int index);
    void drawEmptyHint(QPainter &painter);
    void drawPlayhead(QPainter &painter);

    /** 刻度标签：短刻度用秒，长刻度用 分:秒 */
    QString formatTime(double t, double step) const;

    double timeAtX(double x) const { return m_viewStart + x / m_pps; }
    double xForTime(double t) const { return (t - m_viewStart) * m_pps; }
    static double snap(double t);

    TimelineModel *m_model = nullptr;
    double m_pps = 60.0;
    int m_selectedIndex = -1;
    double m_playheadTime = -1.0;

    /** 配置的时间轴总长度（秒），默认 3 分钟 */
    double m_timelineDuration = 180.0;

    /** 视图左边界对应的时间（播放时自动跟随） */
    double m_viewStart = 0.0;

    /** 播放时是否自动滚动视野跟随游标 */
    bool m_autoScroll = true;

    DragMode m_dragMode = DragNone;
    int m_dragIndex = -1;
    double m_dragPressTime = 0.0;
    Segment m_dragOriginal;
};

#endif // TIMELINECANVAS_H

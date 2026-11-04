#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QList>
#include <QStyle>

/**
 * 自动折行的水平布局：控件按行摆放，放不下就换到下一行。
 *
 * 用途：时间轴工具条上有一排按钮（播放/暂停/停止/片段/保存…）。桌面端
 * 宽度足够，一行排开最省高度；横屏平板上可视宽度只有 1280 左右，一行
 * 排不下，而 QHBoxLayout 的 minimumSize 会把所有按钮宽度累加起来
 * （实测 1280px），于是整个窗口被撑得比屏幕宽，右侧按钮被裁掉。
 *
 * 换成本布局后最小宽度只等于「最宽的那个控件」，多余宽度时仍然排成
 * 一行，所以宽屏外观不变，窄屏自动折成多行。
 *
 * 实现取自 Qt 官方 FlowLayout 示例，关键点是：
 *   - minimumSize() 返回单个控件的最大最小尺寸，而不是累加值；
 *   - heightForWidth() 按给定宽度算需要几行；
 *   - sizeHint() 让所有控件排成一行的宽度，宽屏下据此保持单行。
 */
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent = nullptr,
                        int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;

    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

private:
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> m_items;
    int m_hSpace;
    int m_vSpace;
};

#endif // FLOWLAYOUT_H

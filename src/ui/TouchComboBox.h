#ifndef UI_TOUCHCOMBOBOX_H
#define UI_TOUCHCOMBOBOX_H

#include <QComboBox>

/**
 * 移动端可用的下拉框。
 *
 * QComboBox 的下拉列表是一个 native popup 窗口（QComboBoxPrivateContainer），
 * 在 Android 上它收不到触摸事件：列表确实弹出来了，但点任何 item 都没反应，
 * 当前项也就一直变不了。表现和之前 QMessageBox 的问题同源——
 * Android 上凡是独立于主窗口的 native 子窗口，触摸坐标都会对不上。
 *
 * 解决办法是在 Android 上不用 native popup，改成把列表作为**主窗口内的
 * 子控件**盖在下拉框下面（overlay）。子控件和主窗口在同一个原生窗口里，
 * 触摸坐标正常，能可靠地点中。
 *
 * 桌面端行为与原生 QComboBox 完全一致，不做任何改动。
 *
 * 用法：直接 new TouchComboBox，或者在 .ui 里提升（promote）本控件。
 * 外部仍可当普通 QComboBox 用，信号（currentIndexChanged）语义不变。
 */
class TouchComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit TouchComboBox(QWidget *parent = nullptr);

    /** 覆盖弹出行为：Android 走窗口内 overlay，其余平台走原生实现 */
    void showPopup() override;

    /** 收起：Android 上要一并把 overlay 藏掉 */
    void hidePopup() override;

protected:
    /** 监听主窗口的点击，用于点列表外部时收起 */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    /** 构建并显示窗口内下拉列表 */
    void showTouchPopup();
    void hideTouchPopup();

    /** 覆盖在主窗口上的列表容器（仅 Android 使用） */
    QWidget *m_overlay = nullptr;
    class QListWidget *m_list = nullptr;
};

#endif // UI_TOUCHCOMBOBOX_H

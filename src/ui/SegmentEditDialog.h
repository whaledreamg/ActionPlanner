#ifndef SEGMENTEDITDIALOG_H
#define SEGMENTEDITDIALOG_H

#include <QDialog>
#include <QVector>
#include <QStringList>

#include "timeline/TimelineModel.h"
#include "timeline/JointLimits.h"

class QDialogButtonBox;
class QDoubleSpinBox;
class QGridLayout;
class QGroupBox;
class QLineEdit;
class QScrollArea;
class QVBoxLayout;
class QWidget;

/**
 * 片段编辑对话框的公共基类。
 *
 * 只负责所有片段都有的部分：标签、起始时间、时长、滚动区、底部按钮，
 * 以及「确定」时的公共流程。各类型的编辑区由子类在 buildEditor() 里搭：
 *   - ArmSegmentEditDialog：左右臂关节角（同页，无页签）
 *   - HandSegmentEditDialog：左右手手位（同页，无页签）
 *
 * 早先用一个 QTabWidget 把「左臂 / 右臂 / 夹爪」放在同一个窗口里，靠
 * setTabVisible() 按片段类型切换页签；在 Android 上切换后页签头不重绘，
 * 于是拆成两个独立窗口，各自只显示自己相关的内容，不再有页签。
 *
 * 角度在内部一律以弧度保存，界面上按用户选择的单位（度 / 弧度）显示。
 */
class SegmentEditDialog : public QDialog
{
    Q_OBJECT

public:
    SegmentEditDialog(const Segment& segment,
                      const JointState& derivedStart,
                      QWidget *parent = nullptr);

    /** 用户点击「确定」后的结果。 */
    Segment result() const { return m_result; }

    /** 供「当前实测角 → 结束角」使用的实时关节角。 */
    void setCurrentAngles(const QVector<double>& leftArm, const QVector<double>& rightArm);

    /** 供「取当前手位」使用的当前手位（归一化 0~1000），未连接时传空。 */
    virtual void setCurrentHand(const QVector<int>& leftHand, const QVector<int>& rightHand,
                                bool handAvailable);

signals:
    /** 机械臂预览：previewEnd=true 预览结束角，否则预览起点 */
    void previewChanged(bool previewEnd,
                        const QVector<double>& leftArm,
                        const QVector<double>& rightArm);

protected:
    /** 子类在此搭建自己的编辑区，body 为滚动区内部的主布局。 */
    virtual void buildEditor(QVBoxLayout *body) = 0;

    /** 把编辑区控件上的值写回 m_result。 */
    virtual void syncResultFromUi() = 0;

    /** 模型值或单位变化后刷新编辑区显示。 */
    virtual void refreshEditor() {}

    /** 类型专属的提交前校验，返回告警文本（空表示无告警）。 */
    virtual QStringList collectWarnings() const { return QStringList(); }

    /** 编辑过程中发出预览；无预览概念的类型（夹爪）无需重写。 */
    virtual void emitPreview() {}

    /**
     * 由子类构造函数末尾调用：搭好公共部分 + 调 buildEditor() + 移动端适配。
     * 必须在子类构造函数里调（不能在基类构造函数里，那时虚函数还没生效）。
     */
    void finishSetup();

    /** 统一变更入口：写回模型 + 发预览。m_updating 期间自动忽略。 */
    void notifyChanged();

    Segment m_result;
    JointState m_derivedStart;
    JointLimitsLoader::Limits m_limits;
    QVector<double> m_currentLeft;
    QVector<double> m_currentRight;
    /** 灵巧手当前实测手位（归一化 0~1000） */
    QVector<int> m_currentHandLeft;
    QVector<int> m_currentHandRight;
    bool m_handAvailable = false;
    /** 防止填充/批量更新时触发连锁回调 */
    bool m_updating = false;

    /** 滚动区内部的主体 widget，子类新建控件时作为 parent */
    QWidget *m_body = nullptr;

private:
    void validateAndAccept();

    QLineEdit *m_labelEdit = nullptr;
    QDoubleSpinBox *m_startSpin = nullptr;
    QDoubleSpinBox *m_durationSpin = nullptr;
    QScrollArea *m_scroll = nullptr;
    QVBoxLayout *m_bodyLayout = nullptr;
    QGroupBox *m_basicGroup = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
};

#endif // SEGMENTEDITDIALOG_H

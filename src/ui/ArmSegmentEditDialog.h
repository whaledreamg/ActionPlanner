#ifndef ARMSEGMENTEDITDIALOG_H
#define ARMSEGMENTEDITDIALOG_H

#include "ui/SegmentEditDialog.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QSlider;
class QVBoxLayout;

/**
 * 机械臂片段编辑窗口。
 *
 * 左右臂同页显示，不再用页签：Android 上切换页签后页签头不重绘，
 * 而左右臂本来就需要对照着调（例如先摆左臂再复制给右臂），
 * 放在同一页反而更顺手。
 *
 * 只编辑「结束关节角」：起点由上一段终点（第一段则用当前实测姿态）
 * 自动推导，界面上只读显示，避免同一个角度要在两处维护。
 */
class ArmSegmentEditDialog : public SegmentEditDialog
{
    Q_OBJECT

public:
    explicit ArmSegmentEditDialog(const Segment& segment,
                                  const JointState& derivedStart,
                                  QWidget *parent = nullptr);

protected:
    void buildEditor(QVBoxLayout *body) override;
    void syncResultFromUi() override;
    void refreshEditor() override;
    QStringList collectWarnings() const override;
    void emitPreview() override;

private:
    QWidget* createArmGroup(bool leftArm);
    /** 工具条：批量操作 + 单位/插值/预览选项 */
    QWidget* createToolbar();

    void onUnitChanged();
    void onCurrentToEnd();
    void onEndFromStart();
    void onResetBoth();
    void onCopyLeftToRight();

    double endValueOf(bool leftArm, int joint) const;
    void setEndValue(bool leftArm, int joint, double rad);
    double startValueOf(bool leftArm, int joint) const;

    bool m_degrees = true;      // 界面单位：默认度
    bool m_previewEnd = true;   // 预览结束角还是起点

    QComboBox *m_interpCombo = nullptr;
    QComboBox *m_unitCombo = nullptr;
    QComboBox *m_previewCombo = nullptr;

    // [左臂, 右臂] x [J1..J7]
    QDoubleSpinBox *m_spin[2][7] = {};
    QSlider *m_slider[2][7] = {};
    QLabel *m_limitLabel[2][7] = {};
    QLabel *m_startLabel[2][7] = {};
};

#endif // ARMSEGMENTEDITDIALOG_H

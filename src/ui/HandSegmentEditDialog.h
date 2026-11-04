#ifndef HANDSEGMENTEDITDIALOG_H
#define HANDSEGMENTEDITDIALOG_H

#include "ui/SegmentEditDialog.h"

class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class QVBoxLayout;

/**
 * 夹爪（灵巧手）片段编辑窗口。
 *
 * 左右手同页显示，不再用页签。整段只下发一次手位，因此没有插值和
 * 预览页签——机械臂在整段期间保持不动。
 */
class HandSegmentEditDialog : public SegmentEditDialog
{
    Q_OBJECT

public:
    explicit HandSegmentEditDialog(const Segment& segment,
                                   const JointState& derivedStart,
                                   QWidget *parent = nullptr);

    /** 覆盖基类：未连接灵巧手时「取当前手位」按钮置灰。 */
    void setCurrentHand(const QVector<int>& leftHand, const QVector<int>& rightHand,
                        bool handAvailable);

protected:
    void buildEditor(QVBoxLayout *body) override;
    void syncResultFromUi() override;
    void refreshEditor() override;

private:
    QWidget* createHead();
    QWidget* createHandGroup(bool leftHand);

    int handValueOf(bool leftHand, int dof) const;
    void setHandValue(bool leftHand, int dof, int value);
    void syncValueLabels();

    QComboBox *m_presetCombo = nullptr;
    QPushButton *m_btnFromCurrent = nullptr;
    QSlider *m_slider[2][HandPose::kDofCount] = {};   // [左, 右] x [DOF]
    QLabel *m_valueLabel[2][HandPose::kDofCount] = {};
};

#endif // HANDSEGMENTEDITDIALOG_H

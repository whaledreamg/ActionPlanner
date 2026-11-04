#ifndef TIMELINESETTINGSDIALOG_H
#define TIMELINESETTINGSDIALOG_H

#include <QDialog>
#include <QString>

class QComboBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;
class QLineEdit;

/**
 * 设置页。
 *
 * 这里放的是「本机偏好」而非动作内容：时间轴总长度、新增片段的默认时长、
 * 播放时是否自动跟随游标，以及 UDP 关节流的下发频率。
 *
 * 这些都不写进动作序列文件——换台机器打开同一个动作，
 * 轴多长、发多快是设备与个人偏好，不该跟着动作走。
 */
class TimelineSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /** 配置项，时间值统一用秒。 */
    struct Settings {
        double timelineDuration = 180.0;   ///< 时间轴总长度，默认 3 分钟
        double defaultArmDuration = 2.0;   ///< 新增机械臂片段的默认时长
        double defaultHandDuration = 1.0;  ///< 新增夹爪片段的默认时长
        bool autoScrollToPlayhead = true;  ///< 播放时自动横向跟随游标
        int streamFrequency = 100;         ///< UDP 关节流下发频率（Hz）

        /**
         * 加载动作序列时文件对话框的起始目录。
         *
         * 空表示「不指定」，交给系统对话框决定（通常会沿用上次打开的位置）。
         * 存的是用户显式选定的目录，而不是「上次加载过的目录」——
         * 后者会让起始位置随一次意外操作漂走，且无法回到初始状态。
         */
        QString loadFolder;

        bool operator==(const Settings &o) const {
            return qFuzzyCompare(timelineDuration + 1.0, o.timelineDuration + 1.0)
                && qFuzzyCompare(defaultArmDuration + 1.0, o.defaultArmDuration + 1.0)
                && qFuzzyCompare(defaultHandDuration + 1.0, o.defaultHandDuration + 1.0)
                && autoScrollToPlayhead == o.autoScrollToPlayhead
                && streamFrequency == o.streamFrequency
                && loadFolder == o.loadFolder;
        }
        bool operator!=(const Settings &o) const { return !(*this == o); }
    };

    explicit TimelineSettingsDialog(const Settings &current, QWidget *parent = nullptr);

    /** 用户点击「确定」后的结果 */
    Settings result() const { return m_result; }

private:
    void setupUi();
    void fillFrom(const Settings &s);
    void collectToResult();
    /** 把当前长度换算成人话显示在旁边 */
    void updateDurationHint();

    /**
     * 把用户输入的目录规范化成绝对路径。
     *
     * 展开 "~"、补全相对路径、去掉多余的 "/" 与末尾分隔符。
     * 空输入返回空串（表示不指定）。
     */
    static QString normalizeFolder(const QString &input);

    Settings m_result;
    bool m_updating = false;

    QDoubleSpinBox *m_durationSpin = nullptr;
    QLabel *m_durationHint = nullptr;
    QDoubleSpinBox *m_armDurationSpin = nullptr;
    QDoubleSpinBox *m_handDurationSpin = nullptr;
    QCheckBox *m_autoScrollCheck = nullptr;
    QComboBox *m_frequencyCombo = nullptr;
    QLineEdit *m_loadFolderEdit = nullptr;
};

#endif // TIMELINESETTINGSDIALOG_H

#ifndef UI_MESSAGECENTER_H
#define UI_MESSAGECENTER_H

#include <QObject>
#include <QString>
#include <QList>

class QWidget;
class MessageToast;

/// 消息类型（配色与主界面的深色风格一致）
enum class MessageLevel {
    Info = 0,
    Success = 1,
    Warning = 2,
    Error = 3
};

/**
 * @brief 浮动消息提示（Toast），替代 QMessageBox
 *
 * 为什么不用 QMessageBox：
 *  Android 上模态 QMessageBox 在部分机型/窗口形态下按钮收不到触摸事件，
 *  表现为"弹出来但点不掉"，直接把整个界面卡死。
 *  参考 RobotControl 的做法：全局提示统一走非阻塞的浮动消息条，
 *  按钮依旧是界面上原有的控件，不依赖任何模态窗口。
 *
 * 每个提示是宿主窗口的一个普通子控件，只占据自己那一小块矩形，
 * 不会遮挡其它区域的点击；错误类型带手动关闭按钮且不自动消失。
 */
class MessageCenter : public QObject
{
    Q_OBJECT

public:
    static MessageCenter *instance();

    // ---- 便捷接口：context 只用来定位宿主窗口 ----
    static void info(const QWidget *context, const QString &text);
    static void success(const QWidget *context, const QString &text);
    static void warning(const QWidget *context, const QString &text);
    static void error(const QWidget *context, const QString &text);

    /** 通用入口。durationMs <= 0 时用默认时长（错误类型不自动消失） */
    void show(const QWidget *context, MessageLevel level, const QString &text,
              int durationMs = 0);

    /** 立即清空所有提示（切换界面时用） */
    void clear();

protected:
    /// 宿主窗口尺寸变化时重新居中提示条
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onToastDismissed(MessageToast *toast);

private:
    explicit MessageCenter(QObject *parent = nullptr);

    /** 把提示挂到 context 所属的顶层窗口上，首次调用时建立宿主关系 */
    QWidget *ensureHost(const QWidget *context);

    void relayout();
    void removeOldest();

    QWidget *m_host = nullptr;
    QList<MessageToast *> m_toasts;
};

#endif // UI_MESSAGECENTER_H

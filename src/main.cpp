#include "ui/MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 窗口图标：桌面端显示在标题栏和任务栏，Android 上由 manifest 的
    // android:icon 指定启动器图标，这里设置的是任务切换器里的图标。
    a.setWindowIcon(QIcon(QStringLiteral(":/resources/icons/appicon.png")));
    a.setApplicationName(QStringLiteral("ActionPlanner"));
    a.setApplicationDisplayName(QStringLiteral("双臂机械臂动作规划器"));

    MainWindow w;
    w.show();
    return a.exec();
}

QT += core gui widgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Qt6 requires explicit OpenGL widgets module
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += openglwidgets
}

# Check if websockets module is available
qtHaveModule(websockets) {
    QT += websockets
} else {
    message("WebSockets module not found - some features will be limited")
}

# Optional 3D modules for rendering
qtHaveModule(3dcore) {
    QT += 3dcore 3drender 3dinput
} else {
    message("Qt3D modules not found - using OpenGL only")
}

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/ui/MainWindow.cpp \
    src/ui/RobotControlPanel.cpp \
    src/ui/SimulationWidget.cpp \
    src/ui/TimelineEditor.cpp \
    src/ui/TimelineCanvas.cpp \
    src/ui/FlowLayout.cpp \
    src/ui/SegmentEditDialog.cpp \
    src/ui/ArmSegmentEditDialog.cpp \
    src/ui/HandSegmentEditDialog.cpp \
    src/ui/TimelineSettingsDialog.cpp \
    src/ui/MessageCenter.cpp \
    src/ui/TouchComboBox.cpp \
    src/robot/RobotConnection.cpp \
    src/robot/RobotStateManager.cpp \
    src/robot/RealtimeController.cpp \
    src/robot/JointStreamer.cpp \
    src/robot/HandController.cpp \
    src/simulation/URDFLoader.cpp \
    src/simulation/RobotRenderer.cpp \
    src/simulation/JointUpdater.cpp \
    src/simulation/STLParser.cpp \
    src/timeline/TimelineModel.cpp \
    src/timeline/SegmentItem.cpp \
    src/timeline/PlaybackEngine.cpp \
    src/timeline/JointLimits.cpp \
    src/timeline/InterpolationEngine.cpp \
    src/utils/MathUtils.cpp \
    src/utils/FileIO.cpp

HEADERS += \
    src/ui/MainWindow.h \
    src/ui/RobotControlPanel.h \
    src/ui/SimulationWidget.h \
    src/ui/TimelineEditor.h \
    src/ui/TimelineCanvas.h \
    src/ui/FlowLayout.h \
    src/ui/SegmentEditDialog.h \
    src/ui/ArmSegmentEditDialog.h \
    src/ui/HandSegmentEditDialog.h \
    src/ui/TimelineSettingsDialog.h \
    src/ui/DialogUtil.h \
    src/ui/MessageCenter.h \
    src/ui/TouchComboBox.h \
    src/robot/RobotConnection.h \
    src/robot/RobotStateManager.h \
    src/robot/RealtimeController.h \
    src/robot/JointStreamer.h \
    src/robot/HandController.h \
    src/simulation/URDFLoader.h \
    src/simulation/RobotRenderer.h \
    src/simulation/JointUpdater.h \
    src/simulation/STLParser.h \
    src/simulation/STLMesh.h \
    src/timeline/TimelineModel.h \
    src/timeline/SegmentItem.h \
    src/timeline/PlaybackEngine.h \
    src/timeline/JointLimits.h \
    src/timeline/InterpolationEngine.h \
    src/utils/MathUtils.h \
    src/utils/FileIO.h

FORMS += \
    src/ui/MainWindow.ui \
    src/ui/RobotControlPanel.ui \
    src/ui/SimulationWidget.ui \
    src/ui/TimelineEditor.ui

RESOURCES += \
    resources.qrc

# 应用图标：桌面端由 main.cpp 的 setWindowIcon 从 qrc 读取，
# 这行让 Linux 打包时也能生成 .desktop 需要的图标文件。
unix:!android {
    icon.files = resources/icons/appicon.png
    icon.path = /usr/share/icons/hicolor/256x256/apps
    INSTALLS += icon
}

INCLUDEPATH += src

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# URDF 解析使用 Qt 自带的 QXmlStreamReader（QtCore），
# 因此不再依赖第三方 tinyxml2，Android/桌面端都无需额外链接。

# Eigen for matrix operations（仅桌面端存在，Android 构建跳过）
unix:!android {
    exists(/usr/include/eigen3): INCLUDEPATH += /usr/include/eigen3
}

android: include(/home/gxh/Android/Sdk/android_openssl/openssl.pri)

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/res/values/styles.xml \
    android/res/mipmap/appicon.png \
    android/res/xml/qtprovider_paths.xml

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

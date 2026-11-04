#ifndef ROBOT_RENDERER_H
#define ROBOT_RENDERER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QMatrix4x4>
#include <QMap>
#include <QHash>
#include <QSet>
#include <QVector3D>
#include <QColor>
#include <QPoint>

#include "simulation/URDFLoader.h"

struct MeshBuffer {
    QOpenGLBuffer vertexBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer indexBuffer{QOpenGLBuffer::IndexBuffer};
    QOpenGLVertexArrayObject vao;
    int indexCount = 0;
    bool isValid = false;
};

/**
 * 参考 xcoredatatool/form/sim/urdfviewer 的实现：
 *  - 着色器管线（#version 330 core），光照在片元着色器里算
 *  - Z 轴向上的球面轨道相机（yaw / pitch / distance / target），
 *    不对模型额外施加任何"世界修正旋转"
 *  - 每个 link 先乘自己的关节链变换，再乘 visual 的 origin(rpy)
 */
class RobotRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit RobotRenderer(QWidget *parent = nullptr);
    ~RobotRenderer() override;

    void setURDFData(const QVector<Link>& links, const QVector<Joint>& joints);

    /** 按"运动关节"（revolute / continuous / prismatic）出现顺序设置角度，单位弧度 */
    void setJointAngles(const QVector<double>& angles);

    /** 按关节名称设置角度，单位弧度（优先于按顺序设置） */
    void setJointAngleByName(const QString& jointName, double angle);

    /** 重新计算全部 link 的世界变换并请求重绘 */
    void updateVisualization();

    /** 复位相机 */
    void resetView();

    void setShowGrid(bool on);
    void setShowAxes(bool on);
    void setWireframe(bool on);

    /** 查询某个 link 的世界位置（用于末端显示等） */
    QVector3D linkPosition(const QString& linkName) const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // 初始化
    void initShaders();
    void initGrid();
    void initAxes();
    void ensureMeshBuffers();
    void createMeshBuffer(const Link& link);
    void cleanupBuffers();

    // 渲染
    void applyPolygonMode(bool wireframe);
    void updateCamera();
    void renderScene();
    void renderModel();
    void renderGrid();
    void renderAxes();
    void renderLink(const Link& link, const QMatrix4x4& transform);

    // 运动学
    void calculateLinkTransformRecursive(const QString& linkName,
                                         const QMatrix4x4& parentTransform,
                                         QMap<QString, QMatrix4x4>& linkTransforms,
                                         QSet<QString>& visited);
    double jointAngle(int jointIndex) const;

    /** 根据当前所有 link 的包围盒自动调整相机 */
    void frameModel();

    QVector<Link> m_links;
    QVector<Joint> m_joints;
    QVector<double> m_jointAngles;        // 按运动关节顺序
    QHash<int, int> m_movableIndexByJoint; // m_joints 下标 -> 运动关节序号
    QHash<QString, double> m_namedAngles;  // 按名称指定的角度
    QMap<QString, QMatrix4x4> m_linkTransforms;

    QMap<QString, MeshBuffer*> m_meshBuffers;
    bool m_buffersReady = false;
    bool m_cameraFramed = false;

    // 着色器
    QOpenGLShaderProgram m_mainShader;
    QOpenGLShaderProgram m_gridShader;
    QOpenGLShaderProgram m_flatShader;

    // 网格 / 坐标轴几何
    QOpenGLVertexArrayObject m_gridVao;
    QOpenGLBuffer m_gridVbo{QOpenGLBuffer::VertexBuffer};
    int m_gridVertexCount = 0;
    QOpenGLVertexArrayObject m_axesVao;
    QOpenGLBuffer m_axesVbo{QOpenGLBuffer::VertexBuffer};
    int m_axesVertexCount = 0;

    // 相机
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;
    QVector3D m_cameraPosition;
    QVector3D m_cameraTarget;
    QVector3D m_cameraUp;
    float m_cameraDistance;
    float m_cameraYaw;
    float m_cameraPitch;
    float m_fieldOfView;
    float m_nearPlane;
    float m_farPlane;

    QPoint m_lastMousePos;
    Qt::MouseButton m_lastMouseButton = Qt::NoButton;

    // 渲染选项
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_wireframe = false;
    QColor m_backgroundColor{32, 32, 32};
};

#endif // ROBOT_RENDERER_H

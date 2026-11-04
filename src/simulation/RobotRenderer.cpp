#include "simulation/RobotRenderer.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QSurfaceFormat>
#include <QtMath>
#include <QDebug>
#include <QSize>

#include <cmath>

// Android 上是 OpenGL ES，桌面端是 OpenGL Core Profile。
// 两者的 GLSL 版本声明、可用常量、glPolygonMode 等 API 都有差异，
// 这里统一用 RP_GLES 做条件编译。
#if defined(Q_OS_ANDROID) || defined(QT_OPENGL_ES_2)
#  define RP_GLES 1
#else
#  define RP_GLES 0
#endif

static inline float D2R(float d) { return d * float(M_PI) / 180.0f; }
static inline float R2D(float r) { return r * 180.0f / float(M_PI); }

/**
 * 给着色器源码补上版本声明。
 *  - 桌面端：GLSL 330 core
 *  - GLES  ：GLSL ES 300（片元着色器必须显式声明浮点精度）
 */
static QByteArray withShaderVersion(const char *body, bool fragment)
{
#if RP_GLES
    QByteArray src = "#version 300 es\n";
    if (fragment) {
        src += "precision highp float;\nprecision highp int;\n";
    }
#else
    Q_UNUSED(fragment);
    QByteArray src = "#version 330 core\n";
#endif
    src += body;
    return src;
}

// ---------------------------------------------------------------------------
// 着色器源码（与 xcoredatatool/resource/shader 保持一致，版本头由上面统一添加）
// ---------------------------------------------------------------------------
static const char *kMainVertexShader = R"(

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

static const char *kMainFragmentShader = R"(

in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 viewPos;

struct Light {
    vec3 position;
    vec3 color;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

uniform Light light;

void main() {
    vec3 ambient = light.ambient * light.color;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * light.color;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), light.shininess);
    vec3 specular = light.specular * spec * light.color;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

static const char *kGridVertexShader = R"(
layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 worldPos;

void main()
{
    vec4 wp = model * vec4(aPos, 1.0);
    worldPos = wp.xyz;
    gl_Position = projection * view * wp;
}
)";

static const char *kGridFragmentShader = R"(
in vec3 worldPos;

uniform vec3 gridColor;
uniform vec3 bgColor;
uniform float majorStep;
uniform float minorStep;
uniform float lineWidth;

out vec4 FragColor;

float gridLine(float coord, float step, float width)
{
    float d = abs(mod(coord + step * 0.5, step) - step * 0.5);
    float w = max(width, fwidth(coord) * 1.5);
    return 1.0 - smoothstep(0.0, w, d);
}

void main()
{
    float minor = max(gridLine(worldPos.x, minorStep, lineWidth),
                      gridLine(worldPos.y, minorStep, lineWidth));
    float major = max(gridLine(worldPos.x, majorStep, lineWidth * 1.6),
                      gridLine(worldPos.y, majorStep, lineWidth * 1.6));

    vec3 color = mix(bgColor, gridColor, minor * 0.45);
    color = mix(color, gridColor, major * 0.85);

    FragColor = vec4(color, 1.0);
}
)";

static const char *kAxesVertexShader = R"(
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 Color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    Color = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char *kAxesFragmentShader = R"(
in vec3 Color;
out vec4 FragColor;

void main() {
    FragColor = vec4(Color, 1.0);
}
)";

// ---------------------------------------------------------------------------

RobotRenderer::RobotRenderer(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_cameraDistance(3.0f)
    , m_cameraYaw(145.0f)
    , m_cameraPitch(20.0f)
    , m_fieldOfView(45.0f)
    , m_nearPlane(0.01f)
    , m_farPlane(100.0f)
{
    setMinimumSize(480, 320);

    // 与参考实现相同的 Z 轴向上相机
    m_cameraUp = QVector3D(0, 0, 1);
    m_cameraTarget = QVector3D(0, 0, 0.3f);
    updateCamera();

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
#if RP_GLES
    // Android 走 OpenGL ES 3.0（对应 GLSL ES 3.00）
    format.setVersion(3, 0);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
#endif
    format.setSamples(4);
    setFormat(format);

    setFocusPolicy(Qt::StrongFocus);
}

RobotRenderer::~RobotRenderer()
{
    makeCurrent();
    cleanupBuffers();
    m_gridVao.destroy();
    m_gridVbo.destroy();
    m_axesVao.destroy();
    m_axesVbo.destroy();
    doneCurrent();
}

// ---------------------------------------------------------------------------
// 数据 / 运动学
// ---------------------------------------------------------------------------

void RobotRenderer::setURDFData(const QVector<Link> &links, const QVector<Joint> &joints)
{
    m_links = links;
    m_joints = joints;

    // 建立"运动关节"顺序索引（与参考实现的 joints_ 遍历顺序一致：即为 URDF 中 joint 出现顺序）
    m_jointAngles.fill(0.0, 0);
    m_movableIndexByJoint.clear();
    m_namedAngles.clear();

    int movableCount = 0;
    for (int i = 0; i < m_joints.size(); ++i) {
        const QString &t = m_joints[i].type;
        if (t == "revolute" || t == "continuous" || t == "prismatic") {
            m_movableIndexByJoint.insert(i, movableCount);
            ++movableCount;
        }
    }
    m_jointAngles.fill(0.0, movableCount);

    m_buffersReady = false;
    m_cameraFramed = false;

    int meshCount = 0;
    for (const Link &l : m_links) {
        if (l.hasMesh) ++meshCount;
    }

    qDebug() << "RobotRenderer::setURDFData  links:" << m_links.size()
             << " joints:" << m_joints.size()
             << " movable:" << movableCount
             << " meshes:" << meshCount;

    updateVisualization();
    update();
}

void RobotRenderer::setJointAngles(const QVector<double> &angles)
{
    m_jointAngles = angles;
    m_namedAngles.clear();
    updateVisualization();
    update();
}

void RobotRenderer::setJointAngleByName(const QString &jointName, double angle)
{
    m_namedAngles.insert(jointName, angle);
    // 不在这里刷新，批量设置后由调用方统一调用 updateVisualization()
}

double RobotRenderer::jointAngle(int jointIndex) const
{
    if (jointIndex < 0 || jointIndex >= m_joints.size()) {
        return 0.0;
    }

    const Joint &joint = m_joints[jointIndex];
    auto named = m_namedAngles.constFind(joint.name);
    if (named != m_namedAngles.constEnd()) {
        return named.value();
    }

    int movableIdx = m_movableIndexByJoint.value(jointIndex, -1);
    if (movableIdx >= 0 && movableIdx < m_jointAngles.size()) {
        return m_jointAngles[movableIdx];
    }
    return joint.currentAngle;
}

void RobotRenderer::updateVisualization()
{
    m_linkTransforms.clear();
    if (m_links.isEmpty()) {
        return;
    }

    // 找根 link（没有任何 joint 把它当 child）
    QSet<QString> childLinks;
    for (const Joint &j : m_joints) {
        childLinks.insert(j.childLink);
    }

    QString rootLink;
    for (const Link &l : m_links) {
        if (!childLinks.contains(l.name)) {
            rootLink = l.name;
            break;
        }
    }
    if (rootLink.isEmpty()) {
        rootLink = m_links.first().name;
    }

    QSet<QString> visited;
    calculateLinkTransformRecursive(rootLink, QMatrix4x4(), m_linkTransforms, visited);
}

void RobotRenderer::calculateLinkTransformRecursive(
    const QString &linkName,
    const QMatrix4x4 &parentTransform,
    QMap<QString, QMatrix4x4> &linkTransforms,
    QSet<QString> &visited)
{
    if (visited.contains(linkName)) {
        return;  // 防止 URDF 写成环
    }
    visited.insert(linkName);

    QMatrix4x4 transform = parentTransform;

    // 找到以该 link 为 child 的关节，套用 origin(xyz,rpy) 与关节运动
    for (int i = 0; i < m_joints.size(); ++i) {
        const Joint &joint = m_joints[i];
        if (joint.childLink != linkName) {
            continue;
        }

        // origin xyz
        transform.translate(joint.origin);

        // origin rpy（URDF 为弧度；旋转顺序 Rz * Ry * Rx）
        if (!joint.originRPY.isNull()) {
            transform.rotate(R2D(joint.originRPY.z()), 0, 0, 1);
            transform.rotate(R2D(joint.originRPY.y()), 0, 1, 0);
            transform.rotate(R2D(joint.originRPY.x()), 1, 0, 0);
        }

        // 关节运动
        if (joint.type == "revolute" || joint.type == "continuous") {
            QVector3D axis = joint.axis;
            if (axis.isNull()) {
                axis = QVector3D(0, 0, 1);
            }
            transform.rotate(R2D(float(jointAngle(i))), axis);
        } else if (joint.type == "prismatic") {
            QVector3D axis = joint.axis;
            if (axis.isNull()) {
                axis = QVector3D(0, 0, 1);
            }
            transform.translate(axis * float(jointAngle(i)));
        }

        break;
    }

    linkTransforms[linkName] = transform;

    for (const Joint &joint : m_joints) {
        if (joint.parentLink == linkName) {
            calculateLinkTransformRecursive(joint.childLink, transform,
                                            linkTransforms, visited);
        }
    }
}

QVector3D RobotRenderer::linkPosition(const QString &linkName) const
{
    if (!m_linkTransforms.contains(linkName)) {
        return QVector3D();
    }
    return m_linkTransforms.value(linkName) * QVector3D(0, 0, 0);
}

// ---------------------------------------------------------------------------
// 相机
// ---------------------------------------------------------------------------

void RobotRenderer::updateCamera()
{
    const float yawRad = D2R(m_cameraYaw);
    const float pitchRad = D2R(m_cameraPitch);

    // Z 轴向上的球面坐标
    m_cameraPosition.setX(m_cameraDistance * std::cos(yawRad) * std::cos(pitchRad));
    m_cameraPosition.setY(m_cameraDistance * std::sin(yawRad) * std::cos(pitchRad));
    m_cameraPosition.setZ(m_cameraDistance * std::sin(pitchRad));
    m_cameraPosition += m_cameraTarget;

    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);
}

void RobotRenderer::frameModel()
{
    bool hasAny = false;
    QVector3D lo;
    QVector3D hi;

    for (const Link &link : m_links) {
        if (!link.hasMesh || !m_linkTransforms.contains(link.name)) {
            continue;
        }
        const QMatrix4x4 &m = m_linkTransforms[link.name];

        const QVector3D &mn = link.mesh.minBound;
        const QVector3D &mx = link.mesh.maxBound;

        for (int c = 0; c < 8; ++c) {
            QVector3D p((c & 1) ? mx.x() : mn.x(),
                        (c & 2) ? mx.y() : mn.y(),
                        (c & 4) ? mx.z() : mn.z());
            QVector3D w = m * p;
            if (!hasAny) {
                lo = hi = w;
                hasAny = true;
            } else {
                lo.setX(qMin(lo.x(), w.x()));
                lo.setY(qMin(lo.y(), w.y()));
                lo.setZ(qMin(lo.z(), w.z()));
                hi.setX(qMax(hi.x(), w.x()));
                hi.setY(qMax(hi.y(), w.y()));
                hi.setZ(qMax(hi.z(), w.z()));
            }
        }
    }

    if (!hasAny) {
        m_cameraTarget = QVector3D(0, 0, 0.3f);
        m_cameraDistance = 3.0f;
        updateCamera();
        return;
    }

    m_cameraTarget = (lo + hi) * 0.5f;
    const float radius = qMax(0.25f, (hi - lo).length() * 0.5f);
    m_cameraDistance = radius / std::tan(D2R(m_fieldOfView) * 0.5f) * 1.35f;
    updateCamera();
    m_cameraFramed = true;
}

void RobotRenderer::resetView()
{
    m_cameraYaw = 145.0f;
    m_cameraPitch = -20.0f;
    m_cameraFramed = false;
    frameModel();
    update();
}

void RobotRenderer::setShowGrid(bool on) { m_showGrid = on; update(); }
void RobotRenderer::setShowAxes(bool on) { m_showAxes = on; update(); }
void RobotRenderer::setWireframe(bool on) { m_wireframe = on; update(); }

// ---------------------------------------------------------------------------
// OpenGL
// ---------------------------------------------------------------------------

void RobotRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
#if !RP_GLES
    // GLES 中多重采样由 EGL 配置里的 samples 决定，没有 GL_MULTISAMPLE 开关
    glEnable(GL_MULTISAMPLE);
#endif
    glDisable(GL_CULL_FACE);

    initShaders();
    initGrid();
    initAxes();

    qDebug() << "RobotRenderer::initializeGL done";
}

void RobotRenderer::initShaders()
{
    m_mainShader.addShaderFromSourceCode(
        QOpenGLShader::Vertex, withShaderVersion(kMainVertexShader, false));
    m_mainShader.addShaderFromSourceCode(
        QOpenGLShader::Fragment, withShaderVersion(kMainFragmentShader, true));
    if (!m_mainShader.link()) {
        qWarning() << "main shader link failed:" << m_mainShader.log();
    }

    m_gridShader.addShaderFromSourceCode(
        QOpenGLShader::Vertex, withShaderVersion(kGridVertexShader, false));
    m_gridShader.addShaderFromSourceCode(
        QOpenGLShader::Fragment, withShaderVersion(kGridFragmentShader, true));
    if (!m_gridShader.link()) {
        qWarning() << "grid shader link failed:" << m_gridShader.log();
    }

    m_flatShader.addShaderFromSourceCode(
        QOpenGLShader::Vertex, withShaderVersion(kAxesVertexShader, false));
    m_flatShader.addShaderFromSourceCode(
        QOpenGLShader::Fragment, withShaderVersion(kAxesFragmentShader, true));
    if (!m_flatShader.link()) {
        qWarning() << "flat shader link failed:" << m_flatShader.log();
    }
}

void RobotRenderer::initGrid()
{
    // 直接建在 XY 平面（z=0），Z 轴向上时即为水平地面
    const int halfLines = 20;
    const float spacing = 0.5f;

    QVector<float> verts;
    verts.reserve((halfLines * 2 + 1) * 4 * 3);

    const float extent = halfLines * spacing;
    for (int i = -halfLines; i <= halfLines; ++i) {
        const float c = i * spacing;
        verts << c << -extent << 0.0f;
        verts << c <<  extent << 0.0f;
        verts << -extent << c << 0.0f;
        verts <<  extent << c << 0.0f;
    }
    m_gridVertexCount = verts.size() / 3;

    m_gridVao.create();
    m_gridVao.bind();
    m_gridVbo.create();
    m_gridVbo.bind();
    m_gridVbo.allocate(verts.constData(), verts.size() * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    m_gridVao.release();
}

void RobotRenderer::initAxes()
{
    // 位置 + 颜色
    const float axesData[] = {
        // X 轴（红）
        0, 0, 0,   1, 0, 0,
        1, 0, 0,   1, 0, 0,
        // Y 轴（绿）
        0, 0, 0,   0, 1, 0,
        0, 1, 0,   0, 1, 0,
        // Z 轴（蓝）
        0, 0, 0,   0, 0.4f, 1,
        0, 0, 1,   0, 0.4f, 1,
    };
    m_axesVertexCount = 6;

    m_axesVao.create();
    m_axesVao.bind();
    m_axesVbo.create();
    m_axesVbo.bind();
    m_axesVbo.allocate(axesData, sizeof(axesData));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    m_axesVao.release();
}

void RobotRenderer::ensureMeshBuffers()
{
    if (m_buffersReady) {
        return;
    }
    cleanupBuffers();

    for (const Link &link : m_links) {
        if (link.hasMesh && link.mesh.isValid && !link.mesh.indices.isEmpty()) {
            createMeshBuffer(link);
        }
    }
    m_buffersReady = true;
    qDebug() << "RobotRenderer::ensureMeshBuffers ->" << m_meshBuffers.size() << "buffers";
}

void RobotRenderer::createMeshBuffer(const Link &link)
{
    const STLMesh &mesh = link.mesh;

    // 交错 (pos.xyz, normal.xyz)
    QVector<float> vertexData;
    vertexData.reserve(mesh.vertices.size() * 2);

    const int vertCount = mesh.vertices.size() / 3;
    for (int i = 0; i < vertCount; ++i) {
        vertexData << mesh.vertices[i * 3]
                   << mesh.vertices[i * 3 + 1]
                   << mesh.vertices[i * 3 + 2];
        if (i < mesh.normals.size() / 3) {
            vertexData << mesh.normals[i * 3]
                       << mesh.normals[i * 3 + 1]
                       << mesh.normals[i * 3 + 2];
        } else {
            vertexData << 0.0f << 0.0f << 1.0f;
        }
    }

    MeshBuffer *buffer = new MeshBuffer();
    buffer->vao.create();
    buffer->vao.bind();

    buffer->vertexBuffer.create();
    buffer->vertexBuffer.bind();
    buffer->vertexBuffer.allocate(vertexData.constData(),
                                  vertexData.size() * sizeof(float));

    buffer->indexBuffer.create();
    buffer->indexBuffer.bind();
    buffer->indexBuffer.allocate(mesh.indices.constData(),
                                 mesh.indices.size() * sizeof(uint));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));

    buffer->indexCount = mesh.indices.size();
    buffer->isValid = true;

    buffer->vao.release();
    buffer->vertexBuffer.release();
    buffer->indexBuffer.release();

    m_meshBuffers.insert(link.name, buffer);
}

void RobotRenderer::cleanupBuffers()
{
    for (auto it = m_meshBuffers.begin(); it != m_meshBuffers.end(); ++it) {
        MeshBuffer *b = it.value();
        if (!b) continue;
        b->vertexBuffer.destroy();
        b->indexBuffer.destroy();
        b->vao.destroy();
        delete b;
    }
    m_meshBuffers.clear();
    m_buffersReady = false;
}

void RobotRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(m_fieldOfView, float(w) / float(qMax(1, h)),
                                   m_nearPlane, m_farPlane);
}

void RobotRenderer::paintGL()
{
    ensureMeshBuffers();

    if (!m_cameraFramed) {
        frameModel();
    }

    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(),
                 m_backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderScene();
}

void RobotRenderer::renderScene()
{
    applyPolygonMode(m_wireframe);

    if (m_showGrid)  renderGrid();
    if (m_showAxes)  renderAxes();

    renderModel();

    applyPolygonMode(false);
}

// GLES 没有 glPolygonMode：在 Android 上退化为"线框不可用"，
// 仅在需要时提示一次，不阻塞渲染。
void RobotRenderer::applyPolygonMode(bool wireframe)
{
#if RP_GLES
    static bool warned = false;
    if (wireframe && !warned) {
        warned = true;
        qWarning() << "RobotRenderer: wireframe mode is not supported on OpenGL ES";
    }
    Q_UNUSED(wireframe);
#else
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
#endif
}

void RobotRenderer::renderGrid()
{
    if (!m_gridShader.isLinked()) {
        return;
    }

    m_gridShader.bind();

    QMatrix4x4 model;
    m_gridShader.setUniformValue("projection", m_projectionMatrix);
    m_gridShader.setUniformValue("view", m_viewMatrix);
    m_gridShader.setUniformValue("model", model);
    m_gridShader.setUniformValue("gridColor", QVector3D(0.42f, 0.45f, 0.5f));
    m_gridShader.setUniformValue("bgColor", QVector3D(0.125f, 0.125f, 0.125f));
    m_gridShader.setUniformValue("majorStep", 1.0f);
    m_gridShader.setUniformValue("minorStep", 0.1f);
    m_gridShader.setUniformValue("lineWidth", 0.006f);

    m_gridVao.bind();
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVao.release();

    m_gridShader.release();
}

void RobotRenderer::renderAxes()
{
    m_flatShader.bind();

    QMatrix4x4 model;
    model.scale(0.45f, 0.45f, 0.45f);

    m_flatShader.setUniformValue("projection", m_projectionMatrix);
    m_flatShader.setUniformValue("view", m_viewMatrix);
    m_flatShader.setUniformValue("model", model);

    m_axesVao.bind();
    glDrawArrays(GL_LINES, 0, m_axesVertexCount);
    m_axesVao.release();

    m_flatShader.release();
}

void RobotRenderer::renderModel()
{
    if (m_links.isEmpty() || m_linkTransforms.isEmpty()) {
        return;
    }

    m_mainShader.bind();
    m_mainShader.setUniformValue("projection", m_projectionMatrix);
    m_mainShader.setUniformValue("view", m_viewMatrix);
    m_mainShader.setUniformValue("viewPos", m_cameraPosition);

    m_mainShader.setUniformValue("light.position", m_cameraPosition);
    m_mainShader.setUniformValue("light.color", QVector3D(1.0f, 1.0f, 1.0f));
    m_mainShader.setUniformValue("light.ambient", 0.30f);
    m_mainShader.setUniformValue("light.diffuse", 0.75f);
    m_mainShader.setUniformValue("light.specular", 0.40f);
    m_mainShader.setUniformValue("light.shininess", 48.0f);

    for (const Link &link : m_links) {
        if (!link.hasMesh) continue;
        if (!m_linkTransforms.contains(link.name)) continue;
        renderLink(link, m_linkTransforms.value(link.name));
    }

    m_mainShader.release();
}

void RobotRenderer::renderLink(const Link &link, const QMatrix4x4 &transform)
{
    if (!m_meshBuffers.contains(link.name)) {
        return;
    }
    MeshBuffer *buffer = m_meshBuffers.value(link.name);
    if (!buffer || !buffer->isValid) {
        return;
    }

    // link 变换 -> visual origin(xyz, rpy)
    QMatrix4x4 visualTransform = transform;
    visualTransform.translate(link.visualOrigin);
    if (!link.visualRPY.isNull()) {
        visualTransform.rotate(R2D(link.visualRPY.z()), 0, 0, 1);
        visualTransform.rotate(R2D(link.visualRPY.y()), 0, 1, 0);
        visualTransform.rotate(R2D(link.visualRPY.x()), 1, 0, 0);
    }

    m_mainShader.setUniformValue("model", visualTransform);
    m_mainShader.setUniformValue("objectColor", link.color);

    buffer->vao.bind();
    glDrawElements(GL_TRIANGLES, buffer->indexCount, GL_UNSIGNED_INT, nullptr);
    buffer->vao.release();
}

// ---------------------------------------------------------------------------
// 交互
// ---------------------------------------------------------------------------

void RobotRenderer::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    m_lastMouseButton = event->button();
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
    }
}

void RobotRenderer::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint delta = event->pos() - m_lastMousePos;

    if (m_lastMouseButton == Qt::LeftButton) {
        m_cameraYaw += delta.x() * 0.4f;
        m_cameraPitch += delta.y() * 0.4f;
        m_cameraPitch = qBound(-89.0f, m_cameraPitch, 89.0f);
        updateCamera();
        update();
    } else if (m_lastMouseButton == Qt::RightButton) {
        const float panSpeed = 0.0015f * m_cameraDistance;
        const QVector3D forward = (m_cameraTarget - m_cameraPosition).normalized();
        const QVector3D right = QVector3D::crossProduct(m_cameraUp, forward).normalized();
        const QVector3D up = QVector3D::crossProduct(forward, right).normalized();

        m_cameraTarget += right * (-delta.x() * panSpeed);
        m_cameraTarget += up * (delta.y() * panSpeed);
        updateCamera();
        update();
    }

    m_lastMousePos = event->pos();
}

void RobotRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
    m_lastMouseButton = Qt::NoButton;
}

void RobotRenderer::wheelEvent(QWheelEvent *event)
{
    const float steps = event->angleDelta().y() / 120.0f;
    m_cameraDistance *= std::pow(1.1f, -steps);
    m_cameraDistance = qBound(0.15f, m_cameraDistance, 200.0f);
    updateCamera();
    update();
}

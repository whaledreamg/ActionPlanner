#include "simulation/URDFLoader.h"
#include "simulation/STLParser.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QXmlStreamReader>
#include <QStringList>

// 使用 Qt 自带的 QXmlStreamReader 解析 URDF，避免依赖第三方 tinyxml2，
// 这样在桌面端和 Android 端都不需要额外编译/链接 tinyxml2。

namespace {

// 解析 "x y z" / "r p y" 形式、以空白分隔的属性值
QVector3D parseVec3(const QString &text, bool *ok = nullptr)
{
    const QStringList parts = text.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.size() < 3) {
        if (ok) *ok = false;
        return QVector3D();
    }

    bool okX = false, okY = false, okZ = false;
    const float x = parts.at(0).toFloat(&okX);
    const float y = parts.at(1).toFloat(&okY);
    const float z = parts.at(2).toFloat(&okZ);

    if (ok) *ok = (okX && okY && okZ);
    return QVector3D(x, y, z);
}

// 解析 <color rgba="r g b a"/>
// 与原实现保持一致：至少解析出 3 个分量才生效，alpha 忽略
bool parseRGBA(const QString &text, QVector3D &color)
{
    const QStringList parts = text.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (parts.size() < 3) {
        return false;
    }

    bool okR = false, okG = false, okB = false;
    const float r = parts.at(0).toFloat(&okR);
    const float g = parts.at(1).toFloat(&okG);
    const float b = parts.at(2).toFloat(&okB);
    if (!(okR && okG && okB)) {
        return false;
    }

    color = QVector3D(r, g, b);
    return true;
}

// 当前元素为 <link>，读取直到其 EndElement
Link parseLinkElement(QXmlStreamReader &xml, STLParser &stlParser, const QString &baseDir)
{
    Link link;
    link.name = xml.attributes().value(QStringLiteral("name")).toString();

    bool visualParsed = false;

    while (xml.readNextStartElement()) {
        if (xml.name() != QLatin1String("visual") || visualParsed) {
            xml.skipCurrentElement();
            continue;
        }
        visualParsed = true;

        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("origin")) {
                const QXmlStreamAttributes attrs = xml.attributes();
                bool ok = false;

                QVector3D value = parseVec3(attrs.value(QStringLiteral("xyz")).toString(), &ok);
                if (ok) {
                    link.visualOrigin = value;
                }

                value = parseVec3(attrs.value(QStringLiteral("rpy")).toString(), &ok);
                if (ok) {
                    link.visualRPY = value;
                }

                xml.skipCurrentElement();
            } else if (xml.name() == QLatin1String("geometry")) {
                while (xml.readNextStartElement()) {
                    if (xml.name() == QLatin1String("mesh")) {
                        QString meshFile = xml.attributes()
                                               .value(QStringLiteral("filename"))
                                               .toString();
                        // Handle package:// URLs
                        if (meshFile.startsWith(QLatin1String("package://"))) {
                            meshFile.remove(0, int(qstrlen("package://")));
                        }

                        link.meshFile = meshFile;
                        link.meshPath = QDir(baseDir).filePath(meshFile);

                        // 加载STL文件
                        if (QFile::exists(link.meshPath)) {
                            if (stlParser.loadSTL(link.meshPath, link.mesh)) {
                                link.mesh.calculateBounds();
                                link.hasMesh = link.mesh.isValid;
                                qDebug() << "    Success:" << link.mesh.vertices.size() / 3
                                         << "vertices"
                                         << "visualOrigin:" << link.visualOrigin;
                            } else {
                                qWarning() << "    Failed to load STL mesh:" << link.meshPath;
                            }
                        } else {
                            qWarning() << "  STL file not found:" << link.meshPath;
                        }

                        xml.skipCurrentElement();
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            } else if (xml.name() == QLatin1String("material")) {
                // 解析材质颜色
                while (xml.readNextStartElement()) {
                    if (xml.name() == QLatin1String("color")) {
                        QVector3D color;
                        if (parseRGBA(xml.attributes()
                                          .value(QStringLiteral("rgba"))
                                          .toString(),
                                      color)) {
                            link.color = color;
                        }
                        xml.skipCurrentElement();
                    } else {
                        xml.skipCurrentElement();
                    }
                }
            } else {
                xml.skipCurrentElement();
            }
        }
    }

    return link;
}

// 当前元素为 <joint>，读取直到其 EndElement
Joint parseJointElement(QXmlStreamReader &xml)
{
    Joint joint;
    joint.name = xml.attributes().value(QStringLiteral("name")).toString();
    joint.type = xml.attributes().value(QStringLiteral("type")).toString();

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("parent")) {
            joint.parentLink = xml.attributes().value(QStringLiteral("link")).toString();
            xml.skipCurrentElement();
        } else if (xml.name() == QLatin1String("child")) {
            joint.childLink = xml.attributes().value(QStringLiteral("link")).toString();
            xml.skipCurrentElement();
        } else if (xml.name() == QLatin1String("origin")) {
            const QXmlStreamAttributes attrs = xml.attributes();
            bool ok = false;

            QVector3D value = parseVec3(attrs.value(QStringLiteral("xyz")).toString(), &ok);
            if (ok) {
                joint.origin = value;
            }

            value = parseVec3(attrs.value(QStringLiteral("rpy")).toString(), &ok);
            if (ok) {
                joint.originRPY = value;
            }

            xml.skipCurrentElement();
        } else if (xml.name() == QLatin1String("axis")) {
            bool ok = false;
            const QVector3D value =
                parseVec3(xml.attributes().value(QStringLiteral("xyz")).toString(), &ok);
            if (ok) {
                joint.axis = value;
            }
            xml.skipCurrentElement();
        } else if (xml.name() == QLatin1String("limit")) {
            const QXmlStreamAttributes attrs = xml.attributes();

            bool ok = false;
            const double lower = attrs.value(QStringLiteral("lower")).toDouble(&ok);
            if (ok) {
                joint.lowerLimit = lower;
            }

            const double upper = attrs.value(QStringLiteral("upper")).toDouble(&ok);
            if (ok) {
                joint.upperLimit = upper;
            }

            xml.skipCurrentElement();
        } else {
            xml.skipCurrentElement();
        }
    }

    return joint;
}

} // namespace

URDFLoader::URDFLoader() {}

bool URDFLoader::loadURDF(const QString& filePath) {
    m_links.clear();
    m_joints.clear();

    // 支持资源文件和普通文件
    QByteArray xmlData;
    if (filePath.startsWith(":/")) {
        // 从Qt资源文件读取
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open resource file:" << filePath;
            return false;
        }
        xmlData = file.readAll();
        file.close();

        // 提取基础目录
        QFileInfo fileInfo(filePath);
        m_baseDirectory = fileInfo.path(); // 例如 ":/models/model/urdf_dualarm"
    } else {
        // 普通文件系统
        if (!QFile::exists(filePath)) {
            qWarning() << "URDF file not found:" << filePath;
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open file:" << filePath;
            return false;
        }
        xmlData = file.readAll();
        file.close();

        QFileInfo fileInfo(filePath);
        m_baseDirectory = fileInfo.absolutePath();
    }

    qDebug() << "URDFLoader: Loading URDF from" << filePath;
    qDebug() << "URDFLoader: Base directory:" << m_baseDirectory;

    // 解析XML
    QXmlStreamReader xml(xmlData);
    xml.setNamespaceProcessing(false);

    STLParser stlParser;
    bool robotFound = false;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.hasError()) {
            break;
        }
        if (!xml.isStartElement() || xml.name() != QLatin1String("robot")) {
            continue;
        }

        robotFound = true;
        qDebug() << "URDFLoader: Found robot element";

        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("link")) {
                const Link link = parseLinkElement(xml, stlParser, m_baseDirectory);
                m_links.append(link);
                qDebug() << "URDFLoader: Added link:" << link.name
                         << "visualOrigin:" << link.visualOrigin;
            } else if (xml.name() == QLatin1String("joint")) {
                const Joint joint = parseJointElement(xml);
                m_joints.append(joint);
                qDebug() << "URDFLoader: Added joint:" << joint.name
                         << "type:" << joint.type
                         << "origin:" << joint.origin
                         << "originRPY:" << joint.originRPY;
            } else {
                // <material> 等顶层元素暂不使用
                xml.skipCurrentElement();
            }
        }

        break; // 只处理第一个 <robot> 元素
    }

    if (xml.hasError()) {
        qWarning() << "Failed to parse URDF XML:" << xml.errorString()
                   << "at line" << xml.lineNumber();
        m_links.clear();
        m_joints.clear();
        return false;
    }

    if (!robotFound) {
        qWarning() << "No robot element found in URDF";
        m_links.clear();
        m_joints.clear();
        return false;
    }

    qDebug() << "URDFLoader: Total links parsed:" << m_links.size();
    qDebug() << "Loaded URDF:" << m_links.size() << "links," << m_joints.size() << "joints";
    return true;
}

QVector<Link> URDFLoader::getLinks() const {
    return m_links;
}

QVector<Joint> URDFLoader::getJoints() const {
    return m_joints;
}

#include "timeline/JointLimits.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QXmlStreamReader>
#include <QRegularExpression>

namespace {
QString g_urdfPath;
bool g_cached = false;
JointLimitsLoader::Limits g_cache;
}

void JointLimitsLoader::setUrdfPath(const QString& path)
{
    if (g_urdfPath != path) {
        g_urdfPath = path;
        g_cached = false;   // 换了模型就重新读
    }
}

QString JointLimitsLoader::findUrdf()
{
    if (!g_urdfPath.isEmpty() && QFileInfo::exists(g_urdfPath)) {
        return g_urdfPath;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates;
    // 从可执行文件目录逐级往上找，覆盖在 build 子目录里运行的情况
    QDir dir(appDir);
    for (int i = 0; i < 6; ++i) {
        candidates << dir.filePath("model/urdf_dualarm/dualarm.urdf");
        candidates << dir.filePath("urdf_dualarm/dualarm.urdf");
        if (!dir.cdUp()) {
            break;
        }
    }

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return QFileInfo(path).absoluteFilePath();
        }
    }
    return QString();
}

JointLimitsLoader::Limits JointLimitsLoader::load()
{
    if (g_cached) {
        return g_cache;
    }

    Limits result;

    const QString path = findUrdf();
    if (path.isEmpty()) {
        qWarning() << "JointLimitsLoader: 未找到 URDF，使用默认 ±180° 限位";
        g_cached = true;
        g_cache = result;
        return result;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "JointLimitsLoader: 无法打开" << path;
        g_cached = true;
        g_cache = result;
        return result;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement) {
            continue;
        }
        if (xml.name().toString() != QStringLiteral("joint")) {
            continue;
        }

        const QString jointName = xml.attributes().value("name").toString();
        const QString type = xml.attributes().value("type").toString();
        if (type != QStringLiteral("revolute") && type != QStringLiteral("continuous")) {
            continue;
        }

        // 关节名形如 left_j3_joint / right_j5_joint
        QString arm;
        int index = -1;
        if (jointName.startsWith(QStringLiteral("left_j"))) {
            arm = QStringLiteral("left");
        } else if (jointName.startsWith(QStringLiteral("right_j"))) {
            arm = QStringLiteral("right");
        } else {
            // 保险起见，也接受名称里带 left/right 的写法
            const QString lower = jointName.toLower();
            if (lower.contains(QStringLiteral("left"))) {
                arm = QStringLiteral("left");
            } else if (lower.contains(QStringLiteral("right"))) {
                arm = QStringLiteral("right");
            }
        }
        if (arm.isEmpty()) {
            continue;
        }

        const QRegularExpression re(QStringLiteral("_j(\\d)_"));
        const auto m = re.match(jointName);
        if (m.hasMatch()) {
            index = m.captured(1).toInt() - 1;
        }
        if (index < 0 || index > 6) {
            continue;
        }

        // 读 <limit lower upper>
        while (xml.readNextStartElement()) {
            if (xml.name().toString() == QStringLiteral("limit")) {
                const double lo = xml.attributes().value("lower").toDouble();
                const double hi = xml.attributes().value("upper").toDouble();
                if (hi > lo) {
                    JointLimits& target = (arm == QStringLiteral("left"))
                                              ? result.left : result.right;
                    target.lower[index] = lo;
                    target.upper[index] = hi;
                    result.loaded = true;
                }
                xml.skipCurrentElement();
            } else {
                xml.skipCurrentElement();
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "JointLimitsLoader: URDF 解析出错:" << xml.errorString();
    }

    qDebug() << "JointLimitsLoader: 限位已加载，来源" << path << "loaded =" << result.loaded;

    g_cached = true;
    g_cache = result;
    return result;
}

#ifndef URDF_LOADER_H
#define URDF_LOADER_H

#include <QString>
#include <QVector>
#include <QVector3D>
#include "STLMesh.h"

struct Link {
    QString name;
    QString meshFile;
    QVector3D position;
    
    // STL mesh data
    bool hasMesh = false;
    QString meshPath;
    STLMesh mesh;
    
    // Visual元素的origin变换 (相对于link坐标系)
    QVector3D visualOrigin;
    QVector3D visualRPY;  // Roll, Pitch, Yaw (弧度)
    
    // 材质颜色（来自 <material><color rgba="...">）
    QVector3D color{0.78f, 0.80f, 0.85f};
};

struct Joint {
    QString name;
    QString type;
    QString parentLink;
    QString childLink;
    QVector3D origin;      // Joint的位置偏移 (xyz)
    QVector3D originRPY;   // Joint的旋转偏移 (roll, pitch, yaw 弧度)
    QVector3D axis;        // 旋转轴
    double lowerLimit;
    double upperLimit;
    double currentAngle = 0.0;
};

class URDFLoader
{
public:
    URDFLoader();
    bool loadURDF(const QString& filePath);
    QVector<Link> getLinks() const;
    QVector<Joint> getJoints() const;
    QString getBaseDirectory() const { return m_baseDirectory; }

private:
    QVector<Link> m_links;
    QVector<Joint> m_joints;
    QString m_baseDirectory;
};

#endif

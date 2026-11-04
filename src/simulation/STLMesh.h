#ifndef STLMESH_H
#define STLMESH_H

#include <QVector>
#include <QString>
#include <QVector3D>

struct STLMesh {
    QString filename;
    QVector<float> vertices;  // xyz xyz xyz...
    QVector<float> normals;   // nx ny nz nx ny nz...
    QVector<uint> indices;    // 三角形索引
    bool isValid = false;
    
    QVector3D minBound;
    QVector3D maxBound;
    
    void calculateBounds() {
        if (vertices.size() < 3) {
            isValid = false;
            return;
        }
        
        minBound = QVector3D(vertices[0], vertices[1], vertices[2]);
        maxBound = minBound;
        
        for (int i = 0; i < vertices.size(); i += 3) {
            QVector3D v(vertices[i], vertices[i+1], vertices[i+2]);
            minBound.setX(qMin(minBound.x(), v.x()));
            minBound.setY(qMin(minBound.y(), v.y()));
            minBound.setZ(qMin(minBound.z(), v.z()));
            maxBound.setX(qMax(maxBound.x(), v.x()));
            maxBound.setY(qMax(maxBound.y(), v.y()));
            maxBound.setZ(qMax(maxBound.z(), v.z()));
        }
        
        isValid = true;
    }
};

#endif // STLMESH_H

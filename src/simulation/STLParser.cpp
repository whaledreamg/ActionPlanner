#include "STLParser.h"
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QDebug>
#include <QtEndian>
#include <QFileInfo>
#include <cmath>

STLParser::STLParser(QObject *parent) : QObject(parent) {
}

bool STLParser::loadSTL(const QString &filename, STLMesh &mesh) {
    // 检查缓存
    if (meshCache_.contains(filename)) {
        mesh = meshCache_[filename];
        return mesh.isValid;
    }
    
    bool success = autoDetectAndParse(filename, mesh);
    if (success) {
        mesh.filename = filename;
        mesh.calculateBounds();
        meshCache_[filename] = mesh;
    }
    
    return success;
}

bool STLParser::parseBinarySTL(const QString &filename, STLMesh &mesh) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open STL file:" << filename;
        return false;
    }
    
    // 跳过80字节的文件头
    file.seek(80);
    
    // 读取三角形数量（32位无符号整数，小端序）
    QByteArray triangleCountData = file.read(4);
    if (triangleCountData.size() != 4) {
        file.close();
        return false;
    }
    
    uint32_t triangleCount = *reinterpret_cast<const uint32_t*>(triangleCountData.constData());
    triangleCount = qFromLittleEndian(triangleCount);
    
    qDebug() << "Loading binary STL:" << triangleCount << "triangles from" << filename;
    
    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.indices.clear();
    
    uint vertexIndex = 0;
    
    for (uint i = 0; i < triangleCount; ++i) {
        // 读取法线（3个float）
        float normal[3];
        if (file.read((char*)normal, 12) != 12) break;
        
        // 读取三个顶点
        for (int v = 0; v < 3; ++v) {
            float vertex[3];
            if (file.read((char*)vertex, 12) != 12) break;
            
            addVertex(mesh, vertex[0], vertex[1], vertex[2], 
                     normal[0], normal[1], normal[2]);
            mesh.indices.append(vertexIndex++);
        }
        
        // 跳过属性字节计数（2字节）
        file.read(2);
        
        if (file.atEnd()) break;
    }
    
    file.close();
    
    if (mesh.vertices.isEmpty()) {
        qDebug() << "No vertices loaded from STL file";
        return false;
    }
    
    qDebug() << "Loaded" << mesh.vertices.size() / 3 << "vertices";
    return true;
}

bool STLParser::parseAsciiSTL(const QString &filename, STLMesh &mesh) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream in(&file);
    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.indices.clear();
    
    uint vertexIndex = 0;
    bool inFacet = false;
    QVector3D currentNormal;
    QVector<QVector3D> currentVertices;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line.startsWith("facet normal")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 5) {
                currentNormal.setX(parts[2].toFloat());
                currentNormal.setY(parts[3].toFloat());
                currentNormal.setZ(parts[4].toFloat());
            }
            inFacet = true;
            currentVertices.clear();
        } 
        else if (line.startsWith("vertex") && inFacet) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                float x = parts[1].toFloat();
                float y = parts[2].toFloat();
                float z = parts[3].toFloat();
                currentVertices.append(QVector3D(x, y, z));
            }
        } 
        else if (line.startsWith("endfacet")) {
            if (currentVertices.size() == 3) {
                for (const QVector3D &vertex : currentVertices) {
                    addVertex(mesh, vertex.x(), vertex.y(), vertex.z(),
                             currentNormal.x(), currentNormal.y(), currentNormal.z());
                    mesh.indices.append(vertexIndex++);
                }
            }
            inFacet = false;
        }
    }
    
    file.close();
    
    if (mesh.vertices.isEmpty()) {
        qDebug() << "No vertices loaded from ASCII STL file";
        return false;
    }
    
    qDebug() << "Loaded ASCII STL:" << mesh.vertices.size() / 3 << "vertices";
    return true;
}

bool STLParser::autoDetectAndParse(const QString &filename, STLMesh &mesh) {
    if (!QFile::exists(filename)) {
        qDebug() << "STL file does not exist:" << filename;
        return false;
    }
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open STL file:" << filename;
        return false;
    }
    
    // 读取前512字节用于检测格式
    QByteArray header = file.read(512);
    file.close();
    
    bool isBinary = isBinarySTL(header);
    bool success;
    
    if (isBinary) {
        success = parseBinarySTL(filename, mesh);
    } else {
        success = parseAsciiSTL(filename, mesh);
    }
    
    // 如果没有法线，计算法线
    if (success && mesh.normals.isEmpty()) {
        calculateNormals(mesh);
    }
    
    mesh.isValid = success;
    return success;
}

bool STLParser::isBinarySTL(const QByteArray &data) {
    if (data.size() < 84) return true;
    
    // 检查前80个字节是否包含非ASCII字符
    for (int i = 0; i < 80; ++i) {
        unsigned char c = data[i];
        if (c < 32 || c > 126) {
            if (c != 9 && c != 10 && c != 13) {  // 排除制表符、换行、回车
                return true;
            }
        }
    }
    
    // 检查是否包含"solid"关键词
    QString header = QString::fromLatin1(data.left(80)).trimmed().toLower();
    if (header.startsWith("solid")) {
        // 进一步检查：看看后面是否有"facet"
        if (data.contains("facet")) {
            return false;  // ASCII格式
        }
    }
    
    return true;  // 默认二进制
}

void STLParser::addVertex(STLMesh &mesh, float x, float y, float z, 
                          float nx, float ny, float nz) {
    mesh.vertices.append(x);
    mesh.vertices.append(y);
    mesh.vertices.append(z);
    
    mesh.normals.append(nx);
    mesh.normals.append(ny);
    mesh.normals.append(nz);
}

void STLParser::calculateNormals(STLMesh &mesh) {
    if (mesh.vertices.size() < 9 || mesh.indices.size() < 3) return;
    
    // 清除现有法线
    mesh.normals.clear();
    mesh.normals.resize(mesh.vertices.size());
    
    // 初始化法线为零
    for (int i = 0; i < mesh.normals.size(); ++i) {
        mesh.normals[i] = 0.0f;
    }
    
    // 计算每个面的法线并累加到顶点
    for (int i = 0; i < mesh.indices.size(); i += 3) {
        if (i + 2 >= mesh.indices.size()) break;
        
        uint i0 = mesh.indices[i];
        uint i1 = mesh.indices[i + 1];
        uint i2 = mesh.indices[i + 2];
        
        if (i0 * 3 + 2 >= mesh.vertices.size() ||
            i1 * 3 + 2 >= mesh.vertices.size() ||
            i2 * 3 + 2 >= mesh.vertices.size()) {
            continue;
        }
        
        QVector3D v0(mesh.vertices[i0 * 3], mesh.vertices[i0 * 3 + 1], mesh.vertices[i0 * 3 + 2]);
        QVector3D v1(mesh.vertices[i1 * 3], mesh.vertices[i1 * 3 + 1], mesh.vertices[i1 * 3 + 2]);
        QVector3D v2(mesh.vertices[i2 * 3], mesh.vertices[i2 * 3 + 1], mesh.vertices[i2 * 3 + 2]);
        
        QVector3D edge1 = v1 - v0;
        QVector3D edge2 = v2 - v0;
        QVector3D normal = QVector3D::crossProduct(edge1, edge2).normalized();
        
        // 累加到顶点法线
        for (int j = 0; j < 3; ++j) {
            uint idx = mesh.indices[i + j] * 3;
            mesh.normals[idx] += normal.x();
            mesh.normals[idx + 1] += normal.y();
            mesh.normals[idx + 2] += normal.z();
        }
    }
    
    // 归一化所有法线
    for (int i = 0; i < mesh.normals.size(); i += 3) {
        float length = sqrt(mesh.normals[i] * mesh.normals[i] +
                           mesh.normals[i + 1] * mesh.normals[i + 1] +
                           mesh.normals[i + 2] * mesh.normals[i + 2]);
        
        if (length > 0.0001f) {
            mesh.normals[i] /= length;
            mesh.normals[i + 1] /= length;
            mesh.normals[i + 2] /= length;
        } else {
            mesh.normals[i] = 0.0f;
            mesh.normals[i + 1] = 1.0f;
            mesh.normals[i + 2] = 0.0f;
        }
    }
}

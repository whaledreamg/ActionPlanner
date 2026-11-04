#ifndef STLPARSER_H
#define STLPARSER_H

#include <QObject>
#include <QString>
#include <QMap>
#include "STLMesh.h"

class STLParser : public QObject {
    Q_OBJECT
public:
    explicit STLParser(QObject *parent = nullptr);
    
    bool loadSTL(const QString &filename, STLMesh &mesh);
    bool parseBinarySTL(const QString &filename, STLMesh &mesh);
    bool parseAsciiSTL(const QString &filename, STLMesh &mesh);
    bool autoDetectAndParse(const QString &filename, STLMesh &mesh);
    
    static bool isBinarySTL(const QByteArray &data);
    
private:
    void addVertex(STLMesh &mesh, float x, float y, float z, 
                   float nx = 0, float ny = 0, float nz = 0);
    void calculateNormals(STLMesh &mesh);
    
    QMap<QString, STLMesh> meshCache_;
};

#endif // STLPARSER_H

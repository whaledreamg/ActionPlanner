#ifndef FILE_IO_H
#define FILE_IO_H

#include <QString>
#include <QJsonObject>

class FileIO
{
public:
    static bool saveToJSON(const QString& filePath, const QJsonObject& data);
    static QJsonObject loadFromJSON(const QString& filePath);
};

#endif

// プロジェクトシリアライザーヘッダー (.lpdf保存/読み込み)
// Author: Antigravity Assistant

#ifndef PROJECTSERIALIZER_H
#define PROJECTSERIALIZER_H

#include <QString>
#include <QJsonObject>

class ProjectSerializer {
public:
    ProjectSerializer() = default;
    ~ProjectSerializer() = default;

    bool saveProject(const QString &lpdfPath, const QJsonObject &projectData);
    QJsonObject loadProject(const QString &lpdfPath);
};

#endif // PROJECTSERIALIZER_H

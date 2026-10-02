// プロジェクトシリアライザー実装
// Author: Antigravity Assistant

#include "ProjectSerializer.h"
#include <QFile>
#include <QJsonDocument>

bool ProjectSerializer::saveProject(const QString &lpdfPath, const QJsonObject &projectData) {
    QFile file(lpdfPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QJsonDocument doc(projectData);
    file.write(doc.toJson());
    return true;
}

QJsonObject ProjectSerializer::loadProject(const QString &lpdfPath) {
    QFile file(lpdfPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return doc.object();
}

// 画像配置アイテム実装
// Author: Antigravity Assistant

#include "ImageItem.h"

ImageItem::ImageItem() = default;

QImage ImageItem::image() const {
    return m_image;
}

void ImageItem::setImage(const QImage &image) {
    m_image = image;
    setSize(m_image.size());
}

QJsonObject ImageItem::serialize() const {
    QJsonObject json;
    json["type"] = "image";
    json["x"] = position().x();
    json["y"] = position().y();
    json["width"] = size().width();
    json["height"] = size().height();
    return json;
}

void ImageItem::deserialize(const QJsonObject &json) {
    setPosition(QPointF(json["x"].toDouble(), json["y"].toDouble()));
    setSize(QSizeF(json["width"].toDouble(), json["height"].toDouble()));
}

// 矩形・白塗りアイテム実装
// Author: Antigravity Assistant

#include "RectangleItem.h"

RectangleItem::RectangleItem()
    : m_fillColor(Qt::white), m_strokeColor(Qt::transparent), m_strokeWidth(0.0) {}

QColor RectangleItem::fillColor() const {
    return m_fillColor;
}

void RectangleItem::setFillColor(const QColor &color) {
    m_fillColor = color;
}

QColor RectangleItem::strokeColor() const {
    return m_strokeColor;
}

void RectangleItem::setStrokeColor(const QColor &color) {
    m_strokeColor = color;
}

double RectangleItem::strokeWidth() const {
    return m_strokeWidth;
}

void RectangleItem::setStrokeWidth(double width) {
    m_strokeWidth = width;
}

QJsonObject RectangleItem::serialize() const {
    QJsonObject json;
    json["type"] = "rectangle";
    json["x"] = position().x();
    json["y"] = position().y();
    json["width"] = size().width();
    json["height"] = size().height();
    json["fillColor"] = m_fillColor.name();
    json["strokeColor"] = m_strokeColor.name();
    json["strokeWidth"] = m_strokeWidth;
    return json;
}

void RectangleItem::deserialize(const QJsonObject &json) {
    setPosition(QPointF(json["x"].toDouble(), json["y"].toDouble()));
    setSize(QSizeF(json["width"].toDouble(), json["height"].toDouble()));
    m_fillColor = QColor(json["fillColor"].toString());
    m_strokeColor = QColor(json["strokeColor"].toString());
    m_strokeWidth = json["strokeWidth"].toDouble();
}

// マーカーハイライトアイテム実装
// Author: Antigravity Assistant

#include "HighlightItem.h"

HighlightItem::HighlightItem()
    : m_color(QColor(255, 255, 0, 128)) {}

QColor HighlightItem::color() const {
    return m_color;
}

void HighlightItem::setColor(const QColor &color) {
    m_color = color;
}

QJsonObject HighlightItem::serialize() const {
    QJsonObject json;
    json["type"] = "highlight";
    json["x"] = position().x();
    json["y"] = position().y();
    json["width"] = size().width();
    json["height"] = size().height();
    json["color"] = m_color.name();
    json["alpha"] = m_color.alpha();
    return json;
}

void HighlightItem::deserialize(const QJsonObject &json) {
    setPosition(QPointF(json["x"].toDouble(), json["y"].toDouble()));
    setSize(QSizeF(json["width"].toDouble(), json["height"].toDouble()));
    QColor c(json["color"].toString());
    c.setAlpha(json["alpha"].toInt(128));
    m_color = c;
}

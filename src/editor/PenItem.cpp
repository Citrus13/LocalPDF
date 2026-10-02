// フリーハンド描画アイテム実装
// Author: Antigravity Assistant

#include "PenItem.h"

PenItem::PenItem()
    : m_color(Qt::black), m_width(2.0) {}

QPolygonF PenItem::points() const {
    return m_points;
}

void PenItem::addPoint(const QPointF &point) {
    m_points.append(point);
}

QColor PenItem::color() const {
    return m_color;
}

void PenItem::setColor(const QColor &color) {
    m_color = color;
}

double PenItem::width() const {
    return m_width;
}

void PenItem::setWidth(double width) {
    m_width = width;
}

QJsonObject PenItem::serialize() const {
    QJsonObject json;
    json["type"] = "pen";
    json["color"] = m_color.name();
    json["width"] = m_width;
    return json;
}

void PenItem::deserialize(const QJsonObject &json) {
    m_color = QColor(json["color"].toString());
    m_width = json["width"].toDouble();
}

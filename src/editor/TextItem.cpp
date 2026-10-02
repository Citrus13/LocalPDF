// テキスト追加アイテム実装
// Author: Antigravity Assistant

#include "TextItem.h"

TextItem::TextItem()
    : m_text("テキスト"), m_font("sans-serif", 12), m_color(Qt::black) {}

QString TextItem::text() const {
    return m_text;
}

void TextItem::setText(const QString &text) {
    m_text = text;
}

QFont TextItem::font() const {
    return m_font;
}

void TextItem::setFont(const QFont &font) {
    m_font = font;
}

QColor TextItem::color() const {
    return m_color;
}

void TextItem::setColor(const QColor &color) {
    m_color = color;
}

QJsonObject TextItem::serialize() const {
    QJsonObject json;
    json["type"] = "text";
    json["text"] = m_text;
    json["x"] = position().x();
    json["y"] = position().y();
    json["color"] = m_color.name();
    return json;
}

void TextItem::deserialize(const QJsonObject &json) {
    m_text = json["text"].toString();
    setPosition(QPointF(json["x"].toDouble(), json["y"].toDouble()));
    m_color = QColor(json["color"].toString());
}

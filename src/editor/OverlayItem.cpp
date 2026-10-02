// オーバーレイアイテム基底クラス実装
// Author: Antigravity Assistant

#include "OverlayItem.h"

OverlayItem::OverlayItem()
    : m_id(QUuid::createUuid()), m_pageIndex(0), m_position(0, 0),
      m_size(100, 100), m_rotation(0.0), m_opacity(1.0) {}

QUuid OverlayItem::id() const {
    return m_id;
}

int OverlayItem::pageIndex() const {
    return m_pageIndex;
}

void OverlayItem::setPageIndex(int index) {
    m_pageIndex = index;
}

QPointF OverlayItem::position() const {
    return m_position;
}

void OverlayItem::setPosition(const QPointF &pos) {
    m_position = pos;
}

QSizeF OverlayItem::size() const {
    return m_size;
}

void OverlayItem::setSize(const QSizeF &size) {
    m_size = size;
}

double OverlayItem::rotation() const {
    return m_rotation;
}

void OverlayItem::setRotation(double degrees) {
    m_rotation = degrees;
}

double OverlayItem::opacity() const {
    return m_opacity;
}

void OverlayItem::setOpacity(double opacity) {
    m_opacity = opacity;
}

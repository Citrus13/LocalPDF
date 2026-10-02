// マーカーハイライトアイテムヘッダー
// Author: Antigravity Assistant

#ifndef HIGHLIGHTITEM_H
#define HIGHLIGHTITEM_H

#include "OverlayItem.h"
#include <QColor>

class HighlightItem : public OverlayItem {
public:
    HighlightItem();
    ~HighlightItem() override = default;

    QColor color() const;
    void setColor(const QColor &color);

    QJsonObject serialize() const override;
    void deserialize(const QJsonObject &json) override;

private:
    QColor m_color;
};

#endif // HIGHLIGHTITEM_H

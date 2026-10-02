// フリーハンド描画アイテムヘッダー
// Author: Antigravity Assistant

#ifndef PENITEM_H
#define PENITEM_H

#include "OverlayItem.h"
#include <QPolygonF>
#include <QColor>

class PenItem : public OverlayItem {
public:
    PenItem();
    ~PenItem() override = default;

    QPolygonF points() const;
    void addPoint(const QPointF &point);

    QColor color() const;
    void setColor(const QColor &color);

    double width() const;
    void setWidth(double width);

    QJsonObject serialize() const override;
    void deserialize(const QJsonObject &json) override;

private:
    QPolygonF m_points;
    QColor m_color;
    double m_width;
};

#endif // PENITEM_H

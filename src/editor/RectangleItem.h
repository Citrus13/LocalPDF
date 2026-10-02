// 矩形・白塗りアイテムヘッダー
// Author: Antigravity Assistant

#ifndef RECTANGLEITEM_H
#define RECTANGLEITEM_H

#include "OverlayItem.h"
#include <QColor>

class RectangleItem : public OverlayItem {
public:
    RectangleItem();
    ~RectangleItem() override = default;

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    double strokeWidth() const;
    void setStrokeWidth(double width);

    QJsonObject serialize() const override;
    void deserialize(const QJsonObject &json) override;

private:
    QColor m_fillColor;
    QColor m_strokeColor;
    double m_strokeWidth;
};

#endif // RECTANGLEITEM_H

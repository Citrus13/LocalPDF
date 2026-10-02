// オーバーレイアイテム基底クラスヘッダー
// Author: Antigravity Assistant

#ifndef OVERLAYITEM_H
#define OVERLAYITEM_H

#include <QUuid>
#include <QPointF>
#include <QSizeF>
#include <QJsonObject>
#include <QGraphicsItem>

class OverlayItem {
public:
    OverlayItem();
    virtual ~OverlayItem() = default;

    QUuid id() const;
    int pageIndex() const;
    void setPageIndex(int index);

    QPointF position() const;
    void setPosition(const QPointF &pos);

    QSizeF size() const;
    void setSize(const QSizeF &size);

    double rotation() const;
    void setRotation(double degrees);

    double opacity() const;
    void setOpacity(double opacity);

    virtual QJsonObject serialize() const = 0;
    virtual void deserialize(const QJsonObject &json) = 0;

private:
    QUuid m_id;
    int m_pageIndex;
    QPointF m_position;
    QSizeF m_size;
    double m_rotation;
    double m_opacity;
};

#endif // OVERLAYITEM_H

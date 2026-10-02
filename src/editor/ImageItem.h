// 画像配置アイテムヘッダー
// Author: Antigravity Assistant

#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include "OverlayItem.h"
#include <QImage>

class ImageItem : public OverlayItem {
public:
    ImageItem();
    ~ImageItem() override = default;

    QImage image() const;
    void setImage(const QImage &image);

    QJsonObject serialize() const override;
    void deserialize(const QJsonObject &json) override;

private:
    QImage m_image;
};

#endif // IMAGEITEM_H

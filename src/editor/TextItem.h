// テキスト追加アイテムヘッダー
// Author: Antigravity Assistant

#ifndef TEXTITEM_H
#define TEXTITEM_H

#include "OverlayItem.h"
#include <QString>
#include <QFont>
#include <QColor>

class TextItem : public OverlayItem {
public:
    TextItem();
    ~TextItem() override = default;

    QString text() const;
    void setText(const QString &text);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &color);

    QJsonObject serialize() const override;
    void deserialize(const QJsonObject &json) override;

private:
    QString m_text;
    QFont m_font;
    QColor m_color;
};

#endif // TEXTITEM_H

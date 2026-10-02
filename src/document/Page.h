// ページモデルヘッダー
// Author: Antigravity Assistant

#ifndef PAGE_H
#define PAGE_H

#include <QString>
#include <QSize>
#include <QImage>
#include <QColor>
#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QList>

enum class AnnotationType {
    Text,
    Pen,
    Highlight,
    Rectangle,
    Whiteout,
    Image
};

struct PageAnnotation {
    AnnotationType type;
    QRectF rect;              // 配置矩形 (Text, Rect, Whiteout, Image)
    QPolygonF points;         // 描画点列 (Pen, Highlight)
    QString text;             // 文字列 (Text)
    QFont font;               // フォント (Text)
    QColor color;             // 描画色
    QColor fillColor;         // 塗りつぶし色 (Rect)
    double strokeWidth;       // 線幅
    double opacity;           // 不透明度
    double rotation;          // オブジェクトの回転角度（度）
    QImage image;             // 画像 (Image)
};

class Page {
public:
    explicit Page(const QString &sourcePath = QString(), int sourcePageIndex = 0, const QSize &size = QSize(595, 842), bool isImage = false);
    ~Page() = default;

    QString sourcePath() const;
    void setSourcePath(const QString &path);

    int sourcePageIndex() const;
    void setSourcePageIndex(int index);

    bool isImagePage() const;
    void setIsImagePage(bool isImage);

    int rotation() const;
    void setRotation(int degrees);
    void rotateClockwise();
    void rotateCounterClockwise();

    QSize size() const;
    void setSize(const QSize &size);

    const QList<PageAnnotation>& annotations() const;
    QList<PageAnnotation>& annotations();
    void addAnnotation(const PageAnnotation &ann);
    void clearAnnotations();

    QImage render(const QSize &targetSize) const;

private:
    QString m_sourcePath;
    int m_sourcePageIndex;
    bool m_isImagePage;
    int m_rotation;
    QSize m_size;
    QList<PageAnnotation> m_annotations;
};

#endif // PAGE_H

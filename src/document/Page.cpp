// ページモデル実装
// Author: Antigravity Assistant

#include "Page.h"
#include <QPdfDocument>
#include <QPainter>
#include <QTransform>

Page::Page(const QString &sourcePath, int sourcePageIndex, const QSize &size, bool isImage)
    : m_sourcePath(sourcePath),
      m_sourcePageIndex(sourcePageIndex),
      m_isImagePage(isImage),
      m_rotation(0),
      m_size(size) {}

QString Page::sourcePath() const {
    return m_sourcePath;
}

void Page::setSourcePath(const QString &path) {
    m_sourcePath = path;
}

int Page::sourcePageIndex() const {
    return m_sourcePageIndex;
}

void Page::setSourcePageIndex(int index) {
    m_sourcePageIndex = index;
}

bool Page::isImagePage() const {
    return m_isImagePage;
}

void Page::setIsImagePage(bool isImage) {
    m_isImagePage = isImage;
}

int Page::rotation() const {
    return m_rotation;
}

void Page::setRotation(int degrees) {
    int rot = degrees % 360;
    if (rot < 0) {
        rot += 360;
    }
    m_rotation = rot;
}

void Page::rotateClockwise() {
    setRotation(m_rotation + 90);
}

void Page::rotateCounterClockwise() {
    setRotation(m_rotation - 90);
}

QSize Page::size() const {
    return m_size;
}

void Page::setSize(const QSize &size) {
    m_size = size;
}

const QList<PageAnnotation>& Page::annotations() const {
    return m_annotations;
}

QList<PageAnnotation>& Page::annotations() {
    return m_annotations;
}

void Page::addAnnotation(const PageAnnotation &ann) {
    m_annotations.append(ann);
}

void Page::clearAnnotations() {
    m_annotations.clear();
}

QImage Page::render(const QSize &targetSize) const {
    QImage result;
    QSize baseSize = m_size;
    if (baseSize.isEmpty()) {
        baseSize = QSize(595, 842);
    }

    // 元ページの縦横比を維持した未回転サイズを計算
    double scale = 1.0;
    if (m_rotation == 90 || m_rotation == 270) {
        scale = static_cast<double>(targetSize.width()) / baseSize.height();
    } else {
        scale = static_cast<double>(targetSize.width()) / baseSize.width();
    }
    if (scale <= 0.0) scale = 1.0;

    QSize unrotatedSize = baseSize * scale;

    if (m_isImagePage) {
        QImage srcImg(m_sourcePath);
        if (!srcImg.isNull()) {
            result = srcImg.scaled(unrotatedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    } else {
        QPdfDocument doc;
        if (doc.load(m_sourcePath) == QPdfDocument::Error::None) {
            result = doc.render(m_sourcePageIndex, unrotatedSize);
        }
    }

    if (result.isNull()) {
        result = QImage(unrotatedSize, QImage::Format_ARGB32_Premultiplied);
        result.fill(Qt::white);
    }

    // 注釈のオーバーレイ描画（未回転の座標系で描画）
    if (!m_annotations.isEmpty()) {
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        double scaleX = static_cast<double>(result.width()) / baseSize.width();
        double scaleY = static_cast<double>(result.height()) / baseSize.height();
        painter.scale(scaleX, scaleY);

        for (const auto &ann : m_annotations) {
            painter.save();
            painter.setOpacity(ann.opacity);

            switch (ann.type) {
            case AnnotationType::Text: {
                painter.setFont(ann.font);
                painter.setPen(ann.color);
                painter.drawText(ann.rect.topLeft() + QPointF(0, ann.font.pointSizeF() * 1.2), ann.text);
                break;
            }
            case AnnotationType::Pen: {
                QPen pen(ann.color, ann.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                painter.setPen(pen);
                for (int i = 1; i < ann.points.size(); ++i) {
                    painter.drawLine(ann.points[i - 1], ann.points[i]);
                }
                break;
            }
            case AnnotationType::Highlight: {
                QColor hl = ann.color;
                hl.setAlphaF(0.4);
                QPen pen(hl, ann.strokeWidth * 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                painter.setPen(pen);
                for (int i = 1; i < ann.points.size(); ++i) {
                    painter.drawLine(ann.points[i - 1], ann.points[i]);
                }
                break;
            }
            case AnnotationType::Rectangle: {
                QPen pen(ann.color, ann.strokeWidth);
                painter.setPen(pen);
                painter.setBrush(ann.fillColor.isValid() ? ann.fillColor : Qt::NoBrush);
                painter.drawRect(ann.rect);
                break;
            }
            case AnnotationType::Whiteout: {
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::white);
                painter.drawRect(ann.rect);
                break;
            }
            case AnnotationType::Image: {
                if (!ann.image.isNull()) {
                    painter.drawImage(ann.rect, ann.image);
                }
                break;
            }
            }
            painter.restore();
        }
    }

    // 最後に指定角度回転
    if (m_rotation != 0) {
        QTransform trans;
        trans.rotate(m_rotation);
        result = result.transformed(trans, Qt::SmoothTransformation);
    }
    return result;
}

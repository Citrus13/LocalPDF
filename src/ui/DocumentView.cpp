// ドキュメント表示ビュー実装
// Author: Antigravity Assistant

#include "DocumentView.h"
#include "../document/Page.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QPdfDocument>
#include <QTransform>

DocumentView::DocumentView(QWidget *parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this)),
      m_pdfPageItem(nullptr),
      m_currentPage(nullptr),
      m_currentToolMode(0),
      m_zoomLevel(1.0),
      m_color(Qt::black),
      m_strokeWidth(2),
      m_opacity(1.0),
      m_fontSize(14),
      m_isDrawing(false),
      m_currentPathItem(nullptr),
      m_currentRectItem(nullptr),
      m_spacePressed(false),
      m_middleButtonPressed(false) {

    setScene(m_scene);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::RubberBandDrag);

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &DocumentView::onSelectionChanged);
}

void DocumentView::loadPage(Page *page, double zoomFactor) {
    if (m_currentPage && m_currentPage != page) {
        saveCurrentAnnotations();
    }

    m_currentPage = page;
    m_zoomLevel = zoomFactor;
    renderCurrentPage();
    restoreAnnotations();
}

void DocumentView::saveCurrentAnnotations() {
    if (!m_currentPage) return;

    m_currentPage->clearAnnotations();
    double scale = m_zoomLevel > 0 ? m_zoomLevel : 1.0;

    const auto items = m_scene->items(Qt::AscendingOrder);
    for (QGraphicsItem *item : items) {
        if (item == m_pdfPageItem) continue;

        PageAnnotation ann;
        ann.opacity = item->opacity();

        if (auto *textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            ann.type = AnnotationType::Text;
            ann.text = textItem->toPlainText();
            ann.font = textItem->font();
            ann.color = textItem->defaultTextColor();
            ann.rect = QRectF(textItem->pos().x() / scale, textItem->pos().y() / scale,
                              textItem->boundingRect().width() / scale, textItem->boundingRect().height() / scale);
            m_currentPage->addAnnotation(ann);
        } else if (auto *pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
            QPainterPath path = pathItem->path();
            ann.color = pathItem->pen().color();
            ann.strokeWidth = pathItem->pen().widthF() / scale;

            if (pathItem->data(0).toString() == "highlight") {
                ann.type = AnnotationType::Highlight;
            } else {
                ann.type = AnnotationType::Pen;
            }

            QPolygonF poly;
            for (int i = 0; i < path.elementCount(); ++i) {
                const auto el = path.elementAt(i);
                poly.append(QPointF((pathItem->pos().x() + el.x) / scale, (pathItem->pos().y() + el.y) / scale));
            }
            ann.points = poly;
            m_currentPage->addAnnotation(ann);
        } else if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(item)) {
            QRectF r = rectItem->rect();
            ann.rect = QRectF((rectItem->pos().x() + r.x()) / scale, (rectItem->pos().y() + r.y()) / scale,
                              r.width() / scale, r.height() / scale);

            if (rectItem->data(0).toString() == "whiteout") {
                ann.type = AnnotationType::Whiteout;
            } else {
                ann.type = AnnotationType::Rectangle;
                ann.color = rectItem->pen().color();
                ann.strokeWidth = rectItem->pen().widthF() / scale;
                ann.fillColor = rectItem->brush().color();
            }
            m_currentPage->addAnnotation(ann);
        } else if (auto *pixItem = dynamic_cast<QGraphicsPixmapItem*>(item)) {
            ann.type = AnnotationType::Image;
            ann.image = pixItem->pixmap().toImage();
            ann.rect = QRectF(pixItem->pos().x() / scale, pixItem->pos().y() / scale,
                              pixItem->pixmap().width() / scale, pixItem->pixmap().height() / scale);
            m_currentPage->addAnnotation(ann);
        }
    }
}

void DocumentView::restoreAnnotations() {
    if (!m_currentPage) return;
    double scale = m_zoomLevel > 0 ? m_zoomLevel : 1.0;

    for (const auto &ann : m_currentPage->annotations()) {
        switch (ann.type) {
        case AnnotationType::Text: {
            auto *item = m_scene->addText(ann.text);
            item->setFont(ann.font);
            item->setDefaultTextColor(ann.color);
            item->setPos(ann.rect.topLeft() * scale);
            item->setOpacity(ann.opacity);
            item->setZValue(5);
            item->setTextInteractionFlags(Qt::TextEditorInteraction);
            item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
            break;
        }
        case AnnotationType::Pen: {
            if (ann.points.size() > 0) {
                QPainterPath path(ann.points[0] * scale);
                for (int i = 1; i < ann.points.size(); ++i) {
                    path.lineTo(ann.points[i] * scale);
                }
                QPen pen(ann.color, ann.strokeWidth * scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                auto *item = m_scene->addPath(path, pen);
                item->setOpacity(ann.opacity);
                item->setZValue(3);
                item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            }
            break;
        }
        case AnnotationType::Highlight: {
            if (ann.points.size() > 0) {
                QPainterPath path(ann.points[0] * scale);
                for (int i = 1; i < ann.points.size(); ++i) {
                    path.lineTo(ann.points[i] * scale);
                }
                QPen pen(ann.color, ann.strokeWidth * scale * 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                auto *item = m_scene->addPath(path, pen);
                item->setData(0, "highlight");
                item->setOpacity(ann.opacity > 0 ? ann.opacity : 0.5);
                item->setZValue(3);
                item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            }
            break;
        }
        case AnnotationType::Rectangle: {
            QRectF r(0, 0, ann.rect.width() * scale, ann.rect.height() * scale);
            QPen pen(ann.color, ann.strokeWidth * scale);
            QBrush brush = ann.fillColor.isValid() ? QBrush(ann.fillColor) : QBrush(Qt::transparent);
            auto *item = m_scene->addRect(r, pen, brush);
            item->setPos(ann.rect.topLeft() * scale);
            item->setOpacity(ann.opacity);
            item->setZValue(2);
            item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            break;
        }
        case AnnotationType::Whiteout: {
            QRectF r(0, 0, ann.rect.width() * scale, ann.rect.height() * scale);
            QPen pen(Qt::white, 1);
            QBrush brush(Qt::white);
            auto *item = m_scene->addRect(r, pen, brush);
            item->setData(0, "whiteout");
            item->setPos(ann.rect.topLeft() * scale);
            item->setZValue(2);
            item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            break;
        }
        case AnnotationType::Image: {
            if (!ann.image.isNull()) {
                QPixmap pix = QPixmap::fromImage(ann.image).scaled(
                    (ann.rect.size() * scale).toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                auto *item = m_scene->addPixmap(pix);
                item->setPos(ann.rect.topLeft() * scale);
                item->setZValue(4);
                item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            }
            break;
        }
        }
    }
}

void DocumentView::setZoomFactor(double zoomFactor) {
    if (m_currentPage) {
        saveCurrentAnnotations();
    }
    m_zoomLevel = zoomFactor;
    renderCurrentPage();
    restoreAnnotations();
}

double DocumentView::zoomFactor() const {
    return m_zoomLevel;
}

void DocumentView::renderCurrentPage() {
    m_scene->clear();
    m_pdfPageItem = nullptr;
    m_currentPathItem = nullptr;
    m_currentRectItem = nullptr;
    m_isDrawing = false;

    if (!m_currentPage) return;

    QSize pSize = m_currentPage->size() * m_zoomLevel;
    if (m_currentPage->rotation() == 90 || m_currentPage->rotation() == 270) {
        pSize.transpose();
    }

    // 下絵（元PDFまたは画像）のレンダリング
    QImage img;
    if (m_currentPage->isImagePage()) {
        QImage srcImg(m_currentPage->sourcePath());
        if (!srcImg.isNull()) {
            img = srcImg.scaled(pSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    } else {
        QPdfDocument doc;
        if (doc.load(m_currentPage->sourcePath()) == QPdfDocument::Error::None) {
            img = doc.render(m_currentPage->sourcePageIndex(), pSize);
        }
    }

    if (img.isNull()) {
        img = QImage(pSize, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::white);
    }

    if (m_currentPage->rotation() != 0) {
        QTransform trans;
        trans.rotate(m_currentPage->rotation());
        img = img.transformed(trans, Qt::SmoothTransformation);
    }

    QPixmap pix = QPixmap::fromImage(img);
    m_pdfPageItem = m_scene->addPixmap(pix);
    m_pdfPageItem->setZValue(0);
    m_pdfPageItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

    m_scene->setSceneRect(0, 0, pix.width(), pix.height());
}

void DocumentView::setToolMode(int mode) {
    m_currentToolMode = mode;
    if (mode == 0) {
        setDragMode(QGraphicsView::RubberBandDrag);
        setCursor(Qt::ArrowCursor);
    } else if (mode == 7) { // 手のひら
        setDragMode(QGraphicsView::ScrollHandDrag);
    } else {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
    }
}

int DocumentView::toolMode() const {
    return m_currentToolMode;
}

void DocumentView::setCurrentColor(const QColor &color) {
    m_color = color;
    // 選択中アイテムのリアルタイム更新
    for (auto *item : m_scene->selectedItems()) {
        if (auto *textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            textItem->setDefaultTextColor(color);
        } else if (auto *pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
            QPen p = pathItem->pen();
            p.setColor(color);
            pathItem->setPen(p);
        } else if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(item)) {
            if (rectItem->data(0).toString() != "whiteout") {
                QPen p = rectItem->pen();
                p.setColor(color);
                rectItem->setPen(p);
            }
        }
    }
}

void DocumentView::setStrokeWidth(int width) {
    m_strokeWidth = width;
    for (auto *item : m_scene->selectedItems()) {
        if (auto *pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
            QPen p = pathItem->pen();
            p.setWidthF(width);
            pathItem->setPen(p);
        } else if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(item)) {
            if (rectItem->data(0).toString() != "whiteout") {
                QPen p = rectItem->pen();
                p.setWidthF(width);
                rectItem->setPen(p);
            }
        }
    }
}

void DocumentView::setItemOpacity(double opacity) {
    m_opacity = opacity;
    for (auto *item : m_scene->selectedItems()) {
        if (item != m_pdfPageItem) {
            item->setOpacity(opacity);
        }
    }
}

void DocumentView::setFontSize(int size) {
    m_fontSize = size;
    for (auto *item : m_scene->selectedItems()) {
        if (auto *textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            QFont f = textItem->font();
            f.setPointSize(size);
            textItem->setFont(f);
        }
    }
}

void DocumentView::onSelectionChanged() {
    auto selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;

    QGraphicsItem *item = selected.first();
    if (item == m_pdfPageItem) return;

    QColor col = m_color;
    int width = m_strokeWidth;
    double op = item->opacity();
    int fontSz = m_fontSize;

    if (auto *textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
        col = textItem->defaultTextColor();
        fontSz = textItem->font().pointSize();
    } else if (auto *pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
        col = pathItem->pen().color();
        width = static_cast<int>(pathItem->pen().widthF());
    } else if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(item)) {
        col = rectItem->pen().color();
        width = static_cast<int>(rectItem->pen().widthF());
    }

    emit itemSelected(col, width, op, fontSz);
}

void DocumentView::addImageFromClipboard(const QImage &image) {
    if (image.isNull()) return;

    QPixmap pix = QPixmap::fromImage(image);
    auto *item = m_scene->addPixmap(pix);
    item->setZValue(4);
    item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    item->setPos(50, 50);
}

void DocumentView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = (event->angleDelta().y() > 0) ? 1.15 : 0.85;
        double newZoom = m_zoomLevel * factor;
        if (newZoom >= 0.25 && newZoom <= 5.0) {
            setZoomFactor(newZoom);
            emit zoomChanged(m_zoomLevel);
        }
        event->accept();
    } else {
        int deltaY = event->angleDelta().y();
        QScrollBar *vBar = verticalScrollBar();

        if (deltaY > 0 && (!vBar || vBar->value() <= vBar->minimum())) {
            emit requestPreviousPage();
            event->accept();
            return;
        } else if (deltaY < 0 && (!vBar || vBar->value() >= vBar->maximum())) {
            emit requestNextPage();
            event->accept();
            return;
        }

        QGraphicsView::wheelEvent(event);
    }
}

void DocumentView::mousePressEvent(QMouseEvent *event) {
    // 中ボタンまたはスペースキー押下でのドラッグ移動開始
    if (event->button() == Qt::MiddleButton || m_spacePressed || m_currentToolMode == 7) {
        m_middleButtonPressed = true;
        m_panLastPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    if (m_currentToolMode == 1) { // テキスト
        auto *item = m_scene->addText("テキスト入力");
        QFont f = item->font();
        f.setPointSize(m_fontSize);
        item->setFont(f);
        item->setDefaultTextColor(m_color);
        item->setPos(scenePos);
        item->setZValue(5);
        item->setTextInteractionFlags(Qt::TextEditorInteraction);
        item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
        item->setFocus();
        return;
    } else if (m_currentToolMode == 2) { // 蛍光ペン
        m_isDrawing = true;
        m_drawStartPos = scenePos;
        m_currentPath = QPainterPath(scenePos);

        QColor hlColor = (m_color == Qt::black) ? QColor(255, 235, 59) : m_color;
        QPen pen(hlColor, m_strokeWidth * 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        m_currentPathItem = m_scene->addPath(m_currentPath, pen);
        m_currentPathItem->setData(0, "highlight");
        m_currentPathItem->setOpacity(0.5);
        m_currentPathItem->setZValue(3);
        m_currentPathItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        return;
    } else if (m_currentToolMode == 3) { // ペン
        m_isDrawing = true;
        m_drawStartPos = scenePos;
        m_currentPath = QPainterPath(scenePos);

        QPen pen(m_color, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        m_currentPathItem = m_scene->addPath(m_currentPath, pen);
        m_currentPathItem->setOpacity(m_opacity);
        m_currentPathItem->setZValue(3);
        m_currentPathItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        return;
    } else if (m_currentToolMode == 4) { // 矩形
        m_isDrawing = true;
        m_drawStartPos = scenePos;

        QPen pen(m_color, m_strokeWidth);
        m_currentRectItem = m_scene->addRect(QRectF(scenePos, QSizeF(0, 0)), pen, QBrush(Qt::transparent));
        m_currentRectItem->setOpacity(m_opacity);
        m_currentRectItem->setZValue(2);
        m_currentRectItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        return;
    } else if (m_currentToolMode == 5) { // 白塗り
        m_isDrawing = true;
        m_drawStartPos = scenePos;

        QPen pen(Qt::white, 1);
        QBrush brush(Qt::white);
        m_currentRectItem = m_scene->addRect(QRectF(scenePos, QSizeF(0, 0)), pen, brush);
        m_currentRectItem->setData(0, "whiteout");
        m_currentRectItem->setZValue(2);
        m_currentRectItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void DocumentView::mouseMoveEvent(QMouseEvent *event) {
    if (m_middleButtonPressed) {
        QPoint delta = event->pos() - m_panLastPos;
        m_panLastPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    if (!m_isDrawing) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    if (m_currentToolMode == 2 || m_currentToolMode == 3) {
        if (m_currentPathItem) {
            m_currentPath.lineTo(scenePos);
            m_currentPathItem->setPath(m_currentPath);
        }
    } else if (m_currentToolMode == 4 || m_currentToolMode == 5) {
        if (m_currentRectItem) {
            QRectF rect(m_drawStartPos, scenePos);
            m_currentRectItem->setRect(QRectF(0, 0, rect.normalized().width(), rect.normalized().height()));
            m_currentRectItem->setPos(rect.normalized().topLeft());
        }
    }
}

void DocumentView::mouseReleaseEvent(QMouseEvent *event) {
    if (m_middleButtonPressed) {
        m_middleButtonPressed = false;
        if (m_currentToolMode == 7 || m_spacePressed) {
            setCursor(Qt::OpenHandCursor);
        } else if (m_currentToolMode == 0) {
            setCursor(Qt::ArrowCursor);
        } else {
            setCursor(Qt::CrossCursor);
        }
        event->accept();
        return;
    }

    if (m_isDrawing) {
        m_isDrawing = false;
        m_currentPathItem = nullptr;
        m_currentRectItem = nullptr;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void DocumentView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        auto selected = m_scene->selectedItems();
        for (auto *item : selected) {
            if (item != m_pdfPageItem) {
                m_scene->removeItem(item);
                delete item;
            }
        }
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void DocumentView::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = false;
        if (m_currentToolMode == 7) {
            setCursor(Qt::OpenHandCursor);
        } else if (m_currentToolMode == 0) {
            setCursor(Qt::ArrowCursor);
        } else {
            setCursor(Qt::CrossCursor);
        }
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}

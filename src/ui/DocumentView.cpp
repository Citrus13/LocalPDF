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
#include <QUndoCommand>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDialog>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPainter>

// --- Undo / Redo コマンド定義 ---

class AddItemCommand : public QUndoCommand {
public:
    AddItemCommand(QGraphicsScene *scene, QGraphicsItem *item, QUndoCommand *parent = nullptr)
        : QUndoCommand("アイテム追加", parent), m_scene(scene), m_item(item) {}

    void undo() override {
        if (m_scene && m_item) {
            m_scene->removeItem(m_item);
        }
    }

    void redo() override {
        if (m_scene && m_item && m_item->scene() != m_scene) {
            m_scene->addItem(m_item);
        }
    }

private:
    QGraphicsScene *m_scene;
    QGraphicsItem *m_item;
};

class RemoveItemCommand : public QUndoCommand {
public:
    RemoveItemCommand(QGraphicsScene *scene, QGraphicsItem *item, QUndoCommand *parent = nullptr)
        : QUndoCommand("アイテム削除", parent), m_scene(scene), m_item(item) {}

    void undo() override {
        if (m_scene && m_item && m_item->scene() != m_scene) {
            m_scene->addItem(m_item);
        }
    }

    void redo() override {
        if (m_scene && m_item) {
            m_scene->removeItem(m_item);
        }
    }

private:
    QGraphicsScene *m_scene;
    QGraphicsItem *m_item;
};

// --- DocumentView 実装 ---

DocumentView::DocumentView(QWidget *parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this)),
      m_pdfPageItem(nullptr),
      m_currentPage(nullptr),
      m_undoStack(nullptr),
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
    setAcceptDrops(true);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    if (viewport()) {
        viewport()->setAttribute(Qt::WA_InputMethodEnabled, true);
    }

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &DocumentView::onSelectionChanged);
}

void DocumentView::setUndoStack(QUndoStack *stack) {
    m_undoStack = stack;
}

QUndoStack* DocumentView::undoStack() const {
    return m_undoStack;
}

void DocumentView::loadPage(Page *page, double zoomFactor) {
    if (m_currentPage && m_currentPage != page) {
        saveCurrentAnnotations();
    }

    m_currentPage = page;
    m_zoomLevel = zoomFactor;
    renderCurrentPage();
    restoreAnnotations();
    emit pageContentChanged();
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
        ann.rotation = item->rotation();

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
            double itemSc = pixItem->scale();
            if (itemSc <= 0.0) itemSc = 1.0;
            ann.rect = QRectF(pixItem->pos().x() / scale, pixItem->pos().y() / scale,
                              (pixItem->pixmap().width() * itemSc) / scale,
                              (pixItem->pixmap().height() * itemSc) / scale);
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
            item->setTransformOriginPoint(item->boundingRect().center());
            if (ann.rotation != 0.0) {
                item->setRotation(ann.rotation);
            }
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
                item->setTransformOriginPoint(item->boundingRect().center());
                if (ann.rotation != 0.0) {
                    item->setRotation(ann.rotation);
                }
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
                item->setTransformOriginPoint(item->boundingRect().center());
                if (ann.rotation != 0.0) {
                    item->setRotation(ann.rotation);
                }
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
            item->setTransformOriginPoint(item->boundingRect().center());
            if (ann.rotation != 0.0) {
                item->setRotation(ann.rotation);
            }
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
            item->setTransformOriginPoint(item->boundingRect().center());
            if (ann.rotation != 0.0) {
                item->setRotation(ann.rotation);
            }
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
                item->setTransformOriginPoint(pix.width() / 2.0, pix.height() / 2.0);
                if (ann.rotation != 0.0) {
                    item->setRotation(ann.rotation);
                }
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

    QSize unrotatedSize = m_currentPage->size() * m_zoomLevel;

    QImage img;
    if (m_currentPage->isImagePage()) {
        QImage srcImg(m_currentPage->sourcePath());
        if (!srcImg.isNull()) {
            img = srcImg.scaled(unrotatedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    } else {
        QPdfDocument doc;
        if (doc.load(m_currentPage->sourcePath()) == QPdfDocument::Error::None) {
            img = doc.render(m_currentPage->sourcePageIndex(), unrotatedSize);
        }
    }

    if (img.isNull()) {
        img = QImage(unrotatedSize, QImage::Format_ARGB32_Premultiplied);
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
    if (mode == 0) { // 選択
        setDragMode(QGraphicsView::RubberBandDrag);
        setCursor(Qt::ArrowCursor);
    } else if (mode == 7) { // 手のひら
        setDragMode(QGraphicsView::ScrollHandDrag);
    } else if (mode == 8) { // 消しゴム
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
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

    double rot = item->rotation();
    double sc = (item->scale() <= 0.0 ? 1.0 : item->scale()) * 100.0;
    emit itemTransformSelected(rot, sc);
}

void DocumentView::addImageFromClipboard(const QImage &image) {
    if (image.isNull()) return;

    QPixmap pix = QPixmap::fromImage(image);
    auto *item = m_scene->addPixmap(pix);
    item->setTransformOriginPoint(pix.width() / 2.0, pix.height() / 2.0);
    item->setZValue(4);
    item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    item->setPos(50, 50);

    if (m_undoStack) {
        m_undoStack->push(new AddItemCommand(m_scene, item));
    }
    emit pageContentChanged();
}

void DocumentView::rotateSelectedItems(double angleDelta) {
    for (auto *item : m_scene->selectedItems()) {
        if (item != m_pdfPageItem) {
            QRectF br = item->boundingRect();
            item->setTransformOriginPoint(br.center());
            item->setRotation(item->rotation() + angleDelta);
        }
    }
    auto sel = m_scene->selectedItems();
    if (!sel.isEmpty() && sel.first() != m_pdfPageItem) {
        emit itemTransformSelected(sel.first()->rotation(), (sel.first()->scale() <= 0.0 ? 1.0 : sel.first()->scale()) * 100.0);
    }
    emit pageContentChanged();
}

void DocumentView::scaleSelectedItems(double scaleFactor) {
    for (auto *item : m_scene->selectedItems()) {
        if (item != m_pdfPageItem) {
            QRectF br = item->boundingRect();
            item->setTransformOriginPoint(br.center());
            double currentScale = item->scale() <= 0.0 ? 1.0 : item->scale();
            item->setScale(currentScale * scaleFactor);
        }
    }
    auto sel = m_scene->selectedItems();
    if (!sel.isEmpty() && sel.first() != m_pdfPageItem) {
        emit itemTransformSelected(sel.first()->rotation(), (sel.first()->scale() <= 0.0 ? 1.0 : sel.first()->scale()) * 100.0);
    }
    emit pageContentChanged();
}

void DocumentView::setSelectedItemsRotation(double degrees) {
    for (auto *item : m_scene->selectedItems()) {
        if (item != m_pdfPageItem) {
            QRectF br = item->boundingRect();
            item->setTransformOriginPoint(br.center());
            item->setRotation(degrees);
        }
    }
    emit pageContentChanged();
}

void DocumentView::setSelectedItemsScale(double scalePercent) {
    double sc = scalePercent / 100.0;
    if (sc <= 0.05) sc = 0.05;
    for (auto *item : m_scene->selectedItems()) {
        if (item != m_pdfPageItem) {
            QRectF br = item->boundingRect();
            item->setTransformOriginPoint(br.center());
            item->setScale(sc);
        }
    }
    emit pageContentChanged();
}

QImage DocumentView::captureCurrentPageImage(int maxDimension) const {
    if (!m_scene) return QImage();

    QRectF sceneRect = m_scene->itemsBoundingRect();
    if (m_pdfPageItem) {
        sceneRect = sceneRect.united(m_pdfPageItem->sceneBoundingRect());
    }
    if (sceneRect.isEmpty() || sceneRect.width() <= 0 || sceneRect.height() <= 0) {
        sceneRect = QRectF(0, 0, 595, 842);
    }

    QSizeF sz = sceneRect.size();
    sz.scale(maxDimension, maxDimension, Qt::KeepAspectRatio);

    QImage img(sz.toSize(), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    m_scene->render(&painter, QRectF(0, 0, sz.width(), sz.height()), sceneRect);
    painter.end();

    return img;
}

void DocumentView::openTextEditorDialog(QGraphicsTextItem *item, const QPointF &pos) {
    bool isNew = (item == nullptr);
    QString initialText = isNew ? "" : item->toPlainText();

    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? "テキストを入力（全角・日本語対応）" : "テキストを編集（全角・日本語対応）");
    dlg.resize(420, 200);

    auto *vbox = new QVBoxLayout(&dlg);
    auto *edit = new QPlainTextEdit(&dlg);
    edit->setPlainText(initialText);
    QFont f("Yu Gothic UI", m_fontSize > 0 ? m_fontSize : 14);
    edit->setFont(f);
    vbox->addWidget(edit);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    vbox->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    edit->setFocus();

    if (dlg.exec() == QDialog::Accepted) {
        QString text = edit->toPlainText();
        if (text.trimmed().isEmpty()) {
            if (!isNew && item) {
                if (m_undoStack) {
                    m_undoStack->push(new RemoveItemCommand(m_scene, item));
                } else {
                    m_scene->removeItem(item);
                    delete item;
                }
                emit pageContentChanged();
            }
            return;
        }

        if (isNew) {
            auto *newItem = m_scene->addText(text);
            newItem->setFont(f);
            newItem->setDefaultTextColor(m_color);
            newItem->setPos(pos);
            newItem->setZValue(5);
            newItem->setTextInteractionFlags(Qt::TextEditorInteraction);
            newItem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
            newItem->setTransformOriginPoint(newItem->boundingRect().center());
            newItem->setSelected(true);

            if (m_undoStack) {
                m_undoStack->push(new AddItemCommand(m_scene, newItem));
            }
        } else {
            item->setPlainText(text);
            item->setTransformOriginPoint(item->boundingRect().center());
        }
        emit pageContentChanged();
    }
}

void DocumentView::deleteSelectedItems() {
    auto selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;

    if (m_undoStack) {
        m_undoStack->beginMacro("選択アイテム削除");
    }
    for (auto *item : selected) {
        if (item != m_pdfPageItem) {
            if (m_undoStack) {
                m_undoStack->push(new RemoveItemCommand(m_scene, item));
            } else {
                m_scene->removeItem(item);
                delete item;
            }
        }
    }
    if (m_undoStack) {
        m_undoStack->endMacro();
    }
    emit pageContentChanged();
}

void DocumentView::copySelectedItems() {
    // 選択中アイテムを記憶
    m_clipboardItems.clear();
    for (auto *item : m_scene->selectedItems()) {
        if (item != m_pdfPageItem) {
            m_clipboardItems.append(item);
        }
    }
}

void DocumentView::pasteItems() {
    if (m_clipboardItems.isEmpty()) return;

    m_scene->clearSelection();
    for (auto *orig : m_clipboardItems) {
        QGraphicsItem *newItem = nullptr;

        if (auto *textItem = dynamic_cast<QGraphicsTextItem*>(orig)) {
            auto *t = m_scene->addText(textItem->toPlainText());
            t->setFont(textItem->font());
            t->setDefaultTextColor(textItem->defaultTextColor());
            t->setPos(textItem->pos() + QPointF(20, 20));
            t->setTextInteractionFlags(Qt::TextEditorInteraction);
            t->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
            t->setZValue(textItem->zValue());
            newItem = t;
        } else if (auto *rectItem = dynamic_cast<QGraphicsRectItem*>(orig)) {
            auto *r = m_scene->addRect(rectItem->rect(), rectItem->pen(), rectItem->brush());
            r->setPos(rectItem->pos() + QPointF(20, 20));
            r->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            r->setZValue(rectItem->zValue());
            r->setData(0, rectItem->data(0));
            newItem = r;
        } else if (auto *pathItem = dynamic_cast<QGraphicsPathItem*>(orig)) {
            auto *p = m_scene->addPath(pathItem->path(), pathItem->pen());
            p->setPos(pathItem->pos() + QPointF(20, 20));
            p->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            p->setZValue(pathItem->zValue());
            p->setOpacity(pathItem->opacity());
            p->setData(0, pathItem->data(0));
            newItem = p;
        } else if (auto *pixItem = dynamic_cast<QGraphicsPixmapItem*>(orig)) {
            auto *px = m_scene->addPixmap(pixItem->pixmap());
            px->setPos(pixItem->pos() + QPointF(20, 20));
            px->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
            px->setZValue(pixItem->zValue());
            newItem = px;
        }

        if (newItem) {
            newItem->setSelected(true);
            if (m_undoStack) {
                m_undoStack->push(new AddItemCommand(m_scene, newItem));
            }
        }
    }
}

void DocumentView::eraseAt(const QPointF &scenePos) {
    QRectF eraseRect(scenePos.x() - 10, scenePos.y() - 10, 20, 20);
    auto hitItems = m_scene->items(eraseRect);
    for (auto *item : hitItems) {
        if (item != m_pdfPageItem && !m_erasedItemsInStroke.contains(item)) {
            m_erasedItemsInStroke.insert(item);
            if (m_undoStack) {
                m_undoStack->push(new RemoveItemCommand(m_scene, item));
            } else {
                m_scene->removeItem(item);
                delete item;
            }
        }
    }
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

    if (m_currentToolMode == 8) { // 消しゴム
        m_isDrawing = true;
        m_erasedItemsInStroke.clear();
        if (m_undoStack) {
            m_undoStack->beginMacro("消しゴム消去");
        }
        eraseAt(scenePos);
        event->accept();
        return;
    } else if (m_currentToolMode == 1) { // テキスト入力（全角・日本語ダイアログ）
        openTextEditorDialog(nullptr, scenePos);
        event->accept();
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

    if (m_currentToolMode == 8) { // 消しゴム
        eraseAt(scenePos);
        return;
    } else if (m_currentToolMode == 2 || m_currentToolMode == 3) {
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

    if (m_currentToolMode == 8 && m_isDrawing) {
        m_isDrawing = false;
        m_erasedItemsInStroke.clear();
        if (m_undoStack) {
            m_undoStack->endMacro();
        }
        emit pageContentChanged();
        event->accept();
        return;
    }

    if (m_isDrawing) {
        m_isDrawing = false;
        if (m_currentPathItem) {
            if (m_undoStack) {
                m_undoStack->push(new AddItemCommand(m_scene, m_currentPathItem));
            }
            m_currentPathItem = nullptr;
        }
        if (m_currentRectItem) {
            if (m_undoStack) {
                m_undoStack->push(new AddItemCommand(m_scene, m_currentRectItem));
            }
            m_currentRectItem = nullptr;
        }
        emit pageContentChanged();
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void DocumentView::mouseDoubleClickEvent(QMouseEvent *event) {
    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem *item = m_scene->itemAt(scenePos, transform());
    if (item && item != m_pdfPageItem) {
        if (auto *textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            openTextEditorDialog(textItem);
            event->accept();
            return;
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void DocumentView::keyPressEvent(QKeyEvent *event) {
    // 0. Undo / Redo（オブジェクト選択状態に関わらず確実に元に戻す）
    if (event->matches(QKeySequence::Undo) || 
        ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Z && !(event->modifiers() & Qt::ShiftModifier))) {
        if (m_undoStack) {
            m_undoStack->undo();
            emit pageContentChanged();
            event->accept();
            return;
        }
    }
    if (event->matches(QKeySequence::Redo) || 
        ((event->modifiers() & Qt::ControlModifier) && (event->key() == Qt::Key_Y || 
         ((event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_Z)))) {
        if (m_undoStack) {
            m_undoStack->redo();
            emit pageContentChanged();
            event->accept();
            return;
        }
    }

    // 1. ESCキーで選択・移動ツールに戻る
    if (event->key() == Qt::Key_Escape) {
        if (m_isDrawing) {
            if (m_currentPathItem) {
                m_scene->removeItem(m_currentPathItem);
                delete m_currentPathItem;
                m_currentPathItem = nullptr;
            }
            if (m_currentRectItem) {
                m_scene->removeItem(m_currentRectItem);
                delete m_currentRectItem;
                m_currentRectItem = nullptr;
            }
            m_isDrawing = false;
        }
        setToolMode(0);
        emit escapeTriggered();
        event->accept();
        return;
    }

    // 1. テキスト編集中なら、文字入力を最優先（Backspaceでアイテム消去しない）
    QGraphicsItem *focus = m_scene->focusItem();
    if (focus) {
        if (auto *textItem = dynamic_cast<QGraphicsTextItem*>(focus)) {
            if (textItem->textInteractionFlags() & Qt::TextEditorInteraction) {
                QGraphicsView::keyPressEvent(event);
                return;
            }
        }
    }

    // 2. スペースキーで一時手のひら
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    // 3. コピペショートカット
    if (event->matches(QKeySequence::Copy)) {
        copySelectedItems();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        pasteItems();
        event->accept();
        return;
    }

    // 4. Delete / Backspace で選択アイテム削除
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelectedItems();
        event->accept();
        return;
    }

    // 5. 選択中オブジェクトのキー操作（拡大縮小・回転）
    if (!m_scene->selectedItems().isEmpty()) {
        if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
            scaleSelectedItems(1.1);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Minus) {
            scaleSelectedItems(0.9);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_BracketLeft) {
            rotateSelectedItems(-15.0);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_BracketRight) {
            rotateSelectedItems(15.0);
            event->accept();
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
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

void DocumentView::contextMenuEvent(QContextMenuEvent *event) {
    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem *item = m_scene->itemAt(scenePos, transform());

    if (item && item != m_pdfPageItem) {
        item->setSelected(true);
        QMenu menu(this);
        QAction *rotCwAct = menu.addAction("↻ 90°右回転");
        QAction *rotCcwAct = menu.addAction("↺ 90°左回転");
        menu.addSeparator();
        QAction *scaleUpAct = menu.addAction("🔍 拡大 (+20%)");
        QAction *scaleDownAct = menu.addAction("🔍 縮小 (-20%)");
        QAction *scaleResetAct = menu.addAction("📐 サイズを100%にリセット");
        menu.addSeparator();
        QAction *delAct = menu.addAction("🗑 このオブジェクトを削除 (Delete)");
        QAction *copyAct = menu.addAction("📋 複製（コピー＆ペースト）");
        menu.addSeparator();
        QAction *frontAct = menu.addAction("▲ 最前面へ移動");
        QAction *backAct = menu.addAction("▼ 最背面へ移動");

        QAction *sel = menu.exec(event->globalPos());
        if (sel == rotCwAct) {
            rotateSelectedItems(90.0);
        } else if (sel == rotCcwAct) {
            rotateSelectedItems(-90.0);
        } else if (sel == scaleUpAct) {
            scaleSelectedItems(1.2);
        } else if (sel == scaleDownAct) {
            scaleSelectedItems(0.8);
        } else if (sel == scaleResetAct) {
            setSelectedItemsScale(100.0);
        } else if (sel == delAct) {
            deleteSelectedItems();
        } else if (sel == copyAct) {
            copySelectedItems();
            pasteItems();
        } else if (sel == frontAct) {
            item->setZValue(item->zValue() + 1);
        } else if (sel == backAct) {
            item->setZValue(qMax(1.0, item->zValue() - 1));
        }
    } else {
        QGraphicsView::contextMenuEvent(event);
    }
}

void DocumentView::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragEnterEvent(event);
    }
}

void DocumentView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragMoveEvent(event);
    }
}

void DocumentView::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString localFile = urls.first().toLocalFile();
            if (!localFile.isEmpty()) {
                emit fileDropped(localFile);
                event->acceptProposedAction();
                return;
            }
        }
    }
    QGraphicsView::dropEvent(event);
}

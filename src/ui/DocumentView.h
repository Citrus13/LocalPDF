// ドキュメント表示ビューヘッダー (QGraphicsView / QGraphicsScene)
// Author: Antigravity Assistant

#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QColor>
#include <QFont>
#include <QString>
#include <QUndoStack>

class Page;

class DocumentView : public QGraphicsView {
    Q_OBJECT
public:
    explicit DocumentView(QWidget *parent = nullptr);
    ~DocumentView() override = default;

    void setUndoStack(QUndoStack *stack);
    QUndoStack* undoStack() const;

    void loadPage(Page *page, double zoomFactor = 1.0);
    void saveCurrentAnnotations();

    void setZoomFactor(double zoomFactor);
    double zoomFactor() const;

    // 0: 選択, 1: Text, 2: Highlight, 3: Pen, 4: Rect, 5: Whiteout, 6: Image, 7: 手のひら, 8: 消しゴム
    void setToolMode(int mode);
    int toolMode() const;

    void setCurrentColor(const QColor &color);
    void setStrokeWidth(int width);
    void setItemOpacity(double opacity);
    void setFontSize(int size);

    void addImageFromClipboard(const QImage &image);

    // クリップボード複製
    void copySelectedItems();
    void pasteItems();
    void deleteSelectedItems();

    // 選択アイテムの変形（拡大縮小・回転）
    void rotateSelectedItems(double angleDelta);
    void scaleSelectedItems(double scaleFactor);
    void setSelectedItemsRotation(double degrees);
    void setSelectedItemsScale(double scalePercent);

signals:
    void zoomChanged(double zoomFactor);
    void requestPreviousPage();
    void requestNextPage();
    void itemSelected(const QColor &color, int strokeWidth, double opacity, int fontSize);
    void itemTransformSelected(double rotation, double scalePercent);
    void escapeTriggered();
    void fileDropped(const QString &filePath);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

    // ファイルドラッグ＆ドロップ対応
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onSelectionChanged();

private:
    void renderCurrentPage();
    void restoreAnnotations();
    void eraseAt(const QPointF &scenePos);

    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pdfPageItem;
    Page *m_currentPage;
    QUndoStack *m_undoStack;
    int m_currentToolMode;
    double m_zoomLevel;

    // 描画設定プロパティ
    QColor m_color;
    int m_strokeWidth;
    double m_opacity;
    int m_fontSize;

    // ドラッグ描画用一時状態
    bool m_isDrawing;
    QPointF m_drawStartPos;
    QPainterPath m_currentPath;
    QGraphicsPathItem *m_currentPathItem;
    QGraphicsRectItem *m_currentRectItem;

    // スペースキーまたは中ボタンによる一時パン操作
    bool m_spacePressed;
    bool m_middleButtonPressed;
    QPoint m_panLastPos;

    // コピーバッファ
    QList<QGraphicsItem*> m_clipboardItems;
};

#endif // DOCUMENTVIEW_H

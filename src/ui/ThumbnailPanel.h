// ページサムネイルパネルヘッダー
// Author: Antigravity Assistant

#ifndef THUMBNAILPANEL_H
#define THUMBNAILPANEL_H

#include <QListWidget>
#include <QString>

class Document;

class ThumbnailPanel : public QListWidget {
    Q_OBJECT
public:
    explicit ThumbnailPanel(QWidget *parent = nullptr);
    ~ThumbnailPanel() override = default;

    void updateThumbnails(const Document &doc);
    void updateSingleThumbnail(int index, const Document &doc);

signals:
    void pageSelected(int pageIndex);
    void pageRotateClockwiseRequested(int pageIndex);
    void pageRotateCounterClockwiseRequested(int pageIndex);
    void pageDeleteRequested(int pageIndex);
    void pageExtractRequested(int pageIndex);
    void insertPdfRequested(int insertIndex);
    void insertImagesRequested(int insertIndex);
    void pageMoved(int fromIndex, int toIndex);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    int m_dragRow;
};

#endif // THUMBNAILPANEL_H

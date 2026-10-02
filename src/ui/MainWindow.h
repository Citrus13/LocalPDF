// メインウィンドウヘッダー
// Author: Antigravity Assistant

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QUndoStack>
#include <memory>
#include "../document/Document.h"

class ThumbnailPanel;
class DocumentView;
class PropertyPanel;
class PagePreviewWidget;
class QToolBar;
class QComboBox;
class QAction;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    void openPdfFile(const QString &filePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onOpenTriggered();
    void onSaveTriggered();
    void onExportTriggered();
    void onPageSelected(int pageIndex);
    void onToolTriggered(int mode);
    void onPasteImage();
    void onFileDropped(const QString &filePath);

    // ページ操作スロット
    void onRotateClockwise();
    void onRotateCounterClockwise();
    void onDeletePage();
    void onExtractPages();
    void onMergePdf();
    void onInsertPdfAt(int insertIndex);
    void onInsertImagesAt(int insertIndex);
    void onExportImages();
    void onMovePageUp();
    void onMovePageDown();
    void onPageMoved(int fromIndex, int toIndex);

    // ページ遷移・ズーム
    void onPreviousPage();
    void onNextPage();
    void onZoomComboChanged(int index);
    void onZoomChanged(double zoomFactor);
    void updatePagePreview();

private:
    void createMenusAndActions();
    void createToolbars();
    void updateCurrentPageDisplay();

    ThumbnailPanel *m_thumbnailPanel;
    DocumentView *m_documentView;
    PropertyPanel *m_propertyPanel;
    PagePreviewWidget *m_previewWidget;

    QToolBar *m_mainToolBar;
    QToolBar *m_pageToolBar;
    QToolBar *m_editToolBar;
    QComboBox *m_zoomCombo;
    QAction *m_selectToolAction;

    std::unique_ptr<Document> m_document;
    int m_currentPageIndex;

    QUndoStack *m_undoStack;
};

#endif // MAINWINDOW_H

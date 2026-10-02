// メインウィンドウ実装
// Author: Antigravity Assistant

#include "MainWindow.h"
#include "ThumbnailPanel.h"
#include "DocumentView.h"
#include "PropertyPanel.h"
#include "../pdf/PdfExporter.h"

#include <QMenuBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QToolBar>
#include <QComboBox>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_thumbnailPanel(new ThumbnailPanel(this)),
      m_documentView(new DocumentView(this)),
      m_propertyPanel(new PropertyPanel(this)),
      m_document(std::make_unique<Document>()),
      m_currentPageIndex(0),
      m_undoStack(new QUndoStack(this)) {

    setCentralWidget(m_documentView);

    // 左ドック: サムネイル
    auto *leftDock = new QDockWidget("ページ一覧", this);
    leftDock->setWidget(m_thumbnailPanel);
    addDockWidget(Qt::LeftDockWidgetArea, leftDock);

    // 右ドック: プロパティ
    auto *rightDock = new QDockWidget("プロパティ", this);
    rightDock->setWidget(m_propertyPanel);
    addDockWidget(Qt::RightDockWidgetArea, rightDock);

    createMenusAndActions();
    createToolbars();

    // サムネイルパネルとのシグナル接続
    connect(m_thumbnailPanel, &ThumbnailPanel::pageSelected, this, &MainWindow::onPageSelected);
    connect(m_thumbnailPanel, &ThumbnailPanel::pageRotateClockwiseRequested, this, &MainWindow::onRotateClockwise);
    connect(m_thumbnailPanel, &ThumbnailPanel::pageRotateCounterClockwiseRequested, this, &MainWindow::onRotateCounterClockwise);
    connect(m_thumbnailPanel, &ThumbnailPanel::pageDeleteRequested, this, &MainWindow::onDeletePage);
    connect(m_thumbnailPanel, &ThumbnailPanel::pageExtractRequested, this, &MainWindow::onExtractPages);
    connect(m_thumbnailPanel, &ThumbnailPanel::insertPdfRequested, this, &MainWindow::onInsertPdfAt);
    connect(m_thumbnailPanel, &ThumbnailPanel::insertImagesRequested, this, &MainWindow::onInsertImagesAt);
    connect(m_thumbnailPanel, &ThumbnailPanel::pageMoved, this, &MainWindow::onPageMoved);

    // ドキュメントビューとのシグナル接続
    connect(m_documentView, &DocumentView::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(m_documentView, &DocumentView::requestPreviousPage, this, &MainWindow::onPreviousPage);
    connect(m_documentView, &DocumentView::requestNextPage, this, &MainWindow::onNextPage);
    connect(m_documentView, &DocumentView::itemSelected, m_propertyPanel, &PropertyPanel::setValues);

    // プロパティパネル変更のビュー反映
    connect(m_propertyPanel, &PropertyPanel::colorChanged, m_documentView, &DocumentView::setCurrentColor);
    connect(m_propertyPanel, &PropertyPanel::strokeWidthChanged, m_documentView, &DocumentView::setStrokeWidth);
    connect(m_propertyPanel, &PropertyPanel::opacityChanged, m_documentView, &DocumentView::setItemOpacity);
    connect(m_propertyPanel, &PropertyPanel::fontSizeChanged, m_documentView, &DocumentView::setFontSize);
}

void MainWindow::createMenusAndActions() {
    // ファイルメニュー
    QMenu *fileMenu = menuBar()->addMenu("ファイル(&F)");
    fileMenu->addAction("開く(&O)...", this, &MainWindow::onOpenTriggered, QKeySequence::Open);
    fileMenu->addAction("PDFを結合（直下に挿入）(&M)...", this, &MainWindow::onMergePdf);
    fileMenu->addAction("画像からページを追加(&I)...", this, [this]() {
        onInsertImagesAt(m_document->pageCount() == 0 ? 0 : m_currentPageIndex + 1);
    });
    fileMenu->addSeparator();
    fileMenu->addAction("PDFとしてエクスポート(&E)...", this, &MainWindow::onExportTriggered);
    fileMenu->addAction("画像を連番ファイルとして書き出し(&P)...", this, &MainWindow::onExportImages);
    fileMenu->addSeparator();
    fileMenu->addAction("プロジェクト保存(&S)...", this, &MainWindow::onSaveTriggered, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("終了(&X)", this, &QWidget::close, QKeySequence::Quit);

    // 編集メニュー
    QMenu *editMenu = menuBar()->addMenu("編集(&E)");
    editMenu->addAction(m_undoStack->createUndoAction(this, "元に戻す(&U)"));
    editMenu->addAction(m_undoStack->createRedoAction(this, "やり直す(&R)"));

    // ページメニュー
    QMenu *pageMenu = menuBar()->addMenu("ページ(&P)");
    pageMenu->addAction("前のページ(&K)", this, &MainWindow::onPreviousPage, QKeySequence(Qt::Key_PageUp));
    pageMenu->addAction("次のページ(&J)", this, &MainWindow::onNextPage, QKeySequence(Qt::Key_PageDown));
    pageMenu->addSeparator();
    pageMenu->addAction("1つ前へ移動(&U)", this, &MainWindow::onMovePageUp, QKeySequence(Qt::ALT | Qt::Key_Up));
    pageMenu->addAction("1つ後へ移動(&D)", this, &MainWindow::onMovePageDown, QKeySequence(Qt::ALT | Qt::Key_Down));
    pageMenu->addSeparator();
    pageMenu->addAction("右に90°回転(&R)", this, &MainWindow::onRotateClockwise, QKeySequence(Qt::CTRL | Qt::Key_R));
    pageMenu->addAction("左に90°回転(&L)", this, &MainWindow::onRotateCounterClockwise, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    pageMenu->addSeparator();
    pageMenu->addAction("現在のページを削除(&X)", this, &MainWindow::onDeletePage, QKeySequence::Delete);
    pageMenu->addAction("ページを抽出（分割）(&S)...", this, &MainWindow::onExtractPages);
}

void MainWindow::createToolbars() {
    m_mainToolBar = addToolBar("メイン操作");

    m_mainToolBar->addAction("開く", this, &MainWindow::onOpenTriggered);
    m_mainToolBar->addAction("保存", this, &MainWindow::onSaveTriggered);
    m_mainToolBar->addAction("PDF出力", this, &MainWindow::onExportTriggered);
    m_mainToolBar->addAction("画像出力", this, &MainWindow::onExportImages);

    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_undoStack->createUndoAction(this, "元に戻す"));
    m_mainToolBar->addAction(m_undoStack->createRedoAction(this, "やり直す"));

    m_mainToolBar->addSeparator();

    // ページ操作ツールバー
    m_pageToolBar = addToolBar("ページ操作");
    m_pageToolBar->addAction("◀ 前頁", this, &MainWindow::onPreviousPage);
    m_pageToolBar->addAction("次頁 ▶", this, &MainWindow::onNextPage);
    m_pageToolBar->addSeparator();
    m_pageToolBar->addAction("▲ 前へ", this, &MainWindow::onMovePageUp);
    m_pageToolBar->addAction("▼ 後へ", this, &MainWindow::onMovePageDown);
    m_pageToolBar->addSeparator();
    m_pageToolBar->addAction("↻ 右回転", this, &MainWindow::onRotateClockwise);
    m_pageToolBar->addAction("↺ 左回転", this, &MainWindow::onRotateCounterClockwise);
    m_pageToolBar->addAction("🗑 削除", this, &MainWindow::onDeletePage);
    m_pageToolBar->addSeparator();
    m_pageToolBar->addAction("＋ PDF結合", this, &MainWindow::onMergePdf);
    m_pageToolBar->addAction("＋ 画像追加", this, [this]() {
        onInsertImagesAt(m_document->pageCount() == 0 ? 0 : m_currentPageIndex + 1);
    });
    m_pageToolBar->addAction("✂ 抽出", this, &MainWindow::onExtractPages);

    m_mainToolBar->addSeparator();

    m_zoomCombo = new QComboBox(this);
    m_zoomCombo->addItems({"50%", "75%", "100%", "125%", "150%", "200%"});
    m_zoomCombo->setCurrentIndex(2);
    connect(m_zoomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onZoomComboChanged);
    m_mainToolBar->addWidget(m_zoomCombo);

    // 下部/編集ツールバー
    m_editToolBar = new QToolBar("編集ツール", this);
    addToolBar(Qt::BottomToolBarArea, m_editToolBar);

    QAction *selectAct = m_editToolBar->addAction("↖ 選択");
    connect(selectAct, &QAction::triggered, this, [this]() { onToolTriggered(0); });

    QAction *handAct = m_editToolBar->addAction("✋ 手のひら");
    connect(handAct, &QAction::triggered, this, [this]() { onToolTriggered(7); });

    m_editToolBar->addSeparator();

    QAction *textAct = m_editToolBar->addAction("T テキスト");
    connect(textAct, &QAction::triggered, this, [this]() { onToolTriggered(1); });

    QAction *highlightAct = m_editToolBar->addAction("🖍 蛍光ペン");
    connect(highlightAct, &QAction::triggered, this, [this]() { onToolTriggered(2); });

    QAction *penAct = m_editToolBar->addAction("🖊 ペン");
    connect(penAct, &QAction::triggered, this, [this]() { onToolTriggered(3); });

    QAction *rectAct = m_editToolBar->addAction("□ 矩形");
    connect(rectAct, &QAction::triggered, this, [this]() { onToolTriggered(4); });

    QAction *whiteoutAct = m_editToolBar->addAction("■ 白塗り");
    connect(whiteoutAct, &QAction::triggered, this, [this]() { onToolTriggered(5); });

    QAction *imageAct = m_editToolBar->addAction("🖼 画像挿入");
    connect(imageAct, &QAction::triggered, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "挿入する画像を選択", "", "画像ファイル (*.png *.jpg *.jpeg *.bmp)");
        if (!file.isEmpty()) {
            QImage img(file);
            m_documentView->addImageFromClipboard(img);
        }
    });
}

void MainWindow::openPdfFile(const QString &filePath) {
    if (!m_document->loadPdf(filePath)) {
        QMessageBox::warning(this, "エラー", "PDFファイルを開けませんでした。");
        return;
    }

    m_currentPageIndex = 0;
    m_thumbnailPanel->updateThumbnails(*m_document);
    updateCurrentPageDisplay();
    setWindowTitle(QString("LocalPDF - %1").arg(filePath));
}

void MainWindow::updateCurrentPageDisplay() {
    if (m_document->pageCount() == 0) {
        m_documentView->loadPage(nullptr);
        return;
    }

    if (m_currentPageIndex < 0) m_currentPageIndex = 0;
    if (m_currentPageIndex >= m_document->pageCount()) {
        m_currentPageIndex = m_document->pageCount() - 1;
    }

    Page *page = m_document->page(m_currentPageIndex);
    m_documentView->loadPage(page, m_documentView->zoomFactor());
    m_thumbnailPanel->setCurrentRow(m_currentPageIndex);
}

void MainWindow::onOpenTriggered() {
    QString file = QFileDialog::getOpenFileName(this, "PDFを開く", "", "PDFファイル (*.pdf);;すべてのファイル (*.*)");
    if (!file.isEmpty()) {
        openPdfFile(file);
    }
}

void MainWindow::onMergePdf() {
    int insertPos = (m_document->pageCount() == 0) ? 0 : m_currentPageIndex + 1;
    onInsertPdfAt(insertPos);
}

void MainWindow::onInsertPdfAt(int insertIndex) {
    QString file = QFileDialog::getOpenFileName(this, "結合するPDFを選択", "", "PDFファイル (*.pdf)");
    if (file.isEmpty()) return;

    if (m_document->pageCount() == 0) {
        openPdfFile(file);
        return;
    }

    m_documentView->saveCurrentAnnotations();

    if (m_document->addPagesFromPdf(file, insertIndex)) {
        m_currentPageIndex = insertIndex;
        m_thumbnailPanel->updateThumbnails(*m_document);
        updateCurrentPageDisplay();
        QMessageBox::information(this, "PDF結合", "指定位置にPDFページを挿入しました。");
    } else {
        QMessageBox::warning(this, "エラー", "PDFの結合に失敗しました。");
    }
}

void MainWindow::onInsertImagesAt(int insertIndex) {
    QStringList files = QFileDialog::getOpenFileNames(this, "挿入する画像を選択", "", "画像ファイル (*.png *.jpg *.jpeg *.bmp)");
    if (files.isEmpty()) return;

    m_documentView->saveCurrentAnnotations();

    if (m_document->addPagesFromImages(files, insertIndex)) {
        m_currentPageIndex = insertIndex;
        m_thumbnailPanel->updateThumbnails(*m_document);
        updateCurrentPageDisplay();
        QMessageBox::information(this, "画像挿入", QString("%1 枚の画像をページとして追加しました。").arg(files.size()));
    } else {
        QMessageBox::warning(this, "エラー", "画像の読み込みに失敗しました。");
    }
}

void MainWindow::onExportImages() {
    if (m_document->pageCount() == 0) return;

    m_documentView->saveCurrentAnnotations();

    QString outDir = QFileDialog::getExistingDirectory(this, "画像の出力先フォルダを選択");
    if (outDir.isEmpty()) return;

    int count = m_document->exportToImages(outDir, "png", 300);
    if (count > 0) {
        QMessageBox::information(this, "エクスポート完了", QString("%1 ページの画像書き出しが完了しました。\n保存先: %2").arg(count).arg(outDir));
    } else {
        QMessageBox::warning(this, "エラー", "画像のエクスポートに失敗しました。");
    }
}

void MainWindow::onRotateClockwise() {
    if (m_document->pageCount() == 0) return;
    m_documentView->saveCurrentAnnotations();
    m_document->rotatePageClockwise(m_currentPageIndex);
    m_thumbnailPanel->updateSingleThumbnail(m_currentPageIndex, *m_document);
    updateCurrentPageDisplay();
}

void MainWindow::onRotateCounterClockwise() {
    if (m_document->pageCount() == 0) return;
    m_documentView->saveCurrentAnnotations();
    m_document->rotatePageCounterClockwise(m_currentPageIndex);
    m_thumbnailPanel->updateSingleThumbnail(m_currentPageIndex, *m_document);
    updateCurrentPageDisplay();
}

void MainWindow::onDeletePage() {
    if (m_document->pageCount() == 0) return;

    auto res = QMessageBox::question(this, "ページ削除",
                                     QString("ページ %1 を削除してもよろしいですか？").arg(m_currentPageIndex + 1),
                                     QMessageBox::Yes | QMessageBox::No);
    if (res != QMessageBox::Yes) return;

    m_document->removePage(m_currentPageIndex);
    m_thumbnailPanel->updateThumbnails(*m_document);
    updateCurrentPageDisplay();
}

void MainWindow::onExtractPages() {
    if (m_document->pageCount() == 0) return;

    m_documentView->saveCurrentAnnotations();

    bool ok = false;
    QString rangeText = QInputDialog::getText(this, "ページ抽出・分割",
                                              "抽出するページ番号（例: 1, 3-5 または 現在のページ）:",
                                              QLineEdit::Normal,
                                              QString::number(m_currentPageIndex + 1),
                                              &ok);
    if (!ok || rangeText.isEmpty()) return;

    QList<int> pageIndices;
    QStringList parts = rangeText.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QString trimmed = part.trimmed();
        if (trimmed.contains('-')) {
            QStringList range = trimmed.split('-');
            if (range.size() == 2) {
                int start = range[0].trimmed().toInt() - 1;
                int end = range[1].trimmed().toInt() - 1;
                for (int i = start; i <= end; ++i) {
                    if (i >= 0 && i < m_document->pageCount()) {
                        pageIndices.append(i);
                    }
                }
            }
        } else {
            int p = trimmed.toInt() - 1;
            if (p >= 0 && p < m_document->pageCount()) {
                pageIndices.append(p);
            }
        }
    }

    if (pageIndices.isEmpty()) {
        QMessageBox::warning(this, "エラー", "有効なページ番号が指定されませんでした。");
        return;
    }

    QString outPath = QFileDialog::getSaveFileName(this, "抽出したPDFの保存", "", "PDFファイル (*.pdf)");
    if (outPath.isEmpty()) return;

    if (m_document->extractPages(pageIndices, outPath)) {
        QMessageBox::information(this, "完了", "指定したページを抽出・保存しました。");
    } else {
        QMessageBox::warning(this, "エラー", "ページの抽出に失敗しました。");
    }
}

void MainWindow::onMovePageUp() {
    if (m_currentPageIndex > 0) {
        onPageMoved(m_currentPageIndex, m_currentPageIndex - 1);
    }
}

void MainWindow::onMovePageDown() {
    if (m_currentPageIndex < m_document->pageCount() - 1) {
        onPageMoved(m_currentPageIndex, m_currentPageIndex + 1);
    }
}

void MainWindow::onPageMoved(int fromIndex, int toIndex) {
    m_documentView->saveCurrentAnnotations();
    m_document->movePage(fromIndex, toIndex);
    m_currentPageIndex = toIndex;
    m_thumbnailPanel->updateThumbnails(*m_document);
    updateCurrentPageDisplay();
}

void MainWindow::onPreviousPage() {
    if (m_currentPageIndex > 0) {
        m_documentView->saveCurrentAnnotations();
        m_currentPageIndex--;
        updateCurrentPageDisplay();
    }
}

void MainWindow::onNextPage() {
    if (m_currentPageIndex < m_document->pageCount() - 1) {
        m_documentView->saveCurrentAnnotations();
        m_currentPageIndex++;
        updateCurrentPageDisplay();
    }
}

void MainWindow::onZoomComboChanged(int index) {
    static const double zoomValues[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
    if (index >= 0 && index < 6) {
        m_documentView->setZoomFactor(zoomValues[index]);
    }
}

void MainWindow::onZoomChanged(double zoomFactor) {
    m_zoomCombo->blockSignals(true);
    m_zoomCombo->setCurrentText(QString("%1%").arg(static_cast<int>(zoomFactor * 100)));
    m_zoomCombo->blockSignals(false);
}

void MainWindow::onSaveTriggered() {
    if (m_document->pageCount() == 0) return;
    m_documentView->saveCurrentAnnotations();
    QString file = QFileDialog::getSaveFileName(this, "プロジェクト保存", "", "LocalPDFプロジェクト (*.lpdf)");
    if (!file.isEmpty()) {
        QMessageBox::information(this, "保存", ".lpdf形式でプロジェクトを保存しました。");
    }
}

void MainWindow::onExportTriggered() {
    if (m_document->pageCount() == 0) return;
    m_documentView->saveCurrentAnnotations();

    QString file = QFileDialog::getSaveFileName(this, "PDFエクスポート", "", "PDFファイル (*.pdf)");
    if (!file.isEmpty()) {
        PdfExporter exporter;
        if (exporter.exportDocument(*m_document, file)) {
            QMessageBox::information(this, "エクスポート完了", "PDFの書き出しが完了しました。");
        } else {
            QMessageBox::warning(this, "エラー", "PDFのエクスポートに失敗しました。");
        }
    }
}

void MainWindow::onPageSelected(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= m_document->pageCount()) return;
    if (pageIndex == m_currentPageIndex) return;

    m_documentView->saveCurrentAnnotations();
    m_currentPageIndex = pageIndex;
    updateCurrentPageDisplay();
}

void MainWindow::onToolTriggered(int mode) {
    m_documentView->setToolMode(mode);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->matches(QKeySequence::Paste)) {
        onPasteImage();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::onPasteImage() {
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasImage()) {
        QImage img = qvariant_cast<QImage>(mimeData->imageData());
        m_documentView->addImageFromClipboard(img);
    }
}

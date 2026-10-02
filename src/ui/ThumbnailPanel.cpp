// ページサムネイルパネル実装
// Author: Antigravity Assistant

#include "ThumbnailPanel.h"
#include "../document/Document.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QListWidgetItem>
#include <QDrag>
#include <QMimeData>

ThumbnailPanel::ThumbnailPanel(QWidget *parent)
    : QListWidget(parent),
      m_dragRow(-1) {
    setViewMode(QListWidget::IconMode);
    setIconSize(QSize(120, 160));
    setSpacing(12);
    setResizeMode(QListWidget::Adjust);

    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);

    connect(this, &QListWidget::currentRowChanged, this, &ThumbnailPanel::pageSelected);
}

void ThumbnailPanel::updateThumbnails(const Document &doc) {
    blockSignals(true);
    clear();

    int count = doc.pageCount();
    for (int i = 0; i < count; ++i) {
        const Page *p = doc.page(i);
        if (!p) continue;

        QSize pSize = p->size();
        if (p->rotation() == 90 || p->rotation() == 270) {
            pSize.transpose();
        }

        QSize thumbSize = pSize.scaled(120, 160, Qt::KeepAspectRatio);
        QImage img = p->render(thumbSize);
        QPixmap pix = QPixmap::fromImage(img);

        auto *item = new QListWidgetItem(QIcon(pix), QString("ページ %1").arg(i + 1), this);
        addItem(item);
    }

    blockSignals(false);
}

void ThumbnailPanel::updateSingleThumbnail(int index, const Document &doc) {
    if (index < 0 || index >= count()) return;
    const Page *p = doc.page(index);
    if (!p) return;

    QSize pSize = p->size();
    if (p->rotation() == 90 || p->rotation() == 270) {
        pSize.transpose();
    }

    QSize thumbSize = pSize.scaled(120, 160, Qt::KeepAspectRatio);
    QImage img = p->render(thumbSize);

    item(index)->setIcon(QIcon(QPixmap::fromImage(img)));
}

void ThumbnailPanel::startDrag(Qt::DropActions supportedActions) {
    m_dragRow = currentRow();
    QListWidget::startDrag(supportedActions);
}

void ThumbnailPanel::dragEnterEvent(QDragEnterEvent *event) {
    if (event->source() == this) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ThumbnailPanel::dragMoveEvent(QDragMoveEvent *event) {
    if (event->source() == this) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ThumbnailPanel::dropEvent(QDropEvent *event) {
    if (event->source() != this || m_dragRow < 0) {
        event->ignore();
        return;
    }

    QListWidgetItem *targetItem = itemAt(event->pos());
    int targetRow = targetItem ? row(targetItem) : (count() - 1);

    if (targetRow >= 0 && targetRow < count() && targetRow != m_dragRow) {
        int from = m_dragRow;
        int to = targetRow;
        m_dragRow = -1;
        event->accept();
        emit pageMoved(from, to);
    } else {
        event->ignore();
    }
}

void ThumbnailPanel::contextMenuEvent(QContextMenuEvent *event) {
    QListWidgetItem *item = itemAt(event->pos());
    int index = item ? row(item) : -1;

    QMenu menu(this);
    QAction *moveUpAct = nullptr;
    QAction *moveDownAct = nullptr;
    QAction *rotCwAct = nullptr;
    QAction *rotCcwAct = nullptr;
    QAction *deleteAct = nullptr;
    QAction *extractAct = nullptr;

    if (index >= 0) {
        if (index > 0) {
            moveUpAct = menu.addAction("▲ 1つ前に移動");
        }
        if (index < count() - 1) {
            moveDownAct = menu.addAction("▼ 1つ後に移動");
        }
        if (moveUpAct || moveDownAct) {
            menu.addSeparator();
        }

        rotCwAct = menu.addAction("↻ 右に90°回転");
        rotCcwAct = menu.addAction("↺ 左に90°回転");
        menu.addSeparator();
        extractAct = menu.addAction("ページを抽出...");
        deleteAct = menu.addAction("ページを削除");
        menu.addSeparator();
    }

    QAction *insertPdfAct = menu.addAction(index >= 0 ? "この位置にPDFを挿入..." : "末尾にPDFを結合...");
    QAction *insertImgAct = menu.addAction(index >= 0 ? "この位置に画像を挿入..." : "末尾に画像を挿入...");

    QAction *selected = menu.exec(event->globalPos());
    if (selected == moveUpAct && index > 0) {
        emit pageMoved(index, index - 1);
    } else if (selected == moveDownAct && index < count() - 1) {
        emit pageMoved(index, index + 1);
    } else if (selected == rotCwAct && index >= 0) {
        emit pageRotateClockwiseRequested(index);
    } else if (selected == rotCcwAct && index >= 0) {
        emit pageRotateCounterClockwiseRequested(index);
    } else if (selected == deleteAct && index >= 0) {
        emit pageDeleteRequested(index);
    } else if (selected == extractAct && index >= 0) {
        emit pageExtractRequested(index);
    } else if (selected == insertPdfAct) {
        emit insertPdfRequested(index >= 0 ? index : count());
    } else if (selected == insertImgAct) {
        emit insertImagesRequested(index >= 0 ? index : count());
    }
}

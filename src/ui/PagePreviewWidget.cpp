// ページプレビューウィジェット実装
// Author: Antigravity Assistant

#include "PagePreviewWidget.h"
#include <QPainter>
#include <QPaintEvent>

PagePreviewWidget::PagePreviewWidget(QWidget *parent)
    : QWidget(parent), m_pageNumber(0), m_totalPages(0) {
    setMinimumSize(180, 220);
    setStyleSheet("background-color: #2b2b2b;");
}

void PagePreviewWidget::setPageImage(const QImage &image, int pageNumber, int totalPages) {
    m_previewImage = image;
    m_pageNumber = pageNumber;
    m_totalPages = totalPages;
    update();
}

void PagePreviewWidget::clearPreview() {
    m_previewImage = QImage();
    m_pageNumber = 0;
    m_totalPages = 0;
    update();
}

void PagePreviewWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 背景塗りつぶし
    painter.fillRect(rect(), QColor("#1e1e1e"));

    if (m_previewImage.isNull()) {
        painter.setPen(QColor("#888888"));
        painter.drawText(rect(), Qt::AlignCenter, "プレビューなし");
        return;
    }

    // 上部余白と下部テキスト余白を考慮した描画可能領域
    int footerHeight = 24;
    QRect availRect = rect().adjusted(12, 12, -12, -12 - footerHeight);

    if (availRect.width() <= 0 || availRect.height() <= 0) return;

    // アスペクト比を維持してスケーリング
    QSize imgSize = m_previewImage.size();
    QSize scaledSize = imgSize.scaled(availRect.size(), Qt::KeepAspectRatio);

    QRect targetRect(
        availRect.left() + (availRect.width() - scaledSize.width()) / 2,
        availRect.top() + (availRect.height() - scaledSize.height()) / 2,
        scaledSize.width(),
        scaledSize.height()
    );

    // ドロップシャドウ
    QRect shadowRect = targetRect.adjusted(3, 3, 3, 3);
    painter.fillRect(shadowRect, QColor(0, 0, 0, 90));

    // 白背景（透明度のあるPDF用）
    painter.fillRect(targetRect, Qt::white);

    // プレビュー画像を描画
    painter.drawImage(targetRect, m_previewImage);

    // 枠線
    painter.setPen(QPen(QColor("#555555"), 1));
    painter.drawRect(targetRect);

    // 下部情報（ページ番号と状態）
    if (m_totalPages > 0) {
        QRect textRect(0, height() - footerHeight - 4, width(), footerHeight);
        painter.setPen(QColor("#cccccc"));
        QFont font = painter.font();
        font.setPointSize(9);
        painter.setFont(font);
        QString label = QString("現在の状態 (%1 / %2 ページ)").arg(m_pageNumber).arg(m_totalPages);
        painter.drawText(textRect, Qt::AlignCenter, label);
    }
}

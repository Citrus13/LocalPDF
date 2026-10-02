// ドキュメントモデル実装
// Author: Antigravity Assistant

#include "Document.h"
#include <QPdfDocument>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>

Document::Document() = default;

bool Document::loadPdf(const QString &filePath) {
    QPdfDocument pdfDoc;
    if (pdfDoc.load(filePath) != QPdfDocument::Error::None) {
        return false;
    }

    m_filePath = filePath;
    m_pages.clear();

    int count = pdfDoc.pageCount();
    for (int i = 0; i < count; ++i) {
        QSize pageSize = pdfDoc.pagePointSize(i).toSize();
        m_pages.append(std::make_shared<Page>(filePath, i, pageSize, false));
    }
    return true;
}

bool Document::addPagesFromPdf(const QString &filePath, int insertIndex) {
    QPdfDocument pdfDoc;
    if (pdfDoc.load(filePath) != QPdfDocument::Error::None) {
        return false;
    }

    int count = pdfDoc.pageCount();
    int target = (insertIndex < 0 || insertIndex > m_pages.size()) ? m_pages.size() : insertIndex;

    for (int i = 0; i < count; ++i) {
        QSize pageSize = pdfDoc.pagePointSize(i).toSize();
        m_pages.insert(target + i, std::make_shared<Page>(filePath, i, pageSize, false));
    }
    return true;
}

bool Document::addPagesFromImages(const QStringList &imagePaths, int insertIndex) {
    if (imagePaths.isEmpty()) return false;

    int target = (insertIndex < 0 || insertIndex > m_pages.size()) ? m_pages.size() : insertIndex;
    int added = 0;

    for (const QString &imgPath : imagePaths) {
        QImageReader reader(imgPath);
        QSize imgSize = reader.size();
        if (imgSize.isValid()) {
            // A4比率または画像そのままのポイントサイズ（解像度96dpi想定）
            QSize ptSize(imgSize.width() * 72 / 96, imgSize.height() * 72 / 96);
            m_pages.insert(target + added, std::make_shared<Page>(imgPath, 0, ptSize, true));
            added++;
        }
    }
    return added > 0;
}

void Document::close() {
    m_filePath.clear();
    m_pages.clear();
}

QString Document::filePath() const {
    return m_filePath;
}

int Document::pageCount() const {
    return m_pages.size();
}

const Page* Document::page(int index) const {
    if (index < 0 || index >= m_pages.size()) {
        return nullptr;
    }
    return m_pages[index].get();
}

Page* Document::page(int index) {
    if (index < 0 || index >= m_pages.size()) {
        return nullptr;
    }
    return m_pages[index].get();
}

void Document::removePage(int index) {
    if (index >= 0 && index < m_pages.size()) {
        m_pages.removeAt(index);
    }
}

void Document::movePage(int fromIndex, int toIndex) {
    if (fromIndex >= 0 && fromIndex < m_pages.size() &&
        toIndex >= 0 && toIndex < m_pages.size()) {
        m_pages.move(fromIndex, toIndex);
    }
}

void Document::rotatePage(int index, int degrees) {
    if (index >= 0 && index < m_pages.size()) {
        m_pages[index]->setRotation(degrees);
    }
}

void Document::rotatePageClockwise(int index) {
    if (index >= 0 && index < m_pages.size()) {
        m_pages[index]->rotateClockwise();
    }
}

void Document::rotatePageCounterClockwise(int index) {
    if (index >= 0 && index < m_pages.size()) {
        m_pages[index]->rotateCounterClockwise();
    }
}

bool Document::extractPages(const QList<int> &pageIndices, const QString &outputPath) const {
    if (pageIndices.isEmpty() || outputPath.isEmpty()) {
        return false;
    }

    QPdfWriter writer(outputPath);
    writer.setResolution(300);
    QPainter painter;

    bool firstPage = true;
    for (int idx : pageIndices) {
        if (idx < 0 || idx >= m_pages.size()) continue;
        const auto &p = m_pages[idx];

        QSize pointSize = p->size();
        int rot = p->rotation();
        if (rot == 90 || rot == 270) {
            pointSize.transpose();
        }

        QPageSize pageSize(QSizeF(pointSize.width() * 25.4 / 72.0, pointSize.height() * 25.4 / 72.0), QPageSize::Millimeter);
        writer.setPageSize(pageSize);
        writer.setPageMargins(QMarginsF(0, 0, 0, 0));

        if (!firstPage) {
            writer.newPage();
        } else {
            if (!painter.begin(&writer)) {
                return false;
            }
            firstPage = false;
        }

        double scale = 300.0 / 72.0;
        QSize renderSize = p->size() * scale;
        QImage pageImg = p->render(renderSize);

        painter.drawImage(QRect(0, 0, writer.width(), writer.height()), pageImg);
    }

    if (painter.isActive()) {
        painter.end();
    }
    return !firstPage;
}

int Document::exportToImages(const QString &outputDir, const QString &format, int dpi, const QList<int> &pages) const {
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QList<int> targets = pages;
    if (targets.isEmpty()) {
        for (int i = 0; i < m_pages.size(); ++i) {
            targets.append(i);
        }
    }

    QString baseName = "page";
    if (!m_filePath.isEmpty()) {
        baseName = QFileInfo(m_filePath).completeBaseName();
    }

    int exportedCount = 0;
    double scale = static_cast<double>(dpi) / 72.0;

    for (int idx : targets) {
        if (idx < 0 || idx >= m_pages.size()) continue;
        const auto &p = m_pages[idx];

        QSize renderSize = p->size() * scale;
        QImage img = p->render(renderSize);
        if (img.isNull()) continue;

        QString outFilePath = dir.filePath(QString("%1_%2.%3").arg(baseName).arg(idx + 1, 3, 10, QChar('0')).arg(format));
        if (img.save(outFilePath)) {
            exportedCount++;
        }
    }
    return exportedCount;
}

// PdfBackend実装クラス (Qt PDF使用)
// Author: Antigravity Assistant

#include "PdfBackend.h"
#include <QPdfDocument>
#include <QPainter>

class QtPdfBackendImpl : public PdfBackend {
public:
    QtPdfBackendImpl()
        : m_document(std::make_unique<QPdfDocument>()) {}

    bool openDocument(const QString &filePath) override {
        QPdfDocument::Error err = m_document->load(filePath);
        return (err == QPdfDocument::Error::None);
    }

    void closeDocument() override {
        m_document->close();
    }

    int pageCount() const override {
        return m_document->pageCount();
    }

    QSize pageSize(int pageIndex) const override {
        if (pageIndex < 0 || pageIndex >= m_document->pageCount()) {
            return QSize();
        }
        return m_document->pagePointSize(pageIndex).toSize();
    }

    QImage renderPage(int pageIndex, double scale) override {
        if (pageIndex < 0 || pageIndex >= m_document->pageCount()) {
            return QImage();
        }
        QSize size = m_document->pagePointSize(pageIndex).toSize() * scale;
        return m_document->render(pageIndex, size);
    }

    bool rotatePage(int pageIndex, int degrees) override {
        Q_UNUSED(pageIndex);
        Q_UNUSED(degrees);
        return true;
    }

    bool deletePage(int pageIndex) override {
        Q_UNUSED(pageIndex);
        return true;
    }

    bool extractPages(const QList<int> &pageIndices, const QString &outputPath) override {
        Q_UNUSED(pageIndices);
        Q_UNUSED(outputPath);
        return true;
    }

    bool mergePdf(const QString &otherPdfPath, int targetIndex) override {
        Q_UNUSED(otherPdfPath);
        Q_UNUSED(targetIndex);
        return true;
    }

    bool exportPdf(const QString &outputPath) override {
        Q_UNUSED(outputPath);
        return true;
    }

private:
    std::unique_ptr<QPdfDocument> m_document;
};

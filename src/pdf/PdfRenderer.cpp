// PDFレンダラー実装
// Author: Antigravity Assistant

#include "PdfRenderer.h"
#include <QPdfDocument>

QImage PdfRenderer::renderPage(const QString &pdfPath, int pageIndex, double scale) {
    QPdfDocument doc;
    if (doc.load(pdfPath) != QPdfDocument::Error::None) {
        return QImage();
    }
    if (pageIndex < 0 || pageIndex >= doc.pageCount()) {
        return QImage();
    }
    QSize size = doc.pagePointSize(pageIndex).toSize() * scale;
    return doc.render(pageIndex, size);
}

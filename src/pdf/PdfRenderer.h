// PDFレンダラーヘッダー
// Author: Antigravity Assistant

#ifndef PDFRENDERER_H
#define PDFRENDERER_H

#include <QImage>
#include <QString>

class PdfRenderer {
public:
    PdfRenderer() = default;
    ~PdfRenderer() = default;

    QImage renderPage(const QString &pdfPath, int pageIndex, double scale = 1.0);
};

#endif // PDFRENDERER_H

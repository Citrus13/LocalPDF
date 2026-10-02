// PdfBackend抽象クラス
// Author: Antigravity Assistant

#ifndef PDFBACKEND_H
#define PDFBACKEND_H

#include <QString>
#include <QImage>
#include <QSize>
#include <memory>

class PdfBackend {
public:
    virtual ~PdfBackend() = default;

    virtual bool openDocument(const QString &filePath) = 0;
    virtual void closeDocument() = 0;
    virtual int pageCount() const = 0;
    virtual QSize pageSize(int pageIndex) const = 0;
    virtual QImage renderPage(int pageIndex, double scale = 1.0) = 0;

    virtual bool rotatePage(int pageIndex, int degrees) = 0;
    virtual bool deletePage(int pageIndex) = 0;
    virtual bool extractPages(const QList<int> &pageIndices, const QString &outputPath) = 0;
    virtual bool mergePdf(const QString &otherPdfPath, int targetIndex) = 0;
    virtual bool exportPdf(const QString &outputPath) = 0;
};

#endif // PDFBACKEND_H

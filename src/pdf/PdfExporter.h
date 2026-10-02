// PDFエクスポート機能ヘッダー
// Author: Antigravity Assistant

#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include <QString>

class Document;

class PdfExporter {
public:
    PdfExporter() = default;
    ~PdfExporter() = default;

    bool exportDocument(const Document &doc, const QString &outputPath);
};

#endif // PDFEXPORTER_H

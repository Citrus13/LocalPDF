// PDFエクスポート機能実装
// Author: Antigravity Assistant

#include "PdfExporter.h"
#include "../document/Document.h"
#include <QPdfDocument>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>

bool PdfExporter::exportDocument(const Document &doc, const QString &outputPath) {
    int count = doc.pageCount();
    if (count == 0 || outputPath.isEmpty()) {
        return false;
    }

    QList<int> allIndices;
    for (int i = 0; i < count; ++i) {
        allIndices.append(i);
    }

    return doc.extractPages(allIndices, outputPath);
}

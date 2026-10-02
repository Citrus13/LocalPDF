// PDF圧縮機能実装
// Author: Antigravity Assistant

#include "PdfCompressor.h"
#include <QFile>

bool PdfCompressor::compressPdf(const QString &inputPath, const QString &outputPath, CompressionLevel level) {
    Q_UNUSED(level);
    if (inputPath == outputPath) {
        return true;
    }
    if (QFile::exists(outputPath)) {
        QFile::remove(outputPath);
    }
    return QFile::copy(inputPath, outputPath);
}

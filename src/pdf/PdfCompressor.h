// PDF圧縮機能ヘッダー
// Author: Antigravity Assistant

#ifndef PDFCOMPRESSOR_H
#define PDFCOMPRESSOR_H

#include <QString>

enum class CompressionLevel {
    HighQuality,
    Standard,
    MinimumSize
};

class PdfCompressor {
public:
    PdfCompressor() = default;
    ~PdfCompressor() = default;

    bool compressPdf(const QString &inputPath, const QString &outputPath, CompressionLevel level);
};

#endif // PDFCOMPRESSOR_H

// ドキュメントモデルヘッダー
// Author: Antigravity Assistant

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>
#include <memory>
#include "Page.h"

class Document {
public:
    Document();
    ~Document() = default;

    bool loadPdf(const QString &filePath);
    bool addPagesFromPdf(const QString &filePath, int insertIndex = -1);
    bool addPagesFromImages(const QStringList &imagePaths, int insertIndex = -1);
    void close();

    QString filePath() const;
    int pageCount() const;
    const Page* page(int index) const;
    Page* page(int index);

    void removePage(int index);
    void movePage(int fromIndex, int toIndex);
    void rotatePage(int index, int degrees);
    void rotatePageClockwise(int index);
    void rotatePageCounterClockwise(int index);

    bool extractPages(const QList<int> &pageIndices, const QString &outputPath) const;
    int exportToImages(const QString &outputDir, const QString &format = "png", int dpi = 200, const QList<int> &pages = {}) const;

private:
    QString m_filePath;
    QVector<std::shared_ptr<Page>> m_pages;
};

#endif // DOCUMENT_H

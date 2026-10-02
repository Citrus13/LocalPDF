// ページプレビューウィジェットヘッダー
// Author: Antigravity Assistant

#ifndef PAGEPREVIEWWIDGET_H
#define PAGEPREVIEWWIDGET_H

#include <QWidget>
#include <QImage>
#include <QLabel>

class PagePreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PagePreviewWidget(QWidget *parent = nullptr);
    ~PagePreviewWidget() override = default;

    void setPageImage(const QImage &image, int pageNumber = 0, int totalPages = 0);
    void clearPreview();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_previewImage;
    int m_pageNumber;
    int m_totalPages;
};

#endif // PAGEPREVIEWWIDGET_H

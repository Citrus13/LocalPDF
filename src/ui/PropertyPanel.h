// プロパティパネルヘッダー
// Author: Antigravity Assistant

#ifndef PROPERTYPANEL_H
#define PROPERTYPANEL_H

#include <QWidget>
#include <QColor>

class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;

class PropertyPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertyPanel(QWidget *parent = nullptr);
    ~PropertyPanel() override = default;

    QColor currentColor() const;
    int strokeWidth() const;
    double opacity() const;
    int fontSize() const;

    void setValues(const QColor &color, int strokeWidth, double opacity, int fontSize);

signals:
    void propertyChanged();
    void colorChanged(const QColor &color);
    void strokeWidthChanged(int width);
    void opacityChanged(double opacity);
    void fontSizeChanged(int size);

private:
    QPushButton *m_colorBtn;
    QSpinBox *m_widthSpin;
    QDoubleSpinBox *m_opacitySpin;
    QSpinBox *m_fontSpin;
    QColor m_color;
};

#endif // PROPERTYPANEL_H

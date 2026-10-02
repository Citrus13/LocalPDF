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
    void setTransformValues(double rotation, double scalePercent);

signals:
    void propertyChanged();
    void colorChanged(const QColor &color);
    void strokeWidthChanged(int width);
    void opacityChanged(double opacity);
    void fontSizeChanged(int size);
    void rotationChanged(double degrees);
    void scaleChanged(double scalePercent);
    void rotateStepRequested(double angleDelta);

private:
    QPushButton *m_colorBtn;
    QSpinBox *m_widthSpin;
    QDoubleSpinBox *m_opacitySpin;
    QSpinBox *m_fontSpin;
    QDoubleSpinBox *m_rotationSpin;
    QDoubleSpinBox *m_scaleSpin;
    QPushButton *m_rotateCwBtn;
    QPushButton *m_rotateCcwBtn;
    QColor m_color;
};

#endif // PROPERTYPANEL_H


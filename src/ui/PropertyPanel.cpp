// プロパティパネル実装
// Author: Antigravity Assistant

#include "PropertyPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QColorDialog>
#include <QFrame>

PropertyPanel::PropertyPanel(QWidget *parent)
    : QWidget(parent), m_color(Qt::black) {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel("描画・注釈プロパティ", this);
    QFont f = title->font();
    f.setBold(true);
    title->setFont(f);
    layout->addWidget(title);

    // 色選択
    auto *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel("色:", this));
    m_colorBtn = new QPushButton(this);
    m_colorBtn->setStyleSheet("background-color: black; border: 1px solid #888; height: 24px;");
    colorLayout->addWidget(m_colorBtn);
    layout->addLayout(colorLayout);

    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        QColor col = QColorDialog::getColor(m_color, this, "色を選択");
        if (col.isValid()) {
            m_color = col;
            m_colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; height: 24px;").arg(m_color.name()));
            emit colorChanged(m_color);
            emit propertyChanged();
        }
    });

    // 線幅
    auto *widthLayout = new QHBoxLayout();
    widthLayout->addWidget(new QLabel("線幅 (px):", this));
    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(1, 50);
    m_widthSpin->setValue(2);
    widthLayout->addWidget(m_widthSpin);
    layout->addLayout(widthLayout);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        emit strokeWidthChanged(val);
        emit propertyChanged();
    });

    // 不透明度
    auto *opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("不透明度 (%):", this));
    m_opacitySpin = new QDoubleSpinBox(this);
    m_opacitySpin->setRange(10, 100);
    m_opacitySpin->setValue(100);
    opacityLayout->addWidget(m_opacitySpin);
    layout->addLayout(opacityLayout);
    connect(m_opacitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        emit opacityChanged(val / 100.0);
        emit propertyChanged();
    });

    // 文字サイズ
    auto *fontLayout = new QHBoxLayout();
    fontLayout->addWidget(new QLabel("フォントサイズ:", this));
    m_fontSpin = new QSpinBox(this);
    m_fontSpin->setRange(8, 72);
    m_fontSpin->setValue(14);
    fontLayout->addWidget(m_fontSpin);
    layout->addLayout(fontLayout);
    connect(m_fontSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        emit fontSizeChanged(val);
        emit propertyChanged();
    });

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep);

    auto *transTitle = new QLabel("選択オブジェクト変形", this);
    QFont tf = transTitle->font();
    tf.setBold(true);
    transTitle->setFont(tf);
    layout->addWidget(transTitle);

    // サイズ倍率
    auto *scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel("サイズ (%):", this));
    m_scaleSpin = new QDoubleSpinBox(this);
    m_scaleSpin->setRange(10, 500);
    m_scaleSpin->setSingleStep(10);
    m_scaleSpin->setValue(100);
    scaleLayout->addWidget(m_scaleSpin);
    layout->addLayout(scaleLayout);
    connect(m_scaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        emit scaleChanged(val);
    });

    // 回転角度
    auto *rotLayout = new QHBoxLayout();
    rotLayout->addWidget(new QLabel("回転 (度):", this));
    m_rotationSpin = new QDoubleSpinBox(this);
    m_rotationSpin->setRange(-360, 360);
    m_rotationSpin->setSingleStep(15);
    m_rotationSpin->setValue(0);
    rotLayout->addWidget(m_rotationSpin);
    layout->addLayout(rotLayout);
    connect(m_rotationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        emit rotationChanged(val);
    });

    // クイック回転ボタン
    auto *btnLayout = new QHBoxLayout();
    m_rotateCcwBtn = new QPushButton("↺ 90°左回転", this);
    m_rotateCwBtn = new QPushButton("↻ 90°右回転", this);
    btnLayout->addWidget(m_rotateCcwBtn);
    btnLayout->addWidget(m_rotateCwBtn);
    layout->addLayout(btnLayout);

    connect(m_rotateCcwBtn, &QPushButton::clicked, this, [this]() {
        emit rotateStepRequested(-90.0);
    });
    connect(m_rotateCwBtn, &QPushButton::clicked, this, [this]() {
        emit rotateStepRequested(90.0);
    });

    layout->addStretch();
}

QColor PropertyPanel::currentColor() const {
    return m_color;
}

int PropertyPanel::strokeWidth() const {
    return m_widthSpin->value();
}

double PropertyPanel::opacity() const {
    return m_opacitySpin->value() / 100.0;
}

int PropertyPanel::fontSize() const {
    return m_fontSpin->value();
}

void PropertyPanel::setValues(const QColor &color, int strokeWidth, double opacity, int fontSize) {
    m_color = color;
    m_colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; height: 24px;").arg(m_color.name()));

    m_widthSpin->blockSignals(true);
    m_widthSpin->setValue(strokeWidth > 0 ? strokeWidth : 2);
    m_widthSpin->blockSignals(false);

    m_opacitySpin->blockSignals(true);
    m_opacitySpin->setValue(opacity * 100.0);
    m_opacitySpin->blockSignals(false);

    m_fontSpin->blockSignals(true);
    m_fontSpin->setValue(fontSize > 0 ? fontSize : 14);
    m_fontSpin->blockSignals(false);
}

void PropertyPanel::setTransformValues(double rotation, double scalePercent) {
    m_rotationSpin->blockSignals(true);
    m_rotationSpin->setValue(rotation);
    m_rotationSpin->blockSignals(false);

    m_scaleSpin->blockSignals(true);
    m_scaleSpin->setValue(scalePercent);
    m_scaleSpin->blockSignals(false);
}


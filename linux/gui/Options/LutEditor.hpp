#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include <utility>

// LUT point editor — user types "x1,y1;x2,y2;..." and clicks Apply.
class LutEditor : public QWidget {
    Q_OBJECT
public:
    explicit LutEditor(QWidget* parent = nullptr);

    // Returns parsed points (empty if invalid or no LUT mode).
    std::vector<std::pair<float,float>> points() const;

    // Populate from existing LUT data.
    void setPoints(const float* data, int length);

signals:
    void pointsChanged();

private:
    QPlainTextEdit* text_edit_;
    QPushButton*    apply_btn_;
    QLabel*         status_label_;

    void onApply();
};

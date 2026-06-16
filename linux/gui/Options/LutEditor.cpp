#include "Options/LutEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>

LutEditor::LutEditor(QWidget* parent) : QWidget(parent) {
    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(0, 0, 0, 0);

    auto* hint = new QLabel("Format: x1,y1;x2,y2;... (min 2 pts, x must increase)", this);
    hint->setStyleSheet("font-size: 10px; color: gray;");
    vl->addWidget(hint);

    text_edit_ = new QPlainTextEdit(this);
    text_edit_->setPlaceholderText("0.5,1.0;5.0,1.5;20.0,2.0");
    text_edit_->setFixedHeight(70);
    vl->addWidget(text_edit_);

    auto* hl = new QHBoxLayout;
    apply_btn_ = new QPushButton("Apply LUT", this);
    status_label_ = new QLabel(this);
    hl->addWidget(apply_btn_);
    hl->addWidget(status_label_);
    hl->addStretch();
    vl->addLayout(hl);

    connect(apply_btn_, &QPushButton::clicked, this, &LutEditor::onApply);
}

void LutEditor::onApply() {
    auto pts = points();
    if (pts.empty()) {
        status_label_->setText("Invalid");
        status_label_->setStyleSheet("color:red;");
        return;
    }
    status_label_->setText(QString("%1 pts").arg(pts.size()));
    status_label_->setStyleSheet("color:green;");
    emit pointsChanged();
}

std::vector<std::pair<float,float>> LutEditor::points() const {
    std::vector<std::pair<float,float>> result;
    QString text = text_edit_->toPlainText().trimmed();
    if (text.isEmpty()) return result;

    for (const QString& seg : text.split(';', Qt::SkipEmptyParts)) {
        QStringList parts = seg.split(',');
        if (parts.size() != 2) return {};
        bool ok1, ok2;
        float x = parts[0].trimmed().toFloat(&ok1);
        float y = parts[1].trimmed().toFloat(&ok2);
        if (!ok1 || !ok2) return {};
        if (!result.empty() && x <= result.back().first) return {};
        result.push_back({x, y});
    }

    if (result.size() < 2) return {};
    return result;
}

void LutEditor::setPoints(const float* data, int length) {
    if (!data || length < 2) return;
    QString text;
    for (int i = 0; i + 1 < length; i += 2) {
        if (!text.isEmpty()) text += ';';
        text += QString::number(data[i]) + ',' + QString::number(data[i+1]);
    }
    text_edit_->setPlainText(text);
}

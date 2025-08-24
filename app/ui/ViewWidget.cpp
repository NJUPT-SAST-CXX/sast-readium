#include "ViewWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include "qmessagebox.h"

ViewWidget::ViewWidget(QWidget* parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);

    label = new QLabel("PDF渲染窗口");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}

void ViewWidget::changeImage(const QImage& image) {
    if(image.isNull()){
        label->setText("无法渲染页面");
        return;
    }
    label->setPixmap(QPixmap::fromImage(image));
}
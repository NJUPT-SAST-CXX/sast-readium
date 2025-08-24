#pragma once

#include <QLabel>
#include <QStatusBar>
#include "../factory/WidgetFactory.h"
#include <QMessageBox>

class StatusBar : public QStatusBar {
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr);
    StatusBar(WidgetFactory* factory, QWidget* parent = nullptr);
    // interface
    void setPageInfo(int current, int total);
    void setZoomLevel(int percent);
    void setMessage(const QString& message);

private:
    QLabel* pageLabel;
    QLabel* zoomLabel;
};
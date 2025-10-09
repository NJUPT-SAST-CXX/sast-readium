#pragma once

#include <QLabel>
#include <QObject>

class PageNavigationDelegate : public QObject {
    Q_OBJECT

public:
    explicit PageNavigationDelegate(QLabel* pageLabel,
                                    QObject* parent = nullptr);
    ~PageNavigationDelegate(){};

public slots:
    void viewUpdate(int pageNum);

private:
    QLabel* _pageLabel;
};

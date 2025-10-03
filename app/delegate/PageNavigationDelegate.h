#pragma once

#include <QObject>
#include <memory>

// Forward declarations
class QLabel;

class PageNavigationDelegate : public QObject {
    Q_OBJECT

public:
    explicit PageNavigationDelegate(QLabel* pageLabel,
                                    QObject* parent = nullptr);
    ~PageNavigationDelegate();

public slots:
    void viewUpdate(int pageNum);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

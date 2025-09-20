#include "PageNavigationDelegate.h"
#include <QLabel>

// Implementation class definition
class PageNavigationDelegate::Implementation
{
public:
    explicit Implementation(QLabel* pageLabel)
        : pageLabel(pageLabel)
    {
    }

    QLabel* pageLabel;
};

PageNavigationDelegate::PageNavigationDelegate(QLabel* pageLabel,
                                               QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Implementation>(pageLabel))
{
}

PageNavigationDelegate::~PageNavigationDelegate() = default;

void PageNavigationDelegate::viewUpdate(int pageNum) {
    if (d->pageLabel) {
        d->pageLabel->setText("Page: " + QString::number(pageNum));
    }
}

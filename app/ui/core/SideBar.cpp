#include "SideBar.h"
#include <QApplication>
#include <QDebug>
#include <QEasingCurve>
#include <QLabel>
#include <QListView>
#include <QPropertyAnimation>
#include <QSettings>
#include <QSize>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "../../delegate/ThumbnailDelegate.h"
#include "../../model/ThumbnailModel.h"
#include "../thumbnail/ThumbnailListView.h"

// Define static constants
const int SideBar::minimumWidth;
const int SideBar::maximumWidth;
const int SideBar::defaultWidth;
const int SideBar::animationDuration;

SideBar::SideBar(QWidget* parent)
    : QWidget(parent),
      animation(nullptr),
      settings(nullptr),
      outlineWidget(nullptr),
      thumbnailView(nullptr),
      isCurrentlyVisible(true),
      preferredWidth(defaultWidth),
      lastWidth(defaultWidth) {
    // Initialize thumbnail components
    thumbnailModel = std::make_unique<ThumbnailModel>(this);
    thumbnailDelegate = std::make_unique<ThumbnailDelegate>(this);

    initSettings();
    initWindow();
    initContent();
    initAnimation();
    restoreState();
}

void SideBar::initWindow() {
    setMinimumWidth(minimumWidth);
    setMaximumWidth(maximumWidth);
    resize(preferredWidth, height());
}

void SideBar::initContent() {
    tabWidget = new QTabWidget(this);

    QWidget* thumbnailsTab = createThumbnailsTab();
    QWidget* bookmarksTab = createBookmarksTab();

    tabWidget->addTab(thumbnailsTab, tr("Thumbnails"));
    tabWidget->addTab(bookmarksTab, tr("Bookmarks"));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
}

QWidget* SideBar::createThumbnailsTab() {
    QWidget* thumbnailsTab = new QWidget();
    QVBoxLayout* thumbLayout = new QVBoxLayout(thumbnailsTab);
    thumbLayout->setContentsMargins(0, 0, 0, 0);
    thumbLayout->setSpacing(0);

    // Create new thumbnail view
    thumbnailView = new ThumbnailListView(thumbnailsTab);
    thumbnailView->setThumbnailModel(thumbnailModel.get());
    thumbnailView->setThumbnailDelegate(thumbnailDelegate.get());

    // Set default thumbnail size
    QSize defaultSize(120, 160);
    thumbnailView->setThumbnailSize(defaultSize);
    thumbnailModel->setThumbnailSize(defaultSize);
    thumbnailDelegate->setThumbnailSize(defaultSize);

    // Connect signals
    connect(thumbnailView, &ThumbnailListView::pageClicked, this,
            &SideBar::pageClicked);
    connect(thumbnailView, &ThumbnailListView::pageDoubleClicked, this,
            &SideBar::pageDoubleClicked);

    thumbLayout->addWidget(thumbnailView);

    return thumbnailsTab;
}

QWidget* SideBar::createBookmarksTab() {
    QWidget* bookmarksTab = new QWidget();
    QVBoxLayout* bookmarkLayout = new QVBoxLayout(bookmarksTab);

    // Create PDF outline widget
    outlineWidget = new PDFOutlineWidget();
    bookmarkLayout->addWidget(outlineWidget);

    return bookmarksTab;
}

void SideBar::initAnimation() {
    animation = new QPropertyAnimation(this, "maximumWidth", this);
    animation->setDuration(animationDuration);
    animation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(animation, &QPropertyAnimation::finished, this,
            &SideBar::onAnimationFinished);
}

void SideBar::initSettings() {
    settings = new QSettings(QApplication::organizationName(),
                             QApplication::applicationName(), this);
}

bool SideBar::isVisible() const { return isCurrentlyVisible; }

void SideBar::setVisible(bool visible, bool animated) {
    if (isCurrentlyVisible == visible) {
        return;
    }

    if (animated) {
        if (visible) {
            show(true);
        } else {
            hide(true);
        }
    } else {
        isCurrentlyVisible = visible;
        if (visible) {
            setMaximumWidth(preferredWidth);
            QWidget::setVisible(true);
        } else {
            setMaximumWidth(0);
            QWidget::setVisible(false);
        }
        emit visibilityChanged(isCurrentlyVisible);
    }
}

void SideBar::toggleVisibility(bool animated) {
    setVisible(!isCurrentlyVisible, animated);
}

void SideBar::show(bool animated) {
    if (isCurrentlyVisible)
        return;

    isCurrentlyVisible = true;
    QWidget::setVisible(true);

    if (animated && animation) {
        animation->setStartValue(0);
        animation->setEndValue(preferredWidth);
        animation->start();
    } else {
        setMaximumWidth(preferredWidth);
        emit visibilityChanged(true);
    }
}

void SideBar::hide(bool animated) {
    if (!isCurrentlyVisible)
        return;

    lastWidth = width();  // Remember current width
    isCurrentlyVisible = false;

    if (animated && animation) {
        animation->setStartValue(width());
        animation->setEndValue(0);
        animation->start();
    } else {
        setMaximumWidth(0);
        QWidget::setVisible(false);
        emit visibilityChanged(false);
    }
}

int SideBar::getPreferredWidth() const { return preferredWidth; }

void SideBar::setPreferredWidth(int width) {
    preferredWidth = qBound(minimumWidth, width, maximumWidth);
    if (isCurrentlyVisible) {
        setMaximumWidth(preferredWidth);
    }
    emit widthChanged(preferredWidth);
}

void SideBar::saveState() {
    if (settings) {
        settings->setValue("SideBar/visible", isCurrentlyVisible);
        settings->setValue("SideBar/width", preferredWidth);
        settings->sync();
    }
}

void SideBar::restoreState() {
    if (settings) {
        isCurrentlyVisible = settings->value("SideBar/visible", true).toBool();
        preferredWidth = settings->value("SideBar/width", defaultWidth).toInt();
        preferredWidth = qBound(minimumWidth, preferredWidth, maximumWidth);

        setVisible(isCurrentlyVisible,
                   false);  // Don't use animation when restoring
    }
}

void SideBar::onAnimationFinished() {
    if (!isCurrentlyVisible) {
        QWidget::setVisible(false);
    }
    emit visibilityChanged(isCurrentlyVisible);
}

void SideBar::setOutlineModel(PDFOutlineModel* model) {
    if (outlineWidget) {
        outlineWidget->setOutlineModel(model);
    }
}

void SideBar::setDocument(std::shared_ptr<Poppler::Document> document) {
    if (thumbnailModel) {
        thumbnailModel->setDocument(document);
    }
}

void SideBar::setThumbnailSize(const QSize& size) {
    if (thumbnailView) {
        thumbnailView->setThumbnailSize(size);
    }
    if (thumbnailModel) {
        thumbnailModel->setThumbnailSize(size);
    }
    if (thumbnailDelegate) {
        thumbnailDelegate->setThumbnailSize(size);
    }
    emit thumbnailSizeChanged(size);
}

void SideBar::refreshThumbnails() {
    if (thumbnailModel) {
        thumbnailModel->refreshAllThumbnails();
    }
}

#include "WelcomeScreenManager.h"
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include "../../MainWindow.h"
#include "../../model/DocumentModel.h"
#include "../widgets/WelcomeWidget.h"

const QString WelcomeScreenManager::SETTINGS_GROUP = "ui";
const QString WelcomeScreenManager::SETTINGS_ENABLED_KEY = "showWelcomeScreen";
const QString WelcomeScreenManager::SETTINGS_SHOW_ON_STARTUP_KEY =
    "showWelcomeScreenOnStartup";

WelcomeScreenManager::WelcomeScreenManager(QObject* parent)
    : QObject(parent),
      m_mainWindow(nullptr),
      m_welcomeWidget(nullptr),
      m_documentModel(nullptr),
      m_settings(nullptr),
      m_welcomeScreenEnabled(DEFAULT_ENABLED),
      m_welcomeScreenVisible(false),
      m_isInitialized(false),
      m_visibilityCheckTimer(nullptr) {
    qDebug() << "WelcomeScreenManager: Initializing...";

    initializeSettings();

    // 创建延迟检查定时器
    m_visibilityCheckTimer = new QTimer(this);
    m_visibilityCheckTimer->setSingleShot(true);
    m_visibilityCheckTimer->setInterval(VISIBILITY_CHECK_DELAY);
    connect(m_visibilityCheckTimer, &QTimer::timeout, this,
            &WelcomeScreenManager::onDelayedVisibilityCheck);

    m_isInitialized = true;
    qDebug() << "WelcomeScreenManager: Initialization completed";
}

WelcomeScreenManager::~WelcomeScreenManager() {
    qDebug() << "WelcomeScreenManager: Destroying...";
    saveSettings();
}

void WelcomeScreenManager::setMainWindow(MainWindow* mainWindow) {
    if (m_mainWindow == mainWindow)
        return;

    // 断开旧连接
    if (m_mainWindow) {
        // 这里可以添加断开连接的代码
    }

    m_mainWindow = mainWindow;

    // 建立新连接
    if (m_mainWindow) {
        setupConnections();
    }

    qDebug() << "WelcomeScreenManager: MainWindow set";
}

void WelcomeScreenManager::setWelcomeWidget(WelcomeWidget* welcomeWidget) {
    if (m_welcomeWidget == welcomeWidget)
        return;

    m_welcomeWidget = welcomeWidget;

    if (m_welcomeWidget) {
        m_welcomeWidget->setWelcomeScreenManager(this);
    }

    qDebug() << "WelcomeScreenManager: WelcomeWidget set";
}

void WelcomeScreenManager::setDocumentModel(DocumentModel* documentModel) {
    if (m_documentModel == documentModel)
        return;

    // 断开旧连接
    if (m_documentModel) {
        disconnect(m_documentModel, nullptr, this, nullptr);
    }

    m_documentModel = documentModel;

    // 建立新连接
    if (m_documentModel) {
        connect(m_documentModel, &DocumentModel::documentOpened, this,
                &WelcomeScreenManager::onDocumentOpened);
        connect(m_documentModel, &DocumentModel::documentClosed, this,
                &WelcomeScreenManager::onDocumentClosed);
        connect(m_documentModel, &DocumentModel::currentDocumentChanged, this,
                &WelcomeScreenManager::onDocumentModelChanged);
        connect(m_documentModel, &DocumentModel::allDocumentsClosed, this,
                &WelcomeScreenManager::onDocumentModelChanged);
    }

    qDebug() << "WelcomeScreenManager: DocumentModel set";
}

bool WelcomeScreenManager::isWelcomeScreenEnabled() const {
    return m_welcomeScreenEnabled;
}

void WelcomeScreenManager::setWelcomeScreenEnabled(bool enabled) {
    if (m_welcomeScreenEnabled == enabled)
        return;

    m_welcomeScreenEnabled = enabled;

    qDebug() << "WelcomeScreenManager: Welcome screen enabled changed to:"
             << enabled;

    // 保存设置
    if (m_settings) {
        m_settings->setValue(SETTINGS_GROUP + "/" + SETTINGS_ENABLED_KEY,
                             enabled);
        m_settings->sync();
    }

    // 更新可见性
    updateWelcomeScreenVisibility();

    emit welcomeScreenEnabledChanged(enabled);
}

bool WelcomeScreenManager::shouldShowWelcomeScreen() const {
    return m_welcomeScreenEnabled && !hasOpenDocuments();
}

void WelcomeScreenManager::showWelcomeScreen() {
    if (m_welcomeScreenVisible || !m_welcomeScreenEnabled)
        return;

    qDebug() << "WelcomeScreenManager: Showing welcome screen";

    m_welcomeScreenVisible = true;
    emit showWelcomeScreenRequested();
    emit welcomeScreenVisibilityChanged(true);
}

void WelcomeScreenManager::hideWelcomeScreen() {
    if (!m_welcomeScreenVisible)
        return;

    qDebug() << "WelcomeScreenManager: Hiding welcome screen";

    m_welcomeScreenVisible = false;
    emit hideWelcomeScreenRequested();
    emit welcomeScreenVisibilityChanged(false);
}

bool WelcomeScreenManager::isWelcomeScreenVisible() const {
    return m_welcomeScreenVisible;
}

bool WelcomeScreenManager::hasOpenDocuments() const {
    // 安全检查：确保文档模型存在且不为空
    if (!m_documentModel) {
        qDebug() << "WelcomeScreenManager: DocumentModel is null, assuming no "
                    "documents";
        return false;
    }

    try {
        return !m_documentModel->isEmpty();
    } catch (const std::exception& e) {
        qDebug() << "WelcomeScreenManager: Exception checking document model:"
                 << e.what();
        return false;
    } catch (...) {
        qDebug() << "WelcomeScreenManager: Unknown exception checking document "
                    "model";
        return false;
    }
}

void WelcomeScreenManager::loadSettings() {
    if (!m_settings)
        return;

    qDebug() << "WelcomeScreenManager: Loading settings...";

    // Use hierarchical key format like other UI components
    m_welcomeScreenEnabled =
        m_settings
            ->value(SETTINGS_GROUP + "/" + SETTINGS_ENABLED_KEY,
                    DEFAULT_ENABLED)
            .toBool();

    qDebug() << "WelcomeScreenManager: Settings loaded - enabled:"
             << m_welcomeScreenEnabled;
}

void WelcomeScreenManager::saveSettings() {
    if (!m_settings)
        return;

    qDebug() << "WelcomeScreenManager: Saving settings...";

    // Use hierarchical key format like other UI components
    m_settings->setValue(SETTINGS_GROUP + "/" + SETTINGS_ENABLED_KEY,
                         m_welcomeScreenEnabled);
    m_settings->sync();

    qDebug() << "WelcomeScreenManager: Settings saved";
}

void WelcomeScreenManager::resetToDefaults() {
    qDebug() << "WelcomeScreenManager: Resetting to defaults";

    setWelcomeScreenEnabled(DEFAULT_ENABLED);
}

void WelcomeScreenManager::onApplicationStartup() {
    qDebug() << "WelcomeScreenManager: Application startup";

    loadSettings();
    scheduleVisibilityCheck();
}

void WelcomeScreenManager::onApplicationShutdown() {
    qDebug() << "WelcomeScreenManager: Application shutdown";

    saveSettings();
}

void WelcomeScreenManager::onDocumentOpened() {
    qDebug() << "WelcomeScreenManager: Document opened";
    scheduleVisibilityCheck();
}

void WelcomeScreenManager::onDocumentClosed() {
    qDebug() << "WelcomeScreenManager: Document closed";
    scheduleVisibilityCheck();
}

void WelcomeScreenManager::onAllDocumentsClosed() {
    qDebug() << "WelcomeScreenManager: All documents closed";
    scheduleVisibilityCheck();
}

void WelcomeScreenManager::onDocumentModelChanged() {
    qDebug() << "WelcomeScreenManager: Document model changed";
    scheduleVisibilityCheck();
}

void WelcomeScreenManager::onWelcomeScreenToggleRequested() {
    qDebug() << "WelcomeScreenManager: Welcome screen toggle requested";
    setWelcomeScreenEnabled(!m_welcomeScreenEnabled);
}

void WelcomeScreenManager::checkWelcomeScreenVisibility() {
    updateWelcomeScreenVisibility();
}

void WelcomeScreenManager::onDelayedVisibilityCheck() {
    qDebug() << "WelcomeScreenManager: Performing delayed visibility check";
    updateWelcomeScreenVisibility();
}

void WelcomeScreenManager::initializeSettings() {
    // Use consistent organization and application name like
    // AccessibilityManager
    m_settings = new QSettings("SAST", "Readium", this);
    loadSettings();
}

void WelcomeScreenManager::setupConnections() {
    // 这里可以添加与MainWindow的连接
    // 目前MainWindow的信号还没有定义，所以暂时留空
}

void WelcomeScreenManager::updateWelcomeScreenVisibility() {
    bool shouldShow = shouldShowWelcomeScreen();

    qDebug() << "WelcomeScreenManager: Updating visibility - should show:"
             << shouldShow << "enabled:" << m_welcomeScreenEnabled
             << "has documents:" << hasOpenDocuments();

    if (shouldShow && !m_welcomeScreenVisible) {
        showWelcomeScreen();
    } else if (!shouldShow && m_welcomeScreenVisible) {
        hideWelcomeScreen();
    }
}

void WelcomeScreenManager::scheduleVisibilityCheck() {
    if (m_visibilityCheckTimer && !m_visibilityCheckTimer->isActive()) {
        m_visibilityCheckTimer->start();
    }
}

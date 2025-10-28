#include "KeyboardShortcutManager.h"
#include <QApplication>
#include <QWidget>
#include "../logging/SimpleLogging.h"

// Static instance
KeyboardShortcutManager* KeyboardShortcutManager::s_instance = nullptr;

KeyboardShortcutManager::KeyboardShortcutManager(QObject* parent)
    : QObject(parent),
      m_mainWindow(nullptr),
      m_currentContext(ShortcutContext::Global),
      m_accessibilityMode(false),
      m_logger("KeyboardShortcutManager") {
    m_logger.info("KeyboardShortcutManager initialized");
}

KeyboardShortcutManager& KeyboardShortcutManager::instance() {
    if (!s_instance) {
        s_instance = new KeyboardShortcutManager();
    }
    return *s_instance;
}

void KeyboardShortcutManager::initialize(QWidget* mainWindow) {
    if (m_mainWindow) {
        m_logger.warning("KeyboardShortcutManager already initialized");
        return;
    }

    m_mainWindow = mainWindow;
    m_logger.info("Initializing KeyboardShortcutManager with main window");

    // Connect to application focus changes
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        connect(app, &QApplication::focusChanged, this,
                &KeyboardShortcutManager::onFocusChanged);
    }

    // Setup default shortcuts
    setupDefaultShortcuts();

    m_logger.info("KeyboardShortcutManager initialized successfully");
}

bool KeyboardShortcutManager::registerShortcut(
    const ShortcutInfo& shortcutInfo) {
    QString key =
        getShortcutKey(shortcutInfo.keySequence, shortcutInfo.context);

    // Check for conflicts
    if (m_shortcuts.contains(key)) {
        m_logger.warning(QString("Shortcut conflict detected: %1 in context %2")
                             .arg(shortcutInfo.keySequence.toString())
                             .arg(contextToString(shortcutInfo.context)));
        return false;
    }

    // Validate key sequence
    if (!isValidKeySequence(shortcutInfo.keySequence)) {
        m_logger.error(QString("Invalid key sequence: %1")
                           .arg(shortcutInfo.keySequence.toString()));
        return false;
    }

    // Create QShortcut object
    QShortcut* qshortcut = createShortcut(shortcutInfo);
    if (!qshortcut) {
        m_logger.error("Failed to create QShortcut object");
        return false;
    }

    // Store shortcut info and QShortcut object
    m_shortcuts[key] = shortcutInfo;
    m_qshortcuts[key] = qshortcut;

    m_logger.debug(QString("Registered shortcut: %1 -> %2 in context %3")
                       .arg(shortcutInfo.keySequence.toString())
                       .arg(static_cast<int>(shortcutInfo.action))
                       .arg(contextToString(shortcutInfo.context)));

    emit shortcutsChanged();
    return true;
}

bool KeyboardShortcutManager::unregisterShortcut(
    const QKeySequence& keySequence, ShortcutContext context) {
    QString key = getShortcutKey(keySequence, context);

    if (!m_shortcuts.contains(key)) {
        m_logger.warning(QString("Shortcut not found: %1 in context %2")
                             .arg(keySequence.toString())
                             .arg(contextToString(context)));
        return false;
    }

    // Delete QShortcut object
    if (m_qshortcuts.contains(key)) {
        delete m_qshortcuts[key];
        m_qshortcuts.remove(key);
    }

    m_shortcuts.remove(key);

    m_logger.debug(QString("Unregistered shortcut: %1 from context %2")
                       .arg(keySequence.toString())
                       .arg(contextToString(context)));

    emit shortcutsChanged();
    return true;
}

void KeyboardShortcutManager::setShortcutEnabled(
    const QKeySequence& keySequence, ShortcutContext context, bool enabled) {
    QString key = getShortcutKey(keySequence, context);

    if (!m_shortcuts.contains(key)) {
        m_logger.warning(
            QString("Shortcut not found: %1").arg(keySequence.toString()));
        return;
    }

    m_shortcuts[key].enabled = enabled;

    if (m_qshortcuts.contains(key)) {
        m_qshortcuts[key]->setEnabled(enabled);
    }

    m_logger.debug(QString("Shortcut %1: %2")
                       .arg(keySequence.toString())
                       .arg(enabled ? "enabled" : "disabled"));
}

QList<KeyboardShortcutManager::ShortcutInfo>
KeyboardShortcutManager::getShortcuts(ShortcutContext context) const {
    QList<ShortcutInfo> result;

    for (const auto& info : m_shortcuts) {
        if (info.context == context) {
            result.append(info);
        }
    }

    return result;
}

QKeySequence KeyboardShortcutManager::getShortcutForAction(
    ActionMap action, ShortcutContext context) const {
    for (const auto& info : m_shortcuts) {
        if (info.action == action && info.context == context) {
            return info.keySequence;
        }
    }

    return QKeySequence();
}

bool KeyboardShortcutManager::hasConflict(const QKeySequence& keySequence,
                                          ShortcutContext context) const {
    QString key = getShortcutKey(keySequence, context);
    return m_shortcuts.contains(key);
}

void KeyboardShortcutManager::setupDefaultShortcuts() {
    m_logger.info("Setting up default shortcuts");

    setupFileOperationShortcuts();
    setupNavigationShortcuts();
    setupZoomShortcuts();
    setupAccessibilityShortcuts();

    m_logger.info(
        QString("Registered %1 default shortcuts").arg(m_shortcuts.size()));
}

void KeyboardShortcutManager::setupFileOperationShortcuts() {
    if (!m_mainWindow)
        return;

    // File operations
    registerShortcut(ShortcutInfo(
        QKeySequence::Open, ActionMap::openFile, ShortcutContext::Global,
        ShortcutPriority::High, tr("Open file"), m_mainWindow));

    registerShortcut(ShortcutInfo(
        QKeySequence::Save, ActionMap::saveAs, ShortcutContext::Global,
        ShortcutPriority::High, tr("Save as"), m_mainWindow));

    registerShortcut(ShortcutInfo(
        QKeySequence::Print, ActionMap::printFile, ShortcutContext::Global,
        ShortcutPriority::High, tr("Print document"), m_mainWindow));

    registerShortcut(ShortcutInfo(
        QKeySequence::Close, ActionMap::closeFile, ShortcutContext::Global,
        ShortcutPriority::Normal, tr("Close document"), m_mainWindow));

    registerShortcut(ShortcutInfo(
        QKeySequence::Quit, ActionMap::quit, ShortcutContext::Global,
        ShortcutPriority::Critical, tr("Quit application"), m_mainWindow));
}

void KeyboardShortcutManager::setupNavigationShortcuts() {
    if (!m_mainWindow)
        return;

    // Page navigation
    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::Key_PageDown), ActionMap::nextPage,
                     ShortcutContext::DocumentView, ShortcutPriority::High,
                     tr("Next page"), m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::Key_PageUp), ActionMap::previousPage,
                     ShortcutContext::DocumentView, ShortcutPriority::High,
                     tr("Previous page"), m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::CTRL | Qt::Key_Home),
                     ActionMap::firstPage, ShortcutContext::DocumentView,
                     ShortcutPriority::Normal, tr("First page"), m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::CTRL | Qt::Key_End), ActionMap::lastPage,
                     ShortcutContext::DocumentView, ShortcutPriority::Normal,
                     tr("Last page"), m_mainWindow));

    // Search
    registerShortcut(ShortcutInfo(
        QKeySequence::Find, ActionMap::toggleSearch, ShortcutContext::Global,
        ShortcutPriority::High, tr("Find in document"), m_mainWindow));

    registerShortcut(ShortcutInfo(QKeySequence::FindNext, ActionMap::findNext,
                                  ShortcutContext::SearchWidget,
                                  ShortcutPriority::High, tr("Find next"),
                                  m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence::FindPrevious, ActionMap::findPrevious,
                     ShortcutContext::SearchWidget, ShortcutPriority::High,
                     tr("Find previous"), m_mainWindow));
}

void KeyboardShortcutManager::setupZoomShortcuts() {
    if (!m_mainWindow)
        return;

    // Zoom operations
    registerShortcut(ShortcutInfo(
        QKeySequence::ZoomIn, ActionMap::zoomIn, ShortcutContext::DocumentView,
        ShortcutPriority::High, tr("Zoom in"), m_mainWindow));

    registerShortcut(ShortcutInfo(QKeySequence::ZoomOut, ActionMap::zoomOut,
                                  ShortcutContext::DocumentView,
                                  ShortcutPriority::High, tr("Zoom out"),
                                  m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::CTRL | Qt::Key_0), ActionMap::fitToPage,
                     ShortcutContext::DocumentView, ShortcutPriority::Normal,
                     tr("Fit to page"), m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::CTRL | Qt::Key_1), ActionMap::fitToWidth,
                     ShortcutContext::DocumentView, ShortcutPriority::Normal,
                     tr("Fit to width"), m_mainWindow));

    // Rotation
    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::CTRL | Qt::Key_L), ActionMap::rotateLeft,
                     ShortcutContext::DocumentView, ShortcutPriority::Normal,
                     tr("Rotate left"), m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::CTRL | Qt::Key_R), ActionMap::rotateRight,
                     ShortcutContext::DocumentView, ShortcutPriority::Normal,
                     tr("Rotate right"), m_mainWindow));
}

void KeyboardShortcutManager::setupAccessibilityShortcuts() {
    if (!m_mainWindow)
        return;

    // Accessibility shortcuts
    registerShortcut(ShortcutInfo(
        QKeySequence(Qt::Key_F1), ActionMap::showHelp, ShortcutContext::Global,
        ShortcutPriority::Normal, tr("Show help"), m_mainWindow));

    registerShortcut(
        ShortcutInfo(QKeySequence(Qt::Key_F11), ActionMap::fullScreen,
                     ShortcutContext::Global, ShortcutPriority::Normal,
                     tr("Toggle fullscreen"), m_mainWindow));
}

void KeyboardShortcutManager::setAccessibilityMode(bool enabled) {
    if (m_accessibilityMode != enabled) {
        m_accessibilityMode = enabled;
        m_logger.info(QString("Accessibility mode %1")
                          .arg(enabled ? "enabled" : "disabled"));
        emit accessibilityModeChanged(enabled);
    }
}

void KeyboardShortcutManager::setContextWidget(ShortcutContext context,
                                               QWidget* widget) {
    m_contextWidgets[context] = widget;
    m_logger.debug(
        QString("Set context widget for %1").arg(contextToString(context)));
}

QWidget* KeyboardShortcutManager::getContextWidget(
    ShortcutContext context) const {
    return m_contextWidgets.value(context, nullptr);
}

void KeyboardShortcutManager::onShortcutActivated() {
    QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
    if (!shortcut) {
        m_logger.error("onShortcutActivated called without QShortcut sender");
        return;
    }

    // Find the shortcut info
    for (auto it = m_qshortcuts.begin(); it != m_qshortcuts.end(); ++it) {
        if (it.value() == shortcut) {
            QString key = it.key();
            if (m_shortcuts.contains(key)) {
                const ShortcutInfo& info = m_shortcuts[key];

                // Check if shortcut is enabled
                if (!info.enabled) {
                    m_logger.debug(QString("Shortcut %1 is disabled")
                                       .arg(info.keySequence.toString()));
                    return;
                }

                // Check context
                if (info.context != ShortcutContext::Global) {
                    ShortcutContext currentCtx = getCurrentContext();
                    if (currentCtx != info.context) {
                        m_logger.debug(
                            QString("Shortcut %1 not active in current context")
                                .arg(info.keySequence.toString()));
                        return;
                    }
                }

                m_logger.info(QString("Shortcut activated: %1 -> action %2")
                                  .arg(info.keySequence.toString())
                                  .arg(static_cast<int>(info.action)));

                emit shortcutActivated(info.action, info.context);
                return;
            }
        }
    }

    m_logger.warning("Activated shortcut not found in registry");
}

void KeyboardShortcutManager::onFocusChanged(QWidget* old, QWidget* now) {
    Q_UNUSED(old);

    if (!now) {
        return;
    }

    // Determine new context
    ShortcutContext newContext = getCurrentContext();

    if (newContext != m_currentContext) {
        m_logger.debug(QString("Context changed: %1 -> %2")
                           .arg(contextToString(m_currentContext))
                           .arg(contextToString(newContext)));
        m_currentContext = newContext;
        updateShortcutsForContext(newContext);
    }
}

QShortcut* KeyboardShortcutManager::createShortcut(const ShortcutInfo& info) {
    QWidget* parent = info.contextWidget ? info.contextWidget : m_mainWindow;
    if (!parent) {
        m_logger.error("No parent widget available for shortcut");
        return nullptr;
    }

    QShortcut* shortcut = new QShortcut(info.keySequence, parent);
    shortcut->setContext(info.context == ShortcutContext::Global
                             ? Qt::ApplicationShortcut
                             : Qt::WidgetWithChildrenShortcut);
    shortcut->setEnabled(info.enabled);

    connect(shortcut, &QShortcut::activated, this,
            &KeyboardShortcutManager::onShortcutActivated);

    return shortcut;
}

void KeyboardShortcutManager::updateShortcutsForContext(
    ShortcutContext context) {
    // Enable/disable shortcuts based on context
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        const ShortcutInfo& info = it.value();
        QString key = it.key();

        if (m_qshortcuts.contains(key)) {
            QShortcut* qshortcut = m_qshortcuts[key];

            // Global shortcuts are always active
            if (info.context == ShortcutContext::Global) {
                qshortcut->setEnabled(info.enabled);
            }
            // Context-specific shortcuts only active in their context
            else {
                bool shouldBeEnabled =
                    (info.context == context) && info.enabled;
                qshortcut->setEnabled(shouldBeEnabled);
            }
        }
    }
}

KeyboardShortcutManager::ShortcutContext
KeyboardShortcutManager::getCurrentContext() const {
    QWidget* focusWidget = QApplication::focusWidget();
    if (!focusWidget) {
        return ShortcutContext::Global;
    }

    // Check registered context widgets
    for (auto it = m_contextWidgets.begin(); it != m_contextWidgets.end();
         ++it) {
        if (it.value() && (focusWidget == it.value() ||
                           focusWidget->isAncestorOf(it.value()))) {
            return it.key();
        }
    }

    // Default to document view if focused widget is unknown
    return ShortcutContext::DocumentView;
}

bool KeyboardShortcutManager::isValidKeySequence(
    const QKeySequence& keySequence) const {
    return !keySequence.isEmpty();
}

QString KeyboardShortcutManager::getShortcutKey(const QKeySequence& keySequence,
                                                ShortcutContext context) const {
    return QString("%1:%2")
        .arg(contextToString(context))
        .arg(keySequence.toString());
}

QString KeyboardShortcutManager::contextToString(
    ShortcutContext context) const {
    switch (context) {
        case ShortcutContext::Global:
            return "Global";
        case ShortcutContext::DocumentView:
            return "DocumentView";
        case ShortcutContext::MenuBar:
            return "MenuBar";
        case ShortcutContext::ToolBar:
            return "ToolBar";
        case ShortcutContext::SideBar:
            return "SideBar";
        case ShortcutContext::SearchWidget:
            return "SearchWidget";
        case ShortcutContext::Dialog:
            return "Dialog";
        default:
            return "Unknown";
    }
}

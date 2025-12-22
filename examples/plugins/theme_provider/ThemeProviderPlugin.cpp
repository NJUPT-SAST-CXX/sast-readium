#include "ThemeProviderPlugin.h"
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QStandardPaths>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

ThemeProviderPlugin::ThemeProviderPlugin(QObject* parent)
    : PluginBase(parent),
      m_activeTheme("light"),
      m_themeEditorAction(nullptr),
      m_themeChanges(0) {
    m_metadata.name = "Theme Provider";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Custom theme support with dynamic switching and persistence";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "theme.provider" << "theme.custom"
                              << "settings.persistence" << "ui.extension";
}

ThemeProviderPlugin::~ThemeProviderPlugin() { qDeleteAll(m_menuActions); }

bool ThemeProviderPlugin::onInitialize() {
    m_logger.info("ThemeProviderPlugin: Initializing...");

    // Load built-in themes
    loadBuiltinThemes();

    // Load custom themes from settings
    loadCustomThemes();

    // Get saved active theme
    m_activeTheme = m_configuration.value("activeTheme").toString("light");

    // Create UI actions
    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    for (auto it = m_themes.begin(); it != m_themes.end(); ++it) {
        QAction* action = new QAction(it.value().displayName, this);
        action->setCheckable(true);
        action->setChecked(it.key() == m_activeTheme);
        action->setData(it.key());
        themeGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, action]() {
            onThemeSelected(action->data().toString());
        });
        m_menuActions.append(action);
    }

    // Add separator and theme editor action
    QAction* separator = new QAction(this);
    separator->setSeparator(true);
    m_menuActions.append(separator);

    m_themeEditorAction = new QAction("Theme Editor...", this);
    connect(m_themeEditorAction, &QAction::triggered, this,
            &ThemeProviderPlugin::onOpenThemeEditor);
    m_menuActions.append(m_themeEditorAction);

    // Register hooks
    registerHooks();

    // Setup event subscriptions
    setupEventSubscriptions();

    // Apply initial theme
    applyTheme(m_activeTheme);

    m_logger.info(QString("ThemeProviderPlugin: Loaded %1 themes, active: %2")
                      .arg(m_themes.size())
                      .arg(m_activeTheme));
    return true;
}

void ThemeProviderPlugin::onShutdown() {
    m_logger.info("ThemeProviderPlugin: Shutting down...");

    // Save custom themes
    saveCustomThemes();

    // Unregister
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    eventBus()->unsubscribeAll(this);

    m_logger.info(
        QString("ThemeProviderPlugin: Theme changes: %1").arg(m_themeChanges));
}

void ThemeProviderPlugin::handleMessage(const QString& from,
                                        const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "get_themes") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["themes"] = QStringList(m_themes.keys());
        data["activeTheme"] = m_activeTheme;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "set_theme") {
        QString themeName = msgMap.value("theme").toString();
        if (m_themes.contains(themeName)) {
            applyTheme(themeName);
        }

    } else if (action == "register_theme") {
        ThemeDefinition theme;
        theme.name = msgMap.value("name").toString();
        theme.displayName = msgMap.value("displayName").toString();
        theme.backgroundColor =
            QColor(msgMap.value("backgroundColor").toString());
        theme.textColor = QColor(msgMap.value("textColor").toString());
        theme.accentColor = QColor(msgMap.value("accentColor").toString());
        theme.isDark = msgMap.value("isDark").toBool();

        if (!theme.name.isEmpty()) {
            m_themes[theme.name] = theme;
            m_logger.info(QString("ThemeProviderPlugin: Registered theme '%1'")
                              .arg(theme.name));
        }
    }
}

// ============================================================================
// IUIExtension Implementation
// ============================================================================

QList<QAction*> ThemeProviderPlugin::menuActions() const {
    return m_menuActions;
}

QList<QAction*> ThemeProviderPlugin::toolbarActions() const {
    // Return just a single theme toggle action for toolbar
    return QList<QAction*>();
}

QString ThemeProviderPlugin::statusBarMessage() const {
    if (m_themes.contains(m_activeTheme)) {
        return QString("Theme: %1").arg(m_themes[m_activeTheme].displayName);
    }
    return QString();
}

// ============================================================================
// Theme Management
// ============================================================================

void ThemeProviderPlugin::loadBuiltinThemes() {
    // Light theme
    ThemeDefinition light;
    light.name = "light";
    light.displayName = "Light";
    light.backgroundColor = QColor("#FFFFFF");
    light.textColor = QColor("#212121");
    light.accentColor = QColor("#2196F3");
    light.highlightColor = QColor("#BBDEFB");
    light.borderColor = QColor("#E0E0E0");
    light.isDark = false;
    m_themes["light"] = light;

    // Dark theme
    ThemeDefinition dark;
    dark.name = "dark";
    dark.displayName = "Dark";
    dark.backgroundColor = QColor("#1E1E1E");
    dark.textColor = QColor("#E0E0E0");
    dark.accentColor = QColor("#64B5F6");
    dark.highlightColor = QColor("#424242");
    dark.borderColor = QColor("#424242");
    dark.isDark = true;
    m_themes["dark"] = dark;

    // Sepia theme
    ThemeDefinition sepia;
    sepia.name = "sepia";
    sepia.displayName = "Sepia";
    sepia.backgroundColor = QColor("#F5E6D3");
    sepia.textColor = QColor("#5B4636");
    sepia.accentColor = QColor("#8B7355");
    sepia.highlightColor = QColor("#E8D4BC");
    sepia.borderColor = QColor("#C9B99A");
    sepia.isDark = false;
    m_themes["sepia"] = sepia;

    // High Contrast theme
    ThemeDefinition highContrast;
    highContrast.name = "high_contrast";
    highContrast.displayName = "High Contrast";
    highContrast.backgroundColor = QColor("#000000");
    highContrast.textColor = QColor("#FFFFFF");
    highContrast.accentColor = QColor("#FFFF00");
    highContrast.highlightColor = QColor("#0000FF");
    highContrast.borderColor = QColor("#FFFFFF");
    highContrast.isDark = true;
    m_themes["high_contrast"] = highContrast;

    // Nord theme
    ThemeDefinition nord;
    nord.name = "nord";
    nord.displayName = "Nord";
    nord.backgroundColor = QColor("#2E3440");
    nord.textColor = QColor("#ECEFF4");
    nord.accentColor = QColor("#88C0D0");
    nord.highlightColor = QColor("#4C566A");
    nord.borderColor = QColor("#3B4252");
    nord.isDark = true;
    m_themes["nord"] = nord;

    // Solarized Light
    ThemeDefinition solarizedLight;
    solarizedLight.name = "solarized_light";
    solarizedLight.displayName = "Solarized Light";
    solarizedLight.backgroundColor = QColor("#FDF6E3");
    solarizedLight.textColor = QColor("#657B83");
    solarizedLight.accentColor = QColor("#268BD2");
    solarizedLight.highlightColor = QColor("#EEE8D5");
    solarizedLight.borderColor = QColor("#93A1A1");
    solarizedLight.isDark = false;
    m_themes["solarized_light"] = solarizedLight;
}

void ThemeProviderPlugin::loadCustomThemes() {
    QString themesPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/themes.json";
    QFile file(themesPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonArray themesArray = doc.object()["themes"].toArray();
    for (const auto& val : themesArray) {
        QJsonObject obj = val.toObject();
        ThemeDefinition theme;
        theme.name = obj["name"].toString();
        theme.displayName = obj["displayName"].toString();
        theme.backgroundColor = QColor(obj["backgroundColor"].toString());
        theme.textColor = QColor(obj["textColor"].toString());
        theme.accentColor = QColor(obj["accentColor"].toString());
        theme.highlightColor = QColor(obj["highlightColor"].toString());
        theme.borderColor = QColor(obj["borderColor"].toString());
        theme.customStyleSheet = obj["customStyleSheet"].toString();
        theme.isDark = obj["isDark"].toBool();

        if (!theme.name.isEmpty()) {
            m_themes[theme.name] = theme;
        }
    }
}

void ThemeProviderPlugin::saveCustomThemes() {
    QString themesPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/themes.json";

    QJsonArray themesArray;
    for (auto it = m_themes.begin(); it != m_themes.end(); ++it) {
        // Only save custom themes (not built-in)
        if (it.key().startsWith("custom_")) {
            QJsonObject obj;
            obj["name"] = it.value().name;
            obj["displayName"] = it.value().displayName;
            obj["backgroundColor"] = it.value().backgroundColor.name();
            obj["textColor"] = it.value().textColor.name();
            obj["accentColor"] = it.value().accentColor.name();
            obj["highlightColor"] = it.value().highlightColor.name();
            obj["borderColor"] = it.value().borderColor.name();
            obj["customStyleSheet"] = it.value().customStyleSheet;
            obj["isDark"] = it.value().isDark;
            themesArray.append(obj);
        }
    }

    if (themesArray.isEmpty()) {
        return;
    }

    QJsonObject root;
    root["themes"] = themesArray;

    QFile file(themesPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void ThemeProviderPlugin::applyTheme(const QString& themeName) {
    if (!m_themes.contains(themeName)) {
        m_logger.warning(QString("ThemeProviderPlugin: Theme '%1' not found")
                             .arg(themeName));
        return;
    }

    m_activeTheme = themeName;
    const ThemeDefinition& theme = m_themes[themeName];

    // Generate and apply stylesheet
    QString styleSheet = generateStyleSheet(theme);

    // Publish theme changed event
    Event* event = new Event("theme.changed");
    QVariantMap data;
    data["themeName"] = themeName;
    data["displayName"] = theme.displayName;
    data["isDark"] = theme.isDark;
    data["backgroundColor"] = theme.backgroundColor.name();
    data["textColor"] = theme.textColor.name();
    data["accentColor"] = theme.accentColor.name();
    data["styleSheet"] = styleSheet;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);

    // Update action states
    for (QAction* action : m_menuActions) {
        if (action->data().toString() == themeName) {
            action->setChecked(true);
        }
    }

    m_themeChanges++;
    m_logger.info(
        QString("ThemeProviderPlugin: Applied theme '%1'").arg(themeName));
}

QString ThemeProviderPlugin::generateStyleSheet(
    const ThemeDefinition& theme) const {
    QString css;

    // Main window
    css += QString("QMainWindow { background-color: %1; color: %2; }\n")
               .arg(theme.backgroundColor.name())
               .arg(theme.textColor.name());

    // Widgets
    css += QString("QWidget { background-color: %1; color: %2; }\n")
               .arg(theme.backgroundColor.name())
               .arg(theme.textColor.name());

    // Buttons
    css += QString(
               "QPushButton { background-color: %1; color: %2; border: 1px "
               "solid %3; "
               "padding: 5px 15px; border-radius: 3px; }\n")
               .arg(theme.accentColor.name())
               .arg(theme.isDark ? "#FFFFFF" : "#FFFFFF")
               .arg(theme.accentColor.darker(120).name());

    css += QString("QPushButton:hover { background-color: %1; }\n")
               .arg(theme.accentColor.lighter(110).name());

    // Line edits
    css += QString(
               "QLineEdit { background-color: %1; color: %2; border: 1px solid "
               "%3; "
               "padding: 5px; border-radius: 3px; }\n")
               .arg(theme.backgroundColor.name())
               .arg(theme.textColor.name())
               .arg(theme.borderColor.name());

    // Lists and trees
    css += QString(
               "QListView, QTreeView { background-color: %1; color: %2; "
               "selection-background-color: %3; }\n")
               .arg(theme.backgroundColor.name())
               .arg(theme.textColor.name())
               .arg(theme.highlightColor.name());

    // Scrollbars
    css +=
        QString("QScrollBar:vertical { background-color: %1; width: 12px; }\n")
            .arg(theme.backgroundColor.name());
    css += QString(
               "QScrollBar::handle:vertical { background-color: %1; "
               "border-radius: 6px; }\n")
               .arg(theme.borderColor.name());

    // Append custom stylesheet if any
    if (!theme.customStyleSheet.isEmpty()) {
        css += theme.customStyleSheet;
    }

    return css;
}

// ============================================================================
// Slots
// ============================================================================

void ThemeProviderPlugin::onThemeSelected(const QString& themeName) {
    applyTheme(themeName);
}

void ThemeProviderPlugin::onOpenThemeEditor() {
    m_logger.info(
        "ThemeProviderPlugin: Theme editor requested (not implemented in "
        "example)");
    // In a real implementation, open a theme editor dialog
}

// ============================================================================
// Hook Registration
// ============================================================================

void ThemeProviderPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        "settings.changed", name(),
        [this](const QVariantMap& ctx) { return onSettingsChanged(ctx); });
}

void ThemeProviderPlugin::unregisterHooks() {
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
}

void ThemeProviderPlugin::setupEventSubscriptions() {
    eventBus()->subscribe("app.started", this, [this](Event*) {
        // Re-apply theme after app fully started
        applyTheme(m_activeTheme);
    });
}

QVariant ThemeProviderPlugin::onSettingsChanged(const QVariantMap& context) {
    QString key = context.value("key").toString();
    if (key == "theme" || key == "appearance.theme") {
        QString newTheme = context.value("value").toString();
        if (m_themes.contains(newTheme)) {
            applyTheme(newTheme);
        }
    }
    return QVariant();
}

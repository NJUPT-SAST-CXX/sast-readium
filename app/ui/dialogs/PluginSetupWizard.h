#pragma once

#include <QDialog>
#include <QPointer>
#include <QStackedWidget>
#include <memory>

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class ElaText;
class ElaPushButton;
class PluginConfigWidget;
class PluginConfigModel;
class PluginManager;

/**
 * @brief PluginSetupWizard - First-run configuration wizard for plugins
 *
 * This wizard guides users through the initial setup of a plugin when
 * it is first enabled. It focuses on required configuration settings
 * and provides a friendly step-by-step interface.
 *
 * Pages:
 * 1. Welcome page (plugin introduction)
 * 2. Required configuration page (only required settings)
 * 3. Optional configuration page (optional settings)
 * 4. Completion page (summary and finish)
 */
class PluginSetupWizard : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Wizard page enum
     */
    enum WizardPage {
        WelcomePage = 0,
        RequiredConfigPage = 1,
        OptionalConfigPage = 2,
        CompletionPage = 3,
        PageCount = 4
    };

    /**
     * @brief Constructor
     * @param pluginName Name of the plugin to configure
     * @param parent Parent widget
     */
    explicit PluginSetupWizard(const QString& pluginName,
                               QWidget* parent = nullptr);
    ~PluginSetupWizard() override;

    /**
     * @brief Get the plugin name
     * @return Plugin name
     */
    QString pluginName() const { return m_pluginName; }

    /**
     * @brief Check if wizard was completed successfully
     * @return True if configuration was saved
     */
    bool wasCompleted() const { return m_completed; }

    /**
     * @brief Static helper to show setup wizard
     * @param pluginName Plugin name
     * @param parent Parent widget
     * @return True if wizard was completed and configuration saved
     */
    static bool showSetupWizard(const QString& pluginName,
                                QWidget* parent = nullptr);

    /**
     * @brief Check if a plugin needs setup wizard
     * @param pluginName Plugin name
     * @return True if plugin has required config that is not set
     */
    static bool needsSetupWizard(const QString& pluginName);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onNextClicked();
    void onBackClicked();
    void onSkipClicked();
    void onFinishClicked();
    void onValidationStateChanged(bool isValid);

private:
    void setupUI();
    void createWelcomePage();
    void createRequiredConfigPage();
    void createOptionalConfigPage();
    void createCompletionPage();
    void setupNavigation();
    void retranslateUI();
    void loadPluginInfo();
    void goToPage(WizardPage page);
    void updateNavigation();
    bool validateCurrentPage();
    void saveConfiguration();

    // Plugin info
    QString m_pluginName;
    QPointer<PluginManager> m_pluginManager;

    // Model
    std::unique_ptr<PluginConfigModel> m_configModel;

    // UI Components - Main
    QVBoxLayout* m_mainLayout;
    QStackedWidget* m_stackedWidget;

    // UI Components - Welcome Page
    QWidget* m_welcomePage;
    ElaText* m_welcomeTitle;
    ElaText* m_welcomeDescription;
    ElaText* m_pluginInfoText;

    // UI Components - Required Config Page
    QWidget* m_requiredPage;
    ElaText* m_requiredTitle;
    ElaText* m_requiredDescription;
    PluginConfigWidget* m_requiredConfigWidget;

    // UI Components - Optional Config Page
    QWidget* m_optionalPage;
    ElaText* m_optionalTitle;
    ElaText* m_optionalDescription;
    PluginConfigWidget* m_optionalConfigWidget;

    // UI Components - Completion Page
    QWidget* m_completionPage;
    ElaText* m_completionTitle;
    ElaText* m_completionDescription;
    ElaText* m_summaryText;

    // UI Components - Navigation
    QWidget* m_navWidget;
    ElaPushButton* m_backBtn;
    ElaPushButton* m_nextBtn;
    ElaPushButton* m_skipBtn;
    ElaPushButton* m_finishBtn;
    ElaPushButton* m_cancelBtn;

    // UI Components - Progress
    ElaText* m_progressLabel;

    // State
    WizardPage m_currentPage;
    bool m_completed;
    bool m_hasRequiredConfig;
    bool m_hasOptionalConfig;
};

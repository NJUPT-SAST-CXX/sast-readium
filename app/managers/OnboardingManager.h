#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QSettings>
#include <memory>

class QWidget;
class OnboardingWidget;
class OnboardingManagerImpl;

/**
 * Onboarding steps definition
 */
enum class OnboardingStep : std::uint8_t {
    Welcome = 0,
    OpenFile,
    Navigation,
    Search,
    Bookmarks,
    Annotations,
    ViewModes,
    Settings,
    KeyboardShortcuts,
    Complete
};

/**
 * Onboarding tutorial categories
 */
enum class TutorialCategory : std::uint8_t {
    GettingStarted,
    BasicFeatures,
    AdvancedFeatures,
    ProductivityTips
};

/**
 * OnboardingManager
 *
 * Manages the onboarding experience for first-time users,
 * tracking progress and providing guided tours through the application.
 */
class OnboardingManager : public QObject {
    Q_OBJECT

public:
    explicit OnboardingManager(QObject* parent = nullptr);
    ~OnboardingManager() override;

    // Singleton instance
    static OnboardingManager& instance();

    // Onboarding control
    [[nodiscard]] bool isFirstTimeUser() const;
    [[nodiscard]] bool isOnboardingCompleted() const;
    [[nodiscard]] bool isOnboardingActive() const;

    void startOnboarding();
    void stopOnboarding();
    void resetOnboarding();
    void skipOnboarding();

    // Step management
    [[nodiscard]] OnboardingStep currentStep() const;
    void nextStep();
    void previousStep();
    void jumpToStep(OnboardingStep step);
    [[nodiscard]] bool isStepCompleted(OnboardingStep step) const;
    void markStepCompleted(OnboardingStep step);

    // Tutorial management
    void startTutorial(TutorialCategory category);
    void startSpecificTutorial(const QString& tutorialId);
    [[nodiscard]] QJsonArray getAvailableTutorials() const;
    [[nodiscard]] QJsonObject getTutorialInfo(const QString& tutorialId) const;

    // Progress tracking
    [[nodiscard]] int getCompletedStepsCount() const;
    [[nodiscard]] int getTotalStepsCount() const;
    [[nodiscard]] float getProgressPercentage() const;
    [[nodiscard]] QList<OnboardingStep> getCompletedSteps() const;
    [[nodiscard]] QList<OnboardingStep> getRemainingSteps() const;

    // Widget management
    void setOnboardingWidget(OnboardingWidget* widget);
    [[nodiscard]] OnboardingWidget* onboardingWidget() const;
    void attachToWidget(QWidget* widget);
    void detachFromWidget();

    // Settings management
    void loadSettings();
    void saveSettings();
    void resetSettings();

    // User preferences
    [[nodiscard]] bool shouldShowTips() const;
    void setShowTips(bool show);
    [[nodiscard]] bool shouldShowOnStartup() const;
    void setShowOnStartup(bool show);

    // Deleted copy/move operations (public for better error messages)
    OnboardingManager(const OnboardingManager&) = delete;
    OnboardingManager& operator=(const OnboardingManager&) = delete;
    OnboardingManager(OnboardingManager&&) = delete;
    OnboardingManager& operator=(OnboardingManager&&) = delete;

    // Analytics (for improvement tracking)
    void trackStepStarted(OnboardingStep step);
    void trackStepCompleted(OnboardingStep step);
    void trackStepSkipped(OnboardingStep step);
    void trackTutorialStarted(const QString& tutorialId);
    void trackTutorialCompleted(const QString& tutorialId);

signals:
    void onboardingStarted();
    void onboardingStopped();
    void onboardingCompleted();
    void onboardingSkipped();

    void stepChanged(OnboardingStep newStep);
    void stepCompleted(OnboardingStep step);
    void progressUpdated(float percentage);

    void tutorialStarted(const QString& tutorialId);
    void tutorialCompleted(const QString& tutorialId);

    void showTipsChanged(bool show);
    void showOnStartupChanged(bool show);

public slots:
    void onApplicationStarted();
    void onDocumentOpened();
    void onFeatureUsed(const QString& featureName);

private slots:
    void onStepTimeout();
    void updateProgress();

private:
    // Singleton instance
    static OnboardingManager* s_instance;

    std::unique_ptr<OnboardingManagerImpl> m_pImpl;

    // Constants (using constexpr to avoid static initialization exceptions)
    static constexpr const char* SETTINGS_GROUP = "Onboarding";
    static constexpr const char* SETTINGS_FIRST_TIME_KEY = "FirstTimeUser";
    static constexpr const char* SETTINGS_COMPLETED_KEY = "Completed";
    static constexpr const char* SETTINGS_COMPLETED_STEPS_KEY =
        "CompletedSteps";
    static constexpr const char* SETTINGS_SHOW_TIPS_KEY = "ShowTips";
    static constexpr const char* SETTINGS_SHOW_ON_STARTUP_KEY = "ShowOnStartup";
    static constexpr const char* SETTINGS_ANALYTICS_KEY = "Analytics";
};

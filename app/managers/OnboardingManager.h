#pragma once

#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

class QWidget;
class OnboardingWidget;
class OnboardingManagerImpl;

/**
 * Onboarding steps definition
 */
enum class OnboardingStep {
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
enum class TutorialCategory {
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
    ~OnboardingManager();

    // Singleton instance
    static OnboardingManager& instance();

    // Onboarding control
    bool isFirstTimeUser() const;
    bool isOnboardingCompleted() const;
    bool isOnboardingActive() const;
    
    void startOnboarding();
    void stopOnboarding();
    void resetOnboarding();
    void skipOnboarding();
    
    // Step management
    OnboardingStep currentStep() const;
    void nextStep();
    void previousStep();
    void jumpToStep(OnboardingStep step);
    bool isStepCompleted(OnboardingStep step) const;
    void markStepCompleted(OnboardingStep step);
    
    // Tutorial management
    void startTutorial(TutorialCategory category);
    void startSpecificTutorial(const QString& tutorialId);
    QJsonArray getAvailableTutorials() const;
    QJsonObject getTutorialInfo(const QString& tutorialId) const;
    
    // Progress tracking
    int getCompletedStepsCount() const;
    int getTotalStepsCount() const;
    float getProgressPercentage() const;
    QList<OnboardingStep> getCompletedSteps() const;
    QList<OnboardingStep> getRemainingSteps() const;
    
    // Widget management
    void setOnboardingWidget(OnboardingWidget* widget);
    OnboardingWidget* onboardingWidget() const;
    void attachToWidget(QWidget* widget);
    void detachFromWidget();
    
    // Settings management
    void loadSettings();
    void saveSettings();
    void resetSettings();
    
    // User preferences
    bool shouldShowTips() const;
    void setShowTips(bool show);
    bool shouldShowOnStartup() const;
    void setShowOnStartup(bool show);
    
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

    std::unique_ptr<OnboardingManagerImpl> pImpl;
    
    // Constants
    static const QString SETTINGS_GROUP;
    static const QString SETTINGS_FIRST_TIME_KEY;
    static const QString SETTINGS_COMPLETED_KEY;
    static const QString SETTINGS_COMPLETED_STEPS_KEY;
    static const QString SETTINGS_SHOW_TIPS_KEY;
    static const QString SETTINGS_SHOW_ON_STARTUP_KEY;
    static const QString SETTINGS_ANALYTICS_KEY;
};

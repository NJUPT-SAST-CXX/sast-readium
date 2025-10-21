#include "OnboardingManager.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QWidget>
#include "../ui/widgets/OnboardingWidget.h"

// Private implementation class
class OnboardingManagerImpl {
public:
    OnboardingManagerImpl() { m_settings = std::make_unique<QSettings>(); }

    void initializeSteps();
    void initializeTutorials();
    void setupConnections();
    [[nodiscard]] QString stepToString(OnboardingStep step) const;
    [[nodiscard]] OnboardingStep stringToStep(const QString& str) const;

    // State
    bool m_isActive{false};
    bool m_isFirstTimeUser{true};
    OnboardingStep m_currentStep{OnboardingStep::Welcome};
    QList<OnboardingStep> m_completedSteps;

    // Widgets
    OnboardingWidget* m_onboardingWidget{nullptr};
    QWidget* m_attachedWidget{nullptr};

    // Settings
    std::unique_ptr<QSettings> m_settings;
    bool m_showTips{true};
    bool m_showOnStartup{true};

    // Tutorials
    QJsonArray m_availableTutorials;
    QJsonObject m_tutorialData;

    // Analytics data
    QJsonObject m_analyticsData;
};

// Static member definitions
OnboardingManager* OnboardingManager::s_instance = nullptr;

OnboardingManager::OnboardingManager(QObject* parent)
    : QObject(parent), m_pImpl(std::make_unique<OnboardingManagerImpl>()) {
    m_pImpl->initializeSteps();
    m_pImpl->initializeTutorials();
    loadSettings();
    m_pImpl->setupConnections();
}

OnboardingManager::~OnboardingManager() { saveSettings(); }

OnboardingManager& OnboardingManager::instance() {
    if (!s_instance) {
        s_instance = new OnboardingManager();
    }
    return *s_instance;
}

bool OnboardingManager::isFirstTimeUser() const {
    return m_pImpl->m_isFirstTimeUser;
}

bool OnboardingManager::isOnboardingCompleted() const {
    return m_pImpl->m_completedSteps.size() >=
           getTotalStepsCount() - 1;  // Exclude Complete step
}

bool OnboardingManager::isOnboardingActive() const {
    return m_pImpl->m_isActive;
}

void OnboardingManager::startOnboarding() {
    if (m_pImpl->m_isActive) {
        return;
    }

    m_pImpl->m_isActive = true;
    m_pImpl->m_currentStep = OnboardingStep::Welcome;

    if (m_pImpl->m_onboardingWidget) {
        m_pImpl->m_onboardingWidget->showStep(m_pImpl->m_currentStep);
    }

    trackStepStarted(m_pImpl->m_currentStep);
    emit onboardingStarted();
    emit stepChanged(m_pImpl->m_currentStep);
}

void OnboardingManager::stopOnboarding() {
    if (!m_pImpl->m_isActive) {
        return;
    }

    m_pImpl->m_isActive = false;

    if (m_pImpl->m_onboardingWidget) {
        m_pImpl->m_onboardingWidget->hideStep();
    }

    saveSettings();
    emit onboardingStopped();
}

void OnboardingManager::resetOnboarding() {
    m_pImpl->m_completedSteps.clear();
    m_pImpl->m_currentStep = OnboardingStep::Welcome;
    m_pImpl->m_isFirstTimeUser = true;
    m_pImpl->m_analyticsData = QJsonObject();

    saveSettings();

    if (m_pImpl->m_isActive) {
        stopOnboarding();
        startOnboarding();
    }
}

void OnboardingManager::skipOnboarding() {
    if (!m_pImpl->m_isActive) {
        return;
    }

    // Mark all steps as completed
    for (int i = 0; i < static_cast<int>(OnboardingStep::Complete); ++i) {
        OnboardingStep step = static_cast<OnboardingStep>(i);
        if (!isStepCompleted(step)) {
            markStepCompleted(step);
            trackStepSkipped(step);
        }
    }

    m_pImpl->m_isFirstTimeUser = false;
    stopOnboarding();
    emit onboardingSkipped();
    emit onboardingCompleted();
}

OnboardingStep OnboardingManager::currentStep() const {
    return m_pImpl->m_currentStep;
}

void OnboardingManager::nextStep() {
    if (!m_pImpl->m_isActive) {
        return;
    }

    markStepCompleted(m_pImpl->m_currentStep);

    int nextStepInt = static_cast<int>(m_pImpl->m_currentStep) + 1;
    if (nextStepInt >= static_cast<int>(OnboardingStep::Complete)) {
        m_pImpl->m_currentStep = OnboardingStep::Complete;
        m_pImpl->m_isFirstTimeUser = false;
        stopOnboarding();
        emit onboardingCompleted();
    } else {
        m_pImpl->m_currentStep = static_cast<OnboardingStep>(nextStepInt);

        if (m_pImpl->m_onboardingWidget) {
            m_pImpl->m_onboardingWidget->showStep(m_pImpl->m_currentStep);
        }

        trackStepStarted(m_pImpl->m_currentStep);
        emit stepChanged(m_pImpl->m_currentStep);
    }

    updateProgress();
}

void OnboardingManager::previousStep() {
    if (!m_pImpl->m_isActive) {
        return;
    }

    int prevStepInt = static_cast<int>(m_pImpl->m_currentStep) - 1;
    if (prevStepInt < 0) {
        return;
    }

    m_pImpl->m_currentStep = static_cast<OnboardingStep>(prevStepInt);

    if (m_pImpl->m_onboardingWidget) {
        m_pImpl->m_onboardingWidget->showStep(m_pImpl->m_currentStep);
    }

    emit stepChanged(m_pImpl->m_currentStep);
}

void OnboardingManager::jumpToStep(OnboardingStep step) {
    if (!m_pImpl->m_isActive) {
        return;
    }

    m_pImpl->m_currentStep = step;

    if (m_pImpl->m_onboardingWidget) {
        m_pImpl->m_onboardingWidget->showStep(m_pImpl->m_currentStep);
    }

    trackStepStarted(m_pImpl->m_currentStep);
    emit stepChanged(m_pImpl->m_currentStep);
}

bool OnboardingManager::isStepCompleted(OnboardingStep step) const {
    return m_pImpl->m_completedSteps.contains(step);
}

void OnboardingManager::markStepCompleted(OnboardingStep step) {
    if (!m_pImpl->m_completedSteps.contains(step)) {
        m_pImpl->m_completedSteps.append(step);
        trackStepCompleted(step);
        emit stepCompleted(step);
        updateProgress();
        saveSettings();
    }
}

void OnboardingManager::startTutorial(TutorialCategory category) {
    QString tutorialId;

    switch (category) {
        case TutorialCategory::GettingStarted:
            tutorialId = "getting_started";
            break;
        case TutorialCategory::BasicFeatures:
            tutorialId = "basic_features";
            break;
        case TutorialCategory::AdvancedFeatures:
            tutorialId = "advanced_features";
            break;
        case TutorialCategory::ProductivityTips:
            tutorialId = "productivity_tips";
            break;
    }

    startSpecificTutorial(tutorialId);
}

void OnboardingManager::startSpecificTutorial(const QString& tutorialId) {
    trackTutorialStarted(tutorialId);

    // Map tutorial ID to specific onboarding step
    if (tutorialId == "open_file") {
        jumpToStep(OnboardingStep::OpenFile);
    } else if (tutorialId == "navigation") {
        jumpToStep(OnboardingStep::Navigation);
    } else if (tutorialId == "search") {
        jumpToStep(OnboardingStep::Search);
    } else if (tutorialId == "bookmarks") {
        jumpToStep(OnboardingStep::Bookmarks);
    } else if (tutorialId == "annotations") {
        jumpToStep(OnboardingStep::Annotations);
    } else if (tutorialId == "view_modes") {
        jumpToStep(OnboardingStep::ViewModes);
    } else if (tutorialId == "keyboard_shortcuts") {
        jumpToStep(OnboardingStep::KeyboardShortcuts);
    }

    if (!m_pImpl->m_isActive) {
        startOnboarding();
    }

    emit tutorialStarted(tutorialId);
}

QJsonArray OnboardingManager::getAvailableTutorials() const {
    return m_pImpl->m_availableTutorials;
}

QJsonObject OnboardingManager::getTutorialInfo(
    const QString& tutorialId) const {
    for (const auto& tutorial : m_pImpl->m_availableTutorials) {
        QJsonObject obj = tutorial.toObject();
        if (obj["id"].toString() == tutorialId) {
            return obj;
        }
    }
    return QJsonObject();
}

int OnboardingManager::getCompletedStepsCount() const {
    return static_cast<int>(m_pImpl->m_completedSteps.size());
}

int OnboardingManager::getTotalStepsCount() const {
    return static_cast<int>(OnboardingStep::Complete);
}

float OnboardingManager::getProgressPercentage() const {
    int total = getTotalStepsCount() - 1;  // Exclude Complete step
    if (total == 0) {
        return 100.0F;
    }
    return (static_cast<float>(getCompletedStepsCount()) /
            static_cast<float>(total)) *
           100.0F;
}

QList<OnboardingStep> OnboardingManager::getCompletedSteps() const {
    return m_pImpl->m_completedSteps;
}

QList<OnboardingStep> OnboardingManager::getRemainingSteps() const {
    QList<OnboardingStep> remaining;
    for (int i = 0; i < static_cast<int>(OnboardingStep::Complete); ++i) {
        auto step = static_cast<OnboardingStep>(i);
        if (!isStepCompleted(step)) {
            remaining.append(step);
        }
    }
    return remaining;
}

void OnboardingManager::setOnboardingWidget(OnboardingWidget* widget) {
    m_pImpl->m_onboardingWidget = widget;

    if (m_pImpl->m_onboardingWidget) {
        connect(m_pImpl->m_onboardingWidget, &OnboardingWidget::nextClicked,
                this, &OnboardingManager::nextStep);
        connect(m_pImpl->m_onboardingWidget, &OnboardingWidget::previousClicked,
                this, &OnboardingManager::previousStep);
        connect(m_pImpl->m_onboardingWidget, &OnboardingWidget::skipClicked,
                this, &OnboardingManager::skipOnboarding);
        connect(m_pImpl->m_onboardingWidget, &OnboardingWidget::closeClicked,
                this, &OnboardingManager::stopOnboarding);
    }
}

OnboardingWidget* OnboardingManager::onboardingWidget() const {
    return m_pImpl->m_onboardingWidget;
}

void OnboardingManager::attachToWidget(QWidget* widget) {
    m_pImpl->m_attachedWidget = widget;

    if (m_pImpl->m_onboardingWidget && m_pImpl->m_attachedWidget) {
        m_pImpl->m_onboardingWidget->setParent(m_pImpl->m_attachedWidget);
        m_pImpl->m_onboardingWidget->resize(m_pImpl->m_attachedWidget->size());
        m_pImpl->m_onboardingWidget->raise();
    }
}

void OnboardingManager::detachFromWidget() {
    if (m_pImpl->m_onboardingWidget) {
        m_pImpl->m_onboardingWidget->setParent(nullptr);
    }
    m_pImpl->m_attachedWidget = nullptr;
}

void OnboardingManager::loadSettings() {
    m_pImpl->m_settings->beginGroup(SETTINGS_GROUP);

    m_pImpl->m_isFirstTimeUser =
        m_pImpl->m_settings->value(SETTINGS_FIRST_TIME_KEY, true).toBool();
    m_pImpl->m_showTips =
        m_pImpl->m_settings->value(SETTINGS_SHOW_TIPS_KEY, true).toBool();
    m_pImpl->m_showOnStartup =
        m_pImpl->m_settings->value(SETTINGS_SHOW_ON_STARTUP_KEY, true).toBool();

    // Load completed steps
    QStringList completedStepStrings =
        m_pImpl->m_settings->value(SETTINGS_COMPLETED_STEPS_KEY).toStringList();
    m_pImpl->m_completedSteps.clear();
    for (const QString& stepStr : completedStepStrings) {
        m_pImpl->m_completedSteps.append(m_pImpl->stringToStep(stepStr));
    }

    // Load analytics data
    QString analyticsJson =
        m_pImpl->m_settings->value(SETTINGS_ANALYTICS_KEY).toString();
    if (!analyticsJson.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(analyticsJson.toUtf8());
        m_pImpl->m_analyticsData = doc.object();
    }

    m_pImpl->m_settings->endGroup();
}

void OnboardingManager::saveSettings() {
    m_pImpl->m_settings->beginGroup(SETTINGS_GROUP);

    m_pImpl->m_settings->setValue(SETTINGS_FIRST_TIME_KEY,
                                  m_pImpl->m_isFirstTimeUser);
    m_pImpl->m_settings->setValue(SETTINGS_COMPLETED_KEY,
                                  isOnboardingCompleted());
    m_pImpl->m_settings->setValue(SETTINGS_SHOW_TIPS_KEY, m_pImpl->m_showTips);
    m_pImpl->m_settings->setValue(SETTINGS_SHOW_ON_STARTUP_KEY,
                                  m_pImpl->m_showOnStartup);

    // Save completed steps
    QStringList completedStepStrings;
    for (OnboardingStep step : m_pImpl->m_completedSteps) {
        completedStepStrings.append(m_pImpl->stepToString(step));
    }
    m_pImpl->m_settings->setValue(SETTINGS_COMPLETED_STEPS_KEY,
                                  completedStepStrings);

    // Save analytics data
    QJsonDocument doc(m_pImpl->m_analyticsData);
    m_pImpl->m_settings->setValue(
        SETTINGS_ANALYTICS_KEY,
        QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    m_pImpl->m_settings->endGroup();
    m_pImpl->m_settings->sync();
}

void OnboardingManager::resetSettings() {
    m_pImpl->m_settings->beginGroup(SETTINGS_GROUP);
    m_pImpl->m_settings->remove("");
    m_pImpl->m_settings->endGroup();

    m_pImpl->m_isFirstTimeUser = true;
    m_pImpl->m_completedSteps.clear();
    m_pImpl->m_showTips = true;
    m_pImpl->m_showOnStartup = true;
    m_pImpl->m_analyticsData = QJsonObject();
}

bool OnboardingManager::shouldShowTips() const { return m_pImpl->m_showTips; }

void OnboardingManager::setShowTips(bool show) {
    if (m_pImpl->m_showTips != show) {
        m_pImpl->m_showTips = show;
        emit showTipsChanged(show);
        saveSettings();
    }
}

bool OnboardingManager::shouldShowOnStartup() const {
    return m_pImpl->m_showOnStartup;
}

void OnboardingManager::setShowOnStartup(bool show) {
    if (m_pImpl->m_showOnStartup != show) {
        m_pImpl->m_showOnStartup = show;
        emit showOnStartupChanged(show);
        saveSettings();
    }
}

void OnboardingManager::trackStepStarted(OnboardingStep step) {
    QString stepStr = m_pImpl->stepToString(step);
    QJsonObject stepData = m_pImpl->m_analyticsData[stepStr].toObject();
    stepData["started_count"] = stepData["started_count"].toInt() + 1;
    stepData["last_started"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    m_pImpl->m_analyticsData[stepStr] = stepData;
}

void OnboardingManager::trackStepCompleted(OnboardingStep step) {
    QString stepStr = m_pImpl->stepToString(step);
    QJsonObject stepData = m_pImpl->m_analyticsData[stepStr].toObject();
    stepData["completed_count"] = stepData["completed_count"].toInt() + 1;
    stepData["last_completed"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    m_pImpl->m_analyticsData[stepStr] = stepData;
}

void OnboardingManager::trackStepSkipped(OnboardingStep step) {
    QString stepStr = m_pImpl->stepToString(step);
    QJsonObject stepData = m_pImpl->m_analyticsData[stepStr].toObject();
    stepData["skipped_count"] = stepData["skipped_count"].toInt() + 1;
    stepData["last_skipped"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    m_pImpl->m_analyticsData[stepStr] = stepData;
}

void OnboardingManager::trackTutorialStarted(const QString& tutorialId) {
    QJsonObject tutorialData = m_pImpl->m_analyticsData["tutorials"].toObject();
    QJsonObject specific = tutorialData[tutorialId].toObject();
    specific["started_count"] = specific["started_count"].toInt() + 1;
    specific["last_started"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    tutorialData[tutorialId] = specific;
    m_pImpl->m_analyticsData["tutorials"] = tutorialData;
}

void OnboardingManager::trackTutorialCompleted(const QString& tutorialId) {
    QJsonObject tutorialData = m_pImpl->m_analyticsData["tutorials"].toObject();
    QJsonObject specific = tutorialData[tutorialId].toObject();
    specific["completed_count"] = specific["completed_count"].toInt() + 1;
    specific["last_completed"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);
    tutorialData[tutorialId] = specific;
    m_pImpl->m_analyticsData["tutorials"] = tutorialData;

    emit tutorialCompleted(tutorialId);
}

void OnboardingManager::onApplicationStarted() {
    if (m_pImpl->m_isFirstTimeUser && m_pImpl->m_showOnStartup) {
        // Start onboarding after a short delay to let the UI settle
        QTimer::singleShot(500, this, [this]() { startOnboarding(); });
    }
}

void OnboardingManager::onDocumentOpened() {
    if (m_pImpl->m_isActive &&
        m_pImpl->m_currentStep == OnboardingStep::OpenFile) {
        markStepCompleted(m_pImpl->m_currentStep);
        nextStep();
    }
}

void OnboardingManager::onFeatureUsed(const QString& featureName) {
    // Track feature usage and potentially advance onboarding
    if (!m_pImpl->m_isActive) {
        return;
    }

    if (featureName == "search" &&
        m_pImpl->m_currentStep == OnboardingStep::Search) {
        markStepCompleted(m_pImpl->m_currentStep);
    } else if (featureName == "bookmark" &&
               m_pImpl->m_currentStep == OnboardingStep::Bookmarks) {
        markStepCompleted(m_pImpl->m_currentStep);
    } else if (featureName == "annotation" &&
               m_pImpl->m_currentStep == OnboardingStep::Annotations) {
        markStepCompleted(m_pImpl->m_currentStep);
    }
}

void OnboardingManager::onStepTimeout() {
    // Handle timeout for automatic progression
    if (m_pImpl->m_isActive) {
        nextStep();
    }
}

void OnboardingManager::updateProgress() {
    float percentage = getProgressPercentage();
    emit progressUpdated(percentage);
}

// Note: initializeSteps, initializeTutorials, setupConnections, stepToString,
// and stringToStep are now implemented as methods of the OnboardingManagerImpl
// class

void OnboardingManagerImpl::initializeSteps() {
    // Initialize the onboarding steps sequence
    // This method sets up the initial state for onboarding steps

    // Ensure we start with a clean state
    m_completedSteps.clear();
    m_currentStep = OnboardingStep::Welcome;

    // Initialize analytics data structure for all steps
    m_analyticsData = QJsonObject();

    // Pre-populate analytics structure for all onboarding steps
    for (int i = 0; i < static_cast<int>(OnboardingStep::Complete); ++i) {
        OnboardingStep step = static_cast<OnboardingStep>(i);
        QString stepStr = stepToString(step);

        QJsonObject stepData;
        stepData["started"] = false;
        stepData["completed"] = false;
        stepData["skipped"] = false;
        stepData["start_time"] = QString();
        stepData["completion_time"] = QString();
        stepData["duration_seconds"] = 0;

        m_analyticsData[stepStr] = stepData;
    }
}

void OnboardingManagerImpl::initializeTutorials() {
    m_availableTutorials = QJsonArray{
        QJsonObject{{"id", "open_file"},
                    {"title", "Opening Files"},
                    {"description", "Learn how to open PDF files and folders"},
                    {"category", "getting_started"},
                    {"duration", "2 min"},
                    {"difficulty", "Beginner"}},
        QJsonObject{
            {"id", "navigation"},
            {"title", "Document Navigation"},
            {"description", "Navigate through pages and sections efficiently"},
            {"category", "basic_features"},
            {"duration", "3 min"},
            {"difficulty", "Beginner"}},
        QJsonObject{{"id", "search"},
                    {"title", "Search Features"},
                    {"description", "Master the powerful search capabilities"},
                    {"category", "basic_features"},
                    {"duration", "5 min"},
                    {"difficulty", "Intermediate"}},
        QJsonObject{{"id", "bookmarks"},
                    {"title", "Managing Bookmarks"},
                    {"description", "Organize your reading with bookmarks"},
                    {"category", "basic_features"},
                    {"duration", "3 min"},
                    {"difficulty", "Beginner"}},
        QJsonObject{{"id", "annotations"},
                    {"title", "Annotations & Notes"},
                    {"description", "Add highlights and notes to documents"},
                    {"category", "advanced_features"},
                    {"duration", "4 min"},
                    {"difficulty", "Intermediate"}},
        QJsonObject{{"id", "view_modes"},
                    {"title", "View Modes"},
                    {"description", "Customize your reading experience"},
                    {"category", "basic_features"},
                    {"duration", "2 min"},
                    {"difficulty", "Beginner"}},
        QJsonObject{{"id", "keyboard_shortcuts"},
                    {"title", "Keyboard Shortcuts"},
                    {"description", "Work faster with keyboard shortcuts"},
                    {"category", "productivity_tips"},
                    {"duration", "5 min"},
                    {"difficulty", "Advanced"}}};
}

void OnboardingManagerImpl::setupConnections() {
    // Setup internal connections if needed
}

QString OnboardingManagerImpl::stepToString(OnboardingStep step) const {
    switch (step) {
        case OnboardingStep::Welcome:
            return "Welcome";
        case OnboardingStep::OpenFile:
            return "OpenFile";
        case OnboardingStep::Navigation:
            return "Navigation";
        case OnboardingStep::Search:
            return "Search";
        case OnboardingStep::Bookmarks:
            return "Bookmarks";
        case OnboardingStep::Annotations:
            return "Annotations";
        case OnboardingStep::ViewModes:
            return "ViewModes";
        case OnboardingStep::Settings:
            return "Settings";
        case OnboardingStep::KeyboardShortcuts:
            return "KeyboardShortcuts";
        case OnboardingStep::Complete:
            return "Complete";
        default:
            return "Unknown";
    }
}

OnboardingStep OnboardingManagerImpl::stringToStep(const QString& str) const {
    if (str == "Welcome") {
        return OnboardingStep::Welcome;
    }
    if (str == "OpenFile") {
        return OnboardingStep::OpenFile;
    }
    if (str == "Navigation") {
        return OnboardingStep::Navigation;
    }
    if (str == "Search") {
        return OnboardingStep::Search;
    }
    if (str == "Bookmarks") {
        return OnboardingStep::Bookmarks;
    }
    if (str == "Annotations") {
        return OnboardingStep::Annotations;
    }
    if (str == "ViewModes") {
        return OnboardingStep::ViewModes;
    }
    if (str == "Settings") {
        return OnboardingStep::Settings;
    }
    if (str == "KeyboardShortcuts") {
        return OnboardingStep::KeyboardShortcuts;
    }
    if (str == "Complete") {
        return OnboardingStep::Complete;
    }
    return OnboardingStep::Welcome;
}

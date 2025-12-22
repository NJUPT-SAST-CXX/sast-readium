#include <QSignalSpy>
#include <QTest>
#include "../../app/managers/OnboardingManager.h"
#include "../TestUtilities.h"

class TestOnboardingManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_manager = new OnboardingManager(); }

    void cleanup() {
        delete m_manager;
        m_manager = nullptr;
    }

    void testConstruction() { QVERIFY(m_manager != nullptr); }

    void testIsFirstRun() { bool firstRun = m_manager->isFirstRun(); }

    void testMarkOnboardingComplete() {
        m_manager->markOnboardingComplete();

        QVERIFY(m_manager->isOnboardingComplete());
    }

    void testIsOnboardingComplete() {
        bool complete = m_manager->isOnboardingComplete();
    }

    void testResetOnboarding() {
        m_manager->markOnboardingComplete();
        m_manager->resetOnboarding();

        QVERIFY(!m_manager->isOnboardingComplete());
    }

    void testGetCurrentStep() {
        int step = m_manager->getCurrentStep();
        QVERIFY(step >= 0);
    }

    void testSetCurrentStep() {
        m_manager->setCurrentStep(3);
        QCOMPARE(m_manager->getCurrentStep(), 3);

        m_manager->setCurrentStep(0);
        QCOMPARE(m_manager->getCurrentStep(), 0);
    }

    void testGetTotalSteps() {
        int total = m_manager->getTotalSteps();
        QVERIFY(total > 0);
    }

    void testNextStep() {
        m_manager->setCurrentStep(0);
        int initial = m_manager->getCurrentStep();

        m_manager->nextStep();
        QCOMPARE(m_manager->getCurrentStep(), initial + 1);
    }

    void testPreviousStep() {
        m_manager->setCurrentStep(3);

        m_manager->previousStep();
        QCOMPARE(m_manager->getCurrentStep(), 2);

        m_manager->setCurrentStep(0);
        m_manager->previousStep();
        QCOMPARE(m_manager->getCurrentStep(), 0);
    }

    void testSkipOnboarding() {
        QSignalSpy spy(m_manager, &OnboardingManager::onboardingSkipped);

        m_manager->skipOnboarding();

        QCOMPARE(spy.count(), 1);
        QVERIFY(m_manager->isOnboardingComplete());
    }

    void testStepCompletedSignal() {
        QSignalSpy spy(m_manager, &OnboardingManager::stepCompleted);

        m_manager->setCurrentStep(0);
        m_manager->nextStep();

        QCOMPARE(spy.count(), 1);
    }

    void testOnboardingCompleteSignal() {
        QSignalSpy spy(m_manager, &OnboardingManager::onboardingCompleted);

        m_manager->markOnboardingComplete();

        QCOMPARE(spy.count(), 1);
    }

    void testGetStepInfo() {
        OnboardingManager::StepInfo info = m_manager->getStepInfo(0);
        QVERIFY(!info.title.isEmpty());
    }

    void testGetAllSteps() {
        QList<OnboardingManager::StepInfo> steps = m_manager->getAllSteps();
        QVERIFY(!steps.isEmpty());
        QCOMPARE(steps.size(), m_manager->getTotalSteps());
    }

    void testIsStepCompleted() {
        m_manager->resetOnboarding();
        m_manager->setCurrentStep(0);

        QVERIFY(!m_manager->isStepCompleted(0));

        m_manager->nextStep();
        QVERIFY(m_manager->isStepCompleted(0));
    }

    void testMarkStepCompleted() {
        m_manager->resetOnboarding();

        m_manager->markStepCompleted(2);
        QVERIFY(m_manager->isStepCompleted(2));
    }

    void testGetCompletedSteps() {
        m_manager->resetOnboarding();

        m_manager->markStepCompleted(0);
        m_manager->markStepCompleted(2);

        QList<int> completed = m_manager->getCompletedSteps();
        QVERIFY(completed.contains(0));
        QVERIFY(completed.contains(2));
    }

    void testShouldShowOnboarding() {
        m_manager->resetOnboarding();
        QVERIFY(m_manager->shouldShowOnboarding());

        m_manager->markOnboardingComplete();
        QVERIFY(!m_manager->shouldShowOnboarding());
    }

    void testNavigationSequence() {
        m_manager->resetOnboarding();
        m_manager->setCurrentStep(0);

        m_manager->nextStep();
        QCOMPARE(m_manager->getCurrentStep(), 1);

        m_manager->nextStep();
        QCOMPARE(m_manager->getCurrentStep(), 2);

        m_manager->previousStep();
        QCOMPARE(m_manager->getCurrentStep(), 1);

        m_manager->previousStep();
        QCOMPARE(m_manager->getCurrentStep(), 0);
    }

private:
    OnboardingManager* m_manager = nullptr;
};

QTEST_MAIN(TestOnboardingManager)
#include "test_onboarding_manager.moc"

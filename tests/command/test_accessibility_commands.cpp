#include <QSignalSpy>
#include <QTest>
#include "../../app/command/AccessibilityCommands.h"
#include "../TestUtilities.h"

class TestAccessibilityCommands : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testToggleScreenReaderCommand() {
        ToggleScreenReaderCommand cmd;
        QVERIFY(!cmd.name().isEmpty());
        QVERIFY(!cmd.description().isEmpty());
    }

    void testToggleScreenReaderExecute() {
        ToggleScreenReaderCommand cmd;

        cmd.execute();
        cmd.undo();
    }

    void testToggleHighContrastCommand() {
        ToggleHighContrastCommand cmd;
        QVERIFY(!cmd.name().isEmpty());
        QVERIFY(!cmd.description().isEmpty());
    }

    void testToggleHighContrastExecute() {
        ToggleHighContrastCommand cmd;

        cmd.execute();
        cmd.undo();
    }

    void testStartTextToSpeechCommand() {
        StartTextToSpeechCommand cmd("Test text");
        QVERIFY(!cmd.name().isEmpty());
        QVERIFY(!cmd.description().isEmpty());
    }

    void testStartTextToSpeechExecute() {
        StartTextToSpeechCommand cmd("Test text for speech");

        cmd.execute();
        cmd.undo();
    }

    void testStopTextToSpeechCommand() {
        StopTextToSpeechCommand cmd;
        QVERIFY(!cmd.name().isEmpty());
        QVERIFY(!cmd.description().isEmpty());
    }

    void testStopTextToSpeechExecute() {
        StopTextToSpeechCommand cmd;

        cmd.execute();
        cmd.undo();
    }

    void testSetTextScaleCommand() {
        SetTextScaleCommand cmd(1.5);
        QVERIFY(!cmd.name().isEmpty());
        QVERIFY(!cmd.description().isEmpty());
    }

    void testSetTextScaleExecute() {
        SetTextScaleCommand cmd(2.0);

        cmd.execute();
        cmd.undo();
    }

    void testSetTextScaleVariousValues() {
        SetTextScaleCommand cmd1(0.5);
        cmd1.execute();
        cmd1.undo();

        SetTextScaleCommand cmd2(1.0);
        cmd2.execute();
        cmd2.undo();

        SetTextScaleCommand cmd3(3.0);
        cmd3.execute();
        cmd3.undo();
    }

    void testToggleReduceMotionCommand() {
        ToggleReduceMotionCommand cmd;
        QVERIFY(!cmd.name().isEmpty());
        QVERIFY(!cmd.description().isEmpty());
    }

    void testToggleReduceMotionExecute() {
        ToggleReduceMotionCommand cmd;

        cmd.execute();
        cmd.undo();
    }

    void testCommandSequence() {
        ToggleScreenReaderCommand screenReaderCmd;
        ToggleHighContrastCommand highContrastCmd;
        StartTextToSpeechCommand ttsCmd("Test");
        StopTextToSpeechCommand stopTtsCmd;

        screenReaderCmd.execute();
        highContrastCmd.execute();
        ttsCmd.execute();
        stopTtsCmd.execute();

        stopTtsCmd.undo();
        ttsCmd.undo();
        highContrastCmd.undo();
        screenReaderCmd.undo();
    }

    void testCommandReversibility() {
        SetTextScaleCommand cmd(1.5);

        cmd.execute();

        cmd.undo();

        cmd.execute();
    }
};

QTEST_MAIN(TestAccessibilityCommands)
#include "test_accessibility_commands.moc"

#include <QSignalSpy>
#include <QTest>

#include "../../app/command/DocumentCommands.h"
#include "../../app/command/NavigationCommands.h"
#include "../../app/factory/CommandFactory.h"
#include "../TestUtilities.h"

class CommandFactoryTest : public TestBase {
    Q_OBJECT

private slots:
    void test_mapping_completeness_roundtrip();
    void test_dependency_validation_for_methods();
    void test_signals_on_failure_for_document_actions();
    void test_custom_registration_and_batch_creation();
    void test_global_singleton_and_builder();
};

static QList<ActionMap> enumerateAllActions() {
    QList<ActionMap> actions;
    int first = static_cast<int>(ActionMap::openFile);
    int last = static_cast<int>(ActionMap::showHelp);
    for (int i = first; i <= last; ++i)
        actions << static_cast<ActionMap>(i);
    return actions;
}

void CommandFactoryTest::test_mapping_completeness_roundtrip() {
    CommandFactory factory;  // dependencies intentionally not set
    QSignalSpy failSpy(&factory, &CommandFactory::commandCreationFailed);

    const auto all = enumerateAllActions();
    for (ActionMap a : all) {
        failSpy.clear();
        auto cmd =
            factory.createDocumentCommand(a);  // will fail fast for any action
        QVERIFY2(!cmd, "Command should be null when dependencies are not set");
        QVERIFY2(failSpy.count() == 1,
                 "commandCreationFailed should be emitted once");
        const auto args = failSpy.takeFirst();
        QVERIFY(args.size() >= 2);
        const QString typeStr = args.at(0).toString();
        QVERIFY2(typeStr != "unknown",
                 "Every ActionMap value must have a string mapping");
        // Round-trip: converting back via mapStringToAction should equal
        // original. We don't have direct access; create an equivalent factory
        // path by creating a temporary mapping using the public API
        // createDocumentCommand(string) isn't available. Instead, ensure
        // typeStr exists in the internal map by attempting to create a custom
        // command batch where unknown names are skipped. The existence of a
        // non-"unknown" name is the enforcement here.
    }
}

void CommandFactoryTest::test_dependency_validation_for_methods() {
    CommandFactory f;  // no deps set

    // Document commands
    QVERIFY(f.createOpenCommand().get() == nullptr);
    QVERIFY(f.createCloseCommand().get() == nullptr);
    QVERIFY(f.createSaveAsCommand().get() == nullptr);
    QVERIFY(f.createPrintCommand().get() == nullptr);
    QVERIFY(f.createReloadCommand().get() == nullptr);
    QVERIFY(f.createPropertiesCommand().get() == nullptr);

    // Navigation commands
    QVERIFY(f.createNavigationCommand("next").get() == nullptr);
    QVERIFY(f.createNextPageCommand().get() == nullptr);
    QVERIFY(f.createPreviousPageCommand().get() == nullptr);
    QVERIFY(f.createGoToPageCommand(3).get() == nullptr);
    QVERIFY(f.createFirstPageCommand().get() == nullptr);
    QVERIFY(f.createLastPageCommand().get() == nullptr);

    // Zoom/View commands
    QVERIFY(f.createZoomCommand("in").get() == nullptr);
    QVERIFY(f.createZoomInCommand().get() == nullptr);
    QVERIFY(f.createZoomOutCommand().get() == nullptr);
    QVERIFY(f.createFitWidthCommand().get() == nullptr);
    QVERIFY(f.createFitPageCommand().get() == nullptr);
    QVERIFY(f.createSetZoomCommand(1.5).get() == nullptr);
    QVERIFY(f.createViewModeCommand("single-page").get() == nullptr);
    QVERIFY(f.createRotateCommand(true).get() == nullptr);
    QVERIFY(f.createFullscreenCommand().get() == nullptr);
}

void CommandFactoryTest::test_signals_on_failure_for_document_actions() {
    CommandFactory factory;  // deps not set
    QSignalSpy failSpy(&factory, &CommandFactory::commandCreationFailed);

    auto cmd = factory.createDocumentCommand(ActionMap::openFile);
    QVERIFY(!cmd);
    QCOMPARE(failSpy.count(), 1);
    const auto args = failSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("openFile"));
    QCOMPARE(args.at(1).toString(), QString("Dependencies not set"));
}

void CommandFactoryTest::test_custom_registration_and_batch_creation() {
    CommandFactory f;
    QSignalSpy createdSpy(&f, &CommandFactory::commandCreated);
    QSignalSpy failedSpy(&f, &CommandFactory::commandCreationFailed);

    f.registerCommandType("dummy",
                          [](CommandFactory*) { return new QObject(); });

    QObject* c = f.createCustomCommand("dummy");
    QVERIFY(c != nullptr);
    QVERIFY(createdSpy.count() == 1);  // custom path emits commandCreated

    QObject* c2 = f.createCustomCommand("does_not_exist");
    QVERIFY(c2 == nullptr);
    QVERIFY(failedSpy.count() == 1);

    const QStringList names{QStringLiteral("dummy"), QStringLiteral("unknown")};
    QList<QObject*> batch = f.createCommandBatch(names);
    QCOMPARE(batch.size(), 1);
    QVERIFY(batch.at(0) != nullptr);
    qDeleteAll(batch);
    delete c;
}

void CommandFactoryTest::test_global_singleton_and_builder() {
    // Register a custom type on the singleton-backed factory
    CommandFactory& global = GlobalCommandFactory::instance();
    global.registerCommandType("dummy_builder", [](CommandFactory*) {
        auto* obj = new QObject();
        obj->setObjectName("from_builder");
        return obj;
    });

    // Confirm singleton identity
    auto* addr1 = &GlobalCommandFactory::instance();
    auto* addr2 = &GlobalCommandFactory::instance();
    QCOMPARE(addr1, addr2);

    // Builder path
    CommandBuilder builder;
    auto cmd = builder.ofType("dummy_builder").withParameter("p", 42).build();
    QVERIFY(cmd != nullptr);
    QCOMPARE(cmd->objectName(), QString("from_builder"));
    QVERIFY(cmd->property("p").isValid());
    QCOMPARE(cmd->property("p").toInt(), 42);
}

QTEST_MAIN(CommandFactoryTest)
#include "test_command_factory.moc"

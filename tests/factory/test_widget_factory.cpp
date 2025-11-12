#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "../../app/controller/PageController.h"
#include "../../app/factory/WidgetFactory.h"
#include "../../app/model/PageModel.h"
#include "../TestUtilities.h"

class WidgetFactoryTest : public TestBase {
    Q_OBJECT

private slots:
    void test_create_button_success_and_properties();
    void test_signal_wiring_next_page();
    void test_unknown_action_and_null_controller_errors();
    void test_memory_ownership_by_parent();
};

void WidgetFactoryTest::test_create_button_success_and_properties() {
    PageModel model(5);
    PageController controller(&model);
    WidgetFactory factory(&controller);

    QWidget parent;
    QSignalSpy createdSpy(&factory, &WidgetFactory::widgetCreated);

    auto* btn = factory.createButton(actionID::next, "Next", &parent);
    QVERIFY(btn != nullptr);
    QCOMPARE(btn->text(), QString("Next"));
    QVERIFY(btn->objectName().startsWith("Button_Action"));
    QCOMPARE(btn->parent(), &parent);
    QCOMPARE(createdSpy.count(), 1);
}

void WidgetFactoryTest::test_signal_wiring_next_page() {
    // Prepare a real RenderModel with a small test PDF so PageModel validation
    // passes
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString pdf = tmp.filePath("doc.pdf");
    TestDataGenerator::createTestPdfWithoutText(5, pdf);
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(pdf));
    QVERIFY(doc != nullptr);

    RenderModel rm;
    rm.setDocument(doc.get());
    PageModel model(&rm);
    PageController controller(&model);
    WidgetFactory factory(&controller);

    QWidget parent;
    auto* btn = factory.createButton(actionID::next, "Next", &parent);
    QVERIFY(btn != nullptr);

    QSignalSpy pageChangedSpy(&controller, &PageController::pageChanged);

    // Ensure a valid starting page (PageModel is 1-based)
    model.setCurrentPage(1);
    QMetaObject::invokeMethod(btn, "click", Qt::DirectConnection);

    // Expect a navigation to next page -> pageChanged emitted
    QVERIFY(pageChangedSpy.count() > 0 || pageChangedSpy.wait(200));
}

void WidgetFactoryTest::test_unknown_action_and_null_controller_errors() {
    // Unknown action
    PageModel model(5);
    PageController controller(&model);
    WidgetFactory factory(&controller);
    QSignalSpy errSpy(&factory, &WidgetFactory::creationError);
    QWidget parent;

    auto* btnUnknown =
        factory.createButton(static_cast<actionID>(999), "X", &parent);
    QVERIFY(btnUnknown == nullptr);
    QVERIFY(errSpy.count() == 1);

    // Null controller
    WidgetFactory factoryNull(nullptr);
    QSignalSpy errSpy2(&factoryNull, &WidgetFactory::creationError);
    auto* btnNull = factoryNull.createButton(actionID::next, "Next", &parent);
    QVERIFY(btnNull == nullptr);
    QVERIFY(errSpy2.count() == 1);
}

void WidgetFactoryTest::test_memory_ownership_by_parent() {
    PageModel model(5);
    PageController controller(&model);
    WidgetFactory factory(&controller);

    auto* parent = new QWidget();
    QPointer<QPushButton> btn =
        factory.createButton(actionID::prev, "Prev", parent);
    QVERIFY(!btn.isNull());
    delete parent;  // should delete child button via Qt parent-child ownership
    QVERIFY(btn.isNull());
}

QTEST_MAIN(WidgetFactoryTest)
#include "test_widget_factory.moc"

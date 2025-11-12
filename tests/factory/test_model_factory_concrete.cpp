#include <QSignalSpy>
#include <QTest>

#include "../../app/factory/ModelFactory.h"
#include "../TestUtilities.h"

class ModelFactoryConcreteTest : public TestBase {
    Q_OBJECT

private slots:
    void test_concrete_model_creators();
    void test_composite_model_sets();
    void test_singleton_getters_and_reset();
    void test_model_builder_fluent();
};

void ModelFactoryConcreteTest::test_concrete_model_creators() {
    ModelFactory f;
    QSignalSpy createdSpy(&f, &ModelFactory::modelCreated);

    auto* render = f.createRenderModel(96, 96);
    QVERIFY(render != nullptr);

    auto* doc = f.createDocumentModel(render);
    QVERIFY(doc != nullptr);

    auto* page = f.createPageModel(render);
    QVERIFY(page != nullptr);

    auto* outline = f.createPDFOutlineModel(doc);
    QVERIFY(outline != nullptr);

    auto* search = f.createSearchModel(doc);
    QVERIFY(search != nullptr);

    auto* thumb = f.createThumbnailModel(doc);
    QVERIFY(thumb != nullptr);

    auto* bookmark = f.createBookmarkModel(doc);
    QVERIFY(bookmark != nullptr);

    auto* annot = f.createAnnotationModel(doc);
    QVERIFY(annot != nullptr);

    auto* loader = f.createAsyncDocumentLoader(doc);
    QVERIFY(loader != nullptr);

    QVERIFY(createdSpy.count() >= 9);
}

void ModelFactoryConcreteTest::test_composite_model_sets() {
    ModelFactory f;
    QSignalSpy setSpy(&f, &ModelFactory::modelSetCreated);

    auto full = f.createCompleteModelSet(96, 96);
    QVERIFY(full.renderModel && full.documentModel && full.pageModel);

    auto minimal = f.createMinimalModelSet(96, 96);
    QVERIFY(minimal.renderModel && minimal.documentModel && minimal.pageModel);

    auto viewer = f.createViewerModelSet(96, 96);
    QVERIFY(viewer.renderModel && viewer.documentModel && viewer.pageModel);

    QVERIFY(setSpy.count() >= 3);
}

void ModelFactoryConcreteTest::test_singleton_getters_and_reset() {
    auto& s = SingletonModelFactory::instance();
    auto* r1 = s.getRenderModel();
    auto* d1 = s.getDocumentModel();
    auto* p1 = s.getPageModel();
    QVERIFY(r1 && d1 && p1);

    s.reset();
    auto* r2 = s.getRenderModel();
    auto* d2 = s.getDocumentModel();
    auto* p2 = s.getPageModel();
    QVERIFY(r2 && d2 && p2);
    QVERIFY(r1 != r2 || d1 != d2 || p1 != p2);  // at least one got recreated
}

void ModelFactoryConcreteTest::test_model_builder_fluent() {
    ModelBuilder b;
    auto set = b.withDpi(96, 96)
                   .withThumbnails(true)
                   .withBookmarks(true)
                   .withAnnotations(true)
                   .withSearch(true)
                   .withOutline(true)
                   .withAsyncLoading(true)
                   .build();

    QVERIFY(set.renderModel && set.documentModel && set.pageModel);
}

QTEST_MAIN(ModelFactoryConcreteTest)
#include "test_model_factory_concrete.moc"

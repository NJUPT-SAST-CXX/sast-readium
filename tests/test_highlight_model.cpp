#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include "../app/model/HighlightModel.h"

/**
 * @brief Test fixture for HighlightModel tests
 */
class HighlightModelTest : public ::testing::Test {
protected:
    void SetUp() override { model = new HighlightModel(); }

    void TearDown() override { delete model; }

    TextHighlight createTestHighlight(int pageNumber = 0,
                                      const QString& text = "test") {
        TextHighlight highlight;
        highlight.pageNumber = pageNumber;
        highlight.text = text;
        highlight.rects.append(QRectF(10, 20, 100, 15));
        highlight.color = Qt::yellow;
        highlight.opacity = 0.4;
        highlight.author = "test_user";
        return highlight;
    }

    HighlightModel* model;
};

// Basic Operations Tests

TEST_F(HighlightModelTest, AddHighlight) {
    TextHighlight highlight = createTestHighlight();

    EXPECT_TRUE(model->addHighlight(highlight));
    EXPECT_EQ(model->getTotalHighlightCount(), 1);
}

TEST_F(HighlightModelTest, AddEmptyHighlight) {
    TextHighlight emptyHighlight;

    EXPECT_FALSE(model->addHighlight(emptyHighlight));
    EXPECT_EQ(model->getTotalHighlightCount(), 0);
}

TEST_F(HighlightModelTest, RemoveHighlight) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    EXPECT_TRUE(model->removeHighlight(highlight.id));
    EXPECT_EQ(model->getTotalHighlightCount(), 0);
}

TEST_F(HighlightModelTest, RemoveNonexistentHighlight) {
    EXPECT_FALSE(model->removeHighlight("nonexistent-id"));
}

TEST_F(HighlightModelTest, UpdateHighlight) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    highlight.text = "updated text";
    highlight.note = "new note";

    EXPECT_TRUE(model->updateHighlight(highlight.id, highlight));

    TextHighlight retrieved = model->getHighlight(highlight.id);
    EXPECT_EQ(retrieved.text, "updated text");
    EXPECT_EQ(retrieved.note, "new note");
}

TEST_F(HighlightModelTest, GetHighlight) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    TextHighlight retrieved = model->getHighlight(highlight.id);
    EXPECT_FALSE(retrieved.isEmpty());
    EXPECT_EQ(retrieved.text, highlight.text);
    EXPECT_EQ(retrieved.pageNumber, highlight.pageNumber);
}

// Page Operations Tests

TEST_F(HighlightModelTest, GetHighlightsForPage) {
    model->addHighlight(createTestHighlight(0, "page 0 text 1"));
    model->addHighlight(createTestHighlight(0, "page 0 text 2"));
    model->addHighlight(createTestHighlight(1, "page 1 text"));

    auto page0Highlights = model->getHighlightsForPage(0);
    EXPECT_EQ(page0Highlights.size(), 2);

    auto page1Highlights = model->getHighlightsForPage(1);
    EXPECT_EQ(page1Highlights.size(), 1);
}

TEST_F(HighlightModelTest, GetHighlightCountForPage) {
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(1));

    EXPECT_EQ(model->getHighlightCountForPage(0), 2);
    EXPECT_EQ(model->getHighlightCountForPage(1), 1);
    EXPECT_EQ(model->getHighlightCountForPage(2), 0);
}

TEST_F(HighlightModelTest, RemoveHighlightsForPage) {
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(1));

    EXPECT_TRUE(model->removeHighlightsForPage(0));
    EXPECT_EQ(model->getTotalHighlightCount(), 1);
    EXPECT_EQ(model->getHighlightCountForPage(0), 0);
    EXPECT_EQ(model->getHighlightCountForPage(1), 1);
}

// Search and Filter Tests

TEST_F(HighlightModelTest, SearchHighlights) {
    model->addHighlight(createTestHighlight(0, "important information"));
    model->addHighlight(createTestHighlight(0, "regular text"));
    model->addHighlight(createTestHighlight(1, "another important note"));

    auto results = model->searchHighlights("important");
    EXPECT_EQ(results.size(), 2);
}

TEST_F(HighlightModelTest, SearchHighlightsCaseInsensitive) {
    model->addHighlight(createTestHighlight(0, "Important Information"));

    auto results = model->searchHighlights("important");
    EXPECT_EQ(results.size(), 1);
}

TEST_F(HighlightModelTest, GetHighlightsByColor) {
    TextHighlight yellow = createTestHighlight();
    yellow.colorPreset = HighlightColor::Yellow;

    TextHighlight green = createTestHighlight();
    green.colorPreset = HighlightColor::Green;

    model->addHighlight(yellow);
    model->addHighlight(green);
    model->addHighlight(yellow);

    auto yellowHighlights = model->getHighlightsByColor(HighlightColor::Yellow);
    EXPECT_EQ(yellowHighlights.size(), 2);

    auto greenHighlights = model->getHighlightsByColor(HighlightColor::Green);
    EXPECT_EQ(greenHighlights.size(), 1);
}

TEST_F(HighlightModelTest, GetHighlightsByAuthor) {
    TextHighlight h1 = createTestHighlight();
    h1.author = "user1";

    TextHighlight h2 = createTestHighlight();
    h2.author = "user2";

    model->addHighlight(h1);
    model->addHighlight(h2);
    model->addHighlight(h1);

    auto user1Highlights = model->getHighlightsByAuthor("user1");
    EXPECT_EQ(user1Highlights.size(), 2);
}

TEST_F(HighlightModelTest, GetHighlightsWithNotes) {
    TextHighlight withNote = createTestHighlight();
    withNote.note = "This is a note";

    TextHighlight withoutNote = createTestHighlight();

    model->addHighlight(withNote);
    model->addHighlight(withoutNote);

    auto notedHighlights = model->getHighlightsWithNotes();
    EXPECT_EQ(notedHighlights.size(), 1);
}

// Editing Operations Tests

TEST_F(HighlightModelTest, EditHighlightNote) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    EXPECT_TRUE(model->editHighlightNote(highlight.id, "new note"));

    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_EQ(updated.note, "new note");
}

TEST_F(HighlightModelTest, ChangeHighlightColor) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    QColor newColor = Qt::green;
    EXPECT_TRUE(model->changeHighlightColor(highlight.id, newColor));

    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_EQ(updated.color, newColor);
}

TEST_F(HighlightModelTest, ChangeHighlightOpacity) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    EXPECT_TRUE(model->changeHighlightOpacity(highlight.id, 0.8));

    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_DOUBLE_EQ(updated.opacity, 0.8);
}

TEST_F(HighlightModelTest, ToggleHighlightVisibility) {
    TextHighlight highlight = createTestHighlight();
    highlight.isVisible = true;
    model->addHighlight(highlight);

    EXPECT_TRUE(model->toggleHighlightVisibility(highlight.id));
    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_FALSE(updated.isVisible);

    EXPECT_TRUE(model->toggleHighlightVisibility(highlight.id));
    updated = model->getHighlight(highlight.id);
    EXPECT_TRUE(updated.isVisible);
}

// Batch Operations Tests

TEST_F(HighlightModelTest, RemoveAllHighlights) {
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(1));
    model->addHighlight(createTestHighlight(2));

    EXPECT_TRUE(model->removeAllHighlights());
    EXPECT_EQ(model->getTotalHighlightCount(), 0);
}

// Statistics Tests

TEST_F(HighlightModelTest, GetTotalPages) {
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(2));
    model->addHighlight(createTestHighlight(5));

    EXPECT_EQ(model->getTotalPages(), 3);
}

TEST_F(HighlightModelTest, GetAverageHighlightsPerPage) {
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(0));
    model->addHighlight(createTestHighlight(1));
    model->addHighlight(createTestHighlight(2));

    EXPECT_DOUBLE_EQ(model->getAverageHighlightsPerPage(), 4.0 / 3.0);
}

// Signal Tests

TEST_F(HighlightModelTest, SignalHighlightAdded) {
    QSignalSpy spy(model, &HighlightModel::highlightAdded);

    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(HighlightModelTest, SignalHighlightRemoved) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    QSignalSpy spy(model, &HighlightModel::highlightRemoved);
    model->removeHighlight(highlight.id);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(HighlightModelTest, SignalHighlightUpdated) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    QSignalSpy spy(model, &HighlightModel::highlightUpdated);
    model->editHighlightNote(highlight.id, "new note");

    EXPECT_EQ(spy.count(), 1);
}

// Main function
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

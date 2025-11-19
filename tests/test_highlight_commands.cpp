#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QUndoStack>
#include "../app/command/HighlightCommands.h"
#include "../app/model/HighlightModel.h"

/**
 * @brief Test fixture for HighlightCommands tests
 */
class HighlightCommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        model = new HighlightModel();
        undoStack = new QUndoStack();
    }

    void TearDown() override {
        delete undoStack;
        delete model;
    }

    TextHighlight createTestHighlight(const QString& text = "test") {
        TextHighlight highlight;
        highlight.pageNumber = 0;
        highlight.text = text;
        highlight.rects.append(QRectF(10, 20, 100, 15));
        highlight.color = Qt::yellow;
        highlight.opacity = 0.4;
        return highlight;
    }

    HighlightModel* model;
    QUndoStack* undoStack;
};

// AddHighlightCommand Tests

TEST_F(HighlightCommandsTest, AddHighlightCommand) {
    TextHighlight highlight = createTestHighlight();

    auto* cmd = new AddHighlightCommand(model, highlight);
    undoStack->push(cmd);

    EXPECT_EQ(model->getTotalHighlightCount(), 1);
}

TEST_F(HighlightCommandsTest, AddHighlightCommandUndo) {
    TextHighlight highlight = createTestHighlight();

    auto* cmd = new AddHighlightCommand(model, highlight);
    undoStack->push(cmd);
    EXPECT_EQ(model->getTotalHighlightCount(), 1);

    undoStack->undo();
    EXPECT_EQ(model->getTotalHighlightCount(), 0);
}

TEST_F(HighlightCommandsTest, AddHighlightCommandRedo) {
    TextHighlight highlight = createTestHighlight();

    auto* cmd = new AddHighlightCommand(model, highlight);
    undoStack->push(cmd);
    undoStack->undo();

    undoStack->redo();
    EXPECT_EQ(model->getTotalHighlightCount(), 1);
}

// RemoveHighlightCommand Tests

TEST_F(HighlightCommandsTest, RemoveHighlightCommand) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    auto* cmd = new RemoveHighlightCommand(model, highlight.id);
    undoStack->push(cmd);

    EXPECT_EQ(model->getTotalHighlightCount(), 0);
}

TEST_F(HighlightCommandsTest, RemoveHighlightCommandUndo) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);
    QString highlightId = highlight.id;

    auto* cmd = new RemoveHighlightCommand(model, highlightId);
    undoStack->push(cmd);
    EXPECT_EQ(model->getTotalHighlightCount(), 0);

    undoStack->undo();
    EXPECT_EQ(model->getTotalHighlightCount(), 1);

    TextHighlight restored = model->getHighlight(highlightId);
    EXPECT_FALSE(restored.isEmpty());
    EXPECT_EQ(restored.text, highlight.text);
}

// EditHighlightNoteCommand Tests

TEST_F(HighlightCommandsTest, EditHighlightNoteCommand) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    auto* cmd = new EditHighlightNoteCommand(model, highlight.id, "new note");
    undoStack->push(cmd);

    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_EQ(updated.note, "new note");
}

TEST_F(HighlightCommandsTest, EditHighlightNoteCommandUndo) {
    TextHighlight highlight = createTestHighlight();
    highlight.note = "original note";
    model->addHighlight(highlight);

    auto* cmd = new EditHighlightNoteCommand(model, highlight.id, "new note");
    undoStack->push(cmd);

    undoStack->undo();
    TextHighlight restored = model->getHighlight(highlight.id);
    EXPECT_EQ(restored.note, "original note");
}

TEST_F(HighlightCommandsTest, EditHighlightNoteCommandMerge) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    auto* cmd1 = new EditHighlightNoteCommand(model, highlight.id, "note 1");
    undoStack->push(cmd1);

    auto* cmd2 = new EditHighlightNoteCommand(model, highlight.id, "note 2");
    undoStack->push(cmd2);

    // Commands should merge, so only one undo needed
    undoStack->undo();
    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_EQ(updated.note, "");  // Back to original (empty)
}

// ChangeHighlightColorCommand Tests

TEST_F(HighlightCommandsTest, ChangeHighlightColorCommand) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    QColor newColor = Qt::green;
    auto* cmd = new ChangeHighlightColorCommand(model, highlight.id, newColor);
    undoStack->push(cmd);

    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_EQ(updated.color, newColor);
}

TEST_F(HighlightCommandsTest, ChangeHighlightColorCommandUndo) {
    TextHighlight highlight = createTestHighlight();
    QColor originalColor = Qt::yellow;
    highlight.color = originalColor;
    model->addHighlight(highlight);

    QColor newColor = Qt::green;
    auto* cmd = new ChangeHighlightColorCommand(model, highlight.id, newColor);
    undoStack->push(cmd);

    undoStack->undo();
    TextHighlight restored = model->getHighlight(highlight.id);
    EXPECT_EQ(restored.color, originalColor);
}

// ChangeHighlightOpacityCommand Tests

TEST_F(HighlightCommandsTest, ChangeHighlightOpacityCommand) {
    TextHighlight highlight = createTestHighlight();
    model->addHighlight(highlight);

    auto* cmd = new ChangeHighlightOpacityCommand(model, highlight.id, 0.8);
    undoStack->push(cmd);

    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_DOUBLE_EQ(updated.opacity, 0.8);
}

TEST_F(HighlightCommandsTest, ChangeHighlightOpacityCommandUndo) {
    TextHighlight highlight = createTestHighlight();
    highlight.opacity = 0.4;
    model->addHighlight(highlight);

    auto* cmd = new ChangeHighlightOpacityCommand(model, highlight.id, 0.8);
    undoStack->push(cmd);

    undoStack->undo();
    TextHighlight restored = model->getHighlight(highlight.id);
    EXPECT_DOUBLE_EQ(restored.opacity, 0.4);
}

// ToggleHighlightVisibilityCommand Tests

TEST_F(HighlightCommandsTest, ToggleHighlightVisibilityCommand) {
    TextHighlight highlight = createTestHighlight();
    highlight.isVisible = true;
    model->addHighlight(highlight);

    auto* cmd = new ToggleHighlightVisibilityCommand(model, highlight.id);
    undoStack->push(cmd);

    TextHighlight updated = model->getHighlight(highlight.id);
    EXPECT_FALSE(updated.isVisible);
}

TEST_F(HighlightCommandsTest, ToggleHighlightVisibilityCommandUndo) {
    TextHighlight highlight = createTestHighlight();
    highlight.isVisible = true;
    model->addHighlight(highlight);

    auto* cmd = new ToggleHighlightVisibilityCommand(model, highlight.id);
    undoStack->push(cmd);

    undoStack->undo();
    TextHighlight restored = model->getHighlight(highlight.id);
    EXPECT_TRUE(restored.isVisible);
}

// ClearAllHighlightsCommand Tests

TEST_F(HighlightCommandsTest, ClearAllHighlightsCommand) {
    model->addHighlight(createTestHighlight("text 1"));
    model->addHighlight(createTestHighlight("text 2"));
    model->addHighlight(createTestHighlight("text 3"));

    auto* cmd = new ClearAllHighlightsCommand(model);
    undoStack->push(cmd);

    EXPECT_EQ(model->getTotalHighlightCount(), 0);
}

TEST_F(HighlightCommandsTest, ClearAllHighlightsCommandUndo) {
    model->addHighlight(createTestHighlight("text 1"));
    model->addHighlight(createTestHighlight("text 2"));
    model->addHighlight(createTestHighlight("text 3"));

    auto* cmd = new ClearAllHighlightsCommand(model);
    undoStack->push(cmd);

    undoStack->undo();
    EXPECT_EQ(model->getTotalHighlightCount(), 3);
}

// BatchAddHighlightsCommand Tests

TEST_F(HighlightCommandsTest, BatchAddHighlightsCommand) {
    QList<TextHighlight> highlights;
    highlights.append(createTestHighlight("text 1"));
    highlights.append(createTestHighlight("text 2"));
    highlights.append(createTestHighlight("text 3"));

    auto* cmd = new BatchAddHighlightsCommand(model, highlights);
    undoStack->push(cmd);

    EXPECT_EQ(model->getTotalHighlightCount(), 3);
}

TEST_F(HighlightCommandsTest, BatchAddHighlightsCommandUndo) {
    QList<TextHighlight> highlights;
    highlights.append(createTestHighlight("text 1"));
    highlights.append(createTestHighlight("text 2"));

    auto* cmd = new BatchAddHighlightsCommand(model, highlights);
    undoStack->push(cmd);

    undoStack->undo();
    EXPECT_EQ(model->getTotalHighlightCount(), 0);
}

// BatchRemoveHighlightsCommand Tests

TEST_F(HighlightCommandsTest, BatchRemoveHighlightsCommand) {
    TextHighlight h1 = createTestHighlight("text 1");
    TextHighlight h2 = createTestHighlight("text 2");
    TextHighlight h3 = createTestHighlight("text 3");

    model->addHighlight(h1);
    model->addHighlight(h2);
    model->addHighlight(h3);

    QStringList ids;
    ids << h1.id << h2.id;

    auto* cmd = new BatchRemoveHighlightsCommand(model, ids);
    undoStack->push(cmd);

    EXPECT_EQ(model->getTotalHighlightCount(), 1);
}

TEST_F(HighlightCommandsTest, BatchRemoveHighlightsCommandUndo) {
    TextHighlight h1 = createTestHighlight("text 1");
    TextHighlight h2 = createTestHighlight("text 2");

    model->addHighlight(h1);
    model->addHighlight(h2);

    QStringList ids;
    ids << h1.id << h2.id;

    auto* cmd = new BatchRemoveHighlightsCommand(model, ids);
    undoStack->push(cmd);

    undoStack->undo();
    EXPECT_EQ(model->getTotalHighlightCount(), 2);
}

// UpdateHighlightCommand Tests

TEST_F(HighlightCommandsTest, UpdateHighlightCommand) {
    TextHighlight highlight = createTestHighlight("original");
    model->addHighlight(highlight);

    TextHighlight updated = highlight;
    updated.text = "updated";
    updated.note = "new note";
    updated.color = Qt::green;

    auto* cmd = new UpdateHighlightCommand(model, highlight.id, updated);
    undoStack->push(cmd);

    TextHighlight result = model->getHighlight(highlight.id);
    EXPECT_EQ(result.text, "updated");
    EXPECT_EQ(result.note, "new note");
    EXPECT_EQ(result.color, Qt::green);
}

TEST_F(HighlightCommandsTest, UpdateHighlightCommandUndo) {
    TextHighlight highlight = createTestHighlight("original");
    highlight.note = "original note";
    model->addHighlight(highlight);

    TextHighlight updated = highlight;
    updated.text = "updated";
    updated.note = "new note";

    auto* cmd = new UpdateHighlightCommand(model, highlight.id, updated);
    undoStack->push(cmd);

    undoStack->undo();
    TextHighlight restored = model->getHighlight(highlight.id);
    EXPECT_EQ(restored.text, "original");
    EXPECT_EQ(restored.note, "original note");
}

// Command Factory Tests

TEST_F(HighlightCommandsTest, CommandFactory) {
    TextHighlight highlight = createTestHighlight();

    auto* addCmd = HighlightCommandFactory::createAddCommand(model, highlight);
    EXPECT_NE(addCmd, nullptr);
    delete addCmd;

    auto* removeCmd =
        HighlightCommandFactory::createRemoveCommand(model, "test-id");
    EXPECT_NE(removeCmd, nullptr);
    delete removeCmd;

    auto* editNoteCmd = HighlightCommandFactory::createEditNoteCommand(
        model, "test-id", "note");
    EXPECT_NE(editNoteCmd, nullptr);
    delete editNoteCmd;
}

// Main function
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

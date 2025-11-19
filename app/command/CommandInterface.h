#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

/**
 * @brief CommandInterface - Base interface for all commands
 *
 * Defines the standard command interface that all commands should implement.
 * Provides the basic command pattern functionality with execution, undo,
 * serialization support.
 */
class CommandInterface {
public:
    // Explicitly delete copy and move operations
    CommandInterface(const CommandInterface&) = delete;
    CommandInterface& operator=(const CommandInterface&) = delete;
    CommandInterface(CommandInterface&&) = delete;
    CommandInterface& operator=(CommandInterface&&) = delete;

    // Command identification
    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual QString description() const = 0;

    // Command execution
    [[nodiscard]] virtual bool canExecute() const = 0;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual void reset() = 0;

    // Serialization
    [[nodiscard]] virtual QJsonObject serialize() const = 0;
    virtual void deserialize(const QJsonObject& data) = 0;

protected:
    CommandInterface() = default;
    virtual ~CommandInterface() = default;
};

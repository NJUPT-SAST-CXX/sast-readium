#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 * @brief CommandInterface - Base interface for all commands
 *
 * Defines the standard command interface that all commands should implement.
 * Provides the basic command pattern functionality with execution, undo,
 * serialization support.
 */
class CommandInterface {
public:
    virtual ~CommandInterface() = default;

    // Command identification
    virtual QString name() const = 0;
    virtual QString description() const = 0;

    // Command execution
    virtual bool canExecute() const = 0;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual void reset() = 0;

    // Serialization
    virtual QJsonObject serialize() const = 0;
    virtual void deserialize(const QJsonObject& data) = 0;
};
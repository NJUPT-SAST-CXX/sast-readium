#pragma once

#include <QObject>
#include <QMessageBox>

class PageController;

class Command : public QObject {
    Q_OBJECT

public:
    explicit Command(QObject* parent = nullptr);
    virtual ~Command() = default;

public slots:
    virtual void execute() = 0;
};

class PrevPageCommand : public Command {
    Q_OBJECT

public:
    explicit PrevPageCommand(PageController* controller, QObject* parent = nullptr);

public slots:
    void execute() override;

private:
    PageController* _controller;
};

class NextPageCommand : public Command {
    Q_OBJECT

public:
    explicit NextPageCommand(PageController* controller, QObject* parent = nullptr);

public slots:
    void execute() override;

private:
    PageController* _controller;
};
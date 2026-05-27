/*
 * LXQt Organizer - personal information manager for LXQt.
 * Copyright (C) 2026 Basil Crow
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "taskdialogflow.h"

#include "calendareditordata.h"
#include "calendareditormapper.h"
#include "calendarsnapshot.h"
#include "taskdialog.h"

#include <QCoreApplication>
#include <QDialog>

namespace {

QLocale currentLocale(const WindowServices *services)
{
    return services ? services->locale() : QLocale::system();
}

QString controllerText(const char *sourceText)
{
    return QCoreApplication::translate("CalendarPaneController", sourceText);
}

} // namespace

TaskDialogFlow::TaskDialogFlow(QWidget *parentWidget, const WindowServices &services)
    : m_parent(parentWidget)
    , m_services(&services)
{}

std::optional<TaskDialogFlow::EditResult>
TaskDialogFlow::create(const QDate &selectedDate, const QList<QPair<QString, QString>> &collectionOptions) const
{
    TaskFields taskData;
    Task task = CalendarEditorMapper::createTask(taskData);
    return editTaskInDialog(task, controllerText("New Task"), selectedDate, collectionOptions);
}

std::optional<TaskDialogFlow::EditResult> TaskDialogFlow::edit(
    const Task &existing, const QDate &selectedDate, const QList<QPair<QString, QString>> &collectionOptions) const
{
    return editTaskInDialog(
        CalendarSnapshot::cloneTask(existing), controllerText("Edit Task"), selectedDate, collectionOptions);
}

std::optional<TaskDialogFlow::EditResult>
TaskDialogFlow::editTaskInDialog(Task task,
                                 const QString &windowTitle,
                                 const QDate &selectedDate,
                                 const QList<QPair<QString, QString>> &collectionOptions) const
{
    if (!m_parent || task.todo.isNull())
    {
        return std::nullopt;
    }

    TaskDialog::State dialogState;
    dialogState.data = CalendarEditorMapper::fromTask(task, selectedDate);
    dialogState.windowTitle = windowTitle;
    dialogState.collectionOptions = collectionOptions;
    dialogState.locale = currentLocale(m_services);

    TaskDialog taskDialog(m_parent, dialogState);
    if (taskDialog.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    const TaskFields taskData = taskDialog.data();
    return EditResult{CalendarEditorMapper::applyToTask(task, taskData), taskData.collectionId};
}

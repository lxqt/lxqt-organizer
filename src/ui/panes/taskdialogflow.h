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

#ifndef TASKDIALOGFLOW_H
#define TASKDIALOGFLOW_H

#include "calendaritem.h"
#include "windowservices.h"

#include <QDate>
#include <QList>
#include <QPair>
#include <QString>

#include <optional>

class QWidget;

// "Flow" = stateless dialog runner that returns a DTO.
class TaskDialogFlow
{
public:
    struct EditResult
    {
        Task task;
        QString destinationCollectionId;
    };

    TaskDialogFlow(QWidget *parentWidget, const WindowServices &services);

    std::optional<EditResult> create(const QDate &selectedDate,
                                     const QList<QPair<QString, QString>> &collectionOptions) const;
    std::optional<EditResult> edit(const Task &existing,
                                   const QDate &selectedDate,
                                   const QList<QPair<QString, QString>> &collectionOptions) const;

private:
    std::optional<EditResult> editTaskInDialog(Task task,
                                               const QString &windowTitle,
                                               const QDate &selectedDate,
                                               const QList<QPair<QString, QString>> &collectionOptions) const;

    QWidget *m_parent = nullptr;
    const WindowServices *m_services = nullptr;
};

#endif // TASKDIALOGFLOW_H

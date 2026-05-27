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

#ifndef EVENTDIALOGFLOW_H
#define EVENTDIALOGFLOW_H

#include "calendareditordata.h"
#include "calendaritem.h"
#include "windowservices.h"

#include <QDate>
#include <QList>
#include <QPair>
#include <QString>
#include <QTime>

#include <optional>

class EventReader;
class QWidget;

// "Flow" = stateless dialog runner that returns a DTO.
class EventDialogFlow
{
public:
    struct EditResult
    {
        EventFields fields;
        QString destinationCollectionId;
    };

    EventDialogFlow(QWidget *parentWidget, const EventReader *reader, const WindowServices &services);

    std::optional<EditResult> create(const QDate &on,
                                     const QList<QPair<QString, QString>> &collectionOptions,
                                     const QTime &initialStart = {},
                                     const QTime &initialEnd = {}) const;
    std::optional<EditResult> edit(const EventOccurrence &occurrence,
                                   const QList<QPair<QString, QString>> &collectionOptions) const;

private:
    QWidget *m_parent = nullptr;
    const EventReader *m_reader = nullptr;
    const WindowServices *m_services = nullptr;
};

#endif // EVENTDIALOGFLOW_H

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

#include "eventdialogflow.h"

#include "eventdialog.h"
#include "eventservice.h"
#include "preferencescontroller.h"

#include <QDialog>

EventDialogFlow::EventDialogFlow(QWidget *parentWidget,
                                 const EventService *eventService,
                                 const PreferencesController &preferences)
    : m_parent(parentWidget)
    , m_eventService(eventService)
    , m_preferences(&preferences)
{}

std::optional<EventDialogFlow::EditResult>
EventDialogFlow::create(const QDate &on,
                        const QList<QPair<QString, QString>> &collectionOptions,
                        const QTime &initialStart,
                        const QTime &initialEnd) const
{
    EventDialog::State dialogState;
    dialogState.data = m_eventService->defaultEventEditorData(on);
    dialogState.collectionOptions = collectionOptions;
    dialogState.locale = m_preferences->locale();
    if (initialStart.isValid() && initialEnd.isValid() && initialStart < initialEnd)
    {
        dialogState.data.startTime = initialStart;
        dialogState.data.endTime = initialEnd;
    }

    EventDialog eventDialog(m_parent, dialogState);
    eventDialog.setModal(true);
    if (eventDialog.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    const EventFields eventData = eventDialog.data();
    return EditResult{eventData, eventData.collectionId};
}

std::optional<EventDialogFlow::EditResult>
EventDialogFlow::edit(const EventOccurrence &occurrence, const QList<QPair<QString, QString>> &collectionOptions) const
{
    EventDialog::State dialogState;
    dialogState.data = m_eventService->editorDataForOccurrence(occurrence);
    dialogState.collectionOptions = collectionOptions;
    dialogState.locale = m_preferences->locale();
    dialogState.mode = EventDialog::Mode::Edit;

    EventDialog eventDialog(m_parent, dialogState);
    eventDialog.setModal(true);
    if (eventDialog.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    const EventFields eventData = eventDialog.data();
    const QString destinationCollectionId =
        eventData.collectionId.isEmpty() ? occurrence.ref.item.collectionId : eventData.collectionId;
    return EditResult{eventData, destinationCollectionId};
}

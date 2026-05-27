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

#include "calendarpaneutils.h"

#include "eventdatetime.h"
#include "eventoccurrences.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QAction>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QStringList>
#include <QWidget>

#include <algorithm>

namespace CalendarPaneUtils {

bool eventStartsBefore(const EventOccurrence &first, const EventOccurrence &second)
{
    return first.start.time() < second.start.time();
}

QDate firstDateOfMonth(int year, int month)
{
    return QDate(year, month, 1);
}

QDate lastDateOfMonth(int year, int month)
{
    const QDate first = firstDateOfMonth(year, month);
    return first.isValid() ? first.addMonths(1).addDays(-1) : QDate();
}

bool sameOccurrence(const EventOccurrence &first, const EventOccurrence &second)
{
    return first.ref.item.collectionId == second.ref.item.collectionId && first.ref.item.href == second.ref.item.href &&
           first.ref.uid == second.ref.uid && first.start == second.start;
}

bool sameItemKey(const ItemRef &ref, const ItemKey &key)
{
    return ref.collectionId == key.collectionId && ref.href == key.href;
}

void appendUniqueEventOccurrences(QList<EventOccurrence> &target, const QList<EventOccurrence> &source)
{
    for (const EventOccurrence &event : source)
    {
        const auto same = [&event](const EventOccurrence &existing) { return sameOccurrence(existing, event); };
        if (std::find_if(target.cbegin(), target.cend(), same) == target.cend())
        {
            target.append(event);
        }
    }
}

bool removeEventsByKey(QList<EventOccurrence> &events, const ItemKey &key)
{
    const qsizetype originalSize = events.size();
    events.erase(std::remove_if(events.begin(),
                                events.end(),
                                [&key](const EventOccurrence &event) { return sameItemKey(event.ref.item, key); }),
                 events.end());
    return events.size() != originalSize;
}

bool removeEventRef(QList<EventOccurrence> &events, const OccurrenceRef &event)
{
    const qsizetype originalSize = events.size();
    events.erase(std::remove_if(events.begin(),
                                events.end(),
                                [&event](const EventOccurrence &candidate) {
                                    return sameItemKey(candidate.ref.item, event.item.key()) &&
                                           candidate.ref.uid == event.uid;
                                }),
                 events.end());
    return events.size() != originalSize;
}

bool sameTask(const Task &first, const Task &second)
{
    return first.ref.collectionId == second.ref.collectionId && first.ref.href == second.ref.href &&
           first.uid == second.uid;
}

bool taskOccursOn(const Task &task, const QDate &date)
{
    return date.isValid() && CalendarSnapshot::hasTask(task) && CalendarSnapshot::taskDueDate(task) == date;
}

QTime taskSortTime(const Task &task)
{
    if (!CalendarSnapshot::hasTask(task))
    {
        return QTime(23, 59);
    }
    const QTime dueTime = CalendarSnapshot::taskDueTime(task);
    return dueTime.isValid() ? dueTime : QTime(23, 59);
}

QString priorityStatusText(int priority)
{
    switch (EventDateTime::normalizedPriority(priority))
    {
    case 1:
        return QCoreApplication::translate("CalendarPane", "Priority: High");
    case 9:
        return QCoreApplication::translate("CalendarPane", "Priority: Low");
    default:
        return QCoreApplication::translate("CalendarPane", "Priority: Medium");
    }
}

QUrl openableUrlFromText(const QString &text, bool preferLocalFile)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
    {
        return {};
    }
    const QUrl url(trimmed);
    if (url.isValid() && !url.scheme().isEmpty())
    {
        return url;
    }
    return preferLocalFile ? QUrl::fromLocalFile(trimmed) : QUrl::fromUserInput(trimmed);
}

CalendarPaneActions createCalendarPaneActions(QObject *owner)
{
    auto tr = [](const char *text) { return QCoreApplication::translate("CalendarPane", text); };
    auto action = [owner](const QString &icon,
                          const QString &text,
                          const QString &objectName,
                          const QStringList &contextIds,
                          const QKeySequence &shortcut = {},
                          const QString &iconText = {}) {
        QAction *created = new QAction(QIcon::fromTheme(icon), text, owner);
        created->setObjectName(objectName);
        created->setProperty("contextActionIds", contextIds);
        if (!shortcut.isEmpty())
        {
            created->setShortcut(shortcut);
        }
        if (!iconText.isEmpty())
        {
            created->setIconText(iconText);
        }
        return created;
    };

    CalendarPaneActions actions;
    actions.newEvent = action(QStringLiteral("view-calendar-day"),
                              tr("New Event"),
                              QStringLiteral("actionNew_Event"),
                              {QStringLiteral("actionNew"), QStringLiteral("actionNew_Event")},
                              QKeySequence::New,
                              tr("New"));
    actions.newTask = action(QStringLiteral("view-calendar-tasks"),
                             tr("New Task"),
                             QStringLiteral("actionNew_Task"),
                             {QStringLiteral("actionNew_Task")},
                             QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_N),
                             tr("Task"));
    actions.editItem = action(
        QStringLiteral("document-edit"), tr("Edit"), QStringLiteral("actionEdit"), {QStringLiteral("actionEdit")});
    actions.deleteItem = action(QStringLiteral("edit-delete"),
                                tr("Delete"),
                                QStringLiteral("actionDelete"),
                                {QStringLiteral("actionDelete")},
                                QKeySequence::Delete);
    actions.find = action(QStringLiteral("edit-find"),
                          tr("Find"),
                          QStringLiteral("actionFind"),
                          {QStringLiteral("actionFind")},
                          QKeySequence::Find);
    actions.findNext = action(QStringLiteral("go-down"),
                              tr("Find Next"),
                              QStringLiteral("actionFindNext"),
                              {QStringLiteral("actionFindNext")},
                              QKeySequence::FindNext);
    actions.findPrevious = action(QStringLiteral("go-up"),
                                  tr("Find Previous"),
                                  QStringLiteral("actionFindPrev"),
                                  {QStringLiteral("actionFindPrev")},
                                  QKeySequence::FindPrevious);
    return actions;
}

void showTimelineContextMenu(QWidget *parent,
                             const EventOccurrence &occurrence,
                             const QTime &startTime,
                             const QTime &endTime,
                             const QPoint &globalPos,
                             const std::function<void(const QTime &, const QTime &)> &newEvent,
                             const std::function<void(const EventOccurrence &)> &editEvent,
                             const std::function<void(const EventOccurrence &, bool)> &setEventCompleted,
                             const std::function<void(const EventOccurrence &)> &deleteEvent)
{
    auto tr = [](const char *text) { return QCoreApplication::translate("CalendarPane", text); };
    QMenu menu(parent);
    QAction *newAction = menu.addAction(QIcon::fromTheme(QStringLiteral("view-calendar-day")), tr("New Event"));
    menu.addSeparator();
    QAction *editAction = menu.addAction(QIcon::fromTheme(QStringLiteral("document-edit")), tr("Edit Event"));
    const bool eventCompleted = CalendarSnapshot::eventDisplay(occurrence).completed;
    QAction *completedAction = menu.addAction(eventCompleted ? tr("Mark as Not Completed") : tr("Mark as Completed"));
    menu.addSeparator();
    QAction *openUrlAction = menu.addAction(tr("Open URL"));
    QAction *openAttachmentAction =
        menu.addAction(QIcon::fromTheme(QStringLiteral("mail-attachment")), tr("Open Attachment"));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), tr("Delete Event"));
    const bool hasEvent = !occurrence.ref.item.href.isEmpty();
    const EventDisplay display = CalendarSnapshot::eventDisplay(occurrence);
    const QUrl eventUrl = hasEvent ? openableUrlFromText(display.url.toString(), false) : QUrl();
    const QUrl attachmentUrl = hasEvent ? openableUrlFromText(display.attachmentUri, true) : QUrl();
    editAction->setEnabled(hasEvent);
    completedAction->setEnabled(hasEvent);
    openUrlAction->setEnabled(eventUrl.isValid() && !eventUrl.isEmpty());
    openAttachmentAction->setEnabled(attachmentUrl.isValid() && !attachmentUrl.isEmpty());
    deleteAction->setEnabled(hasEvent);

    QAction *selectedAction = menu.exec(globalPos);
    if (selectedAction == newAction)
    {
        newEvent(startTime, endTime);
    }
    else if (selectedAction == editAction && hasEvent)
    {
        editEvent(occurrence);
    }
    else if (selectedAction == completedAction && hasEvent)
    {
        setEventCompleted(occurrence, !eventCompleted);
    }
    else if (selectedAction == openUrlAction && openUrlAction->isEnabled() && !QDesktopServices::openUrl(eventUrl))
    {
        QMessageBox::warning(parent, tr("Organizer"), tr("Could not open the event URL."));
    }
    else if (selectedAction == openAttachmentAction && openAttachmentAction->isEnabled() &&
             !QDesktopServices::openUrl(attachmentUrl))
    {
        QMessageBox::warning(parent, tr("Organizer"), tr("Could not open the event attachment."));
    }
    else if (selectedAction == deleteAction && hasEvent)
    {
        deleteEvent(occurrence);
    }
}

bool selectAdjacentCalendarItem(int delta,
                                const QDate &date,
                                const QList<EventOccurrence> &dayEvents,
                                const QList<Task> &tasks,
                                const EventOccurrence &selectedEvent,
                                const Task &selectedTask,
                                const std::function<bool(const EventOccurrence &)> &selectEvent,
                                const std::function<bool(const Task &)> &selectTask)
{
    if (delta == 0 || !date.isValid())
    {
        return false;
    }

    struct CalendarSelection
    {
        enum class Type
        {
            Event,
            Task
        };
        Type type;
        QTime sortTime;
        EventOccurrence event;
        Task task;
    };

    QList<CalendarSelection> items;
    for (const EventOccurrence &event : dayEvents)
    {
        items.append({CalendarSelection::Type::Event, event.start.time(), event, {}});
    }
    for (const Task &task : tasks)
    {
        if (taskOccursOn(task, date))
        {
            items.append({CalendarSelection::Type::Task, taskSortTime(task), {}, task});
        }
    }
    if (items.isEmpty())
    {
        return false;
    }

    std::sort(items.begin(), items.end(), [](const CalendarSelection &first, const CalendarSelection &second) {
        return first.sortTime == second.sortTime ? static_cast<int>(first.type) < static_cast<int>(second.type)
                                                 : first.sortTime < second.sortTime;
    });

    int currentIndex = -1;
    for (int i = 0; i < items.size(); ++i)
    {
        const CalendarSelection &item = items.at(i);
        if ((item.type == CalendarSelection::Type::Event && CalendarSnapshot::hasEventRef(selectedEvent) &&
             sameOccurrence(item.event, selectedEvent)) ||
            (item.type == CalendarSelection::Type::Task && CalendarSnapshot::hasTask(selectedTask) &&
             sameTask(item.task, selectedTask)))
        {
            currentIndex = i;
            break;
        }
    }

    const int nextIndex =
        currentIndex < 0 ? (delta > 0 ? 0 : items.size() - 1) : (currentIndex + delta + items.size()) % items.size();
    const CalendarSelection item = items.at(nextIndex);
    return item.type == CalendarSelection::Type::Event ? selectEvent(item.event) : selectTask(item.task);
}

} // namespace CalendarPaneUtils

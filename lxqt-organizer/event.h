/***************************************************************************
 *   Author aka. crispina                 *
 *   crispinalan@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/


#ifndef EVENT_H
#define EVENT_H


#include <QString>


struct Event {
    int id;
    QString title;
    QString location;
    int day;
    int month;
    int year;
    int startTime;
    int endTime;
    int reminderDay;
    int reminderMonth;
    int reminderYear;
    int reminderTime;

    Event(int id, const QString& title, const QString& location,
                int day, int month, int year,
                int startTime, int endTime,
                int reminderDay, int reminderMonth, int reminderYear,
                int reminderTime ) :
        id(id),
        title(title),
        location(location),
        day(day),
        month(month),
        year(year),
        startTime(startTime),
        endTime(endTime),
        reminderDay(reminderDay),
        reminderMonth(reminderMonth),
        reminderYear(reminderYear),
        reminderTime(reminderTime)
    {}
};


#endif // EVENT_H

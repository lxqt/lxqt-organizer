/***************************************************************************
 *   Author Alan Crispin aka. crispina                                     *
 *   crispinalan@gmail.com                                                 *
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


#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include <QString>
struct Appointment {

    Appointment(int id=0,
                const QString& title= QString(),
                const QString& location= QString(),
                const QString& description= QString(),
                const QString& date=QString(),
                const QString& startTime=QString(),                
                const QString& endTime=QString(),                
                int isFullDay =0,
                int hasReminder=0,
                int reminderMins=0
                ):
        m_id(id),
        m_title(title),
        m_location(location),
        m_description(description),
        m_date(date),
        m_startTime(startTime),
        m_endTime(endTime),        
        m_isFullDay(isFullDay),
        m_hasReminder(hasReminder),
        m_reminderMinutes(reminderMins)
    {
    }
   int m_id;
   QString m_title;
   QString m_location;
   QString m_description;
   QString m_date;
   QString m_startTime;
   QString m_endTime;
   int m_isFullDay;
   int m_hasReminder;
   int m_reminderMinutes;
};


#endif // APPOINTMENT_H

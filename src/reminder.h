/***************************************************************************
 *   Author Alan Crispin aka. crispina                 *
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


#ifndef REMINDER_H
#define REMINDER_H

#include <QString>
struct Reminder {

    Reminder( int appointmentId=0,
                const QString& details= QString(),
                const QString& reminderDate=QString(),
                const QString& reminderTime=QString()
                //int reminderRequest=0 //bool 0=no 1 =yes
                ):

        m_appointmentId(appointmentId),
        m_details(details),
        m_reminderDate(reminderDate),
        m_reminderTime(reminderTime)
        //m_reminderRequest(reminderRequest)
    {
    }

   int m_appointmentId;
   QString m_details;
   QString m_reminderDate;
   QString m_reminderTime;
   //int m_reminderRequest;

};





#endif // REMINDER_H

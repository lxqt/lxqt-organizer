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

#ifndef DIALOGUPCOMINGSCHEDULE_H
#define DIALOGUPCOMINGSCHEDULE_H

#include <QDialog>
#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QLocale>

#include "appointment.h"
#include "schedulelistmodel.h"
#include "dbmanager.h"

namespace Ui {
class DialogUpcomingSchedule;
}

class DialogUpcomingSchedule : public QDialog
{
    Q_OBJECT

public:
    //explicit DialogUpcomingSchedule(QWidget *parent = nullptr);


    explicit DialogUpcomingSchedule(QWidget *parent = nullptr,
                                  DbManager *theDbm=nullptr);
     QList<Appointment> getSortedDayList(QDate theDate);
    static bool compare(const Appointment& first, const Appointment& second);
    ~DialogUpcomingSchedule();

    QString t_title=QStringLiteral("Upcoming Schedule (Next Seven Days)");
    QString t_label_upcoming_schedule=QStringLiteral("Upcoming Schedule For Date");

    void setTitleTranslation(QString translation);
    void setLabelUpcomingScheduleTranslation(QString translation);

    QLocale locale=QLocale::English;
    void setLocale(QLocale locale);

private:
    Ui::DialogUpcomingSchedule *ui;
    QList<Appointment> appointmentList;
    ScheduleListModel *scheduleListModel;
};

#endif // DIALOGUPCOMINGSCHEDULE_H

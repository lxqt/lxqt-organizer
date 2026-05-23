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

#include "dialogupcomingschedule.h"
#include "ui_dialogupcomingschedule.h"


DialogUpcomingSchedule::DialogUpcomingSchedule(QWidget *parent,
                                               DbManager *theDbm) :
    QDialog(parent),
    ui(new Ui::DialogUpcomingSchedule)
{
     ui->setupUi(this);

     setWindowTitle(t_title);
     ui->labelSchedule->setText(t_label_upcoming_schedule);

     this->appointmentList= theDbm->getAllAppointments();

     QList<Appointment> schedule =QList<Appointment>();
     //QList<Appointment> dayList =QList<Appointment>();
     QDate currentDate =QDate::currentDate();
     QString date =locale.toString(currentDate,QStringLiteral("dddd dd MMMM yyyy"));
     ui->labelDate->setText(date);

     //selectedDateLabel->setText(date);
     //QDate currentDate =QDate::currentDate();
     ui->labelDate->setText(currentDate.toString());

     QDate currentDatePlusOne=currentDate.addDays(1);
     QDate currentDatePlusTwo=currentDate.addDays(2);
     QDate currentDatePlusThree=currentDate.addDays(3);
     QDate currentDatePlusFour=currentDate.addDays(4);
     QDate currentDatePlusFive=currentDate.addDays(5);
     QDate currentDatePlusSix=currentDate.addDays(6);
     QDate currentDatePlusSeven=currentDate.addDays(7);

     QList<Appointment> dayList1 =getSortedDayList(currentDatePlusOne);
     QList<Appointment> dayList2 =getSortedDayList(currentDatePlusTwo);
     QList<Appointment> dayList3 =getSortedDayList(currentDatePlusThree);
     QList<Appointment> dayList4 =getSortedDayList(currentDatePlusFour);
     QList<Appointment> dayList5 =getSortedDayList(currentDatePlusFive);
     QList<Appointment> dayList6 =getSortedDayList(currentDatePlusSix);
     QList<Appointment> dayList7 =getSortedDayList(currentDatePlusSeven);

     schedule.append(dayList1);
     schedule.append(dayList2);
     schedule.append(dayList3);
     schedule.append(dayList4);
     schedule.append(dayList5);
     schedule.append(dayList6);
     schedule.append(dayList7);

     scheduleListModel = new ScheduleListModel(schedule);
     ui->listViewSchedule->setModel(scheduleListModel);
}

QList<Appointment> DialogUpcomingSchedule::getSortedDayList(QDate theDate)
{
    QList<Appointment> dayList =QList<Appointment>();
    foreach(Appointment a, appointmentList)
    {
        QDate adate = QDate::fromString(a.m_date);

        if(adate==theDate)
        {
            dayList.append(a);
        }
    }
    std::sort(dayList.begin(), dayList.end(), compare);
   return dayList;

}

bool DialogUpcomingSchedule::compare(const Appointment &first, const Appointment &second)
{
    QTime firstStarts =QTime::fromString(first.m_startTime);
    QTime secondStarts =QTime::fromString(second.m_startTime);

    int fx=60*60*firstStarts.hour()+60*firstStarts.minute(); //seconds
    int sx=60*60*secondStarts.hour()+60*secondStarts.minute();

    if (fx < sx)
    {
        return true;
    }
    else if (fx > sx)
    {
        return false;
    }
    return false;
}

DialogUpcomingSchedule::~DialogUpcomingSchedule()
{
    delete ui;
}

void DialogUpcomingSchedule::setTitleTranslation(QString translation)
{
    t_title=translation;
    setWindowTitle(t_title);
}

void DialogUpcomingSchedule::setLabelUpcomingScheduleTranslation(QString translation)
{
    t_label_upcoming_schedule=translation;
    ui->labelSchedule->setText(t_label_upcoming_schedule);
}

void DialogUpcomingSchedule::setLocale(QLocale locale)
{
    this->locale=locale;
    QDate currentDate =QDate::currentDate();
    QString date =locale.toString(currentDate,QStringLiteral("dddd dd MMMM yyyy"));
    ui->labelDate->setText(date);
}



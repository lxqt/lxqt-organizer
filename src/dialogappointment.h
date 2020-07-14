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


#ifndef DIALOGAPPOINTMENT_H
#define DIALOGAPPOINTMENT_H

#include <QDialog>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QMessageBox>
#include "appointment.h"

namespace Ui {
class DialogAppointment;
}

class DialogAppointment : public QDialog
{
    Q_OBJECT

public:
    explicit DialogAppointment(QWidget *parent = nullptr, QDate *theAppointmentDate=nullptr);

    ~DialogAppointment();

    //Appointment appointment;

    QDate appointmentDate;
    QTime startTime;
    QTime endTime;
    QString category;    

    int isAllDay=0; //no
    bool deleteRequested=false;
    QString currentTitle=QStringLiteral("");

    QString getTitle();
    QString getLocation();
    QString getDescription();
    QDate getAppointmentDate();
    QTime getStartTime();
    QTime getEndTime(); 
    int getAllDay();


    bool getDeleteRequested();
    void setupComboBoxes();

    //Reminders
    int hasReminder=0; //no
    int getHasReminder();
    int reminderMins=0; //none
    int getReminderMins();

    void setComboBoxReminderTranslations();

     QLocale locale=QLocale::English;
     void setLocaleDate(QLocale locale);

private slots:

    void accept();
    void reject();

    void on_timeEditStartTime_userTimeChanged(const QTime &time);

    void on_timeEditEndTime_userTimeChanged(const QTime &time);

    void on_checkBoxAllDay_stateChanged(int arg1);

    void on_comboBoxReminder_currentIndexChanged(int index);

    void on_checkBoxReminder_stateChanged(int arg1);

private:
    Ui::DialogAppointment *ui;
};

#endif // DIALOGAPPOINTMENT_H

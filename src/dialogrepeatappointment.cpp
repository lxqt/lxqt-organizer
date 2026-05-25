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

#include "dialogrepeatappointment.h"
#include "ui_dialogrepeatappointment.h"

DialogRepeatAppointment::DialogRepeatAppointment(QWidget *parent, QDate *theAppointmentDate):
    QDialog(parent),
    ui(new Ui::DialogRepeatAppointment)
{
    ui->setupUi(this);

    this->appointmentDate=*theAppointmentDate;
    QTime timeNow =QTime::currentTime();
    this->startTime=timeNow;
    this->endTime=timeNow;
    ui->timeEditStarts->setTime(timeNow);
    ui->timeEditEnds->setTime(timeNow);
    ui->spinBoxDays->setRange(1,28); //upto 4 weeks apart
    ui->spinBoxDays->setValue(1);
    ui->spinBoxRepeatNumber->setRange(0,12); //over 3 months
    ui->spinBoxRepeatNumber->setValue(1);

}

DialogRepeatAppointment::~DialogRepeatAppointment()
{
    delete ui;
}

int DialogRepeatAppointment::getRepeatDayInterval()
{
    return repeatDayInterval;
}

int DialogRepeatAppointment::getRepeatNumber()
{
    return repeatNumber;
}

QString DialogRepeatAppointment::getTitle()
{
   return  ui->lineEditTitle->text();
}

QString DialogRepeatAppointment::getLocation()
{
    return ui->lineEditLocation->text();
}

QString DialogRepeatAppointment::getDescription()
{

    return ui->textEditDescription->toPlainText();
}

QDate DialogRepeatAppointment::getAppointmentDate()
{
    return this->appointmentDate;
}

QTime DialogRepeatAppointment::getStartTime()
{
    return this->startTime;
}

QTime DialogRepeatAppointment::getEndTime()
{
    return this->endTime;
}

int DialogRepeatAppointment::getAllDay()
{
    return isAllDay;
}

void DialogRepeatAppointment::setDialogTitleTranslation(QString translation)
{
    t_dialog_title=translation;
}

void DialogRepeatAppointment::setStartDateDisplayTranslation(QString translation)
{
    t_start_date_display=translation;
}

void DialogRepeatAppointment::setTitleTranslation(QString translation)
{
    t_title=translation;
}

void DialogRepeatAppointment::setLocationTranslation(QString translation)
{
    t_location=translation;
}

void DialogRepeatAppointment::setNotesTranslations(QString translation)
{
    t_notes=translation;
}

void DialogRepeatAppointment::setAllDaytranslation(QString translation)
{
    t_all_day=translation;
}

void DialogRepeatAppointment::setStartTimeTranslation(QString translation)
{
    t_start_time=translation;
}

void DialogRepeatAppointment::setEndTimeTranslation(QString translation)
{
    t_end_time=translation;
}

void DialogRepeatAppointment::setRepeatEveryTranslation(QString translation)
{
    t_repeat_every=translation;
}

void DialogRepeatAppointment::setDaysTranslation(QString translation)
{
    t_days=translation;
}

void DialogRepeatAppointment::setOccurrencesTranslation(QString translation)
{
    t_occurrences=translation;
}

void DialogRepeatAppointment::setLabelTranslations()
{
    setWindowTitle(t_dialog_title);
    ui->labelInfo->setText(t_start_date_display);
    ui->labelTitle->setText(t_title);
    ui->labelLocation->setText(t_location);
    ui->labelNotes->setText(t_notes);
    ui->checkBox->setText(t_all_day);
    ui->labelStartTime->setText(t_start_time);
    ui->labelEndTime->setText(t_end_time);
    ui->labelRepeatEvery->setText(t_repeat_every);
    ui->labelDays->setText(t_days);
    ui->labelOccurrences->setText(t_occurrences);

}

void DialogRepeatAppointment::setLocaleDate(QLocale locale)
{
    this->locale=locale;
    QString date =locale.toString(appointmentDate,QLocale::LongFormat);
    ui->labelDate->setText(date);
}


void DialogRepeatAppointment::accept()
{
    if (this->getTitle().isEmpty() || this->getLocation().isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("Empty Details"),
                                 QStringLiteral("Must enter a title and location"));
        return;
    }
    else {
        QDialog::accept();
    }
}

void DialogRepeatAppointment::reject()
{
    QDialog::reject();
}

void DialogRepeatAppointment::on_timeEditStarts_userTimeChanged(const QTime &time)
{
    this->startTime=time;
}

void DialogRepeatAppointment::on_timeEditEnds_userTimeChanged(const QTime &time)
{
    this->endTime=time;
}

void DialogRepeatAppointment::on_checkBox_stateChanged(int arg1)
{
    if (arg1==Qt::Unchecked)
        {
            isAllDay=0;
            ui->timeEditStarts->setEnabled(true);
            ui->timeEditEnds->setEnabled(true);
            ui->labelStartTime->setEnabled(true);
            ui->labelEndTime->setEnabled(true);

        }
        else if (arg1==Qt::Checked) {
            isAllDay=1;
            ui->timeEditStarts->setEnabled(false);
            ui->timeEditEnds->setEnabled(false);
            ui->labelStartTime->setEnabled(false);
            ui->labelEndTime->setEnabled(false);
        }
}

void DialogRepeatAppointment::on_spinBoxDays_valueChanged(int arg1)
{
    repeatDayInterval=arg1;
}

void DialogRepeatAppointment::on_spinBoxRepeatNumber_valueChanged(int arg1)
{
    repeatNumber=arg1;
}

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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QTextEdit>
#include <QDebug>
#include <QDate>
#include <QTimer>
#include <QLabel>
#include <QtWidgets>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileDialog>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QTableWidgetItem>

#include <QSoundEffect>
#include <QLocale>

#include "appointment.h"
#include "contact.h"
#include "holiday.h"

#include "dbmanager.h"

#include "dialogappointment.h"
#include "dialogappointmentupdate.h"
#include "dialogcontact.h"
#include "contactmodel.h"
#include "proxymodelcontacts.h"
#include "dialogupcomingschedule.h"
#include "dialogupcomingbirthdays.h"
#include "dialogrepeatappointment.h"
#include "daylistmodel.h"
#include "dialogpreferences.h"
#include "preferences.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DbManager &dbm, QWidget *parent = nullptr);
    ~MainWindow();

    //Database
    DbManager &dbm;

    //Timers
    QTimer *timer;

    //Audio
    QSoundEffect *soundEffect;

    //Reminders
    void checkForReminders();



    //Preferences default

    int playAudio=0;   
    int darkCalendar=0;
    void setDarkCalendar(int darkCalendar);

   //QLocale locale=QLocale::English;
   QLocale locale=QLocale::system();

    //Translation strings
    QString t_title;
    //Holidays
    QString t_holiday_christmas;
    QString t_holiday_boxing;
    QString t_holiday_new_year;
    QString t_holiday_easter;
    QString t_holiday_good_friday;
    QString t_holiday_easter_monday;
    QString t_holiday_may_bank;
    QString t_holiday_spring_bank;
    QString t_holiday_summer_bank;
    //File
    QString t_file;
    QString t_file_export_appointments;
    QString t_file_import_appointments;
    QString t_file_export_contacts;
    QString t_file_import_contacts;
    QString t_file_exit;
    //Edit
    QString t_edit;
    QString t_edit_delete_all_appointments;
    QString t_edit_delete_all_contacts;
    QString t_edit_preferences;
    //Appointments
    QString t_appointments;
    QString t_appointments_new_appointment;
    QString t_appointments_generate_repeat_appointments;
    QString t_appointments_upcoming_schedule;
    //Calendar
    QString t_calendar;
    QString t_calendar_show_holidays;
    QString t_calendar_show_birthdays;
    QString t_calendar_next_month;
    QString t_calendar_previous_month;
    QString t_calendar_today;
    QString t_calendar_increase_font;
    QString t_calendar_decrease_font;
    QString t_calendar_reset_font;


    //Contacts
    QString t_contacts;
    QString t_contacts_new_contact;
    QString t_contacts_upcoming_birthdays;
    //Help
    QString t_help;
    QString t_help_about;
    QString t_about_text;
    //Contact tab
    QString t_contact_tab_first_name;
    QString t_contact_tab_mid_name;
    QString t_contact_tab_last_name;
    QString t_contact_tab_email;
    QString t_contact_tab_street;
    QString t_contact_tab_district;
    QString t_contact_tab_city;
    QString t_contact_tab_county;
    QString t_contact_tab_postcode;
    QString t_contact_tab_country;
    QString t_contact_tab_telephone;
    QString t_contact_tab_birthday;

    QString t_contact_tab_quick_full_view;
    QString t_contact_tab_mailto;

    //Preferences
    QString t_preferences_title;
    QString t_preferences_font_size;
    QString t_preferences_locale;
    QString t_preferences_play_audio;
    QString t_preferences_dark_calendar;
    QString t_preferences_line_spacing;

    //Upcoming dialogs
    QString t_upcoming_birthdays_title;
    QString t_upcoming_schedule_title;
    QString t_label_upcoming_schedule;

    //Appointment dialogs
    QString t_dialog_appointment_dialog_title;
    QString t_dialog_appointment_date_display;
    QString t_dialog_appointment_title;
    QString t_dialog_appointment_location;
    QString t_dialog_appointment_notes;
    QString t_dialog_appointment_all_day;
    QString t_dialog_appointment_start_time;
    QString t_dialog_appointment_end_time;    
    QString t_dialog_appointment_reminder;

    //update appointment
    QString t_dialog_appointment_update;
    QString t_dialog_appointment_delete;

    QString t_reminder_5min;
    QString t_reminder_10min;
    QString t_reminder_30min;
    QString t_reminder_1hour;
    QString t_reminder_1day;

    QString t_reminder_message_on;
    QString t_reminder_message_starts_at;

    //repeat appointments
    QString t_dialog_repeat_title;
    QString t_dialog_repeat_date_message;
    QString t_dialog_repeat_repeat_every;
    QString t_dialog_repeat_days;
    QString t_dialog_repeat_occurrences;

    //Contacts dialog

    QString t_dialog_contact_title;
    QString t_dialog_contact_first_name;
    QString t_dialog_contact_mid_name;
    QString t_dialog_contact_last_name;
    QString t_dialog_contact_email;
    QString t_dialog_contact_street;
    QString t_dialog_contact_district;
    QString t_dialog_contact_city;
    QString t_dialog_contact_county;
    QString t_dialog_contact_postcode;
    QString t_dialog_contact_country;
    QString t_dialog_contact_telephone;

    QString t_dialog_contact_born_on;
    QString t_dialog_contact_birthday_unknown;
    QString t_dialog_contact_add_to_calendar;
    QString t_dialog_contact_delete;

    int applicationFontSize=14;
    int newLineSpacing=0;
    Preferences currentPreferences;
    void SetPreferences();

    //Appointments
    QString title=QLatin1String("");
    QString location=QLatin1String("");
    QString description=QLatin1String("");

    QDate appointmentDate;
    QTime appointmentStartTime;
    QTime appointmentEndTime;

    int isAllDay=0;
    int appointmentId =0;
    int hasReminder=0;
    int reminderMinutes=0;

    int isRepeating=0;
    int repeatDayInterval=0;
    int repeatNumber=0;

    QList<Appointment> appointmentList;

    void NewAppointment();
    void UpdateAppointment(int dbID, int selectedRowindex);
    void GenerateRepeatAppointments();
    void LoadDatabaseAppointmentsToAppointmentList();
    void UpdateAppointmentInAppointmentList(Appointment app, int appointmentId);
    void RemoveAppointmentFromAppointmentList(int appointmentId);

    void ShowAppointmentsOnListView(QDate theSelectedDate);

    //Sorting
    static bool compare(const Appointment& first, const Appointment& second);
    QList<Appointment> getSortedDayList(QDate theDate);

    //Models
    DayListModel *dayListModel;

    //Selected date
     QDate selectedDate;
     int selectedYear=2000;
     int selectedMonth=1;
     int selectedDay=1;
     int firstDay=1;
     QLabel *selectedDateLabel;
     QLabel *dateInfoLabel;


    //Widget Calendar
    int columnCount=7;
    int rowCount=6;
    //int dayArray[6][7];
    int dayArray[6*7];
    void UpdateCalendar();

    int prevCellRow=0;
    int prevCellCol=0;

   void SetApplicationFontSize(int fontsize);

    void increaseFont();
    void decreaseFont();
    void resetFont();

    QAction *increaseFontAction;
    QAction *decreaseFontAction;
    QAction *resetFontAction;

    QTableWidgetItem* dayItem;
    QTableWidgetItem* calendarItem;
    QTableWidgetItem* holidayItem;
    QTableWidgetItem* birthdayItem;

    //Helper methods
    QDate CalculateEaster(int year);
    void checkForBirthdaysNextSevenDays();
    void checkAppointmentsNextSevenDays();

    //Navigation
    void gotoNextMonth();
    void gotoPreviousMonth();
    void gotoToday();

    //Keyboard navigation actions
    QAction *gotoNextMonthAction;
    QAction *gotoPreviousMonthAction;
    QAction *gotoTodayAction;

    QList<Holiday> holidayList;
    void AddHolidaysToHolidayList(int year);

    //Contacts
    QString contactFirstName =QLatin1String("");
    QString contactMiddleNames=QLatin1String("");
    QString contactLastName=QLatin1String("");
    QString contactEmail=QLatin1String("");
    QString street=QLatin1String("");
    QString district=QLatin1String("");
    QString city=QLatin1String("");
    QString county=QLatin1String("");
    QString postcode =QLatin1String("");
    QString country =QLatin1String("");
    QString phoneNumber=QLatin1String("");

    QDate birthDate=QDate(); //(0,0,0)
    int addBirthdayToCalendar =0;
    int selectedContactDbId=0;
    Contact selectedContact;
     int contactId=0;

    ContactModel *contactModel;
    ProxyModelContacts *proxyModelContacts;

    QAction *newContactAction;

    void LoadDatebaseContactsToContactList();
    void NewContact();
    void UpdateContact(int dbID);
    void DisplayContactsOnTableView();

    //ContactList methods
    void RemoveContactFromContactList(int contactId);

    QList <Contact> contactList;

    //flags
    bool flagShowBirthdays=true;
    bool flagShowHolidays =true;
    bool flagQuickView=false;


    //Export Import
    void ExportContactsXML();
    void ImportContactsXML();
    void ExportAppointmentsXML();
    void ImportAppointmentsXML();


private slots:

    void timerUpdateSlot();

    void gotoNextMonthSlot();
    void gotoPreviousMonthSlot();
    void gotoTodaySlot();    
    void newContactSlot();

    void increaseFontSlot();
    void decreaseFontSlot();
    void resetFontSlot();

    void on_actionExit_triggered();

    void on_tableWidgetCalendar_cellClicked(int row, int column);

    void on_actionNew_Appointment_triggered();

    void on_listViewDay_doubleClicked(const QModelIndex &index);

    void on_tableWidgetCalendar_cellDoubleClicked(int row, int column);

    void on_actionGeneerate_Repeat_Appointments_triggered();

    void on_actionUpcoming_Schedule_triggered();

    void on_actionNew_Contact_triggered();

    void on_actionDelete_All_Appointments_triggered();

    void on_actionDelete_All_Contacts_triggered();

    void on_actionAbout_triggered();

    void on_tableViewContacts_clicked(const QModelIndex &index);

    void on_pushButtonMailTo_clicked();

    void on_pushButtonShowQuickFullView_clicked();

    void on_tableViewContacts_doubleClicked(const QModelIndex &index);

    void on_actionNext_Month_triggered();

    void on_actionPrevious_Month_triggered();

    void on_actionToday_triggered();

    void on_actionCheck_For_Birthdays_triggered();

    void on_actionShow_Birthdays_triggered();

    void on_actionShow_Holidays_triggered();

    void on_actionExport_Appointments_triggered();

    void on_actionImport_Appointments_triggered();

    void on_actionExport_Contacts_triggered();

    void on_actionImport_Contacts_triggered();

    void on_actionPreferences_triggered();

    void on_actionIncrease_Font_triggered();

    void on_actionDecrease_Font_triggered();

    void on_actionReset_Font_triggered();

private:
    Ui::MainWindow *ui;
    void loadSettings();
    void saveSettings();
};

#endif // MAINWINDOW_H

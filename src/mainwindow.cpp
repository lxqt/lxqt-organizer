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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <LXQt/Settings>

#include <utility>

MainWindow::MainWindow(DbManager &dbm, QWidget *parent) :
    QMainWindow(parent),
    dbm(dbm),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //set layout first
    QMainWindow::centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);


    t_title =tr("LXQt Organizer");
    setWindowTitle(t_title);
    //Holidays
    t_holiday_christmas=tr("Christmas");
    t_holiday_boxing=tr("Boxing Day");
    t_holiday_new_year=tr("New Year's Day");
    t_holiday_easter=tr("Easter");
    t_holiday_good_friday=tr("Good Friday");
    t_holiday_easter_monday=tr("Easter Monday");
    t_holiday_may_bank=tr("May Bank Holiday");
    t_holiday_spring_bank=tr("Spring Bank Holiday");
    t_holiday_summer_bank=tr("Summer Bank Holiday");
    //File
    t_file=tr("File");
    t_file_export_appointments=tr("Export Appointments");
    t_file_import_appointments=tr("Import Appointments");
    t_file_export_contacts=tr("Export Contacts");
    t_file_import_contacts=tr("Import Contacts");
    t_file_exit=tr("Exit");
    //Edit
    t_edit=tr("Edit");
    t_edit_delete_all_appointments=tr("Delete All Appointments");
    t_edit_delete_all_contacts=tr("Delete All Contacts");
    t_edit_preferences=tr("Preferences");

    //Appointments
    t_appointments=tr("Appointments");
    t_appointments_new_appointment=tr("New Appointment");
    t_appointments_generate_repeat_appointments=tr("Generate Repeat Appointments");
    t_appointments_upcoming_schedule=tr("Upcoming Schedule");
    //Calendar
    t_calendar=tr("Calendar");
    t_calendar_show_holidays=tr("Show Holidays");
    t_calendar_show_birthdays=tr("Show Birthdays");
    t_calendar_next_month=tr("Next Month");
    t_calendar_previous_month=tr("Previous Month");
    t_calendar_today=tr("Today");
    t_calendar_increase_font=tr("Increase Font Size");
    t_calendar_decrease_font=tr("Decrease Font Size");
    t_calendar_reset_font=tr("Reset Font Size");


    //Contacts
    t_contacts=tr("Contacts");
    t_contacts_new_contact=tr("New Contact");
    t_contacts_upcoming_birthdays=tr("Check For Birthdays");
    //Help
    t_help=tr("Help");
    t_help_about=tr("About");
    t_about_text=tr("LXQt Organizer is a Qt lightweight personal information"
                                " manager developed by Alan Crispin");
    //Contact tab
    t_contact_tab_first_name=tr("First Name");
    t_contact_tab_mid_name=tr("Mid Name");
    t_contact_tab_last_name=tr("Last Name");
    t_contact_tab_email=tr("Email");
    t_contact_tab_street=tr("Street");
    t_contact_tab_district=tr("District");
    t_contact_tab_city=tr("City");
    t_contact_tab_county=tr("County");
    t_contact_tab_postcode=tr("Postcode");
    t_contact_tab_country=tr("Country");
    t_contact_tab_telephone=tr("Telephone");
    t_contact_tab_birthday=tr("Birthday");

    t_contact_tab_quick_full_view=tr("Quick/Full View");
    t_contact_tab_mailto=tr("Mail To:");

    //Preferences
    t_preferences_title=tr("Preferences");
    t_preferences_font_size=tr("Font Size");
    t_preferences_locale=tr("Locale");
    t_preferences_play_audio=tr("Play Audio");
    t_preferences_dark_calendar=tr("Dark Calendar");
    t_preferences_line_spacing=tr("Line Spacing");

    //Upcoming dialogs
    t_upcoming_birthdays_title=tr("Upcoming Birthdays (Next Seven Days)");
    t_upcoming_schedule_title=tr("Upcoming Schedule (Next Seven Days)");
    t_label_upcoming_schedule=tr("Upcoming Schedule For Date: ");

    //Appointment dialogs
    t_dialog_appointment_dialog_title=tr("New Appointment");
    t_dialog_appointment_date_display=tr("Appointment Date: ");
    t_dialog_appointment_title=tr("Title");
    t_dialog_appointment_location=tr("Location");
    t_dialog_appointment_notes=tr("Notes");
    t_dialog_appointment_all_day=tr("All Day");
    t_dialog_appointment_start_time=tr("Start Time");
    t_dialog_appointment_end_time=tr("End Time");
    t_dialog_appointment_reminder=tr("Reminder");

    //update appointment
    t_dialog_appointment_update=tr("Update Appointment");
    t_dialog_appointment_delete=tr("Delete Appointment");

    t_reminder_5min=tr("5 Minutes Before Start");
    t_reminder_10min=tr("10 Minutes Before Start");
    t_reminder_30min=tr("30 Minutes Before Start");
    t_reminder_1hour=tr("1 Hour Before Start");
    t_reminder_1day=tr("1 Day Before Start");

    t_reminder_message_on=tr("On");
    t_reminder_message_starts_at=tr("Starts At");

    //repeat appointments
    t_dialog_repeat_title=tr("Generate Repeat Appointments", "Dialog title");
    t_dialog_repeat_date_message=tr("Appointment Start Date: ");
    t_dialog_repeat_repeat_every=tr("Repeat Every");
    t_dialog_repeat_days=tr("Days");
    t_dialog_repeat_occurrences=tr("Occurrences");

    //Contacts dialog

    t_dialog_contact_title=tr("default");
    t_dialog_contact_first_name=tr("default");
    t_dialog_contact_mid_name=tr("default");
    t_dialog_contact_last_name=tr("default");
    t_dialog_contact_email=tr("default");
    t_dialog_contact_street=tr("default");
    t_dialog_contact_district=tr("default");
    t_dialog_contact_city=tr("default");
    t_dialog_contact_county=tr("default");
    t_dialog_contact_postcode=tr("default");
    t_dialog_contact_country=tr("default");
    t_dialog_contact_telephone=tr("default");

    t_dialog_contact_born_on=tr("Born On");
    t_dialog_contact_birthday_unknown=tr("Birthday Unknown");
    t_dialog_contact_add_to_calendar=tr("Add To Calendar");
    t_dialog_contact_delete=tr("Delete Contact");

    //Setup empty lists
    appointmentList= QList<Appointment>();
    holidayList=QList<Holiday>();
    contactList =QList<Contact>();
    //set up models
    dayListModel= new DayListModel;
    contactModel =new ContactModel;
    proxyModelContacts = new ProxyModelContacts;

    //default setting values (first run)
    playAudio=1;
    darkCalendar=0;
    applicationFontSize=18;
    newLineSpacing=0;

    loadSettings();

    playAudio=currentPreferences.m_playAudio;
    darkCalendar=currentPreferences.m_darkCalendar;
    applicationFontSize=currentPreferences.m_applicationFontSize;
    newLineSpacing=currentPreferences.m_lineSpacing;

    setDarkCalendar(darkCalendar);
    SetApplicationFontSize(applicationFontSize);

    //setup timer
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdateSlot()));
    timer->start(1000); //check for reminders every 1000ms

    //setup audio
    soundEffect = new QSoundEffect(this);
    soundEffect->setSource(QUrl(QStringLiteral("qrc:/sounds/window-attention.wav")));

    if(playAudio){
    soundEffect->play();
    }

    setWindowIcon(QIcon::fromTheme(QStringLiteral("lxqt-organizer")));
    selectedDate = QDate::currentDate();

    selectedDateLabel = new QLabel(this);
    QFont font1 = selectedDateLabel->font();
    font1.setWeight(QFont::Bold);
    selectedDateLabel->setFont(font1);


    QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
    selectedDateLabel->setText(date);  
    ui->statusBar->addPermanentWidget(selectedDateLabel);

    selectedMonth=selectedDate.month();
    selectedYear=selectedDate.year();

    //Set Contacts TableView
    ui->tableViewContacts->horizontalHeader()->setVisible(true);
    ui->tableViewContacts->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewContacts->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableViewContacts->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewContacts->verticalHeader()->setVisible(false);

    //Setup widget Calendar
    ui->tableWidgetCalendar->setColumnCount(columnCount);
    ui->tableWidgetCalendar->setRowCount(rowCount);
    ui->tableWidgetCalendar->horizontalHeader()->setVisible(true);
    ui->tableWidgetCalendar->verticalHeader()->setVisible(false);
    ui->tableWidgetCalendar->setShowGrid(true);
    ui->tableWidgetCalendar->setGridStyle(Qt::SolidLine);

    ui->tableWidgetCalendar->horizontalHeader()
            ->setSectionResizeMode(QHeaderView::ResizeToContents);

    //Calendar Resizing

    ui->tableWidgetCalendar->setWordWrap(true);
    ui->tableWidgetCalendar->setTextElideMode(Qt::ElideRight);
    ui->tableWidgetCalendar->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidgetCalendar->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableWidgetCalendar->horizontalHeader()->setMinimumSectionSize(120);
    ui->tableWidgetCalendar->verticalHeader()->setMinimumSectionSize(60);

    for (int row=0; row<ui->tableWidgetCalendar->rowCount(); ++row)
    {
        //ui->tableWidgetCalendar->verticalHeader()->setSectionResizeMode(row, QHeaderView::Stretch);
        ui->tableWidgetCalendar->verticalHeader()
                ->setSectionResizeMode(row, QHeaderView::ResizeToContents);
    }

    for(int col=0; col<ui->tableWidgetCalendar->columnCount(); ++col)
    {
        ui->tableWidgetCalendar->horizontalHeader()->
                setSectionResizeMode(col, QHeaderView::Stretch);     
    }

    firstDay = Qt::Monday;

     QString month_year =locale.toString(selectedDate,QStringLiteral("MMMM yyyy"));
     ui->labelCalendarHeader->setText(month_year);
     ui->labelCalendarHeader->setStyleSheet
            (QStringLiteral("QLabel { background-color : rgb(81, 186, 242); color : black; }"));
    QFont fontCalHeader = ui->labelCalendarHeader->font();
    fontCalHeader.setBold(true);
    ui->labelCalendarHeader->setFont(fontCalHeader);


    ui->actionShow_Birthdays->setChecked(true);
    flagShowBirthdays=true;

    ui->actionShow_Holidays->setChecked(true);
    flagShowHolidays=true;


    //Keyboard QActions (shortcuts)
    //----------------------------
    gotoNextMonthAction= new QAction(this);
    gotoNextMonthAction->setShortcut(Qt::Key_Up);
    connect(gotoNextMonthAction, SIGNAL(triggered()), this, SLOT(gotoNextMonthSlot()));
    this->addAction(gotoNextMonthAction);

    gotoPreviousMonthAction= new QAction(this);
    gotoPreviousMonthAction->setShortcut(Qt::Key_Down);
    connect(gotoPreviousMonthAction, SIGNAL(triggered()), this, SLOT(gotoPreviousMonthSlot()));
    this->addAction(gotoPreviousMonthAction);

    gotoTodayAction= new QAction(this);
    gotoTodayAction->setShortcut(Qt::Key_Space);
    connect(gotoTodayAction, SIGNAL(triggered()), this, SLOT(gotoTodaySlot()));
    this->addAction(gotoTodayAction);

    newContactAction=new QAction(this);
    newContactAction->setShortcut(Qt::Key_C);
    connect(newContactAction, SIGNAL(triggered()),
            this, SLOT(newContactSlot()));
    this->addAction(newContactAction);
    //------------------------------------------

    //Calendar font key actions
    increaseFontAction=new QAction(this);
    increaseFontAction->setShortcut(Qt::Key_F);
    connect(increaseFontAction, SIGNAL(triggered()),
            this, SLOT(increaseFontSlot()));
    this->addAction(increaseFontAction);

    resetFontAction=new QAction(this);
    resetFontAction->setShortcut(Qt::Key_R);
    connect(resetFontAction, SIGNAL(triggered()),
            this, SLOT(resetFontSlot()));
    this->addAction(resetFontAction);

    decreaseFontAction=new QAction(this);
    decreaseFontAction->setShortcut(Qt::Key_D);
    connect(decreaseFontAction, SIGNAL(triggered()),
            this, SLOT(decreaseFontSlot()));
    this->addAction(decreaseFontAction);

    //------------------------------------------

    //Add contacts to contactList
    LoadDatebaseContactsToContactList(); //loads contactlist
    DisplayContactsOnTableView();
    AddHolidaysToHolidayList(selectedYear);
    LoadDatabaseAppointmentsToAppointmentList();
    UpdateCalendar();
    ShowAppointmentsOnListView(selectedDate);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timerUpdateSlot()
{
    checkForReminders();
}


void MainWindow::checkForReminders()
{


    QDate currentDate=QDate::currentDate();
    QTime currentTime= QTime::currentTime();
    int currentHour =currentTime.hour();
    int currentMinute = currentTime.minute();
    int currentSecond =currentTime.second();

    for (const Appointment &a : std::as_const(appointmentList)) {

        if(a.m_hasReminder==1)
        {
            //Appointment has reminder
            QDate reminderDate =QDate::fromString(a.m_date);
            QTime reminderTime = QTime::fromString(a.m_startTime);

            if (a.m_reminderMinutes ==5){

                reminderTime =reminderTime.addSecs(-(5*60));
            }
            else if(a.m_reminderMinutes==10) {
                reminderTime =reminderTime.addSecs(-(10*60));
            }
            else if(a.m_reminderMinutes==30)
            {
                reminderTime =reminderTime.addSecs(-(30*60)); // 1hour = 3,600 seconds
            }
            else if(a.m_reminderMinutes==60)
            {
                reminderTime =reminderTime.addSecs(-(60*60)); // 1hour = 3,600 seconds
            }
            else if(a.m_reminderMinutes==1440)
            {
                reminderDate=reminderDate.addDays(-1);
            }

            int reminderHour =reminderTime.hour();
            int reminderMinute=reminderTime.minute();
            //int reminderSecond=reminderTime.second();

            if (reminderDate==currentDate)
            {
                if(currentHour==reminderHour && currentMinute==reminderMinute &&currentSecond==0)
                {
                    //Translation needed

                    //QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
                    //selectedDateLabel->setText(date);

                    QString str=a.m_title;
                    //str.append(QStringLiteral(" on "));
                    str.append(QLatin1Char(' '));
                    str.append(t_reminder_message_on);
                    str.append(QLatin1Char(' '));

                    //QDate reminderDate= QDate::fromString(a.m_date);

                    QString date =locale.toString(QDate::fromString(a.m_date),
                                                  QStringLiteral("dddd dd MMMM yyyy"));
                    str.append(date);
                    //str.append(a.m_date);
                    str.append(QLatin1Char('\n')); //new line
                    //str.append(QStringLiteral("Starts at "));
                    str.append(t_reminder_message_starts_at);
                    str.append(QLatin1Char(' '));
                    QTime startsAt =QTime::fromString(a.m_startTime);
                    str.append(startsAt.toString(QStringLiteral("hh:mm")));
                    if(playAudio){
                        soundEffect->play();
                    }

                    QMessageBox::information(this,t_dialog_appointment_reminder,str);
                }

            }
        }
    }
}





void MainWindow::NewAppointment()
{
    DialogAppointment *appointmentDialog = new  DialogAppointment(this,&selectedDate);


    appointmentDialog->setLocaleDate(this->locale);
    appointmentDialog->setDialogTitleTranslation(t_dialog_appointment_dialog_title);
    appointmentDialog->setDateDisplayTranslation(t_dialog_appointment_date_display);
    appointmentDialog->setTitleTranslation(t_dialog_appointment_title);
    appointmentDialog->setLocationTranslation(t_dialog_appointment_location);
    appointmentDialog->setNotesTranslations(t_dialog_appointment_notes);
    appointmentDialog->setAllDaytranslation(t_dialog_appointment_all_day);
    appointmentDialog->setStartTimeTranslation(t_dialog_appointment_start_time);
    appointmentDialog->setEndTimeTranslation(t_dialog_appointment_end_time);
    appointmentDialog->setReminderTranslation(t_dialog_appointment_reminder);
    appointmentDialog->setLabelTranslations();

    appointmentDialog->setReminder5MinTranslation(t_reminder_5min);
    appointmentDialog->setReminder10MinTranslation(t_reminder_10min);
    appointmentDialog->setReminder30MinTranslation(t_reminder_30min);
    appointmentDialog->setReminder1HourTranslation(t_reminder_1hour);
    appointmentDialog->setReminder1DayTranslation(t_reminder_1day);
    appointmentDialog->setComboBoxReminderTranslations();


    appointmentDialog->setModal(true);
    if (appointmentDialog->exec() == QDialog::Accepted ) {

        title =appointmentDialog->getTitle();
        location =appointmentDialog->getLocation();
        description= appointmentDialog->getDescription();
        selectedDate=appointmentDialog->getAppointmentDate(); //appointment date
        appointmentStartTime=appointmentDialog->getStartTime();
        appointmentEndTime=appointmentDialog->getEndTime();

        isAllDay=appointmentDialog->getAllDay();
        hasReminder=appointmentDialog->getHasReminder();
        reminderMinutes=appointmentDialog->getReminderMins();

        Appointment a;
        a.m_title=title;
        a.m_location=location;
        a.m_description=description;
        a.m_date=selectedDate.toString();
        a.m_startTime=appointmentStartTime.toString();
        a.m_endTime=appointmentEndTime.toString();

        a.m_isFullDay=isAllDay;
        a.m_hasReminder=hasReminder;
        a.m_reminderMinutes=reminderMinutes;

        if (dbm.isOpen())
        {
            appointmentId=dbm.addAppointment(a);
            a.m_id =appointmentId;

            appointmentList.append(a);

            UpdateCalendar();
            ShowAppointmentsOnListView(selectedDate);
        }
    }
 UpdateCalendar();
}

void MainWindow::UpdateAppointment(int dbID, int selectedRowindex)
{
    Appointment currentAppointment = dbm.getAppointmentByID(dbID);

    DialogAppointmentUpdate *appointmentDialog =
            new  DialogAppointmentUpdate(this,&currentAppointment);


    appointmentDialog->setDialogUpdateTranslation(t_dialog_appointment_update);
    appointmentDialog->setDateDisplayTranslation(t_dialog_appointment_date_display);
    appointmentDialog->setTitleTranslation(t_dialog_appointment_title);
    appointmentDialog->setLocationTranslation(t_dialog_appointment_location);
    appointmentDialog->setNotesTranslations(t_dialog_appointment_notes);
    appointmentDialog->setAllDaytranslation(t_dialog_appointment_all_day);
    appointmentDialog->setStartTimeTranslation(t_dialog_appointment_start_time);
    appointmentDialog->setEndTimeTranslation(t_dialog_appointment_end_time);    
    appointmentDialog->setReminderTranslation(t_dialog_appointment_reminder);
    appointmentDialog->setDeleteTranslation(t_dialog_appointment_delete);
    appointmentDialog->setLabelTranslations();
    appointmentDialog->setLocaleDate(this->locale);

    appointmentDialog->setReminder5MinTranslation(t_reminder_5min);
    appointmentDialog->setReminder10MinTranslation(t_reminder_10min);
    appointmentDialog->setReminder30MinTranslation(t_reminder_30min);
    appointmentDialog->setReminder1HourTranslation(t_reminder_1hour);
    appointmentDialog->setReminder1DayTranslation(t_reminder_1day);
    appointmentDialog->setComboBoxReminderTranslations();


    appointmentDialog->setModal(true);
    if (appointmentDialog->exec() == QDialog::Accepted ) {

        if(appointmentDialog->getDeleteRequested())
        {

            RemoveAppointmentFromAppointmentList(dbID);
            dbm.deleteAppointmentById(dbID);
            //update reminder listview
            dayListModel->removeAppointment(selectedRowindex);
            // may have to remove reminder from modellist
            ui->listViewDay->setModel(dayListModel);
            UpdateCalendar();
            return;
        }

        title =appointmentDialog->getTitle();
        location =appointmentDialog->getLocation();
        description= appointmentDialog->getDescription();
        selectedDate=appointmentDialog->getAppointmentDate(); //appointment date
        appointmentStartTime=appointmentDialog->getStartTime();
        appointmentEndTime=appointmentDialog->getEndTime();

        isAllDay=appointmentDialog->getAllDay();
        hasReminder=appointmentDialog->getHasReminder();
        reminderMinutes=appointmentDialog->getReminderMins();

        Appointment a;
        a.m_title=title;
        a.m_location=location;
        a.m_description=description;
        a.m_date=selectedDate.toString();
        a.m_startTime=appointmentStartTime.toString();
        a.m_endTime=appointmentEndTime.toString();

        a.m_isFullDay=isAllDay;
        a.m_hasReminder=hasReminder;
        a.m_reminderMinutes=reminderMinutes;

        if (dbm.isOpen())
        {
            dbm.updateAppointment(a,dbID);

            LoadDatabaseAppointmentsToAppointmentList(); //sync with db
            UpdateCalendar();
            ShowAppointmentsOnListView(selectedDate);
        }
    }

}

void MainWindow::GenerateRepeatAppointments()
{


    DialogRepeatAppointment *repeatDialog = new  DialogRepeatAppointment(this,&selectedDate);

    repeatDialog->setLocaleDate(this->locale);
    repeatDialog->setStartDateDisplayTranslation(t_dialog_repeat_date_message);

    repeatDialog->setTitleTranslation(t_dialog_appointment_title);
    repeatDialog->setLocationTranslation(t_dialog_appointment_location);
    repeatDialog->setNotesTranslations(t_dialog_appointment_notes);
    repeatDialog->setAllDaytranslation(t_dialog_appointment_all_day);
    repeatDialog->setStartTimeTranslation(t_dialog_appointment_start_time);
    repeatDialog->setEndTimeTranslation(t_dialog_appointment_end_time);

    repeatDialog->setRepeatEveryTranslation(t_dialog_repeat_repeat_every);
    repeatDialog->setDaysTranslation(t_dialog_repeat_days);
    repeatDialog->setOccurrencesTranslation(t_dialog_repeat_occurrences);

    repeatDialog->setLabelTranslations();

    repeatDialog->setModal(true);

        if (repeatDialog->exec() == QDialog::Accepted ) {

            title =repeatDialog->getTitle();
            location =repeatDialog->getLocation();
            description= repeatDialog->getDescription();
            selectedDate=repeatDialog->getAppointmentDate(); //appointment date
            appointmentStartTime=repeatDialog->getStartTime();
            appointmentEndTime=repeatDialog->getEndTime();

            isAllDay=repeatDialog->getAllDay();

            repeatDayInterval=repeatDialog->getRepeatDayInterval();
            repeatNumber=repeatDialog->getRepeatNumber();

          Appointment a;
            a.m_title=title;
            a.m_location=location;
            a.m_description=description;
            a.m_date=selectedDate.toString();
            a.m_startTime=appointmentStartTime.toString();
            a.m_endTime=appointmentEndTime.toString();

            a.m_isFullDay=isAllDay;
            a.m_hasReminder=0;
            a.m_reminderMinutes=0;

            Appointment tmp;
            QDate repeatDate=QDate::fromString(a.m_date);

            for (int i=0;i<repeatNumber;i+= 1)
            {
                tmp.m_title=a.m_title;
                //tmp.m_title=a.m_title;
                tmp.m_location=a.m_location;
                tmp.m_description=a.m_description;
                tmp.m_date=repeatDate.addDays(repeatDayInterval*(i)).toString();
                tmp.m_startTime=a.m_startTime;
                tmp.m_endTime=a.m_endTime;

                tmp.m_isFullDay=a.m_isFullDay;
                tmp.m_hasReminder=0;
                tmp.m_reminderMinutes=0;

                if (dbm.isOpen())
                {
                    appointmentId=dbm.addAppointment(tmp);

                    tmp.m_id=appointmentId;
                    appointmentList.append(tmp);

                    UpdateCalendar();
                    ShowAppointmentsOnListView(selectedDate);
                }
            }
            UpdateCalendar();
        }

}

void MainWindow::LoadDatabaseAppointmentsToAppointmentList()
{
    appointmentList.clear();
    QList<Appointment> tmpList = dbm.getAllAppointments();
    for (const Appointment &a : std::as_const(tmpList))
    {
        appointmentList.append(a);

    }
}

void MainWindow::UpdateAppointmentInAppointmentList(Appointment app, int appointmentId)
{
    for(int i=0; i<appointmentList.count(); i++)
    {
        Appointment a =appointmentList.at(i);

        if(a.m_id==appointmentId)
        {
            appointmentList.removeAt(i);
            appointmentList.insert(i,app);
        }
    }
}

void MainWindow::RemoveAppointmentFromAppointmentList(int appointmentId)
{
    for(int i=0; i<appointmentList.count(); i++)
    {
        Appointment a =appointmentList.at(i);

        if (a.m_id ==appointmentId)
        {
            appointmentList.removeAt(i);
        }
    }
}

void MainWindow::ShowAppointmentsOnListView(QDate theSelectedDate)
{
    dayListModel->clearAllAppointment();
    QList<Appointment> sortedDayList =getSortedDayList(theSelectedDate);

    dayListModel = new DayListModel(sortedDayList);
    dayListModel->setNewLineSpacing(newLineSpacing);
    ui->listViewDay->setModel(dayListModel);
}

bool MainWindow::compare(const Appointment &first, const Appointment &second)
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

QList<Appointment> MainWindow::getSortedDayList(QDate theDate)
{
    QList<Appointment> dayList =QList<Appointment>();
        for (const Appointment &a : std::as_const(appointmentList))
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

void MainWindow::UpdateCalendar()
{


    int cellIndex=0;
    ui->tableWidgetCalendar->clearContents();
    //Initialise with empty cells
    for (int row=0; row<ui->tableWidgetCalendar->rowCount(); ++row)
    {
        for(int col=0; col<ui->tableWidgetCalendar->columnCount(); ++col)
        {
            ui->tableWidgetCalendar->setItem(row, col,
                                             new QTableWidgetItem(QLatin1String("")));
            cellIndex = (7 * row) + col;
            dayArray[cellIndex]=0;
        }
    }

    QStringList days;
    for (int weekDay = 1; weekDay <= 7; ++weekDay) {

        QString day =locale.dayName(weekDay);
        days.append(day);
    }
    ui->tableWidgetCalendar->setHorizontalHeaderLabels(days);

    QDate date(selectedDate.year(), selectedDate.month(), 1);

    int row=0;
    int dayValue=0;
    while (date.month() == selectedDate.month()) {
        int weekDay = date.dayOfWeek();
        dayValue= date.day();
        cellIndex = (7 * row) + weekDay-1;
        dayArray[cellIndex]=dayValue;
        dayItem = new QTableWidgetItem(QString::number(dayValue));

        ui->tableWidgetCalendar->setItem(row, weekDay-1,dayItem);

        QString str=QLatin1String("");

        //-----------------------------------------------------
        // Add holidays to calendar
        //-----------------------------------------------------
        if(flagShowHolidays){

            for (const Holiday &h : std::as_const(holidayList))
            {
                QDate d =QDate::fromString(h.m_date);

                if(date ==d)
                {
                    //add holiday to calendar
                    str.append(QLatin1Char('\n')+h.m_name);

                }
            }
        }

        //-----------------------------------------------------
        //Add birthdays using contactList
        //-----------------------------------------------------
        if(flagShowBirthdays)
        {
            for (const Contact &c : std::as_const(contactList))
            {
                QDate birth =QDate::fromString(c.m_birthdate);
                QDate birthday =QDate(selectedYear,birth.month(),birth.day());
                QString name =c.m_firstname+ QLatin1Char(' ')+c.m_lastname;

                if((date ==birthday) && (c.m_addToCalendar ==1))
                {
                    str.append(QStringLiteral("\nBirthday: ")+name);
                }
            }
        }

        //---------------------------------------------------------
        // Add Appointments
        //----------------------------------------------------------

        QList<Appointment> sortedDayList =getSortedDayList(date);

        if (!sortedDayList.empty())
        {
            for (const Appointment &a : std::as_const(sortedDayList))
            {
                 str.append(QLatin1Char('\n')+a.m_title);
            }
            //Display on calendar
        }

        calendarItem = new QTableWidgetItem(QString::number(dayValue)
                                            +str);

        ui->tableWidgetCalendar->setItem(row,weekDay-1, calendarItem);

        if(date==QDate::currentDate())
        {

            if(darkCalendar==0)
            {
                calendarItem->setData(
                            Qt::BackgroundRole,
                            QBrush(QColor(135,206,235)));//skyblue

            }
            else if (darkCalendar==1) {
                calendarItem->setData(
                            Qt::BackgroundRole,                            
                            QBrush(QColor(70,130,180))); //steel
            }
            else {
                calendarItem->setData(
                            Qt::BackgroundRole,
                            QBrush(QColor(Qt::lightGray))); //default
            }
        }

        //Move on  ...
        date = date.addDays(1); //move to next day
        if (weekDay == 7 && date.month() == selectedDate.month())
        {
            row=row+1;
        }
    }//selected month

    QString month_year =locale.toString(selectedDate,QStringLiteral("MMMM yyyy"));


    ui->labelCalendarHeader->setText(month_year);
}


void MainWindow::SetApplicationFontSize(int fontsize)
{
    QFont itemFontSize =ui->tableWidgetCalendar->font();
    itemFontSize.setPixelSize(fontsize);
    ui->tableWidgetCalendar->setFont(itemFontSize);

    QFont headerFontSize = ui->tableWidgetCalendar->horizontalHeader()->font();
    headerFontSize.setPixelSize(fontsize);
    ui->tableWidgetCalendar->horizontalHeader()->setFont(headerFontSize);

    QFont labelCalendarFont=ui->labelCalendarHeader->font();
    labelCalendarFont.setPixelSize(fontsize);
    ui->labelCalendarHeader->setFont(labelCalendarFont);

}

void MainWindow::SetPreferences()
{
    DialogPreferences *preferencesDialog = new  DialogPreferences(this,&currentPreferences);
    preferencesDialog->setModal(true);
    preferencesDialog->SetLableDescriptions();


    if (preferencesDialog->exec() == QDialog::Accepted ) {


        playAudio=preferencesDialog->isPlayAudio();

        //locale=getLocale(localeStr);
        QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
        selectedDateLabel->setText(date);

        //get new default font sizes
        applicationFontSize=preferencesDialog->getApplicationFont();
        newLineSpacing=preferencesDialog->getLineSpacing();

        darkCalendar=preferencesDialog->isDarkCalendar();
        setDarkCalendar(darkCalendar);

        setWindowTitle(t_title);
        AddHolidaysToHolidayList(selectedYear);
        UpdateCalendar();
        ShowAppointmentsOnListView(selectedDate);

        //reset preferences
        currentPreferences.m_playAudio=playAudio;        
        currentPreferences.m_darkCalendar=darkCalendar;
        currentPreferences.m_applicationFontSize=applicationFontSize;
        currentPreferences.m_lineSpacing=newLineSpacing;
        saveSettings();
    }
    SetApplicationFontSize(applicationFontSize);
}

void MainWindow::increaseFont()
{
    applicationFontSize=applicationFontSize+1;
    if (applicationFontSize>=36) applicationFontSize=36;
    currentPreferences.m_applicationFontSize=applicationFontSize;
    SetApplicationFontSize(applicationFontSize);
    }

void MainWindow::decreaseFont()
{
    applicationFontSize=applicationFontSize-1;
    if (applicationFontSize<=8) applicationFontSize=8;
    currentPreferences.m_applicationFontSize=applicationFontSize;
    SetApplicationFontSize(applicationFontSize);
}

void MainWindow::resetFont()
{    
    applicationFontSize=16;
    currentPreferences.m_applicationFontSize=applicationFontSize;
    SetApplicationFontSize(applicationFontSize);
}

QDate MainWindow::CalculateEaster(int year)
{
    QDate easter;

    int Yr = year;
    int a = Yr % 19;
    int b = Yr / 100;
    int c = Yr % 100;
    int d = b / 4;
    int e = b % 4;
    int f = (b + 8) / 25;
    int g = (b - f + 1) / 3;
    int h = (19 * a + b - d - g + 15) % 30;
    int i = c / 4;
    int k = c % 4;
    int L = (32 + 2 * e + 2 * i - h - k) % 7;
    int m = (a + 11 * h + 22 * L) / 451;
    int month = (h + L - 7 * m + 114) / 31;
    int day = ((h + L - 7 * m + 114) % 31) + 1;

    easter.setDate(year,month,day);
    return easter;
}

void MainWindow::checkForBirthdaysNextSevenDays()
{
    DialogUpcomingBirthdays  *birthdayDialog =
            new  DialogUpcomingBirthdays(this,&contactList);
    birthdayDialog->setTitleTranslation(t_upcoming_birthdays_title);
    birthdayDialog->setModal(true);

    if (birthdayDialog->exec() == QDialog::Accepted ) {
    }
}

void MainWindow::checkAppointmentsNextSevenDays()
{
    DialogUpcomingSchedule  *scheduleDialog =
            new  DialogUpcomingSchedule(this, &dbm);

    scheduleDialog->setTitleTranslation(t_upcoming_schedule_title);
    scheduleDialog->setLabelUpcomingScheduleTranslation(t_label_upcoming_schedule);
    scheduleDialog->setLocale(this->locale);
    scheduleDialog->setModal(true);

    if (scheduleDialog->exec() == QDialog::Accepted ) {
    }
}

void MainWindow::gotoNextMonth()
{
    selectedMonth =selectedDate.month()+1;
    selectedYear =selectedDate.year();
    selectedDay=1;

    if (selectedMonth >= 13) {
        selectedMonth = 1;
        selectedYear =selectedYear+1;
        AddHolidaysToHolidayList(selectedYear);
        UpdateCalendar();
        selectedDate.setDate(selectedYear,selectedMonth,selectedDay);        
        QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
        selectedDateLabel->setText(date);
    }
    selectedDate.setDate(selectedYear,selectedMonth,selectedDay);

    ShowAppointmentsOnListView(selectedDate);    
    QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
    selectedDateLabel->setText(date);
    UpdateCalendar();
}

void MainWindow::gotoPreviousMonth()
{
    selectedMonth =selectedDate.month()-1;
    selectedYear =selectedDate.year();
    selectedDay=1;

    if (selectedMonth < 1)
    {
        selectedMonth = 12;
        selectedYear=selectedYear-1;
        AddHolidaysToHolidayList(selectedYear);
        UpdateCalendar();
        selectedDate.setDate(selectedYear,selectedMonth,selectedDay);        
        QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
        selectedDateLabel->setText(date);
    }
    selectedDate.setDate(selectedYear,selectedMonth,selectedDay);
    UpdateCalendar();
    ShowAppointmentsOnListView(selectedDate);   
    QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
    selectedDateLabel->setText(date);
}

void MainWindow::gotoToday()
{
    selectedDate=QDate::currentDate();
    selectedDay=selectedDate.day();
    selectedMonth=selectedDate.month();
    selectedYear=selectedDate.year();
    AddHolidaysToHolidayList(selectedYear);
    UpdateCalendar();
    ShowAppointmentsOnListView(selectedDate);   
    QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
    selectedDateLabel->setText(date);
}

void MainWindow::AddHolidaysToHolidayList(int year)
{
    holidayList.clear();

    Holiday h1;
    h1.m_id=1;
    h1.m_name=t_holiday_christmas;
    h1.m_date=QDate(year,12,25).toString();
    h1.m_addToCalendar=1;
    holidayList.append(h1);

    Holiday h2;
    h2.m_id=2;
    h2.m_name=t_holiday_boxing;
    h2.m_date=QDate(year,12,26).toString();
    h2.m_addToCalendar=1;
    holidayList.append(h2);

    Holiday h3;
    h3.m_id=3;
    h3.m_name=t_holiday_new_year;
    h3.m_date=QDate(year,1,1).toString();
    h3.m_addToCalendar=1;
    holidayList.append(h3);

    QDate easterSunday =CalculateEaster(year);

    Holiday h4;
    h4.m_id=4;
    h4.m_name=t_holiday_easter;
    h4.m_date=easterSunday.toString();
    h4.m_addToCalendar=1;
    holidayList.append(h4);

    Holiday h5;
    h5.m_id=5;
    h5.m_name=t_holiday_good_friday;
    h5.m_date=easterSunday.addDays(-2).toString();
    h5.m_addToCalendar=1;
    holidayList.append(h5);

    Holiday h6;
    h6.m_id=6;
    h6.m_name=t_holiday_easter_monday;
    h6.m_date=easterSunday.addDays(1).toString();
    h6.m_addToCalendar=1;
    holidayList.append(h6);

    Holiday h7;
    h7.m_id=7;
    h7.m_name=t_holiday_may_bank;

    QDate firstMondayMay(year, 5, 1);

    while (firstMondayMay.dayOfWeek() != Qt::Monday)
            firstMondayMay = firstMondayMay.addDays(1);

    h7.m_date=firstMondayMay.toString();
    h7.m_addToCalendar=1;
    holidayList.append(h7);

    //Days in May

    int daysInMay = QDate(year,5,1).daysInMonth();
    int plusDays=0;

    if (firstMondayMay.day()+28<=daysInMay){

        plusDays =28;
    }
    else {
        plusDays=21;
    }


    Holiday h8;
    h8.m_id=8;
    h8.m_name=t_holiday_spring_bank;
    QDate springbank =firstMondayMay.addDays(plusDays);

    if (springbank.isValid())
    {
        h8.m_date=springbank.toString();
        h8.m_addToCalendar=1;
        holidayList.append(h8);
    }   

    Holiday h9;
    h9.m_id=9;
    h9.m_name=t_holiday_summer_bank;

    QDate firstMondayAug(year, 8, 1);

    while (firstMondayAug.dayOfWeek() != Qt::Monday)
            firstMondayAug = firstMondayAug.addDays(1);

    int daysInAugust = QDate(year,8,1).daysInMonth();
    plusDays=0;

    if (firstMondayAug.day()+28<=daysInAugust){

        plusDays =28;
    }
    else {
        plusDays=21;
    }

    QDate summerBankHol =firstMondayAug.addDays(plusDays);

    if(summerBankHol.isValid())
    {
        h9.m_date=summerBankHol.toString();
        h9.m_addToCalendar=1;
        holidayList.append(h9);
    }
}

void MainWindow::LoadDatebaseContactsToContactList()
{
    contactList.clear();
    QList<Contact> tmpList = dbm.getAllContacts();
    for (const Contact &c : std::as_const(tmpList))
    {
        contactList.append(c);
    }
}

void MainWindow::NewContact()
{
    //Add new contact
    DialogContact *contactDialog = new DialogContact(this);

    contactDialog->setDialogTitleTranslation(t_dialog_contact_title);
    contactDialog->setFirstNameTranslation(t_dialog_contact_first_name);
    contactDialog->setMidNameTranslation(t_dialog_contact_mid_name);
    contactDialog->setLastNameTranslation(t_dialog_contact_last_name);
    contactDialog->setEmailTranslation(t_dialog_contact_email);
    contactDialog->setStreetTranslation(t_dialog_contact_street);
    contactDialog->setDistrictTranslation(t_dialog_contact_district);
    contactDialog->setCityTranslation(t_dialog_contact_city);
    contactDialog->setCountyTranslation(t_dialog_contact_county);
    contactDialog->setPostcodeTranslation(t_dialog_contact_postcode);
    contactDialog->setCountryTranslation(t_dialog_contact_country);
    contactDialog->setTelephoneTranslation(t_dialog_contact_telephone);

    contactDialog->setBornOnTranslation(t_dialog_contact_born_on);
    contactDialog->setBirthdayUnknownTranslation(t_dialog_contact_birthday_unknown);
    contactDialog->setAddToCalendarTranslation(t_dialog_contact_add_to_calendar);

    contactDialog->setLabelTranslations();

    contactDialog->setModal(true);

    if (contactDialog->exec() == QDialog::Accepted ) {

        this->contactFirstName=contactDialog->getFirstName();
        this->contactMiddleNames=contactDialog->getMiddleNames();
        this->contactLastName=contactDialog->getLastName();
        this->contactEmail=contactDialog->getEmail();
        this->street=contactDialog->getStreet();
        this->city=contactDialog->getCity();
        this->district=contactDialog->getDistrict();
        this->county=contactDialog->getCounty();
        this->postcode=contactDialog->getPostcode();
        this->country=contactDialog->getCountry();
        this->phoneNumber=contactDialog->getPhoneNumber();
        this->birthDate=contactDialog->getBirthDate();
        this->addBirthdayToCalendar=contactDialog->getAddToCalendar();

        Contact c;
        c.m_firstname=contactFirstName;
        c.m_midnames=contactMiddleNames;
        c.m_lastname=contactLastName;
        c.m_email=contactEmail;
        c.m_street=street;
        c.m_district=district;
        c.m_city=city;
        c.m_county=county;
        c.m_postcode=postcode;
        c.m_country=country;
        c.m_telephone=phoneNumber;
        c.m_birthdate=birthDate.toString();
        c.m_addToCalendar=addBirthdayToCalendar;

        if (dbm.isOpen())
        {
            contactId=dbm.addContact(c);
            c.m_id=contactId;
            contactList.append(c);

        }
    }
    DisplayContactsOnTableView(); //display
    UpdateCalendar();

}

void MainWindow::UpdateContact(int dbID)
{
    Contact currentContact = dbm.getContactByID(dbID);
    DialogContact *contactDialog = new  DialogContact(&currentContact,this);

    contactDialog->setDialogTitleTranslation(t_dialog_contact_title);
    contactDialog->setFirstNameTranslation(t_dialog_contact_first_name);
    contactDialog->setMidNameTranslation(t_dialog_contact_mid_name);
    contactDialog->setLastNameTranslation(t_dialog_contact_last_name);
    contactDialog->setEmailTranslation(t_dialog_contact_email);
    contactDialog->setStreetTranslation(t_dialog_contact_street);
    contactDialog->setDistrictTranslation(t_dialog_contact_district);
    contactDialog->setCityTranslation(t_dialog_contact_city);
    contactDialog->setCountyTranslation(t_dialog_contact_county);
    contactDialog->setPostcodeTranslation(t_dialog_contact_postcode);
    contactDialog->setCountryTranslation(t_dialog_contact_country);
    contactDialog->setTelephoneTranslation(t_dialog_contact_telephone);

    contactDialog->setBornOnTranslation(t_dialog_contact_born_on);
    contactDialog->setBirthdayUnknownTranslation(t_dialog_contact_birthday_unknown);
    contactDialog->setAddToCalendarTranslation(t_dialog_contact_add_to_calendar);
    contactDialog->setDeleteTranslation(t_dialog_contact_delete);

    contactDialog->setLabelTranslations();

    contactDialog->setModal(true);

    if (contactDialog->exec() == QDialog::Accepted ) {

        if(contactDialog->getDeleteRequested())
        {
            dbm.removeContactById(dbID);
            RemoveContactFromContactList(dbID);
            DisplayContactsOnTableView();
            UpdateCalendar();
            return;  //gone!
        }
        this->contactFirstName=contactDialog->getFirstName();
        this->contactLastName=contactDialog->getLastName();
        this->contactMiddleNames=contactDialog->getMiddleNames();
        this->contactEmail=contactDialog->getEmail();
        this->street=contactDialog->getStreet();
        this->district=contactDialog->getDistrict();
        this->city=contactDialog->getCity();
        this->county=contactDialog->getCounty();
        this->postcode=contactDialog->getPostcode();
        this->country=contactDialog->getCountry();
        this->phoneNumber=contactDialog->getPhoneNumber();
        this->birthDate=contactDialog->getBirthDate();
        this->addBirthdayToCalendar=contactDialog->getAddToCalendar();

        Contact c;
        c.m_firstname=contactFirstName;
        c.m_midnames=contactMiddleNames;
        c.m_lastname=contactLastName;
        c.m_email=contactEmail;
        c.m_street=street;
        c.m_district=district;
        c.m_city=city;
        c.m_county=county;
        c.m_postcode=postcode;
        c.m_country=country;
        c.m_telephone=phoneNumber;
        c.m_birthdate=birthDate.toString();
        c.m_addToCalendar=addBirthdayToCalendar;

        if (dbm.isOpen())
        {
            dbm.updateContact(c,dbID);
            LoadDatebaseContactsToContactList();
        }
    }
    DisplayContactsOnTableView();
    UpdateCalendar(); //Birthdays
}

void MainWindow::DisplayContactsOnTableView()
{
    contactModel->clearAllContacts();

    QList<Contact> contactListDb =dbm.getAllContacts();

    for (const Contact &c : std::as_const(contactListDb))
    {

        contactModel->AddContact(c);
    }
    proxyModelContacts->setSourceModel(contactModel);
    ui->tableViewContacts->setModel(proxyModelContacts);
    ui->tableViewContacts->setSortingEnabled(true);
    ui->tableViewContacts->sortByColumn(3, Qt::AscendingOrder); //sort on lastname
    ui->tableViewContacts->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewContacts->setSortingEnabled(false);
    ui->tableViewContacts->hideColumn(0); //index
    ui->tableViewContacts->hideColumn(2); //mid names
    ui->tableViewContacts->hideColumn(5); //street
    ui->tableViewContacts->hideColumn(6); //district
    //city
    ui->tableViewContacts->hideColumn(8); //county
    ui->tableViewContacts->hideColumn(9); //postcode
    ui->tableViewContacts->hideColumn(10); //country
    // ui->tableViewContacts->hideColumn(11); //telephone
    //birthday
    ui->tableViewContacts->hideColumn(12); //birthday
}

void MainWindow::RemoveContactFromContactList(int contactId)
{
    for(int i=0; i<contactList.count(); i++)
    {
        Contact c =contactList.at(i);

        if (c.m_id ==contactId)
        {
            contactList.removeAt(i);
        }
    }
}

void MainWindow::ExportContactsXML()
{
    QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("Contacts"));
    document.appendChild(root);

    QList<Contact> dbContactList =dbm.getAllContacts();
    for (const Contact &c : std::as_const(dbContactList)) {
        QString firstName=c.m_firstname;
        QString midName =c.m_midnames;
        QString lastName=c.m_lastname;
        QString email =c.m_email;
        QString street =c.m_street;
        QString district =c.m_district;
        QString city =c.m_city;
        QString county =c.m_county;
        QString postcode=c.m_postcode;
        QString country =c.m_country;
        QString telephone =c.m_telephone;
        QString birthdate =c.m_birthdate;
        QString addToCal =QString::number(c.m_addToCalendar);

        QDomElement contact = document.createElement(QStringLiteral("Contact"));
        contact.setAttribute(QStringLiteral("FirstName"),firstName );
        contact.setAttribute(QStringLiteral("MidName"),midName);
        contact.setAttribute(QStringLiteral("LastName"), lastName);
        contact.setAttribute(QStringLiteral("Email"),email);
        contact.setAttribute(QStringLiteral("Street"),street);
        contact.setAttribute(QStringLiteral("District"),district);
        contact.setAttribute(QStringLiteral("City"),city);
        contact.setAttribute(QStringLiteral("County"),county);
        contact.setAttribute(QStringLiteral("Postcode"),postcode);
        contact.setAttribute(QStringLiteral("Country"),country);
        contact.setAttribute(QStringLiteral("Telephone"),telephone);
        contact.setAttribute(QStringLiteral("BirthDate"),birthdate);
        contact.setAttribute(QStringLiteral("AddToCal"),addToCal);
        root.appendChild(contact);
    }

    QString filename = QFileDialog::getSaveFileName(this, QStringLiteral("Save Contacts Xml"), QStringLiteral("."), QStringLiteral("Xml files (*.xml)"));
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Text)){

        return;
    }
    else {
        QTextStream stream(&file);
        stream << document.toString();
        file.close();

    }
}

void MainWindow::ImportContactsXML()
{
    QDomDocument document;
    QString filename = QFileDialog::getOpenFileName(this, QStringLiteral("Open Contacts Xml"), QStringLiteral("."), QStringLiteral("Xml files (*.xml)"));
    QFile file(filename);
    
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        //qDebug() << "Failed to open XML file.";
        return;
    }
    else
    {
        if(!document.setContent(&file))
        {
            //qDebug() << "Failed to load document";
            return;
        }
        file.close();
    }

    QDomElement root = document.firstChildElement();
    QDomNodeList contact = root.elementsByTagName(QStringLiteral("Contact"));
    
    for(int i = 0; i < contact.count(); i++)
    {
        QDomNode contactnode = contact.at(i);
        if(contactnode.isElement())
        {
            QDomElement contact = contactnode.toElement();

            Contact c;
            c.m_firstname=contact.attribute(QStringLiteral("FirstName"));
            c.m_midnames=contact.attribute(QStringLiteral("MidName"));
            c.m_lastname=contact.attribute(QStringLiteral("LastName"));
            c.m_email=contact.attribute(QStringLiteral("Email"));
            c.m_street=contact.attribute(QStringLiteral("Street"));
            c.m_district=contact.attribute(QStringLiteral("District"));
            c.m_city=contact.attribute(QStringLiteral("City"));
            c.m_county=contact.attribute(QStringLiteral("County"));
            c.m_postcode=contact.attribute(QStringLiteral("Postcode"));
            c.m_country=contact.attribute(QStringLiteral("Country"));
            c.m_telephone=contact.attribute(QStringLiteral("Telephone"));
            c.m_birthdate=contact.attribute(QStringLiteral("BirthDate"));
            c.m_addToCalendar=contact.attribute(QStringLiteral("AddToCal")).toInt();

            if (dbm.isOpen())
            {
                contactId=dbm.addContact(c);                
            }
        }
    }
    file.close();
    LoadDatebaseContactsToContactList(); //refresh contactslist here
    DisplayContactsOnTableView(); //display
    UpdateCalendar(); //show birthdays

}

void MainWindow::ExportAppointmentsXML()
{
     QDomDocument document;
    QDomElement root = document.createElement(QStringLiteral("Appointments"));
    document.appendChild(root);

    QList<Appointment> dbAppointmentList =dbm.getAllAppointments();
    for (const Appointment &a : std::as_const(dbAppointmentList)) {

        QString title=a.m_title;
        QString location=a.m_location;
        QString description =a.m_description;
        QString date =a.m_date;
        QString startTime =a.m_startTime;
        QString endTime=a.m_endTime;

        QString allDay =QString::number(a.m_isFullDay);
        //Don't save reminder details so compatible with previous versions
       // QString hasReminder=QString::number(a.m_hasReminder);
       // QString reminderMins=QString::number(a.m_reminderMinutes);

        QDomElement appointment = document.createElement(QStringLiteral("Appointment"));
        appointment.setAttribute(QStringLiteral("Title"),title);
        appointment.setAttribute(QStringLiteral("Location"), location);
        appointment.setAttribute(QStringLiteral("Description"),description);
        appointment.setAttribute(QStringLiteral("Date"),date);
        appointment.setAttribute(QStringLiteral("StartTime"),startTime);
        appointment.setAttribute(QStringLiteral("EndTime"),endTime);
        appointment.setAttribute(QStringLiteral("IsAllDay"),allDay);
        //appointment.setAttribute(QStringLiteral("HasReminder"),hasReminder);
        //appointment.setAttribute(QStringLiteral("ReminderMinutes"),reminderMins);
        root.appendChild(appointment);

    }

    QString filename = QFileDialog::getSaveFileName(this, QStringLiteral("Save Appointments Xml"), QStringLiteral("."), QStringLiteral("Xml files (*.xml)"));
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Text)){

        //qDebug() << "Error saving XML file.";
        return;
    }
    else {
        QTextStream stream(&file);
        stream << document.toString();
        file.close();
        //qDebug() << "Finished";
    }

}

void MainWindow::ImportAppointmentsXML()
{
    QDomDocument document;
    QString filename = QFileDialog::getOpenFileName(this, QStringLiteral("Open Appointments Xml"), QStringLiteral("."), QStringLiteral("Xml files (*.xml)"));
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        //qDebug() << "Failed to open XML file.";
        return;
    }
    else
    {
        if(!document.setContent(&file))
        {
            // qDebug() << "Failed to load document";
            return;
        }
        file.close();
    }

    QDomElement root = document.firstChildElement();
    QDomNodeList appointment = root.elementsByTagName(QStringLiteral("Appointment"));

    for(int i = 0; i < appointment.count(); i++)
    {
        QDomNode appointmentnode = appointment.at(i);
        if(appointmentnode.isElement())
        {
            QDomElement appointment = appointmentnode.toElement();

            Appointment a;
            a.m_title=appointment.attribute(QStringLiteral("Title"));
            a.m_location=appointment.attribute(QStringLiteral("Location"));
            a.m_description=appointment.attribute(QStringLiteral("Description"));
            a.m_date=appointment.attribute(QStringLiteral("Date"));
            a.m_startTime=appointment.attribute(QStringLiteral("StartTime"));
            a.m_endTime=appointment.attribute(QStringLiteral("EndTime"));

            a.m_isFullDay=appointment.attribute(QStringLiteral("IsAllDay")).toInt();
            a.m_hasReminder=0;
            a.m_reminderMinutes=0;

            if (dbm.isOpen())
            {
                dbm.addAppointment(a);
            }
        }
    }

    file.close();
    //qDebug() << "Import Finished";
    LoadDatabaseAppointmentsToAppointmentList();
    UpdateCalendar();
}




void MainWindow::gotoNextMonthSlot()
{
    gotoNextMonth();

}

void MainWindow::gotoPreviousMonthSlot()
{
    gotoPreviousMonth();
}

void MainWindow::gotoTodaySlot()
{
    gotoToday();
}

void MainWindow::newContactSlot()
{
    NewContact();
}

void MainWindow::increaseFontSlot()
{
    increaseFont();
}

void MainWindow::decreaseFontSlot()
{
    decreaseFont();
}

void MainWindow::resetFontSlot()
{
    resetFont();
}


void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}


void MainWindow::on_actionNew_Appointment_triggered()
{
    NewAppointment();
}

void MainWindow::on_listViewDay_doubleClicked(const QModelIndex &index)
{
    int selectedRowIdx=index.row();
    Appointment tmp =dayListModel->getAppointment(selectedRowIdx);
    int dbId =tmp.m_id;   
    UpdateAppointment(dbId,selectedRowIdx);

}
void MainWindow::on_actionGeneerate_Repeat_Appointments_triggered()
{
    GenerateRepeatAppointments();
}

void MainWindow::on_actionUpcoming_Schedule_triggered()
{
    checkAppointmentsNextSevenDays();
}

void MainWindow::on_actionNew_Contact_triggered()
{
    NewContact();
}

void MainWindow::on_actionDelete_All_Appointments_triggered()
{
    dbm.deleteAllAppointments();
    appointmentList.clear();
    UpdateCalendar();
    ShowAppointmentsOnListView(selectedDate);
}



void MainWindow::on_actionDelete_All_Contacts_triggered()
{
    dbm.removeAllContacts();
    contactList.clear();
    DisplayContactsOnTableView();
    UpdateCalendar(); //remove birthdays
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, t_title + QStringLiteral(" ") + QStringLiteral(LXQT_ORGANIZER_VERSION),
                             t_about_text);
}

void MainWindow::on_tableViewContacts_clicked(const QModelIndex &index)
{
    QVariant idd = index.sibling(index.row(),0).data();
    selectedContactDbId =idd.toInt();    
    selectedContact=dbm.getContactByID(selectedContactDbId);
}

void MainWindow::on_pushButtonMailTo_clicked()
{
    QString url=QLatin1String("mailto:?to=")+selectedContact.m_email+QLatin1String("&subject=Enter the subject&body=Enter message");
    QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}

void MainWindow::on_pushButtonShowQuickFullView_clicked()
{
    ui->tableViewContacts->hideColumn(0); //index
    //ui->tableViewContacts->hideColumn(13); //BithID

    if (flagQuickView)
    {
        ui->tableViewContacts->hideColumn(2); //mid names
        ui->tableViewContacts->hideColumn(5); //street
        ui->tableViewContacts->hideColumn(6); //district
        ui->tableViewContacts->hideColumn(8); //county
        ui->tableViewContacts->hideColumn(9); //postcode
        ui->tableViewContacts->hideColumn(10); //country
        //ui->tableViewContacts->hideColumn(11); //telephone
        ui->tableViewContacts->hideColumn(12); //birthday
        flagQuickView=false;
    }
    else {

        ui->tableViewContacts->showColumn(2); //mid names
        ui->tableViewContacts->showColumn(5); //street
        ui->tableViewContacts->showColumn(6); //district
        ui->tableViewContacts->showColumn(7); //city
        ui->tableViewContacts->showColumn(8); //county
        ui->tableViewContacts->showColumn(9); //postcode
        ui->tableViewContacts->showColumn(10); //country
        ui->tableViewContacts->showColumn(12); //birthday
        flagQuickView=true;
    }
}

void MainWindow::on_tableViewContacts_doubleClicked(const QModelIndex &index)
{
    QVariant idd = index.sibling(index.row(),0).data();
    selectedContactDbId =idd.toInt();    
    UpdateContact(selectedContactDbId);
}

void MainWindow::on_actionNext_Month_triggered()
{
    gotoNextMonth();
}

void MainWindow::on_actionPrevious_Month_triggered()
{
    gotoPreviousMonth();
}

void MainWindow::on_actionToday_triggered()
{
    gotoToday();
}

void MainWindow::on_actionCheck_For_Birthdays_triggered()
{
    checkForBirthdaysNextSevenDays();
}

void MainWindow::on_actionShow_Birthdays_triggered()
{
    if(ui->actionShow_Birthdays->isChecked())

       {
            ui->actionShow_Birthdays->setChecked(true);
            flagShowBirthdays=true;

        }
        else {
            ui->actionShow_Birthdays->setChecked(false);
            flagShowBirthdays=false;
        }
        UpdateCalendar();
}

void MainWindow::on_actionShow_Holidays_triggered()
{
    if(ui->actionShow_Holidays->isChecked())
       {
          ui->actionShow_Holidays->setChecked(true);
          flagShowHolidays=true;
       }
       else {
          ui->actionShow_Holidays->setChecked(false);
          flagShowHolidays=false;
       }

       UpdateCalendar();
       ShowAppointmentsOnListView(selectedDate);
}



void MainWindow::on_actionExport_Appointments_triggered()
{
    ExportAppointmentsXML();
}

void MainWindow::on_actionImport_Appointments_triggered()
{
    ImportAppointmentsXML();
}

void MainWindow::on_actionExport_Contacts_triggered()
{
    ExportContactsXML();
}

void MainWindow::on_actionImport_Contacts_triggered()
{
    ImportContactsXML();
}


void MainWindow::on_actionPreferences_triggered()
{
    SetPreferences();
}



void MainWindow::setDarkCalendar(int darkCalendar)
{
    if (darkCalendar==0){
        //light background
        ui->tableWidgetCalendar->setStyleSheet(QStringLiteral("color: black; background-color: white"));

    }
    else if (darkCalendar==1){
        //dim gray
        ui->tableWidgetCalendar->setStyleSheet(QStringLiteral("color: white; background-color: rgb(105,105,105)"));
    }
}

void MainWindow::on_tableWidgetCalendar_cellClicked(int row, int column)
{

    if (darkCalendar==0){
    ui->tableWidgetCalendar->item(prevCellRow,prevCellCol)->setData(
                Qt::BackgroundRole,
                QBrush(QColor(Qt::GlobalColor::white)));


    ui->tableWidgetCalendar->item(row,column)->setData(
                Qt::BackgroundRole,
                QBrush(QColor(244, 205, 86))); //orange
    }
    else if(darkCalendar==1){

        ui->tableWidgetCalendar->item(prevCellRow,prevCellCol)->setData(
                    Qt::BackgroundRole,
                    QBrush(QColor(105,105,105)));


        ui->tableWidgetCalendar->item(row,column)->setData(
                    Qt::BackgroundRole,                    
                    QBrush(QColor(240, 80, 80))); //coral

    }


    prevCellRow=row;
    prevCellCol=column;

    int day =dayArray[(7 * row) + column];
    //check if dayVal>0

    if((day<1) || (day>selectedDate.daysInMonth()) ) return;

    selectedDate =QDate(selectedYear,selectedMonth,day);

    QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
    selectedDateLabel->setText(date);
    ShowAppointmentsOnListView(selectedDate);


}

void MainWindow::on_tableWidgetCalendar_cellDoubleClicked(int row, int column)
{

    int day =dayArray[(7 * row) + column];
    if((day<1) || (day>selectedDate.daysInMonth()) ) return;
    selectedDate =QDate(selectedYear,selectedMonth,day);    
    QString date =locale.toString(selectedDate,QStringLiteral("dddd dd MMMM yyyy"));
    selectedDateLabel->setText(date);
    ShowAppointmentsOnListView(selectedDate);
    NewAppointment();
}
void MainWindow::on_actionIncrease_Font_triggered()
{
    increaseFont();
}

void MainWindow::on_actionDecrease_Font_triggered()
{
    decreaseFont();
}

void MainWindow::on_actionReset_Font_triggered()
{
    resetFont();
}

void MainWindow::loadSettings()
{
    LXQt::Settings settings(QStringLiteral("lxqt-organizer"));
     settings.beginGroup(QStringLiteral("CoreSettings"));
     currentPreferences.m_playAudio=settings.value(QStringLiteral("PlayAudio"), playAudio).toInt();
     currentPreferences.m_darkCalendar=settings.value(QStringLiteral("DarkCalendar"), darkCalendar).toInt();
     currentPreferences.m_applicationFontSize=settings.value(QStringLiteral("ApplicationFontSize"), applicationFontSize).toInt();
     currentPreferences.m_lineSpacing=settings.value(QStringLiteral("LineSpacing"), newLineSpacing).toInt();
    settings.endGroup();
    //qDebug()<<"Settings loaded";
}

void MainWindow::saveSettings()
{
    LXQt::Settings settings(QStringLiteral("lxqt-organizer"));
    settings.beginGroup(QStringLiteral("CoreSettings"));
    settings.setValue(QStringLiteral("PlayAudio"),currentPreferences.m_playAudio);
    settings.setValue(QStringLiteral("DarkCalendar"),currentPreferences.m_darkCalendar);
    settings.setValue(QStringLiteral("ApplicationFontSize"),currentPreferences.m_applicationFontSize);
    settings.setValue(QStringLiteral("LineSpacing"),currentPreferences.m_lineSpacing);
    settings.endGroup();
    //qDebug()<<"Settings saved";
}

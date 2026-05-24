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

#include "dbmanager.h"

DbManager::DbManager()
{
}

bool DbManager::isOpen() const
{
    return db.isOpen();
}

void DbManager::openDatabase()
{
    if(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")))
    {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));

        db.setDatabaseName(QStringLiteral("lxqt-organizer.sqlite"));
        if(!db.open())
            qWarning() << "Database connection error: " << db.lastError().text();
    }
    else
        qWarning() << ":Database connection error: no SQLite driver";
}


void DbManager::createDatebaseTables()
{
    QSqlQuery query;
   //QString sql1 = "CREATE TABLE IF NOT EXISTS appointments (AppointmentId INTEGER PRIMARY KEY,";

    QString sql1 = QStringLiteral("CREATE TABLE appointments (AppointmentId INTEGER PRIMARY KEY,");
    sql1.append(QStringLiteral("Title TEXT,"));
    sql1.append(QStringLiteral("Location TEXT,"));
    sql1.append(QStringLiteral("Description TEXT,"));
    sql1.append(QStringLiteral("Date TEXT,"));
    sql1.append(QStringLiteral("StartTime TEXT,"));
    sql1.append(QStringLiteral("EndTime TEXT,"));    
    sql1.append(QStringLiteral("IsFullDay INTEGER,"));
    sql1.append(QStringLiteral("HasReminder INTEGER,"));
    sql1.append(QStringLiteral("ReminderMinutes INTEGER"));
    sql1.append(QStringLiteral(");"));


    QString sql2 = QStringLiteral("CREATE TABLE contacts (ContactId INTEGER PRIMARY KEY,");
    sql2.append(QStringLiteral("FirstName TEXT,"));
    sql2.append(QStringLiteral("MidName TEXT,"));
    sql2.append(QStringLiteral("LastName TEXT,"));
    sql2.append(QStringLiteral("Email TEXT,"));
    sql2.append(QStringLiteral("Street TEXT,"));
    sql2.append(QStringLiteral("District TEXT,"));
    sql2.append(QStringLiteral("City TEXT,"));
    sql2.append(QStringLiteral("County TEXT,"));
    sql2.append(QStringLiteral("Postcode TEXT,"));
    sql2.append(QStringLiteral("Country TEXT,"));
    sql2.append(QStringLiteral("Telephone TEXT,"));
    sql2.append(QStringLiteral("BirthDate TEXT,"));
    sql2.append(QStringLiteral("AddToCalendar INTEGER"));
    sql2.append(QStringLiteral(");"));

    query.prepare(sql1);
    if(!query.exec())
    {
       //qDebug()<<"Table appointments already exists";
    }
    else {
        //qDebug()<<"Table appointments successfully created";
    }
    query.prepare(sql2);
    if(!query.exec())
    {
      //qDebug()<<"Table contacts already exists";
    }
    else {
       //qDebug()<<"Table contacts successfully created";
    }


}

//-------------------------------------------------------------
// Appointments
//--------------------------------------------------------------

int DbManager::addAppointment(Appointment &appointment)
{
    QString title=appointment.m_title;
    QString location=appointment.m_location;
    QString description =appointment.m_description;
    QString date = appointment.m_date;
    QString startTime=appointment.m_startTime;
    QString endTime=appointment.m_endTime;    
    int isFullDay=appointment.m_isFullDay;
    int hasReminder =appointment.m_hasReminder;
    int reminderMins =appointment.m_reminderMinutes;

    //qDebug()<<"Inserting appointment into database table";

    QString sql=QStringLiteral("INSERT INTO appointments(`AppointmentId`,");
        sql.append(QStringLiteral("`Title`,"));
        sql.append(QStringLiteral("`Location`,"));
        sql.append(QStringLiteral("`Description`,"));
        sql.append(QStringLiteral("`Date`,"));
        sql.append(QStringLiteral("`StartTime`,"));
        sql.append(QStringLiteral("`EndTime`,"));        
        sql.append(QStringLiteral("`IsFullDay`,"));
        sql.append(QStringLiteral("`HasReminder`,"));
        sql.append(QStringLiteral("`ReminderMinutes`)"));
        sql.append(QStringLiteral("VALUES (:idin,"));
        sql.append(QStringLiteral(":titlein,"));
        sql.append(QStringLiteral(":locationin,"));
        sql.append(QStringLiteral(":descriptionin,"));
        sql.append(QStringLiteral(":datein,"));
        sql.append(QStringLiteral(":starttimein,"));
        sql.append(QStringLiteral(":endtimein,"));        
        sql.append(QStringLiteral(":isfulldayin,"));
        sql.append(QStringLiteral(":hasreminderin,"));
        sql.append(QStringLiteral(":reminderminsin);"));

        QSqlQuery query;
        query.prepare(sql);
        //query.bindValue(":idin", 1); //autoincrement as primary key
        query.bindValue(QStringLiteral(":titlein"), title);
        query.bindValue(QStringLiteral(":locationin"),location);
        query.bindValue(QStringLiteral(":descriptionin"),description);
        query.bindValue(QStringLiteral(":datein"),date);
        query.bindValue(QStringLiteral(":starttimein"),startTime);
        query.bindValue(QStringLiteral(":endtimein"),endTime);        
        query.bindValue(QStringLiteral(":isfulldayin"),isFullDay);
        query.bindValue(QStringLiteral(":hasreminderin"),hasReminder);
        query.bindValue(QStringLiteral(":reminderminsin"),reminderMins);
        query.exec();

        int id = query.lastInsertId().toInt();
        //qDebug() << "added new appointment with ID = "<<id;
        return id;


}

bool DbManager::updateAppointment(Appointment &appointment, int id)
{
    bool success =false;

       QString sql =QStringLiteral("UPDATE appointments SET ");
       sql.append(QStringLiteral(" Title = :tin"));
       sql.append(QStringLiteral(", Location = :lin"));
       sql.append(QStringLiteral(", Description = :din"));
       sql.append(QStringLiteral(", Date = :datein"));
       sql.append(QStringLiteral(", StartTime = :stin"));
       sql.append(QStringLiteral(", EndTime = :etin"));       
       sql.append(QStringLiteral(", IsFullDay = :isfdin"));
       sql.append(QStringLiteral(", HasReminder = :hasremin"));
       sql.append(QStringLiteral(", ReminderMinutes = :remminsin"));

       sql.append(" WHERE AppointmentId =:idin");

       QSqlQuery qry;
       if(qry.prepare(sql))
       {
         qry.bindValue(QStringLiteral(":tin"), appointment.m_title);
         qry.bindValue(QStringLiteral(":lin"), appointment.m_location);
         qry.bindValue(QStringLiteral(":din"), appointment.m_description);
         qry.bindValue(QStringLiteral(":datein"), appointment.m_date);
         qry.bindValue(QStringLiteral(":stin"),appointment.m_startTime);
         qry.bindValue(QStringLiteral(":etin"),appointment.m_endTime);         
         qry.bindValue(QStringLiteral(":isfdin"),appointment.m_isFullDay);
         qry.bindValue(QStringLiteral(":hasremin"),appointment.m_hasReminder);
         qry.bindValue(QStringLiteral(":remminsin"),appointment.m_reminderMinutes);

         qry.bindValue(":idin", QString::number(id));
         success=qry.exec();
         if (success)
         {
           //qDebug() << "Appointment updated" << " sucess ="<<success;
           return success;  //true
         }
         else
         {
           //qDebug() << "Appointment update failed"<<" sucess ="<<success;
           return success;
         }
       }
       else
       {
         //qDebug() << "SQL is broken"<<" sucess ="<<success;
         return success;
       }}

QList<Appointment> DbManager::getAllAppointments()
{
    QList<Appointment> appointmentList;
    //qDebug()<<"getting all appointments...";
    QSqlQuery query(QStringLiteral("SELECT * FROM appointments"));
    query.exec();

    while (query.next())
    {
        int idName = query.record().indexOf(QStringLiteral("AppointmentId"));
        int aid = query.value(idName).toInt();
        idName = query.record().indexOf(QStringLiteral("Title"));
        QString title = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Location"));
        QString location = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Description"));
        QString description = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Date"));
        QString date = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("StartTime"));
        QString startTime = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("EndTime"));
        QString endTime = query.value(idName).toString();


        idName = query.record().indexOf(QStringLiteral("IsFullDay"));
        int isFullDay = query.value(idName).toInt();
        
         idName = query.record().indexOf(QStringLiteral("HasReminder"));
        int hasReminder = query.value(idName).toInt();

        idName = query.record().indexOf(QStringLiteral("ReminderMinutes"));
        int reminderMins = query.value(idName).toInt();


        Appointment appointment;
        appointment.m_id=aid;
        appointment.m_title=title;
        appointment.m_location=location;
        appointment.m_description=description;
        appointment.m_date=date;
        appointment.m_startTime=startTime;
        appointment.m_endTime=endTime;       
        appointment.m_isFullDay=isFullDay;
        appointment.m_hasReminder=hasReminder;
        appointment.m_reminderMinutes=reminderMins;
        appointmentList.append(appointment);
}
    return appointmentList;
}

Appointment DbManager::getAppointmentByID(int id)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM appointments WHERE AppointmentId = (:idin)"));
    query.bindValue(QStringLiteral(":idin"), id);
    query.exec();
    query.first();

    int idName = query.record().indexOf(QStringLiteral("AppointmentId"));
    int idd = query.value(idName).toInt();

    idName = query.record().indexOf(QStringLiteral("Title"));
    QString title = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Location"));
    QString location = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Description"));
    QString description = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Date"));
    QString date = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("StartTime"));
    QString startTime = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("EndTime"));
    QString endTime = query.value(idName).toString();


    idName = query.record().indexOf(QStringLiteral("IsFullDay"));
    int isFullDay = query.value(idName).toInt();
    
     idName = query.record().indexOf(QStringLiteral("HasReminder"));
    int hasReminder = query.value(idName).toInt();

    idName = query.record().indexOf(QStringLiteral("ReminderMinutes"));
    int reminderMins = query.value(idName).toInt();


    Appointment appointment;
    appointment.m_id=idd;
    appointment.m_title=title;
    appointment.m_location=location;
    appointment.m_description=description;
    appointment.m_date=date;
    appointment.m_startTime=startTime;
    appointment.m_endTime=endTime;    
    appointment.m_isFullDay=isFullDay;
    appointment.m_hasReminder=hasReminder;
    appointment.m_reminderMinutes=reminderMins;

    return appointment;
}

QList<Appointment> DbManager::getAppointmentsOnDate(QDate &diaryDate)
{
    QDate checkDate = QDate(diaryDate);

    QList<Appointment> appointmentList = QList<Appointment>(); //Construct an empty list
    QSqlQuery query(QStringLiteral("SELECT * FROM appointments"));
    query.exec();

    while (query.next())
    {
        int idName = query.record().indexOf(QStringLiteral("AppointmentId"));
        int id = query.value(idName).toInt();
        idName = query.record().indexOf(QStringLiteral("Title"));
        QString title = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Location"));
        QString location = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Description"));
        QString description = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Date"));
        QString date = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("StartTime"));
        QString startTime = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("EndTime"));
        QString endTime = query.value(idName).toString();


        idName = query.record().indexOf(QStringLiteral("IsFullDay"));
        int isFullDay = query.value(idName).toInt();
        
        idName = query.record().indexOf(QStringLiteral("HasReminder"));
        int hasReminder = query.value(idName).toInt();

        idName = query.record().indexOf(QStringLiteral("ReminderMinutes"));
        int reminderMins = query.value(idName).toInt();



        QDate appointmentDate =QDate::fromString(date);
        if(appointmentDate ==checkDate)
        {
            Appointment tmp;
            tmp.m_id=id;
            tmp.m_title=title;
            tmp.m_location=location;
            tmp.m_description=description;
            tmp.m_date=date;
            tmp.m_startTime=startTime;
            tmp.m_endTime=endTime;            
            tmp.m_isFullDay=isFullDay;
            tmp.m_hasReminder=hasReminder;
            tmp.m_reminderMinutes=reminderMins;
            appointmentList.append(tmp);
        }
    }
    return appointmentList;
}

bool DbManager::deleteAllAppointments()
{
    bool success = false;
    QSqlQuery removeQuery;
    removeQuery.prepare(QStringLiteral("DELETE FROM appointments"));
    if (removeQuery.exec())
    {
        //qDebug() << "remove all appointments succeeded";
        success = true;
    }
    else
    {
        //qDebug() << "remove all appointments failed: " << removeQuery.lastError();
    }
    return success;
}



bool DbManager::deleteAppointmentById(const int id)
{
    bool success = false;
    //qDebug() << "Appointment id = "<<id;
    QSqlQuery queryDelete;
    queryDelete.prepare(QStringLiteral("DELETE FROM appointments WHERE AppointmentId = (:idin)"));
    queryDelete.bindValue(QStringLiteral(":idin"), id);
    success = queryDelete.exec();
    if(!success)
    {
       //qDebug() << "remove appointment failed: " << queryDelete.lastError();
    }
   // qDebug()<< "Remove appointment success = "<<success;
    return success;
}


//-----------------------------------------------------------------------
//Contacts
//-----------------------------------------------------------------------


int DbManager::addContact(Contact &contact)
{
    QString firstName =contact.m_firstname;
    QString midName=contact.m_midnames;
    QString lastName =contact.m_lastname;
    QString email =contact.m_email;
    QString street =contact.m_street;
    QString district=contact.m_district;
    QString city =contact.m_city;
    QString county =contact.m_county;
    QString postcode = contact.m_postcode;
    QString country =contact.m_country;
    QString telephone = contact.m_telephone;    
    QString birthdate=contact.m_birthdate;
    int addToCalendar=contact.m_addToCalendar;


    QString sql=QStringLiteral("INSERT INTO contacts(`ContactId`,");
    sql.append(QStringLiteral("`FirstName`,"));
    sql.append(QStringLiteral("`MidName`,"));
    sql.append(QStringLiteral("`LastName`,"));
    sql.append(QStringLiteral("`Email`,"));
    sql.append(QStringLiteral("`Street`,"));
     sql.append(QStringLiteral("`District`,"));
    sql.append(QStringLiteral("`City`,"));
    sql.append(QStringLiteral("`County`,"));
    sql.append(QStringLiteral("`Postcode`,"));
    sql.append(QStringLiteral("`Country`,"));
    sql.append(QStringLiteral("`Telephone`,"));
    sql.append(QStringLiteral("`BirthDate`,"));
    sql.append(QStringLiteral("`AddToCalendar`)"));


    sql.append(QStringLiteral("VALUES (:idin,"));
    sql.append(QStringLiteral(":firstnamein,"));
    sql.append(QStringLiteral(":midnamein,"));
    sql.append(QStringLiteral(":lastnamein,"));
    sql.append(QStringLiteral(":emailin,"));
    sql.append(QStringLiteral(":streetin,"));
    sql.append(QStringLiteral(":districtin,"));
    sql.append(QStringLiteral(":cityin,"));
    sql.append(QStringLiteral(":countyin,"));
    sql.append(QStringLiteral(":postcodein,"));
    sql.append(QStringLiteral(":countryin,"));
    sql.append(QStringLiteral(":telephonein,"));
    sql.append(QStringLiteral(":birthdatein,"));
    sql.append(QStringLiteral(":addtocalendarin);"));


    QSqlQuery query;
    query.prepare(sql);

    //query.bindValue(":idin", 1); //autoincrement as primary key
    query.bindValue(QStringLiteral(":firstnamein"), firstName);
    query.bindValue(QStringLiteral(":midnamein"), midName);
    query.bindValue(QStringLiteral(":lastnamein"), lastName);
    query.bindValue(QStringLiteral(":emailin"), email);
    query.bindValue(QStringLiteral(":streetin"), street);
    query.bindValue(QStringLiteral(":districtin"), district);
    query.bindValue(QStringLiteral(":cityin"), city);
    query.bindValue(QStringLiteral(":countyin"), county);
    query.bindValue(QStringLiteral(":postcodein"), postcode);
    query.bindValue(QStringLiteral(":countryin"), country);
    query.bindValue(QStringLiteral(":telephonein"), telephone);    
    query.bindValue(QStringLiteral(":birthdatein"), birthdate);
    query.bindValue(QStringLiteral(":addtocalendarin"), addToCalendar);

    query.exec();
    int id = query.lastInsertId().toInt();
    //qDebug() << "added new contact with ID = "<<id;
    return id;

}

bool DbManager::updateContact(Contact &contact, int id)
{
    bool success =false;

    QString sql =QStringLiteral("UPDATE contacts SET ");
    sql.append(QStringLiteral(" FirstName = :fnin"));
    sql.append(QStringLiteral(", MidName = :mnin"));
    sql.append(QStringLiteral(", LastName = :lnin"));
    sql.append(QStringLiteral(", Email = :emin"));
    sql.append(QStringLiteral(", Street = :stin"));
    sql.append(QStringLiteral(", District = :disin"));
    sql.append(QStringLiteral(", City = :ctyin"));
    sql.append(QStringLiteral(", County = :cntin"));
    sql.append(QStringLiteral(", Postcode = :pcin"));
    sql.append(QStringLiteral(", Country =:cin"));
    sql.append(QStringLiteral(", Telephone =:tin"));
    sql.append(QStringLiteral(", BirthDate =:bdatein"));
    sql.append(QStringLiteral(", AddToCalendar =:addtocalin"));

    sql.append(QStringLiteral(" WHERE ContactId =:idin"));

    QSqlQuery qry;
    if(qry.prepare(sql))
    {
      qry.bindValue(QStringLiteral(":fnin"), contact.m_firstname);
      qry.bindValue(QStringLiteral(":mnin"),contact.m_midnames);
      qry.bindValue(QStringLiteral(":lnin"),contact.m_lastname);
      qry.bindValue(QStringLiteral(":emin"),contact.m_email);
      qry.bindValue(QStringLiteral(":stin"),contact.m_street);
      qry.bindValue(QStringLiteral(":disin"),contact.m_district);
      qry.bindValue(QStringLiteral(":ctyin"),contact.m_city);
      qry.bindValue(QStringLiteral(":cntin"),contact.m_county);
      qry.bindValue(QStringLiteral(":pcin"),contact.m_postcode);
      qry.bindValue(QStringLiteral(":cin"),contact.m_country);
      qry.bindValue(QStringLiteral(":tin"),contact.m_telephone);      
      qry.bindValue(QStringLiteral(":bdatein"),contact.m_birthdate);
      qry.bindValue(QStringLiteral(":addtocalin"),contact.m_addToCalendar);

      //qry.bindValue(":bidin",contact.m_birthdayAppointmentId);
      qry.bindValue(QStringLiteral(":idin"), QString::number(id));
      success = qry.exec();
      if (success)
      {
          //qDebug() << "Contact updated";
          return success;
      }
      else
      {
          //qDebug() << "Contact update failed";
          return success;
      }
    }
    else
    {
      //qDebug() << "SQL is broken";
      return success;
    }
}



QList<Contact> DbManager::getAllContacts()
{
    QList<Contact> contactList;   
    QSqlQuery query(QStringLiteral("SELECT * FROM contacts"));
    query.exec();

    while (query.next())
    {
        int idName = query.record().indexOf(QStringLiteral("ContactId"));
        int id = query.value(idName).toInt();
        idName = query.record().indexOf(QStringLiteral("FirstName"));
        QString firstname = query.value(idName).toString();        
        idName = query.record().indexOf(QStringLiteral("MidName"));
        QString midname = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("LastName"));
        QString lastname = query.value(idName).toString();        
        idName = query.record().indexOf(QStringLiteral("Email"));
        QString email = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Street"));
        QString street = query.value(idName).toString();        
        idName = query.record().indexOf(QStringLiteral("District"));
        QString district = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("City"));
        QString city = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("County"));
        QString county = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Postcode"));
        QString postcode = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Country"));
        QString country = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("Telephone"));
        QString telephone = query.value(idName).toString();
        idName = query.record().indexOf(QStringLiteral("BirthDate"));
        QString birthdate = query.value(idName).toString();

        idName = query.record().indexOf(QStringLiteral("AddToCalendar"));
        int  addtocal = query.value(idName).toInt();

        Contact tmp;
        tmp.m_id=id;
        tmp.m_firstname=firstname;
        tmp.m_midnames=midname;
        tmp.m_lastname=lastname;
        tmp.m_email=email;
        tmp.m_street=street;
        tmp.m_district=district;
        tmp.m_city=city;
        tmp.m_county=county;
        tmp.m_postcode=postcode;
        tmp.m_country=country;
        tmp.m_telephone=telephone;       
        tmp.m_birthdate=birthdate;
        tmp.m_addToCalendar=addtocal;

        contactList.append(tmp);
    }
    return contactList;
}

Contact DbManager::getContactByID(int id)
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT * FROM contacts WHERE ContactId = (:idin)"));
    query.bindValue(QStringLiteral(":idin"), id);
    query.exec();
    query.first();

    int idName = query.record().indexOf(QStringLiteral("ContactId"));
    int idd = query.value(idName).toInt();
    idName = query.record().indexOf(QStringLiteral("FirstName"));
    QString firstname = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("MidName"));
    QString midname = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("LastName"));
    QString lastname = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Email"));
    QString email = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Street"));
    QString street = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("District"));
    QString district = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("City"));
    QString city = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("County"));
    QString county = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Postcode"));
    QString postcode = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Country"));
    QString country = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("Telephone"));
    QString telephone = query.value(idName).toString();
    idName = query.record().indexOf(QStringLiteral("BirthDate"));
    QString birthdate = query.value(idName).toString();

    idName = query.record().indexOf(QStringLiteral("AddToCalendar"));
    int  addtocal = query.value(idName).toInt();

    Contact contact;
    contact.m_id=idd;
    contact.m_firstname=firstname;
    contact.m_midnames=midname;
    contact.m_lastname=lastname;
    contact.m_email=email;
    contact.m_street=street;
    contact.m_district=district;
    contact.m_city=city;
    contact.m_county=county;
    contact.m_postcode=postcode;
    contact.m_country=country;
    contact.m_telephone=telephone;    
    contact.m_birthdate=birthdate;
    contact.m_addToCalendar=addtocal;

    return contact;
}

bool DbManager::removeAllContacts()
{
    bool success = false;
    QSqlQuery removeQuery;
    removeQuery.prepare(QStringLiteral("DELETE FROM contacts"));
    if (removeQuery.exec())
    {
        //qDebug() << "remove all contacts succeeded";
        success = true;
    }
    else
    {
        //qDebug() << "remove all contacts failed: " << removeQuery.lastError();
    }
    return success;
}

bool DbManager::removeContactById(const int id)
{
    bool success = false;    
    QSqlQuery queryDelete;
    queryDelete.prepare(QStringLiteral("DELETE FROM contacts WHERE ContactId = (:idin)"));
    queryDelete.bindValue(QStringLiteral(":idin"), id);
    success = queryDelete.exec();
    if(!success)
    {
        //qDebug() << "remove contact failed: " << queryDelete.lastError();
    }
    //qDebug()<< "Remove contact success = "<<success;
    return success;
}





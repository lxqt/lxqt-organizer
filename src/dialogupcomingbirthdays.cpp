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

#include "dialogupcomingbirthdays.h"
#include "ui_dialogupcomingbirthdays.h"

#include <utility>


DialogUpcomingBirthdays::DialogUpcomingBirthdays(QWidget *parent,
                                                 QList<Contact> *contactList):
    QDialog(parent),
    ui(new Ui::DialogUpcomingBirthdays)
{
    ui->setupUi(this);
    setWindowTitle(t_title);
    dialogContactList=*contactList;

   birthdayList =QList<QString>();

   QDate currentDate =QDate::currentDate();

   for (const Contact &c : std::as_const(dialogContactList))  //use contacts!!
   {
       //qDebug()<<"Contact:"<<c.m_lastname;
       QDate bornDate = QDate::fromString(c.m_birthdate);

       if (!bornDate.isNull())
       {
       QDate bdayDate=QDate(currentDate.year(),bornDate.month(),bornDate.day());

       QDate d1 =bdayDate.addDays(1); //can change year
       QDate d2 =bdayDate.addDays(2);
       QDate d3 =bdayDate.addDays(3);
       QDate d4 =bdayDate.addDays(4);
       QDate d5 =bdayDate.addDays(5);
       QDate d6 =bdayDate.addDays(6);
       QDate d7 =bdayDate.addDays(7);

       if ((d1<=currentDate.addDays(7))
               ||(d2<=currentDate.addDays(7))
               ||(d3<=currentDate.addDays(7))
               ||(d4<=currentDate.addDays(7))
               ||(d5<=currentDate.addDays(7))
               ||(d6<=currentDate.addDays(7))
               ||(d7<=currentDate.addDays(7))
               )
       {
           QString str=c.m_firstname+QLatin1Char(' ')+c.m_lastname;
           str.append(QStringLiteral(" (")+bdayDate.toString()+QStringLiteral(")"));
           birthdayList.append(str);
       }
      }
   }
    birthdayListModel = new BirthdayListModel(birthdayList);
    ui->listViewBirthdays->setModel(birthdayListModel);
}

DialogUpcomingBirthdays::~DialogUpcomingBirthdays()
{
    delete ui;
}

void DialogUpcomingBirthdays::setTitleTranslation(QString translation)
{
    t_title=translation;
    setWindowTitle(t_title);
}

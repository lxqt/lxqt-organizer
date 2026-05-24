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

#ifndef CONTACTMODEL_H
#define CONTACTMODEL_H

#include <QAbstractTableModel>

#include "contact.h"
#include <QColor>
#include <QList>
#include <QDate>
#include <QDebug>

class ContactModel : public QAbstractTableModel
{
public:
    ContactModel(QObject* parent = nullptr);
    ContactModel(const QList<Contact>& contactList,
                 QObject *parent = nullptr);

    void AddContact(const Contact &contact);
    Contact getContact(int index);
    void clearAllContacts();
    void removeContact(int idx);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data (const QModelIndex & index,
                   int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;


    QString t_first_name=QStringLiteral("First Name");
    void setFirstNameTranslation(QString translation);

    QString t_mid_name=QStringLiteral("Mid Name");
    void setMidNameTranslation(QString translation);

    QString t_last_name=QStringLiteral("Last Name");
    void setLastNameTranslation(QString translation);

    QString t_email=QStringLiteral("Email");
    void setEmailTranslation(QString translation);

    QString t_street=QStringLiteral("Street");
    void setStreetTranslation(QString translation);

    QString t_district=QStringLiteral("District");
    void setDistrictTranslation(QString translation);

    QString t_city=QStringLiteral("City");
    void setCityTranslation(QString translation);

    QString t_county=QStringLiteral("County");
    void setCountyTransaltion(QString translation);

    QString t_postcode=QStringLiteral("Postcode");
    void setPostcodeTranslation(QString translation);

    QString t_country=QStringLiteral("Country");
    void setCountryTranslation(QString translation);

    QString t_telephone=QStringLiteral("Telephone");
    void setTelephoneTranslation(QString translation);

    QString t_birthday=QStringLiteral("Birthday");
    void setBirthdayTranslation(QString translation);







private:
    QList<Contact> modelContactList;


};

#endif // CONTACTMODEL_H

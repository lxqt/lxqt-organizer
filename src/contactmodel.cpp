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

#include "contactmodel.h"

ContactModel::ContactModel(QObject *parent):
   QAbstractTableModel(parent)
{
    Q_UNUSED(parent)
}

ContactModel::ContactModel(const QList<Contact> &contactList, QObject *parent)
{
    Q_UNUSED(parent)
    this->modelContactList=contactList;
}

void ContactModel::AddContact(Contact &contact)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    modelContactList.append(contact);
    endInsertRows();
}

Contact ContactModel::getContact(int index)
{
    Contact c = modelContactList.value(index);
    return c;
}

void ContactModel::clearAllContacts()
{
    beginResetModel();
    modelContactList.clear();
    endResetModel();
}

void ContactModel::removeContact(int idx)
{
    beginRemoveRows(QModelIndex(), 0, rowCount());
    modelContactList.removeAt(idx);
    endRemoveRows();
}

int ContactModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return modelContactList.size();
}

int ContactModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 13; //13 columns (see cases below)
}

QVariant ContactModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
           return QVariant();
       if (index.row() >= modelContactList.size() || index.row() < 0)
           return QVariant();


       if (role == Qt::DisplayRole) {

           const Contact& c = modelContactList.at(index.row());
           QDate birthdayDate =QDate::fromString(c.m_birthdate);

           QString birthdayDateStr="("+QString::number(birthdayDate.day())+"/"
                   +QString::number(birthdayDate.month())+"/"
                   +QString::number(birthdayDate.year())+")";


           switch (index.column()) {
           case 0:
               return c.m_id;
           case 1:
               return c.m_firstname;
           case 2:
               return c.m_midnames;
           case 3:
               return c.m_lastname;
           case 4:
               return c.m_email;
           case 5:
               return c.m_street;
           case 6:
               return c.m_district;
           case 7:
               return c.m_city;
           case 8:
               return c.m_county;
           case 9:
               return c.m_postcode;
           case 10:
               return c.m_country;
           case 11:
               return c.m_telephone;
           case 12:
               return birthdayDateStr;           


           default:
               return QVariant();
           }
       }
       else if (role == Qt::TextAlignmentRole){
           if (index.column() == 3 || index.column()==4) //start and end times
               return Qt::AlignCenter;
           else
               return Qt::AlignLeft;
       }
       return QVariant();
}



QVariant ContactModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return "ContactId";
        case 1:
            return "First Name";
        case 2:
            return "Mid Names";
        case 3:
            return "Last Name";
        case 4:
            return "Email";
        case 5:
            return "Street";
        case 6:
            return "District";
        case 7:
            return "City";
        case 8:
            return "County";
        case 9:
            return "Postcode";
        case 10:
            return "Country";
        case 11:
            return "Telephone";       
        case 12:
            return "Birthday";       
        default:
            return QVariant();
        }
    }
    return section + 1;
}


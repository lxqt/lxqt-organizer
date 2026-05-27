/*
 * LXQt Organizer - personal information manager for LXQt.
 * Author: Alan Crispin aka. crispina <crispinalan@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONTACTMODEL_H
#define CONTACTMODEL_H

#include <QAbstractTableModel>

#include "collectionsummary.h"
#include "contactsnapshot.h"
#include <QColor>
#include <QList>
#include <QDate>
#include <QDebug>

struct ContactListRow
{
    ContactDisplay display;
    Contact edit;
};

class ContactModel : public QAbstractTableModel
{
public:
    ContactModel(QObject *parent = nullptr);
    ContactModel(const QList<Contact> &contactList, QObject *parent = nullptr);

    void setCollectionSummaryProvider(CollectionSummaryProvider provider);
    void addContact(const Contact &contact);
    void setContacts(const QList<Contact> &contacts);
    Contact getContact(int index) const;
    void clearAllContacts();
    void removeContact(int idx);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    CollectionSummary collectionSummary(const QString &collectionId) const;

    QList<ContactListRow> modelContactList;
    CollectionSummaryProvider m_collectionSummaryProvider;
};

#endif // CONTACTMODEL_H

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

#include "contactmodel.h"

#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <utility>

namespace {

ContactListRow contactRow(const Contact &contact, const CollectionSummary &collection)
{
    return {contactDisplay(contact, collection), contact};
}

QList<ContactListRow> contactRows(const QList<Contact> &contacts, const CollectionSummaryProvider &provider)
{
    QList<ContactListRow> rows;
    rows.reserve(contacts.size());
    for (const Contact &contact : contacts)
    {
        const CollectionSummary collection = provider ? provider(contact.ref.collectionId) : CollectionSummary{{}, {}};
        rows.append(contactRow(contact, collection));
    }
    return rows;
}

} // namespace

ContactModel::ContactModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

ContactModel::ContactModel(const QList<Contact> &contactList, QObject *parent)
    : QAbstractTableModel(parent)
    , modelContactList(contactRows(contactList, m_collectionSummaryProvider))
{}

void ContactModel::setCollectionSummaryProvider(CollectionSummaryProvider provider)
{
    m_collectionSummaryProvider = std::move(provider);
    if (!modelContactList.isEmpty())
    {
        Q_EMIT dataChanged(index(0, 0), index(modelContactList.size() - 1, columnCount() - 1));
    }
}

void ContactModel::addContact(const Contact &contact)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    modelContactList.append(contactRow(contact, collectionSummary(contact.ref.collectionId)));
    endInsertRows();
}

void ContactModel::setContacts(const QList<Contact> &contacts)
{
    beginResetModel();
    modelContactList = contactRows(contacts, m_collectionSummaryProvider);
    endResetModel();
}

Contact ContactModel::getContact(int index) const
{
    return modelContactList.value(index).edit;
}

void ContactModel::clearAllContacts()
{
    beginResetModel();
    modelContactList.clear();
    endResetModel();
}

void ContactModel::removeContact(int idx)
{
    if (idx < 0 || idx >= modelContactList.size())
    {
        return;
    }
    beginRemoveRows(QModelIndex(), idx, idx);
    modelContactList.removeAt(idx);
    endRemoveRows();
}

CollectionSummary ContactModel::collectionSummary(const QString &collectionId) const
{
    return m_collectionSummaryProvider ? m_collectionSummaryProvider(collectionId) : CollectionSummary{{}, {}};
}

QVariant colorSwatchDecoration(const QString &color)
{
    const QColor parsed(color);
    if (!parsed.isValid())
    {
        return {};
    }

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(parsed.darker(130)));
    painter.setBrush(parsed);
    painter.drawRoundedRect(QRectF(3, 3, 10, 10), 2, 2);
    return QIcon(pixmap);
}

int ContactModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return modelContactList.size();
}

int ContactModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 12; // 12 columns (see cases below)
}

QVariant ContactModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }
    if (index.row() >= modelContactList.size() || index.row() < 0)
    {
        return QVariant();
    }


    if (role == Qt::DisplayRole)
    {
        const ContactDisplay &c = modelContactList.at(index.row()).display;

        switch (index.column())
        {
        case 0:
            return c.givenName;
        case 1:
            return c.additionalName;
        case 2:
            return c.familyName;
        case 3:
            return c.preferredEmail;
        case 4:
            return c.street;
        case 5:
            return c.district;
        case 6:
            return c.city;
        case 7:
            return c.county;
        case 8:
            return c.postcode;
        case 9:
            return c.country;
        case 10:
            return c.phoneNumber;
        case 11:
            return c.collection.displayName;


        default:
            return QVariant();
        }
    }
    else if (role == Qt::DecorationRole && index.column() == 11)
    {
        const ContactDisplay &c = modelContactList.at(index.row()).display;
        return colorSwatchDecoration(c.collection.color);
    }
    else if (role == Qt::TextAlignmentRole)
    {
        return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
    }
    return QVariant();
}


QVariant ContactModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return tr("First Name");
        case 1:
            return tr("Middle Names");
        case 2:
            return tr("Last Name");
        case 3:
            return tr("Email");
        case 4:
            return tr("Street");
        case 5:
            return tr("District");
        case 6:
            return tr("City");
        case 7:
            return tr("County");
        case 8:
            return tr("Postcode");
        case 9:
            return tr("Country");
        case 10:
            return tr("Telephone");
        case 11:
            return tr("Collection");
        default:
            return QVariant();
        }
    }
    return section + 1;
}

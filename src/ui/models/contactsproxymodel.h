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

#ifndef CONTACTSPROXYMODEL_H
#define CONTACTSPROXYMODEL_H

#include <QSortFilterProxyModel>

class ContactsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ContactsProxyModel(QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // CONTACTSPROXYMODEL_H

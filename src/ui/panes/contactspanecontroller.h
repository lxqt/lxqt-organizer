/*
 * LXQt Organizer - personal information manager for LXQt.
 * Copyright (C) 2026 Basil Crow
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

#ifndef CONTACTSPANECONTROLLER_H
#define CONTACTSPANECONTROLLER_H

#include "contactspane.h"

#include <QObject>

class CollectionService;
class ContactDialog;
class ContactService;
class ReloadCoordinator;

class ContactsPaneController : public QObject
{
    Q_OBJECT

public:
    struct Deps
    {
        ContactService &contactService;
        CollectionService &collectionService;
        ReloadCoordinator &reloads;
    };

    explicit ContactsPaneController(ContactsPane *pane, const Deps &deps, QObject *parent = nullptr);

private:
    void newContact();
    void editContact(const Contact &contact);
    void deleteContactWithPrompt(const Contact &contact);
    void mailContact(const Contact &contact);
    QList<QPair<QString, QString>> writableCollectionOptions() const;
    void configureContactDialog(ContactDialog &contactDialog,
                                const QString &title,
                                const QList<QPair<QString, QString>> &collectionOptions) const;
    void reloadContacts();

    ContactsPane *m_pane = nullptr;
    Deps m_deps;
};

#endif // CONTACTSPANECONTROLLER_H

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

#ifndef CONTACTDIALOGFLOW_H
#define CONTACTDIALOGFLOW_H

#include "contacteditordata.h"
#include "contactsnapshot.h"

#include <QList>
#include <QPair>
#include <QString>

#include <optional>

class QWidget;

// "Flow" = stateless dialog runner that returns a DTO.
class ContactDialogFlow
{
public:
    struct EditResult
    {
        Contact contact;
        QString destinationCollectionId;
    };

    explicit ContactDialogFlow(QWidget *parentWidget);

    std::optional<EditResult> create(const QList<QPair<QString, QString>> &collectionOptions) const;
    std::optional<EditResult> edit(const Contact &existing,
                                   const QList<QPair<QString, QString>> &collectionOptions) const;

private:
    std::optional<EditResult> editContactInDialog(const ContactFields &initial,
                                                  const QString &windowTitle,
                                                  const std::optional<Contact> &existing,
                                                  const QList<QPair<QString, QString>> &collectionOptions) const;

    QWidget *m_parent = nullptr;
};

#endif // CONTACTDIALOGFLOW_H

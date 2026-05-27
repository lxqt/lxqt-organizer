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

#include "contactdialogflow.h"

#include "contactdialog.h"
#include "contacteditormapper.h"

#include <QCoreApplication>
#include <QDialog>

namespace {

QString controllerText(const char *sourceText)
{
    return QCoreApplication::translate("ContactsPaneController", sourceText);
}

} // namespace

ContactDialogFlow::ContactDialogFlow(QWidget *parentWidget)
    : m_parent(parentWidget)
{}

std::optional<ContactDialogFlow::EditResult>
ContactDialogFlow::create(const QList<QPair<QString, QString>> &collectionOptions) const
{
    return editContactInDialog(ContactFields{}, controllerText("New Contact"), std::nullopt, collectionOptions);
}

std::optional<ContactDialogFlow::EditResult>
ContactDialogFlow::edit(const Contact &existing, const QList<QPair<QString, QString>> &collectionOptions) const
{
    return editContactInDialog(
        ContactEditorMapper::fromContact(existing), controllerText("Edit Contact"), existing, collectionOptions);
}

std::optional<ContactDialogFlow::EditResult>
ContactDialogFlow::editContactInDialog(const ContactFields &initial,
                                       const QString &windowTitle,
                                       const std::optional<Contact> &existing,
                                       const QList<QPair<QString, QString>> &collectionOptions) const
{
    ContactDialog::State dialogState;
    dialogState.data = initial;
    dialogState.windowTitle = windowTitle;
    dialogState.collectionOptions = collectionOptions;

    ContactDialog contactDialog(m_parent, dialogState);
    contactDialog.setModal(true);
    if (contactDialog.exec() != QDialog::Accepted)
    {
        return std::nullopt;
    }

    const ContactFields fields = contactDialog.data();
    const Contact contact =
        existing ? ContactEditorMapper::applyToContact(*existing, fields) : ContactEditorMapper::createContact(fields);
    const QString destinationCollectionId =
        fields.collectionId.isEmpty() && existing ? existing->ref.collectionId : fields.collectionId;
    return EditResult{contact, destinationCollectionId};
}

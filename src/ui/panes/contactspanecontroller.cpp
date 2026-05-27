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

#include "contactspanecontroller.h"

#include "collectionservice.h"
#include "contactservice.h"
#include "futurewatch.h"
#include "operationcapabilitymessages.h"
#include "reloadcoordinator.h"
#include "storageerrormessages.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>

ContactsPaneController::ContactsPaneController(ContactsPane *pane, const Deps &deps, QObject *parent)
    : QObject(parent)
    , m_pane(pane)
    , m_deps(deps)
    , m_contactFlow(m_pane)
{
    Q_ASSERT(m_pane);
    connect(m_pane, &ContactsPane::newContactRequested, this, &ContactsPaneController::newContact);
    connect(m_pane, &ContactsPane::editContactRequested, this, &ContactsPaneController::editContact);
    connect(m_pane, &ContactsPane::deleteContactRequested, this, &ContactsPaneController::deleteContactWithPrompt);
    connect(m_pane, &ContactsPane::mailContactRequested, this, &ContactsPaneController::mailContact);
}

void ContactsPaneController::newContact()
{
    const QList<QPair<QString, QString>> collectionOptions = writableCollectionOptions();
    if (collectionOptions.isEmpty())
    {
        QMessageBox::warning(m_pane, tr("Organizer"), tr("No writable address book collection is available."));
        return;
    }

    const auto result = m_contactFlow.create(collectionOptions);
    if (!result)
    {
        return;
    }

    const Contact item = result->contact;
    const QString collectionId = result->destinationCollectionId;
    const OperationCapability capability = m_deps.contactService.canCreateContact(collectionId);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("save"), tr("contact")))
    {
        return;
    }
    FutureWatcher::watch(
        this,
        m_deps.contactService.addContactAsync(item, collectionId),
        [this](const ContactService::ContactSaveResult &saveResult) {
            if (StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("contact"), {saveResult.status}))
            {
                reloadContacts();
            }
        });
}

void ContactsPaneController::editContact(const Contact &contact)
{
    Contact currentContact = contact;
    const OperationCapability editCapability = m_deps.contactService.canEditContact(currentContact);
    if (!OperationCapabilityMessages::presentCapability(m_pane, editCapability, tr("edit"), tr("contact")))
    {
        reloadContacts();
        return;
    }

    const QList<QPair<QString, QString>> collectionOptions = writableCollectionOptions();
    const auto result = m_contactFlow.edit(currentContact, collectionOptions);
    if (!result)
    {
        return;
    }

    currentContact = result->contact;
    const QString destinationCollectionId = result->destinationCollectionId;
    const bool moving = destinationCollectionId != currentContact.ref.collectionId;
    const OperationCapability saveCapability =
        moving ? m_deps.contactService.canMoveContact(currentContact, destinationCollectionId)
               : m_deps.contactService.canEditContact(currentContact);
    if (!OperationCapabilityMessages::presentCapability(m_pane, saveCapability, tr("save"), tr("contact")))
    {
        reloadContacts();
        return;
    }

    if (moving)
    {
        FutureWatcher::watch(this,
                             m_deps.contactService.moveContactAsync(currentContact, destinationCollectionId),
                             [this](const ContactService::ContactMoveResult &moveResult) {
                                 StorageErrorMessages::presentOutcome(
                                     m_pane, tr("move"), tr("contact"), {moveResult.move});
                                 reloadContacts();
                             });
        return;
    }

    FutureWatcher::watch(this,
                         m_deps.contactService.updateContactAsync(currentContact),
                         [this](const ContactService::ContactSaveResult &updateResult) {
                             StorageErrorMessages::presentOutcome(
                                 m_pane, tr("save"), tr("contact"), {updateResult.status});
                             reloadContacts();
                         });
}

void ContactsPaneController::deleteContactWithPrompt(const Contact &contact)
{
    const OperationCapability capability = m_deps.contactService.canDeleteContact(contact);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("delete"), tr("contact")))
    {
        reloadContacts();
        return;
    }
    const ContactDisplay display = contactDisplay(contact);

    QString name = QStringLiteral("%1 %2").arg(display.givenName, display.familyName).simplified();
    if (name.isEmpty())
    {
        name = display.preferredEmail;
    }
    if (name.isEmpty())
    {
        name = tr("selected contact");
    }
    const int answer = QMessageBox::warning(m_pane,
                                            tr("Organizer"),
                                            tr("Delete contact \"%1\"?").arg(name),
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No);
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    FutureWatcher::watch(this, m_deps.contactService.deleteContactAsync(contact), [this](StorageStatus deleteStatus) {
        StorageErrorMessages::presentOutcome(m_pane, tr("delete"), tr("contact"), {deleteStatus});
        reloadContacts();
    });
}

void ContactsPaneController::mailContact(const Contact &contact)
{
    if (!contact.ref.isValid())
    {
        QMessageBox::information(m_pane, tr("Organizer"), tr("Select a contact to email."));
        return;
    }

    if (!contact.addressee)
    {
        QMessageBox::information(m_pane, tr("Organizer"), tr("The selected contact does not have an email address."));
        return;
    }
    const QString email = contactDisplay(contact).preferredEmail.trimmed();
    if (email.isEmpty())
    {
        QMessageBox::information(m_pane, tr("Organizer"), tr("The selected contact does not have an email address."));
        return;
    }

    QUrl url;
    url.setScheme(QStringLiteral("mailto"));
    url.setPath(email);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"), tr("Enter the subject"));
    query.addQueryItem(QStringLiteral("body"), tr("Enter message"));
    url.setQuery(query);

    if (!QDesktopServices::openUrl(url))
    {
        QMessageBox::warning(m_pane, tr("Organizer"), tr("Could not open the default mail application."));
    }
}

QList<QPair<QString, QString>> ContactsPaneController::writableCollectionOptions() const
{
    const std::optional<Collection> collection = m_deps.collectionService.defaultAddressBook();
    if (!collection)
    {
        return {};
    }
    return {qMakePair(collection->displayName, collection->id)};
}

void ContactsPaneController::reloadContacts()
{
    m_deps.reloads.reloadContacts();
}

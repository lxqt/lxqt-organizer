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

#ifndef CONTACTSPANE_H
#define CONTACTSPANE_H

#include "iactivepane.h"
#include "contactsnapshot.h"
#include "paneshelldeps.h"
#include "reloadcoordinator.h"
#include "storageresult.h"

#include <QList>
#include <QWidget>

namespace Ui {
class ContactsPane;
}

class QLabel;
class QAction;
class QLineEdit;
class QTableView;
class QToolButton;
class ContactModel;
class PreferencesController;
class ContactsProxyModel;

class ContactsPane : public QWidget, public IActivePane
{
    Q_OBJECT

public:
    struct Deps : PaneShellDeps
    {};

    explicit ContactsPane(const Deps &deps, QWidget *parent = nullptr);
    ~ContactsPane() override;

    QString displayName() const override;
    CollectionKind collectionKind() const override;
    QList<QAction *> contextActions() const override;
    QString selectionStatusText() const override;
    int visibleItemCount() const override;

    QTableView *tableViewContacts() const;
    QWidget *contactFindBar() const;
    QLineEdit *contactFindLineEdit() const;
    QLabel *contactFindStatusLabel() const;
    QToolButton *closeContactFindButton() const;
    QToolButton *contactFindNextButton() const;
    QToolButton *contactFindPrevButton() const;

    void setContacts(const QList<Contact> &contacts);
    QList<Contact> contacts() const;
    Contact selectedContact(bool *hasSelection = nullptr) const;
    void setContactDetailsVisible(bool visible);
    bool contactDetailsVisible() const;
    void showFindBar();
    void hideFindBar();
    bool findContact(bool forward);
    void mailSelectedContact();
    void newContact();
    void editSelectedContact();
    void editContact(const Contact &contact);
    bool deleteSelectedContact();
    bool deleteContactWithPrompt(const Contact &contact);

Q_SIGNALS:
    void actionStateChanged();
    void itemCountChanged();
    void readFailures(const QList<ReadFailure> &failures);
    void newContactRequested();
    void editContactRequested(const Contact &contact);
    void deleteContactRequested(const Contact &contact);
    void mailContactRequested(const Contact &contact);

private:
    void setupActions();
    void setupUiBehavior();
    void handleContactsReloaded(const ReloadCoordinator::ContactReloadPayload &payload);
    void emitSelectionAndCapabilitiesChanged(const QModelIndex &current);
    void updateContactModel();
    void updateActionState();
    bool contactRowMatches(int sourceRow, const QString &needleLower) const;
    void reloadContacts();

    Ui::ContactsPane *ui = nullptr;
    Deps m_deps;
    ContactModel *m_contactModel = nullptr;
    ContactsProxyModel *m_proxyModelContacts = nullptr;
    QAction *m_actionNewContact = nullptr;
    QAction *m_actionDeleteContact = nullptr;
    QAction *m_actionEditContact = nullptr;
    QAction *m_actionMailContact = nullptr;
    QAction *m_actionFind = nullptr;
    QAction *m_actionFindNext = nullptr;
    QAction *m_actionFindPrev = nullptr;
    QList<Contact> m_contacts;
    int m_visibleItemCount = 0;
    bool m_detailsVisible = false;
    bool m_findVisible = false;
};

#endif // CONTACTSPANE_H

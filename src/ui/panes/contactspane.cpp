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

#include "contactspane.h"
#include "ui_contactspane.h"

#include "contactmodel.h"
#include "reloadcoordinator.h"
#include "contactsproxymodel.h"
#include "windowservices.h"

#include <QAbstractItemView>
#include <QAction>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStringList>
#include <QTableView>
#include <QToolButton>

ContactsPane::ContactsPane(const Deps &deps, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ContactsPane)
    , m_deps(deps)
{
    ui->setupUi(this);
    setupActions();
    setupUiBehavior();
    if (m_contactModel)
    {
        m_contactModel->setCollectionSummaryProvider([services = &m_deps.services](const QString &collectionId) {
            return services->summarizeCollection(CollectionKind::AddressBook, collectionId);
        });
    }
    connect(&m_deps.reloads,
            &ReloadCoordinator::contactsReloaded,
            this,
            &ContactsPane::handleContactsReloaded,
            Qt::UniqueConnection);
}

ContactsPane::~ContactsPane()
{
    delete ui;
}

void ContactsPane::setupActions()
{
    m_actionNewContact = new QAction(QIcon::fromTheme(QStringLiteral("contact-new")), tr("New Contact"), this);
    m_actionNewContact->setObjectName(QStringLiteral("actionNew_Contact"));
    m_actionNewContact->setIconText(tr("New"));
    m_actionNewContact->setShortcut(QKeySequence::New);
    m_actionNewContact->setProperty("contextActionIds",
                                    QStringList{QStringLiteral("actionNew"), QStringLiteral("actionNew_Contact")});
    connect(m_actionNewContact, &QAction::triggered, this, &ContactsPane::newContact);

    m_actionEditContact = new QAction(QIcon::fromTheme(QStringLiteral("document-edit")), tr("Edit"), this);
    m_actionEditContact->setObjectName(QStringLiteral("actionEdit"));
    m_actionEditContact->setProperty("contextActionIds", QStringList{QStringLiteral("actionEdit")});
    connect(m_actionEditContact, &QAction::triggered, this, &ContactsPane::editSelectedContact);

    m_actionDeleteContact = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), tr("Delete"), this);
    m_actionDeleteContact->setObjectName(QStringLiteral("actionDelete"));
    m_actionDeleteContact->setShortcut(QKeySequence::Delete);
    m_actionDeleteContact->setProperty("contextActionIds", QStringList{QStringLiteral("actionDelete")});
    connect(m_actionDeleteContact, &QAction::triggered, this, &ContactsPane::deleteSelectedContact);

    m_actionMailContact = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")), tr("Mail To..."), this);
    m_actionMailContact->setObjectName(QStringLiteral("actionMail_To"));
    m_actionMailContact->setProperty("contextActionIds", QStringList{QStringLiteral("actionMail_To")});
    connect(m_actionMailContact, &QAction::triggered, this, &ContactsPane::mailSelectedContact);

    m_actionFind = new QAction(QIcon::fromTheme(QStringLiteral("edit-find")), tr("Find"), this);
    m_actionFind->setObjectName(QStringLiteral("actionFind"));
    m_actionFind->setShortcut(QKeySequence::Find);
    m_actionFind->setProperty("contextActionIds", QStringList{QStringLiteral("actionFind")});
    connect(m_actionFind, &QAction::triggered, this, &ContactsPane::showFindBar);

    m_actionFindNext = new QAction(QIcon::fromTheme(QStringLiteral("go-down")), tr("Find Next"), this);
    m_actionFindNext->setObjectName(QStringLiteral("actionFindNext"));
    m_actionFindNext->setShortcut(QKeySequence::FindNext);
    m_actionFindNext->setProperty("contextActionIds", QStringList{QStringLiteral("actionFindNext")});
    connect(m_actionFindNext, &QAction::triggered, this, [this]() { findContact(true); });

    m_actionFindPrev = new QAction(QIcon::fromTheme(QStringLiteral("go-up")), tr("Find Previous"), this);
    m_actionFindPrev->setObjectName(QStringLiteral("actionFindPrev"));
    m_actionFindPrev->setShortcut(QKeySequence::FindPrevious);
    m_actionFindPrev->setProperty("contextActionIds", QStringList{QStringLiteral("actionFindPrev")});
    connect(m_actionFindPrev, &QAction::triggered, this, [this]() { findContact(false); });

    updateActionState();
}

QString ContactsPane::displayName() const
{
    return tr("Contacts");
}

CollectionKind ContactsPane::collectionKind() const
{
    return CollectionKind::AddressBook;
}

QList<QAction *> ContactsPane::contextActions() const
{
    return {m_actionNewContact,
            m_actionEditContact,
            m_actionDeleteContact,
            m_actionMailContact,
            m_actionFind,
            m_actionFindNext,
            m_actionFindPrev};
}

QString ContactsPane::selectionStatusText() const
{
    bool hasSelection = false;
    const Contact contact = selectedContact(&hasSelection);
    if (hasSelection && contact.addressee && !contactDisplay(contact).isEmpty)
    {
        const QString collectionName =
            m_deps.services.summarizeCollection(CollectionKind::AddressBook, contact.ref.collectionId).displayName;
        return tr("Address Book: %1").arg(collectionName);
    }
    return tr("%n contact(s)", nullptr, m_visibleItemCount);
}

int ContactsPane::visibleItemCount() const
{
    return m_visibleItemCount;
}

QTableView *ContactsPane::tableViewContacts() const
{
    return ui->tableViewContacts;
}

QWidget *ContactsPane::contactFindBar() const
{
    return ui->widgetContactFindBar;
}

QLineEdit *ContactsPane::contactFindLineEdit() const
{
    return ui->lineEditContactFind;
}

QLabel *ContactsPane::contactFindStatusLabel() const
{
    return ui->labelContactFindStatus;
}

QToolButton *ContactsPane::closeContactFindButton() const
{
    return ui->toolButtonCloseContactFind;
}

QToolButton *ContactsPane::contactFindNextButton() const
{
    return ui->toolButtonContactFindNext;
}

QToolButton *ContactsPane::contactFindPrevButton() const
{
    return ui->toolButtonContactFindPrev;
}

void ContactsPane::setContacts(const QList<Contact> &contacts)
{
    m_contacts = contacts;
    updateContactModel();
}

QList<Contact> ContactsPane::contacts() const
{
    return m_contacts;
}

Contact ContactsPane::selectedContact(bool *hasSelection) const
{
    if (hasSelection)
    {
        *hasSelection = false;
    }
    if (!ui->tableViewContacts || !ui->tableViewContacts->selectionModel() || !m_proxyModelContacts || !m_contactModel)
    {
        return {};
    }

    const QModelIndex index = ui->tableViewContacts->currentIndex();
    if (!index.isValid() || !ui->tableViewContacts->selectionModel()->isSelected(index))
    {
        return {};
    }

    const QModelIndex sourceIndex = m_proxyModelContacts->mapToSource(index);
    if (sourceIndex.isValid() && hasSelection)
    {
        *hasSelection = true;
    }
    return sourceIndex.isValid() ? m_contactModel->getContact(sourceIndex.row()) : Contact();
}

void ContactsPane::setContactDetailsVisible(bool visible)
{
    m_detailsVisible = visible;
    ui->tableViewContacts->setColumnHidden(1, !visible);
    ui->tableViewContacts->setColumnHidden(4, !visible);
    ui->tableViewContacts->setColumnHidden(5, !visible);
    ui->tableViewContacts->setColumnHidden(7, !visible);
    ui->tableViewContacts->setColumnHidden(8, !visible);
    ui->tableViewContacts->setColumnHidden(9, !visible);
    ui->tableViewContacts->setColumnHidden(11, !visible);
}

bool ContactsPane::contactDetailsVisible() const
{
    return m_detailsVisible;
}

void ContactsPane::showFindBar()
{
    m_findVisible = true;
    ui->widgetContactFindBar->show();
    ui->lineEditContactFind->setFocus(Qt::ShortcutFocusReason);
    ui->lineEditContactFind->selectAll();
}

void ContactsPane::hideFindBar()
{
    m_findVisible = false;
    ui->labelContactFindStatus->clear();
    ui->widgetContactFindBar->hide();
}

bool ContactsPane::findContact(bool forward)
{
    if (!m_findVisible)
    {
        showFindBar();
    }
    const QString needle = ui->lineEditContactFind->text();
    if (needle.isEmpty())
    {
        ui->lineEditContactFind->setFocus(Qt::ShortcutFocusReason);
        return false;
    }

    if (!m_proxyModelContacts || m_proxyModelContacts->rowCount() == 0)
    {
        ui->labelContactFindStatus->setText(tr("No more items"));
        return false;
    }
    const QString needleLower = needle.toLower();
    const int rows = m_proxyModelContacts->rowCount();
    const QModelIndex currentProxy = ui->tableViewContacts->currentIndex();
    int startProxy =
        currentProxy.isValid() ? (forward ? currentProxy.row() + 1 : currentProxy.row() - 1) : (forward ? 0 : rows - 1);

    const int step = forward ? 1 : -1;
    for (int r = startProxy; r >= 0 && r < rows; r += step)
    {
        const QModelIndex proxyIdx = m_proxyModelContacts->index(r, 0);
        const QModelIndex srcIdx = m_proxyModelContacts->mapToSource(proxyIdx);
        if (contactRowMatches(srcIdx.row(), needleLower))
        {
            ui->tableViewContacts->setCurrentIndex(proxyIdx);
            ui->tableViewContacts->scrollTo(proxyIdx, QAbstractItemView::PositionAtCenter);
            ui->labelContactFindStatus->clear();
            return true;
        }
    }
    ui->labelContactFindStatus->setText(tr("No more items"));
    return false;
}

void ContactsPane::setupUiBehavior()
{
    m_contactModel = new ContactModel(this);
    m_contactModel->setCollectionSummaryProvider([services = &m_deps.services](const QString &collectionId) {
        return services->summarizeCollection(CollectionKind::AddressBook, collectionId);
    });
    m_proxyModelContacts = new ContactsProxyModel(this);
    m_proxyModelContacts->setSourceModel(m_contactModel);

    ui->tableViewContacts->horizontalHeader()->setVisible(true);
    ui->tableViewContacts->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewContacts->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableViewContacts->horizontalHeader()->setStretchLastSection(false);
    ui->tableViewContacts->horizontalHeader()->setHighlightSections(false);
    ui->tableViewContacts->horizontalHeader()->setSectionsClickable(true);
    ui->tableViewContacts->horizontalHeader()->setSortIndicatorShown(true);
    ui->tableViewContacts->verticalHeader()->setVisible(false);
    ui->tableViewContacts->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 12);
    ui->tableViewContacts->setAlternatingRowColors(true);
    ui->tableViewContacts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewContacts->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewContacts->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewContacts->setShowGrid(false);
    ui->tableViewContacts->setFrameShape(QFrame::NoFrame);
    ui->tableViewContacts->setSortingEnabled(true);
    ui->tableViewContacts->setModel(m_proxyModelContacts);
    ui->tableViewContacts->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->widgetContactFindBar->hide();
    ui->toolButtonCloseContactFind->setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
    ui->lineEditContactFind->setPlaceholderText(tr("Find"));
    ui->lineEditContactFind->addAction(QIcon::fromTheme(QStringLiteral("edit-find")), QLineEdit::LeadingPosition);
    ui->toolButtonContactFindPrev->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    ui->toolButtonContactFindNext->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
    connect(ui->toolButtonCloseContactFind, &QToolButton::clicked, this, &ContactsPane::hideFindBar);
    connect(ui->toolButtonContactFindNext, &QToolButton::clicked, this, [this]() { findContact(true); });
    connect(ui->toolButtonContactFindPrev, &QToolButton::clicked, this, [this]() { findContact(false); });
    connect(ui->lineEditContactFind, &QLineEdit::returnPressed, this, [this]() { findContact(true); });
    connect(ui->lineEditContactFind, &QLineEdit::textChanged, ui->labelContactFindStatus, &QLabel::clear);
    connect(ui->tableViewContacts, &QTableView::doubleClicked, this, [this](const QModelIndex &) {
        editSelectedContact();
    });
    connect(ui->tableViewContacts, &QTableView::clicked, this, [this](const QModelIndex &) { updateActionState(); });
    connect(ui->tableViewContacts, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex index = ui->tableViewContacts->indexAt(pos);
        if (index.isValid())
        {
            ui->tableViewContacts->setCurrentIndex(index);
        }

        QMenu menu(this);
        QAction *newAction = menu.addAction(QIcon::fromTheme(QStringLiteral("contact-new")), tr("New Contact"));
        menu.addSeparator();
        QAction *editAction = menu.addAction(tr("Edit Contact"));
        QAction *mailAction = menu.addAction(QIcon::fromTheme(QStringLiteral("mail-message-new")), tr("Mail To..."));
        menu.addSeparator();
        QAction *deleteAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), tr("Delete Contact"));
        editAction->setEnabled(index.isValid());
        mailAction->setEnabled(index.isValid());
        deleteAction->setEnabled(index.isValid());

        QAction *selectedAction = menu.exec(ui->tableViewContacts->viewport()->mapToGlobal(pos));
        if (selectedAction == newAction)
        {
            newContact();
        }
        else if (selectedAction == editAction && index.isValid())
        {
            editSelectedContact();
        }
        else if (selectedAction == mailAction && index.isValid())
        {
            mailSelectedContact();
        }
        else if (selectedAction == deleteAction && index.isValid())
        {
            deleteSelectedContact();
        }
    });
}

void ContactsPane::updateContactModel()
{
    if (!m_contactModel || !m_proxyModelContacts)
    {
        return;
    }

    QList<Contact> filtered;
    filtered.reserve(m_contacts.size());
    for (const Contact &contact : m_contacts)
    {
        if (!contact.ref.collectionId.isEmpty())
        {
            filtered.append(contact);
        }
    }

    bool hadSelection = false;
    const Contact previouslySelected = selectedContact(&hadSelection);

    m_contactModel->setContacts(filtered);
    m_visibleItemCount = m_contactModel->rowCount();
    ui->tableViewContacts->sortByColumn(2, Qt::AscendingOrder);
    ui->tableViewContacts->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableViewContacts->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->tableViewContacts->horizontalHeader()->setSectionResizeMode(11, QHeaderView::ResizeToContents);
    ui->tableViewContacts->setColumnWidth(0, 140);
    ui->tableViewContacts->setColumnWidth(2, 140);
    ui->tableViewContacts->setColumnWidth(6, 130);
    ui->tableViewContacts->setColumnWidth(10, 130);
    setContactDetailsVisible(m_detailsVisible);
    if (QItemSelectionModel *selectionModel = ui->tableViewContacts->selectionModel())
    {
        connect(selectionModel,
                &QItemSelectionModel::currentChanged,
                this,
                &ContactsPane::emitSelectionAndCapabilitiesChanged,
                Qt::UniqueConnection);
    }

    if (hadSelection && previouslySelected.ref.isValid())
    {
        const int rowCount = m_contactModel->rowCount();
        for (int row = 0; row < rowCount; ++row)
        {
            const Contact candidate = m_contactModel->getContact(row);
            if (candidate.ref.collectionId == previouslySelected.ref.collectionId &&
                candidate.ref.href == previouslySelected.ref.href && candidate.uid == previouslySelected.uid)
            {
                const QModelIndex sourceIndex = m_contactModel->index(row, 0);
                const QModelIndex proxyIndex = m_proxyModelContacts->mapFromSource(sourceIndex);
                ui->tableViewContacts->setCurrentIndex(proxyIndex);
                break;
            }
        }
    }

    Q_EMIT itemCountChanged();
    updateActionState();
}

void ContactsPane::updateActionState()
{
    const bool hasSelection = selectedContact().ref.isValid();
    if (m_actionEditContact)
    {
        m_actionEditContact->setEnabled(hasSelection);
    }
    if (m_actionDeleteContact)
    {
        m_actionDeleteContact->setEnabled(hasSelection);
    }
    if (m_actionMailContact)
    {
        m_actionMailContact->setEnabled(hasSelection);
    }
    Q_EMIT actionStateChanged();
}

bool ContactsPane::contactRowMatches(int sourceRow, const QString &needleLower) const
{
    if (sourceRow < 0 || !m_contactModel || sourceRow >= m_contactModel->rowCount())
    {
        return false;
    }
    const int cols = m_contactModel->columnCount();
    for (int c = 0; c < cols; ++c)
    {
        const QVariant value = m_contactModel->data(m_contactModel->index(sourceRow, c), Qt::DisplayRole);
        if (value.toString().toLower().contains(needleLower))
        {
            return true;
        }
    }
    return false;
}

void ContactsPane::mailSelectedContact()
{
    bool hasSelection = false;
    const Contact contact = selectedContact(&hasSelection);
    Q_EMIT mailContactRequested(hasSelection ? contact : Contact());
}

void ContactsPane::newContact()
{
    Q_EMIT newContactRequested();
}

void ContactsPane::editSelectedContact()
{
    bool hasSelection = false;
    const Contact contact = selectedContact(&hasSelection);
    Q_EMIT editContactRequested(hasSelection ? contact : Contact());
}

void ContactsPane::editContact(const Contact &contact)
{
    Q_EMIT editContactRequested(contact);
}

bool ContactsPane::deleteSelectedContact()
{
    bool hasSelection = false;
    const Contact contact = selectedContact(&hasSelection);
    Q_EMIT deleteContactRequested(hasSelection ? contact : Contact());
    return hasSelection;
}

bool ContactsPane::deleteContactWithPrompt(const Contact &contact)
{
    Q_EMIT deleteContactRequested(contact);
    return contact.ref.isValid();
}

void ContactsPane::handleContactsReloaded(const ReloadCoordinator::ContactReloadPayload &payload)
{
    setContacts(payload.contactList);
    if (!payload.readFailures.isEmpty())
    {
        Q_EMIT readFailures(payload.readFailures);
    }
}

void ContactsPane::emitSelectionAndCapabilitiesChanged(const QModelIndex &current)
{
    Q_UNUSED(current)
    updateActionState();
}

void ContactsPane::reloadContacts()
{
    m_deps.reloads.reloadContacts();
}

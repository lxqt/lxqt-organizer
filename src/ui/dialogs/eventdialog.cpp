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

#include "eventdialog.h"
#include "ui_eventdialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimeEdit>
#include <QToolButton>
#include <QUrl>

EventDialog::EventDialog(QWidget *parent, const State &state)
    : QDialog(parent)
    , m_mode(state.mode)
    , m_allowCollectionChange(state.allowCollectionChange)
    , m_allowTimeZoneEdit(state.allowTimeZoneEdit)
    , ui(std::make_unique<Ui::EventDialog>())
{
    setupUi(state);
    setCollectionOptions(state.collectionOptions, state.currentCollectionIndex);
    setData(state.data);
}

EventDialog::~EventDialog() = default;

EventFields EventDialog::data() const
{
    EventFields value;
    value.title = ui->lineEditTitle->text().trimmed();
    value.location = ui->lineEditLocation->text().trimmed();
    value.description = ui->textEditNotes->toPlainText();
    value.url = ui->lineEditUrl->text().trimmed();
    value.attachmentUri = ui->lineEditAttachment->text().trimmed();
    value.timeZoneId = ui->comboTimeZone->timeZoneId();
    value.displayTimeZoneId = m_displayTimeZoneId;
    value.collectionId = ui->comboCollection->currentData().toString();
    value.date = ui->dateEditDate->date();
    value.startTime = ui->timeEditStart->time();
    value.endTime = ui->timeEditEnd->time();
    value.allDay = ui->checkBoxAllDay->isChecked();
    value.priority = ui->comboPriority->priority();
    value.completed = ui->checkBoxDone->isChecked();
    value.highlightMode = ui->comboHighlight->mode();
    value.locator = m_locator;
    return value;
}

void EventDialog::setCollectionOptions(const QList<QPair<QString, QString>> &collections, int currentIndex)
{
    const QSignalBlocker blocker(ui->comboCollection);
    ui->comboCollection->clear();
    for (const QPair<QString, QString> &collection : collections)
    {
        ui->comboCollection->addItem(collection.first, collection.second);
    }
    if (currentIndex >= 0 && currentIndex < ui->comboCollection->count())
    {
        ui->comboCollection->setCurrentIndex(currentIndex);
    }
    ui->comboCollection->setEnabled(m_allowCollectionChange && ui->comboCollection->count() > 1);
}

void EventDialog::accept()
{
    const EventFields value = data();
    const QString error = value.validationError();
    if (!error.isEmpty())
    {
        QMessageBox::information(this, tr("Empty Details"), error);
        return;
    }
    QDialog::accept();
}

void EventDialog::setupUi(const State &state)
{
    ui->setupUi(this);
    setWindowTitle(m_mode == Mode::Edit ? tr("Edit Event") : tr("Event"));
    ui->dateEditDate->setLocale(state.locale);

    connect(ui->checkBoxAllDay, &QCheckBox::toggled, this, &EventDialog::updateAllDayState);
    connect(ui->toolButtonAttachment, &QToolButton::clicked, this, &EventDialog::browseAttachment);
}

void EventDialog::setData(const EventFields &data)
{
    m_displayTimeZoneId = data.displayTimeZoneId;
    m_locator = data.locator;
    ui->lineEditTitle->setText(data.title);
    ui->lineEditLocation->setText(data.location);
    ui->textEditNotes->setText(data.description);
    ui->lineEditUrl->setText(data.url);
    ui->lineEditAttachment->setText(data.attachmentUri);
    ui->comboHighlight->setMode(data.highlightMode);
    ui->comboPriority->setPriority(data.priority);
    ui->checkBoxDone->setChecked(data.completed);
    ui->dateEditDate->setDate(data.date.isValid() ? data.date : QDate::currentDate());
    ui->timeEditStart->setTime(data.startTime.isValid() ? data.startTime : QTime::currentTime());
    ui->timeEditEnd->setTime(data.endTime.isValid() ? data.endTime : ui->timeEditStart->time());
    ui->checkBoxAllDay->setChecked(data.allDay);
    ui->comboTimeZone->setTimeZoneId(data.timeZoneId);
    ui->comboTimeZone->setEnabled(m_allowTimeZoneEdit);
    ui->labelTimeZone->setEnabled(m_allowTimeZoneEdit);
    ui->labelScope->setVisible(m_mode == Mode::Edit);
    ui->labelScopeValue->setVisible(m_mode == Mode::Edit);
    ui->labelScopeValue->setText(data.locator.has_value() ? tr("This occurrence") : tr("All occurrences"));

    const int collectionIndex = ui->comboCollection->findData(data.collectionId);
    if (collectionIndex >= 0)
    {
        ui->comboCollection->setCurrentIndex(collectionIndex);
    }

    updateAllDayState();
}

void EventDialog::updateAllDayState()
{
    const bool timed = !ui->checkBoxAllDay->isChecked();
    ui->timeEditStart->setEnabled(timed);
    ui->timeEditEnd->setEnabled(timed);
    ui->labelStartTime->setEnabled(timed);
    ui->labelEndTime->setEnabled(timed);
}

void EventDialog::browseAttachment()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select Attachment"));
    if (!fileName.isEmpty())
    {
        ui->lineEditAttachment->setText(QUrl::fromLocalFile(fileName).toString());
    }
}

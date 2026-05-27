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

#include "taskdialog.h"
#include "ui_taskdialog.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextEdit>
#include <QTimeEdit>

TaskDialog::TaskDialog(QWidget *parent, const State &state)
    : QDialog(parent)
    , ui(std::make_unique<Ui::TaskDialog>())
{
    setupUi(state);
    setData(state.data);
    ui->lineEditTitle->setFocus();
}

void TaskDialog::setupUi(const State &state)
{
    ui->setupUi(this);
    setWindowTitle(state.windowTitle);

    for (const QPair<QString, QString> &option : state.collectionOptions)
    {
        ui->comboCollection->addItem(option.first, option.second);
    }
    if (state.currentCollectionIndex >= 0 && state.currentCollectionIndex < ui->comboCollection->count())
    {
        ui->comboCollection->setCurrentIndex(state.currentCollectionIndex);
    }
    ui->comboCollection->setEnabled(ui->comboCollection->count() > 1);

    ui->dateEditDue->setLocale(state.locale);
    connect(ui->checkBoxDue, &QCheckBox::toggled, this, &TaskDialog::setDueControlsEnabled);
}

TaskDialog::~TaskDialog() = default;

TaskFields TaskDialog::data() const
{
    TaskFields value;
    value.title = ui->lineEditTitle->text().trimmed();
    value.description = ui->textEditNotes->toPlainText().trimmed();
    value.collectionId = ui->comboCollection->currentData().toString();
    value.hasDue = ui->checkBoxDue->isChecked();
    value.due = value.hasDue ? QDateTime(ui->dateEditDue->date(), ui->timeEditDue->time()) : QDateTime();
    value.priority = ui->comboPriority->priority();
    value.completed = ui->checkBoxCompleted->isChecked();
    value.rollForward = ui->checkBoxRollForward->isChecked();
    value.highlightMode = ui->comboHighlight->mode();
    return value;
}

void TaskDialog::accept()
{
    const QString error = data().validationError();
    if (!error.isEmpty())
    {
        QMessageBox::information(this, tr("Empty Details"), error);
        return;
    }
    QDialog::accept();
}

void TaskDialog::setData(const TaskFields &data)
{
    ui->lineEditTitle->setText(data.title);
    ui->textEditNotes->setPlainText(data.description);
    ui->comboHighlight->setMode(data.highlightMode);

    const int collectionIndex = ui->comboCollection->findData(data.collectionId);
    if (collectionIndex >= 0)
    {
        ui->comboCollection->setCurrentIndex(collectionIndex);
    }

    ui->dateEditDue->setDate(data.due.date().isValid() ? data.due.date() : QDate::currentDate());
    ui->timeEditDue->setTime(data.due.time().isValid() ? data.due.time() : QTime(9, 0));
    ui->checkBoxDue->setChecked(data.hasDue);
    setDueControlsEnabled(data.hasDue);

    ui->comboPriority->setPriority(data.priority);
    ui->checkBoxCompleted->setChecked(data.completed);
    ui->checkBoxRollForward->setChecked(data.rollForward);
}

void TaskDialog::setDueControlsEnabled(bool enabled)
{
    ui->labelDueDate->setEnabled(enabled);
    ui->labelDueTime->setEnabled(enabled);
    ui->dateEditDue->setEnabled(enabled);
    ui->timeEditDue->setEnabled(enabled);
}

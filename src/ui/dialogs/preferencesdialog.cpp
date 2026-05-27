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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QTime>
#include <QTimeEdit>

PreferencesDialog::PreferencesDialog(QWidget *parent, const Preferences *thePreferences)
    : QDialog(parent)
    , ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
    setLabelDescriptions();

    const Preferences preferences = thePreferences ? *thePreferences : Preferences::defaults();

    ui->spinBoxApplicationFont->setRange(8, 38);
    ui->spinBoxApplicationFont->setValue(preferences.applicationFontSize);
    ui->timeEditDayViewStart->setTime(QTime(qBound(0, preferences.dayViewTimeStart, 23), 0));
    ui->timeEditDayViewFinish->setTime(QTime(qBound(1, preferences.dayViewTimeFinish, 24) % 24, 0));
    connect(ui->timeEditDayViewStart, &QTimeEdit::timeChanged, this, [this](const QTime &time) {
        const int startHour = time.hour();
        if (startHour >= dayViewTimeFinish())
        {
            const int finishHour = startHour + 1;
            ui->timeEditDayViewFinish->setTime(finishHour == 24 ? QTime(0, 0) : QTime(finishHour, 0));
        }
    });
    connect(ui->timeEditDayViewFinish, &QTimeEdit::timeChanged, this, [this](const QTime &time) {
        const int finishHour = time == QTime(0, 0) ? 24 : time.hour();
        if (finishHour <= dayViewTimeStart())
        {
            ui->timeEditDayViewStart->setTime(QTime(qMax(0, finishHour - 1), 0));
        }
    });
}


PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}


Preferences PreferencesDialog::preferences() const
{
    Preferences value = Preferences::defaults();
    value.applicationFontSize = ui->spinBoxApplicationFont->value();
    value.dayViewTimeStart = dayViewTimeStart();
    value.dayViewTimeFinish = dayViewTimeFinish();
    return value;
}

int PreferencesDialog::dayViewTimeStart() const
{
    return ui->timeEditDayViewStart->time().hour();
}

int PreferencesDialog::dayViewTimeFinish() const
{
    const QTime finish = ui->timeEditDayViewFinish->time();
    return finish == QTime(0, 0) ? 24 : finish.hour();
}

void PreferencesDialog::setLabelDescriptions()
{
    setWindowTitle(tr("Preferences"));
    ui->groupBoxDisplay->setTitle(tr("Display"));
    ui->labelFontSize->setText(tr("&Calendar font size:"));
    ui->labelDayViewStart->setText(tr("Day view &starts:"));
    ui->labelDayViewFinish->setText(tr("Day view &ends:"));
}

void PreferencesDialog::on_spinBoxApplicationFont_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
}

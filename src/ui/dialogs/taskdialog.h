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

#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include "calendareditordata.h"

#include <QDialog>
#include <QList>
#include <QLocale>
#include <QPair>
#include <QString>

#include <memory>

namespace Ui {
class TaskDialog;
}

class TaskDialog : public QDialog
{
    Q_OBJECT

public:
    struct State
    {
        TaskFields data;
        QString windowTitle;
        QList<QPair<QString, QString>> collectionOptions;
        int currentCollectionIndex = 0;
        QLocale locale = QLocale::system();
    };

    explicit TaskDialog(QWidget *parent, const State &state);
    ~TaskDialog() override;

    TaskFields data() const;

protected:
    void accept() override;

private:
    void setupUi(const State &state);
    void setData(const TaskFields &data);
    void setDueControlsEnabled(bool enabled);

    std::unique_ptr<Ui::TaskDialog> ui;
};

#endif // TASKDIALOG_H

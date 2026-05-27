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


#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include "calendareditordata.h"

#include <QDialog>
#include <QList>
#include <QLocale>
#include <QPair>

#include <memory>

namespace Ui {
class EventDialog;
}

class EventDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode
    {
        Create,
        Edit
    };

    struct State
    {
        EventFields data;
        QList<QPair<QString, QString>> collectionOptions;
        int currentCollectionIndex = 0;
        QLocale locale = QLocale::system();
        Mode mode = Mode::Create;
        bool allowCollectionChange = true;
        bool allowTimeZoneEdit = true;
    };

    explicit EventDialog(QWidget *parent, const State &state);
    ~EventDialog() override;

    EventFields data() const;

protected:
    void accept() override;

private:
    void setupUi(const State &state);
    void setCollectionOptions(const QList<QPair<QString, QString>> &collections, int currentIndex);
    void setData(const EventFields &data);
    void updateAllDayState();
    void browseAttachment();

    Mode m_mode = Mode::Create;
    bool m_allowCollectionChange = true;
    bool m_allowTimeZoneEdit = true;
    QString m_displayTimeZoneId;
    std::optional<IncidenceLocator> m_locator;
    std::unique_ptr<Ui::EventDialog> ui;
};

#endif // EVENTDIALOG_H

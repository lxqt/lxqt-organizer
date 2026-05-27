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

#ifndef FINDBAR_H
#define FINDBAR_H

#include <QWidget>

namespace Ui {
class FindBar;
}

class FindBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindBar(QWidget *parent = nullptr);
    ~FindBar() override;

    bool isFindActive() const;
    void showFind();
    void hideFind();
    bool requestFind(bool forward);
    void clearStatus();
    void showNoMatch();

Q_SIGNALS:
    void closeRequested();
    void findRequested(const QString &needle, bool forward);

private:
    Ui::FindBar *ui = nullptr;
    bool m_findActive = false;
};

#endif // FINDBAR_H

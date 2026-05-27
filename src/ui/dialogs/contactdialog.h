/*
 * LXQt Organizer - personal information manager for LXQt.
 * Author: Alan Crispin aka. crispina <crispinalan@gmail.com>
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

#ifndef CONTACTDIALOG_H
#define CONTACTDIALOG_H

#include "contacteditordata.h"

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

#include <memory>

namespace Ui {
class ContactDialog;
}

class ContactDialog : public QDialog
{
    Q_OBJECT

public:
    struct State
    {
        ContactFields data;
        QString windowTitle;
        QList<QPair<QString, QString>> collectionOptions;
        int currentCollectionIndex = 0;
    };

    explicit ContactDialog(QWidget *parent, const State &state);
    ~ContactDialog() override;

    ContactFields data() const;

protected:
    void accept() override;

private:
    void setupUi(const State &state);
    void setData(const ContactFields &data);

    std::unique_ptr<Ui::ContactDialog> ui;
};

#endif // CONTACTDIALOG_H

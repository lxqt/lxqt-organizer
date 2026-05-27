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
#include <QDebug>
#include <QList>
#include <QMessageBox>
#include <QPair>

namespace Ui {
class ContactDialog;
}

class ContactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ContactDialog(QWidget *parent = nullptr);
    ~ContactDialog();

    ContactFields data() const;
    void setData(const ContactFields &data);
    void setCollectionOptions(const QList<QPair<QString, QString>> &collections, int currentIndex);


private Q_SLOTS:
    void accept();

private:
    Ui::ContactDialog *ui;
};

#endif // CONTACTDIALOG_H

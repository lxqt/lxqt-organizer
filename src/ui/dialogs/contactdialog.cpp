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

#include "contactdialog.h"
#include "ui_contactdialog.h"

#include "contactutils.h"

#include <QMessageBox>

ContactDialog::ContactDialog(QWidget *parent, const State &state)
    : QDialog(parent)
    , ui(std::make_unique<Ui::ContactDialog>())
{
    setupUi(state);
    setData(state.data);
}

ContactDialog::~ContactDialog() = default;

void ContactDialog::setupUi(const State &state)
{
    ui->setupUi(this);
    setWindowTitle(state.windowTitle);
    ui->labelFirstName->setText(tr("First Name"));
    ui->labelMidNames->setText(tr("Middle Names"));
    ui->labelLastName->setText(tr("Last Name"));
    ui->labelEmail->setText(tr("Email"));
    ui->labelCollection->setText(tr("Collection"));
    ui->labelStreet->setText(tr("Street"));
    ui->labelDistrict->setText(tr("District"));
    ui->labelCity->setText(tr("City"));
    ui->labelCounty->setText(tr("County"));
    ui->labelPostcode->setText(tr("Postcode"));
    ui->labelCountry->setText(tr("Country"));
    ui->labelPhone->setText(tr("Phone"));
    ui->lineEditCollection->hide();

    ui->comboBoxCollection->clear();
    for (const QPair<QString, QString> &option : state.collectionOptions)
    {
        ui->comboBoxCollection->addItem(option.first, option.second);
    }
    if (state.currentCollectionIndex >= 0 && state.currentCollectionIndex < ui->comboBoxCollection->count())
    {
        ui->comboBoxCollection->setCurrentIndex(state.currentCollectionIndex);
    }
    ui->comboBoxCollection->setEnabled(ui->comboBoxCollection->count() > 1);
    ui->labelCollection->setVisible(!state.collectionOptions.isEmpty());
    ui->labelCollection->setBuddy(ui->comboBoxCollection);
    ui->comboBoxCollection->setVisible(!state.collectionOptions.isEmpty());
}

ContactFields ContactDialog::data() const
{
    ContactFields value;
    value.firstName = ui->lineEditFirstName->text().trimmed();
    value.middleNames = ui->lineEditMiddleNames->text().trimmed();
    value.lastName = ui->lineEditLastName->text().trimmed();
    value.email = ui->lineEditEmail->text().trimmed();
    value.street = ui->lineEditStreet->text().trimmed();
    value.district = ui->lineEditDistrict->text().trimmed();
    value.city = ui->lineEditCity->text().trimmed();
    value.county = ui->lineEditCounty->text().trimmed();
    value.postcode = ui->lineEditPostcode->text().trimmed();
    value.country = ui->lineEditCountry->text().trimmed();
    value.phoneNumber = ui->lineEditTelephone->text().trimmed();
    value.collectionId = ui->comboBoxCollection->currentData().toString();
    return value;
}

void ContactDialog::setData(const ContactFields &data)
{
    ui->lineEditFirstName->setText(data.firstName);
    ui->lineEditMiddleNames->setText(data.middleNames);
    ui->lineEditLastName->setText(data.lastName);
    ui->lineEditEmail->setText(data.email);
    ui->lineEditStreet->setText(data.street);
    ui->lineEditDistrict->setText(data.district);
    ui->lineEditCity->setText(data.city);
    ui->lineEditCounty->setText(data.county);
    ui->lineEditPostcode->setText(data.postcode);
    ui->lineEditCountry->setText(data.country);
    ui->lineEditTelephone->setText(data.phoneNumber);
    const int collectionIndex = ui->comboBoxCollection->findData(data.collectionId);
    if (collectionIndex >= 0)
    {
        ui->comboBoxCollection->setCurrentIndex(collectionIndex);
    }
}

void ContactDialog::accept()
{
    const QString error = data().validationError();
    if (!error.isEmpty())
    {
        QMessageBox::information(this, tr("Empty Details"), error);
        return;
    }
    QDialog::accept();
}

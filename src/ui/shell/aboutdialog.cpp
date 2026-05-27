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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QDialogButtonBox>
#include <QIcon>
#include <QString>

AboutDialog::AboutDialog(const QString &version, QWidget *parent)
    : QDialog(parent)
    , ui(std::make_unique<Ui::AboutDialog>())
{
    ui->setupUi(this);
    setupWindowChrome(version);
    setupAboutTab();
    setupAuthorsTab();
    setupLicenseTab();

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AboutDialog::~AboutDialog() = default;

void AboutDialog::setupWindowChrome(const QString &version)
{
    setWindowIcon(QIcon::fromTheme(QStringLiteral("help-about")));
    setWindowModality(Qt::WindowModal);

    const QIcon appIcon = QIcon::fromTheme(QStringLiteral("lxqt-organizer"));
    ui->iconLabel->setPixmap(appIcon.pixmap(64, 64));
    ui->titleLabel->setText(ui->titleLabel->text().arg(version));
}

void AboutDialog::setupAboutTab()
{
    ui->aboutBrowser->setHtml(QStringLiteral("<center>%1<br/><br/>%2</center>")
                                  .arg(tr("LXQt Organizer is a Qt lightweight personal information manager developed "
                                          "by Basil Crow"),
                                       tr("Built with Qt and LXQt libraries.")));
}

void AboutDialog::setupAuthorsTab()
{
    ui->authorsBrowser->setHtml(QStringLiteral("<b>%1</b><br/>"
                                               "Basil Crow<br/><br/>"
                                               "<b>%2</b><br/>"
                                               "<a href=\"https://lxqt-project.org\">LXQt Project</a>")
                                    .arg(tr("Programming:"), tr("Maintained by:")));
}

void AboutDialog::setupLicenseTab()
{
    ui->licenseBrowser->setHtml(QStringLiteral("<p><i>LXQt Organizer</i> %1</p><p>%2</p>")
                                    .arg(tr("is free software; you can redistribute it and/or modify it under the "
                                            "terms of the GNU General Public License, version 2 or later."),
                                         tr("It is distributed in the hope that it will be useful, but without any "
                                            "warranty.")));
}

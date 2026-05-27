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

#ifndef CONTACTVALIDATION_H
#define CONTACTVALIDATION_H

#include <QString>

#include <KContacts/Addressee>

namespace ContactValidation {

bool isSupportedAddressee(const KContacts::Addressee &addressee);
QString contactUid(const KContacts::Addressee &addressee);

} // namespace ContactValidation

#endif // CONTACTVALIDATION_H

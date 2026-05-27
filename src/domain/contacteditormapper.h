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

#ifndef CONTACTEDITORMAPPER_H
#define CONTACTEDITORMAPPER_H

#include "contacteditordata.h"
#include "contactsnapshot.h"

namespace KContacts {
class Addressee;
}

namespace ContactEditorMapper {

ContactFields fromAddressee(const KContacts::Addressee &addressee);
ContactFields fromContact(const Contact &contact);
void applyToAddressee(KContacts::Addressee &addressee, const ContactFields &data);
Contact createContact(const ContactFields &data);
Contact applyToContact(Contact contact, const ContactFields &data);

} // namespace ContactEditorMapper

#endif // CONTACTEDITORMAPPER_H

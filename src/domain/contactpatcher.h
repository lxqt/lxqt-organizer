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

#ifndef CONTACTPATCHER_H
#define CONTACTPATCHER_H

#include "contacteditordata.h"

namespace KContacts {
class Addressee;
}

namespace ContactPatcher {

#define CONTACT_PATCHER_OPTION_FIELDS(FIELD)                                                                           \
    FIELD(name, false)                                                                                                 \
    FIELD(email, false)                                                                                                \
    FIELD(street, false)                                                                                               \
    FIELD(district, false)                                                                                             \
    FIELD(city, false)                                                                                                 \
    FIELD(county, false)                                                                                               \
    FIELD(postcode, false)                                                                                             \
    FIELD(country, false)                                                                                              \
    FIELD(phoneNumber, false)

struct Options
{
#define CONTACT_PATCHER_DECLARE_OPTION(name, defaultValue) bool name = defaultValue;
    CONTACT_PATCHER_OPTION_FIELDS(CONTACT_PATCHER_DECLARE_OPTION)
#undef CONTACT_PATCHER_DECLARE_OPTION
};

template <typename Visitor> void forEachOption(Options &options, Visitor visitor)
{
#define CONTACT_PATCHER_VISIT_OPTION(name, defaultValue) visitor(options.name);
    CONTACT_PATCHER_OPTION_FIELDS(CONTACT_PATCHER_VISIT_OPTION)
#undef CONTACT_PATCHER_VISIT_OPTION
}

template <typename Visitor> void forEachOption(const Options &options, Visitor visitor)
{
#define CONTACT_PATCHER_VISIT_OPTION(name, defaultValue) visitor(options.name);
    CONTACT_PATCHER_OPTION_FIELDS(CONTACT_PATCHER_VISIT_OPTION)
#undef CONTACT_PATCHER_VISIT_OPTION
}

#undef CONTACT_PATCHER_OPTION_FIELDS

Options editableOptions();
Options changedOptionsForAddressee(const KContacts::Addressee &addressee, const ContactFields &patch);
bool hasChanges(const Options &options);
void applyToAddressee(KContacts::Addressee &addressee, const ContactFields &patch, Options options = {});

} // namespace ContactPatcher

#endif // CONTACTPATCHER_H

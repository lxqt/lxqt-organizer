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

#include "vcardcodec.h"

#include <QByteArray>

#include <KContacts/VCardConverter>

namespace VCardCodec {

std::optional<KContacts::Addressee> parse(const QString &text)
{
    if (text.trimmed().isEmpty())
    {
        return std::nullopt;
    }

    KContacts::VCardConverter converter;
    const KContacts::Addressee::List addressees = converter.parseVCards(text.toUtf8());
    if (addressees.size() != 1)
    {
        return std::nullopt;
    }
    return addressees.constFirst();
}

QString serialize(const KContacts::Addressee &addressee)
{
    KContacts::VCardConverter converter;
    const QByteArray data = converter.createVCard(addressee, KContacts::VCardConverter::v4_0);
    return QString::fromUtf8(data);
}

} // namespace VCardCodec

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

#include "contactrepository.h"

#include "contactvalidation.h"
#include "itemidentity.h"
#include "vcardcodec.h"
#include "vdiritemrepository.h"

namespace {

StorageStatus
serializeContact(const KContacts::Addressee &addressee, QString *text, KContacts::Addressee *parsedAddressee)
{
    if (addressee.isEmpty())
    {
        return StorageStatus::Unsupported;
    }

    const QString serialized = VCardCodec::serialize(addressee);
    if (serialized.isEmpty())
    {
        return StorageStatus::ParseError;
    }

    const std::optional<KContacts::Addressee> parsed = VCardCodec::parse(serialized);
    if (!parsed)
    {
        return StorageStatus::ParseError;
    }
    if (!ContactValidation::isSupportedAddressee(*parsed))
    {
        return StorageStatus::Unsupported;
    }

    if (text)
    {
        *text = serialized;
    }
    if (parsedAddressee)
    {
        *parsedAddressee = *parsed;
    }
    return StorageStatus::Ok;
}

} // namespace

ContactRepository::ContactRepository(const StorageCollectionRef &collection, const VdirIo &vdirIo)
    : m_items(collection, vdirIo)
{}

StorageCollectionRef ContactRepository::collection() const
{
    return m_items.collection();
}

void ContactRepository::updateCollection(const StorageCollectionRef &collection)
{
    m_items.updateCollection(collection);
}

bool ContactRepository::isValid() const
{
    return m_items.isValid();
}

StorageStatus
ContactRepository::checkCurrentItem(const QString &href, const QString &expectedEtag, ItemRef *storage) const
{
    return m_items.checkCurrentItem(href, expectedEtag, storage);
}

ContactRepository::ReadResult ContactRepository::readCurrentObject(const QString &href,
                                                                   const QString &expectedEtag) const
{
    ReadResult result;
    result.object.ref = ItemRef{m_items.collection().id, href, expectedEtag};

    const ::ReadResult<VdirItemRepository::ItemPayload> current = m_items.readCurrentItem(href, expectedEtag);
    result.object.ref = current.object.ref;
    if (!current.isOk())
    {
        result.status = current.status;
        return result;
    }

    const std::optional<KContacts::Addressee> parsedAddressee = VCardCodec::parse(current.object.content);
    if (!parsedAddressee)
    {
        result.status = StorageStatus::ParseError;
        return result;
    }
    if (!ContactValidation::isSupportedAddressee(*parsedAddressee))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const QString uid = ContactValidation::contactUid(*parsedAddressee);
    result.object = Contact{result.object.ref, uid, addresseeSnapshot(*parsedAddressee)};
    result.status = StorageStatus::Ok;
    return result;
}

void ContactRepository::forEachObject(const std::function<bool(const ReadResult &)> &visitor,
                                      const std::function<bool()> &stopRequested) const
{
    if (!visitor)
    {
        return;
    }
    if (!isValid())
    {
        ReadResult readResult;
        readResult.status = StorageStatus::IoError;
        visitor(readResult);
        return;
    }

    // Hold the path worker's FIFO slot for the full traversal so a concurrent
    // write to the same vdir cannot interleave between items and tear the snapshot.
    m_items.runOnWorker(QStringLiteral("contact forEachObject"), [&] {
        for (const VdirItemRepository::ItemState &state : m_items.itemStates())
        {
            if (stopRequested && stopRequested())
            {
                break;
            }
            if (!visitor(readCurrentObject(state.href, state.etag)))
            {
                break;
            }
        }
    });
}

QList<ContactRepository::ReadResult> ContactRepository::readObjects() const
{
    QList<ReadResult> result;
    // Scheduler shutdown skips the traversal, so empty can also mean teardown.
    forEachObject([&result](const ReadResult &readResult) {
        result.append(readResult);
        return true;
    });
    return result;
}

QString ContactRepository::hrefForUid(const QString &uid) const
{
    return m_items.hrefForUid(uid);
}

StorageStatus ContactRepository::create(const QString &href, const QString &text, QString *etag) const
{
    return m_items.create(href, text, etag);
}

StorageStatus ContactRepository::replace(const QString &href,
                                         const QString &text,
                                         const QString &expectedEtag,
                                         QString *newEtag) const
{
    return m_items.replace(href, text, expectedEtag, newEtag);
}

StorageStatus ContactRepository::remove(const ItemRef &storage) const
{
    return m_items.remove(storage);
}

StorageStatus ContactRepository::createContact(const QString &href,
                                               const KContacts::Addressee &addressee,
                                               QString *etag,
                                               KContacts::Addressee *parsedAddressee) const
{
    QString text;
    const StorageStatus serializeStatus = serializeContact(addressee, &text, parsedAddressee);
    if (serializeStatus != StorageStatus::Ok)
    {
        return serializeStatus;
    }
    return create(href, text, etag);
}

StorageResult<Contact> ContactRepository::addObject(Contact object) const
{
    StorageResult<Contact> result;
    result.snapshot = object;

    if (!isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }
    if (!object.addressee || object.addressee->isEmpty())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const QString originalUid = object.uid.isEmpty() ? object.addressee->uid() : object.uid;
    const bool organizerAuthoredUid = originalUid.isEmpty() && object.ref.href.isEmpty();
    const int attempts = organizerAuthoredUid ? 2 : 1;

    QString href;
    QString etag;
    KContacts::Addressee parsedAddressee;
    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        KContacts::Addressee addressee = *object.addressee;
        const QString uid = organizerAuthoredUid ? randomItemUid() : originalUid;
        if (uid.isEmpty())
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }
        if (addressee.uid().isEmpty())
        {
            addressee.setUid(uid);
        }

        href = object.ref.href.isEmpty() ? hrefForUid(uid) : object.ref.href;
        const StorageStatus createStatus = createContact(href, addressee, &etag, &parsedAddressee);
        if (createStatus == StorageStatus::Ok)
        {
            break;
        }
        if (createStatus != StorageStatus::Conflict)
        {
            result.status = createStatus;
            return result;
        }
        href.clear();
        etag.clear();
    }
    if (etag.isEmpty())
    {
        result.status = StorageStatus::Conflict;
        return result;
    }

    result.status = StorageStatus::Ok;
    result.snapshot.ref = ItemRef{m_items.collection().id, href, etag};
    result.snapshot.uid = ContactValidation::contactUid(parsedAddressee);
    result.snapshot.addressee = addresseeSnapshot(parsedAddressee);
    return result;
}

StorageStatus ContactRepository::replaceContact(const QString &href,
                                                const KContacts::Addressee &addressee,
                                                const QString &expectedEtag,
                                                QString *newEtag,
                                                KContacts::Addressee *parsedAddressee) const
{
    QString text;
    const StorageStatus serializeStatus = serializeContact(addressee, &text, parsedAddressee);
    if (serializeStatus != StorageStatus::Ok)
    {
        return serializeStatus;
    }
    return replace(href, text, expectedEtag, newEtag);
}

StorageResult<Contact> ContactRepository::replaceObject(Contact object) const
{
    StorageResult<Contact> result;
    result.snapshot = object;

    if (!isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }
    if (!object.addressee || object.addressee->isEmpty())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    QString uid = object.uid.isEmpty() ? object.addressee->uid() : object.uid;
    if (uid.isEmpty())
    {
        uid = randomItemUid();
    }
    if (object.addressee->uid().isEmpty())
    {
        KContacts::Addressee addressee = *object.addressee;
        addressee.setUid(uid);
        object.addressee = addresseeSnapshot(addressee);
    }

    KContacts::Addressee parsedAddressee;
    QString newEtag;
    const StorageStatus updateStatus =
        replaceContact(object.ref.href, *object.addressee, object.ref.etag, &newEtag, &parsedAddressee);
    if (updateStatus != StorageStatus::Ok)
    {
        result.status = updateStatus;
        return result;
    }

    result.status = StorageStatus::Ok;
    result.snapshot.ref = ItemRef{m_items.collection().id, object.ref.href, newEtag};
    result.snapshot.uid = ContactValidation::contactUid(parsedAddressee);
    result.snapshot.addressee = addresseeSnapshot(parsedAddressee);
    return result;
}

# Architecture and Layer Boundaries

LXQt Organizer is organized as four layers with one application-owned context at
the top:

```
UI / window shell / panes / dialogs / models
        |
        | snapshots, display snapshots, editor DTOs, service calls
        v
Services / readers / collection catalog
        |
        | repositories, storage results, domain mutators/mappers
        v
Storage:
  storage/repo/  (repositories + item stores)
        |
        v
  storage/fs/   (Vdir + VdirIoScheduler)
        |
        | (storage/api/: INTERFACE library; vocabulary only)
        v
Domain / snapshots / DTOs / mappers / pure helpers
        |
        v
Qt + KF6 preservation types
```

Dependencies flow downward only:

- UI may depend on services, storage result types, and domain DTOs/snapshots.
- Services may depend on `storage/fs/`, `storage/repo/`, and domain.
- `storage/repo/` may depend on `storage/fs/`, `storage/api/`, and domain.
  Storage must not include `services/` or UI headers. Cross-layer lookup is
  expressed through `CollectionResolver` (declared in `storage/api/`).
- `storage/fs/` may depend on `storage/api/` and domain only. It must not
  include from `storage/repo/`.
- `storage/api/` is an `INTERFACE` library that carries the vocabulary above
  the storage line: `StorageStatus`, `StorageResult<T>`, `StorageMoveResult<T>`,
  `MoveOutcome`, the `CollectionResolver` IoC interface, and the storage
  logging category. It may include from domain only; it must not include from
  `storage/fs/` or `storage/repo/`.
- Domain may depend on Qt and KF6 preservation types only. Domain must not
  include `storage/`, `services/`, or UI headers.
- `ItemRef` and `ItemKey` live in `domain/itemidentity.h`. Domain value types
  (`CalendarItemSnapshot`, `TaskSnapshot`, `ContactSnapshot`, `OccurrenceRef`)
  embed `ItemRef` by value, so it cannot move above domain without creating a
  `domain -> storage/api -> domain` cycle. `ItemRef::key()` returns the matching
  `ItemKey` for cache lookups.

These boundaries are firm and mechanized by `tools/checks.py`, which also
checks service logging categories and snapshot aliasing. If a lower layer needs
information from a higher layer, add a small lower-layer interface or value type
and have the higher layer implement or populate it. Do not include upward
headers for convenience.

## Public API Shape

Domain exposes value projections and editor transformations:

- Storage projections: `CalendarItemSnapshot`, `TaskSnapshot`, `ContactSnapshot`.
- Identity types (in `domain/itemidentity.h`): `ItemRef`, `ItemKey`.
- Display vocabulary (in `domain/collectionsummary.h`): `CollectionSummary`.
- Display projections: `EventDisplaySnapshot`, `TaskDisplaySnapshot`,
  `ContactDisplaySnapshot`.
- Editor DTOs: `EventFields`, `TaskFields`, `ContactEditorData`.
- Preservation helpers: `EventPatcher`, `TodoPatcher`,
  `CalendarItemMutator`, `CalendarEditorMapper`, `ContactEditorMapper`,
  `EventOccurrences`, `EventDateTime`, `EventTimeZone`,
  `CalendarValidation`, and `ContactValidation`.

Storage exposes filesystem and repository APIs:

- `Vdir` is the filesystem primitive.
- `VdirIoScheduler` serializes all disk-backed vdir work by canonical path.
- `VdirItemRepository` is the `ItemRef` contract above `Vdir`.
- `CalendarItemRepository` and `ContactRepository` encode/decode
  `CalendarItemSnapshot` and `ContactSnapshot`.
- `ItemStore` and `CalendarItemStore` cache repositories and enforce collection
  resolution through `CollectionResolver`.
- Storage outcomes are typed: `StorageStatus`, `StorageResult<T>`,
  `StorageMoveResult<T>`, and `MoveOutcome`.

Services expose user operations and read projections:

- `EventService`, `TaskService`, and `ContactService` validate
  collection capabilities, orchestrate writes/moves/deletes, and return typed
  storage outcomes with committed etags.
- `EventReader` is read-only and owns occurrence projection/cache logic.
- `CollectionService` implements `CollectionResolver` and is the read/write
  facade over `CollectionCatalog`.
- `CollectionCatalog` exposes the fixed local personal calendar and contact
  vdirs under the application data directory.

UI consumes snapshots and DTOs. App-specific panes and window shell code live
under `src/ui/panes` and `src/ui/shell`; reusable widgets, dialogs, models, and
formatters stay in their narrower `src/ui/*` packages:

- Models consume display/storage snapshots only. They must not open
  repositories or vdirs.
- Panes and dialogs call services/readers and map user input through editor
  DTOs. UI may translate errors, but services and storage do not produce UI
  strings.
- `StorageErrorMessages` lives under `ui/messages` and formats typed storage
  outcomes for presentation.
- `ReloadCoordinator` lives under `ui/shell` and adapts service/read futures
  into UI reload payloads, read-failure signals, and today-rollover events.
- `MainWindow` receives stable service references from
  `ApplicationContext`; service ownership stays in the application
  context.

## Domain Rules

Treat KF6 object models as the preservation boundary for calendar and contact
data. The application does not promise to preserve properties KF6 cannot parse
and serialize, but it must not corrupt or discard fields KF6 did preserve just
because an editor dialog was opened.

UI save paths apply field-level patches to cloned `KCalendarCore`/`KContacts`
objects. Do not replace a parsed incidence or addressee from a reduced editor
DTO unless the operation is creating a new item. Unchanged editor fields must
not call lossy setters such as attachment, recurrence, alarm, preferred address,
preferred phone, or custom-property writers. No-op saves should avoid rewriting
the vdir item.

Snapshots are immutable at the API boundary. `CalendarSnapshot::*Ptr` members
are `QSharedPointer<const T>` handles to const KF6 objects. `ContactSnapshot`
stores a const addressee payload and has no mutation setter. Callers that need
mutation must clone first and send the edited value through a service.

`CalendarItemSnapshot` uses `std::variant<std::monostate, CalendarPtr, EventPtr,
TodoPtr>` so a stored item has at most one calendar payload shape. UI-facing
tables, reports, and panes must use `ContactDisplaySnapshot` or
`ContactEditorData` instead of inspecting preferred address/phone objects.

Keep calendar utility responsibilities separated. Event patching lives in
`EventPatcher`, todo patching lives in `TodoPatcher`, structural calendar
mutation lives in `CalendarItemMutator`, date/time helpers live in
`eventdatetime.cpp`, and custom incidence fields/attachment helpers live in
`incidencefields.cpp`.

## Storage Rules

Keep vdir filesystem rules below the repository line. `Vdir` owns filesystem
validation, atomic writes, content-hash etags, and metadata IO.
`VdirItemRepository` is the `ItemRef` contract above it.

Keep serialization at the codec boundary. Only `ICalCodec` and `VCardCodec`
should call KF6 serialization APIs directly; item repositories should call the
domain codecs.

Serialize vdir disk IO by canonical filesystem path. `VdirIoScheduler` is owned
by `ApplicationContext`, keyed by canonical vdir path, and shared by all
windows, event/task/contact services, metadata writes, and disk-backed
reads. The scheduler is generic: it accepts labelled callables by path and
provides ordering/threading only. Storage semantics stay in services and
repositories.

`VdirItemRepository` submits synchronous vdir operations to the scheduler. Async
service entry points submit the whole storage operation directly to the
scheduler using the touched vdir path, or `submitComposite()` when an operation
may touch two vdirs, so composite orchestration does not run on a vdir worker and
leaf repository calls serialize independently by path without deadlocking
opposite-direction moves.

The scheduler exposes `QFuture`/`QPromise`-backed asynchronous submission plus a
blocking helper for synchronous repository methods. It uses one Qt event-loop
worker thread per vdir path initially, with one FIFO queue for reads and writes.
Do not let reads bypass queued writes for the same vdir. `runSync()` may run
inline only when already on the same path worker; cross-vdir synchronous calls
from another vdir worker are invalid and asserted against.

Keep vdir etags content-hash based. `Vdir` may cache hashes by href, size, and
mtime to avoid repeated digest work, but local creates, updates, and deletes must
clear stale item-state data before returning. External writers are detected by
etag checks and guarded update/remove operations. Linux item updates use
`renameat2(RENAME_EXCHANGE)` for true compare-and-swap, verified after rename
via an inode guard. On non-Linux platforms the fallback is `rename(2)` with a
best-effort post-rename etag check; concurrent external writers can race on
these platforms. LXQt Organizer targets Linux first-class; non-Linux works
but loses the compare-and-swap guarantee.

Logging for storage and service storage failures uses the single
`lxqt.organizer.storage` category declared in `storage/api/storagelog.h`. Scheduler
submission labels are diagnostic data and should be preserved in logs.

## Diagnostics

To trace storage operations, run:

    QT_LOGGING_RULES="lxqt.organizer.storage.debug=true" lxqt-organizer

Or persist in `~/.config/QtProject/qtlogging.ini`:

    [Rules]
    lxqt.organizer.storage.debug=true

The category covers vdir IO, repository writes, and service-level storage
outcomes including `MoveRollbackFailed` and `MoveSourceRemoveFailed`.

## Service Rules

Services own user operations. `EventService`, `TaskService`, and
`ContactService` validate collection capabilities, perform writes, and translate
repository failures into `StorageStatus` results.

Successful add, update, save, and move results return snapshots with the
committed `ItemRef`. In particular, `snapshot.ref.etag` is the new etag
from storage, not the caller's input precondition etag.

Readers own read-side projection. `EventReader` expands occurrences and
returns snapshots for UI use. UI code should use snapshots and editor data
instead of inspecting `KCalendarCore` objects.

Cross-collection moves are best-effort. The destination item is created first;
if source removal fails, the destination copy is removed. A failed cleanup is
reported as `MoveRollbackFailed` and must be logged.

Cross-vdir moves are orchestrated by services as destination create, source
remove, and optional destination rollback. Because repositories serialize each
file operation at the vdir boundary, same-vdir moves remain single update jobs,
not create-then-delete operations.

Move and save results are discriminated outcomes. Same-collection edits return
an update outcome; cross-collection moves return a move outcome with destination
creation, source removal, and rollback state. Do not synthesize create/remove
statuses for an operation that only updated an existing item.

Metadata writers must use the shared `VdirIoScheduler`. Do not construct a
private scheduler in a service.

## Ownership and Cache Rules

`ApplicationContext` owns the process-wide `VdirIoScheduler`,
`CollectionService`, readers, and write services. `MainWindow` receives those
services from the context. Do not add by-value service ownership back to
`MainWindow`; service lifetime belongs to the application context so references
stay stable across windows and catalog reloads.

Service and storage caches are thread-safe and long-lived per service/repository
instance. `VdirItemRepository` owns a `Vdir` so each cached repository keeps its
vdir etag cache. Normal application access to a `Vdir` is serialized by the
path's scheduler worker; standalone callers without a scheduler must still avoid
concurrent operations on the same instance. `Vdir`'s `m_etagCacheMutex` is a
leaf guard for mutable const-method cache and targeted invalidation, not operation
locks. `CalendarItemStore`, `ContactItemStore`, and
`EventReader` guard their mutable caches with mutexes. Catalog reloads
must go through `ApplicationContext::reloadCollections()` or
`loadCollections()`. Those methods install the new catalog snapshot, compute
the changed collection ids by comparing the old and new catalog, and then call
`CollectionService::notifyReloaded()`. Services, readers, and UI coordinators that
cache collection-derived data must subscribe to
`CollectionService::collectionsReloaded` in their constructor and invalidate
only the entries covered by the changed ids. Because calendar and address-book
collections can share an id, subscribers may receive an id for another
collection kind; targeted cache clear methods must tolerate that and treat empty
sets as a no-op.

Cache mutexes are leaf locks. `Vdir::m_etagCacheMutex`,
`ItemStore::m_repositoriesMutex`, and
`EventReader::m_occurrenceCacheMutex` must not be held while taking another
cache mutex or while calling a user-supplied callback. Const reads and writes are
reentrant; repositories are cached as `std::shared_ptr` so a worker may keep a
repository alive across a catalog reload intentionally.

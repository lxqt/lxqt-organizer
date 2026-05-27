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

#include "statusbarpresenter.h"

#include "collectiontypes.h"
#include "storageerrormessages.h"
#include "iactivepane.h"

#include <QDate>
#include <QFontMetrics>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QStatusBar>

namespace {

bool sameReadFailure(const ReadFailure &first, const ReadFailure &second)
{
    return first.ref.collectionId == second.ref.collectionId && first.ref.href == second.ref.href &&
           first.status == second.status;
}

QList<ReadFailure> uniqueFailures(const QList<ReadFailure> &readFailures)
{
    QList<ReadFailure> failures;
    for (const ReadFailure &failure : readFailures)
    {
        if (!failure.isFailure())
        {
            continue;
        }
        bool found = false;
        for (const ReadFailure &existing : std::as_const(failures))
        {
            if (sameReadFailure(existing, failure))
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            failures.append(failure);
        }
    }
    return failures;
}

class StatusLabel : public QLabel
{
public:
    explicit StatusLabel(QWidget *parent = nullptr)
        : QLabel(parent)
    {
        setTextFormat(Qt::PlainText);
        setWordWrap(false);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setMinimumWidth(0);
    }

    QSize sizeHint() const override
    {
        const QFontMetrics metrics(font());
        return {metrics.horizontalAdvance(QStringLiteral("0000")), metrics.height()};
    }

    QSize minimumSizeHint() const override
    {
        return {0, sizeHint().height()};
    }

    bool hasHeightForWidth() const override
    {
        return false;
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        const QFontMetrics metrics(font());
        painter.setPen(palette().color(foregroundRole()));
        painter.drawText(contentsRect(),
                         alignment() | Qt::TextSingleLine,
                         metrics.elidedText(text(), Qt::ElideRight, contentsRect().width()));
    }
};

} // namespace

StatusBarPresenter::StatusBarPresenter(QStatusBar *statusBar, QLabel *itemCountLabel, QObject *parent)
    : QObject(parent)
    , m_statusBar(statusBar)
    , m_itemCountLabel(new StatusLabel(statusBar))
    , m_selectedDateLabel(new QLabel(statusBar))
{
    if (itemCountLabel)
    {
        itemCountLabel->hide();
        itemCountLabel->deleteLater();
    }
    if (m_statusBar)
    {
        m_selectedDateLabel->setTextFormat(Qt::PlainText);
        m_selectedDateLabel->setWordWrap(false);
        m_statusBar->addWidget(m_itemCountLabel, 1);
        m_statusBar->addPermanentWidget(m_selectedDateLabel);
    }
}

void StatusBarPresenter::setLocaleProvider(std::function<QLocale()> localeProvider)
{
    m_localeProvider = std::move(localeProvider);
}

void StatusBarPresenter::setActivePane(IActivePane *pane)
{
    if (m_activePaneObject)
    {
        disconnect(m_activePaneObject, nullptr, this, nullptr);
    }
    m_activePane = pane;
    m_activePaneObject = dynamic_cast<QObject *>(pane);
    if (m_activePaneObject)
    {
        connect(m_activePaneObject, SIGNAL(actionStateChanged()), this, SLOT(refresh()));
        connect(m_activePaneObject, SIGNAL(itemCountChanged()), this, SLOT(refresh()));
        connect(m_activePaneObject,
                SIGNAL(readFailures(QList<ReadFailure>)),
                this,
                SLOT(showReadFailures(QList<ReadFailure>)));
    }
    refresh();
}

void StatusBarPresenter::setSelectedDate(const QDate &date, const QLocale &locale)
{
    if (m_selectedDateLabel)
    {
        m_selectedDateLabel->setText(locale.toString(date, QLocale::LongFormat));
    }
}

void StatusBarPresenter::setSelectedDate(const QDate &date)
{
    setSelectedDate(date, m_localeProvider ? m_localeProvider() : QLocale::system());
}

void StatusBarPresenter::showReadFailures(const QList<ReadFailure> &readFailures)
{
    const QList<ReadFailure> failures = uniqueFailures(readFailures);
    if (failures.isEmpty() || !m_statusBar)
    {
        return;
    }
    const QString detail = StorageErrorMessages::storageFailureDetail(failures.first().status);
    const QString message =
        detail.isEmpty() ? tr("%n item(s) could not be loaded.", nullptr, failures.size())
                         : tr("%n item(s) could not be loaded. First error: %1", nullptr, failures.size()).arg(detail);
    m_statusBar->showMessage(message, 10000);
}

void StatusBarPresenter::refresh()
{
    if (!m_itemCountLabel)
    {
        return;
    }
    m_itemCountLabel->setText(m_activePane ? m_activePane->selectionStatusText() : QString());
}

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

#ifndef OPERATIONCAPABILITYMESSAGES_H
#define OPERATIONCAPABILITYMESSAGES_H

#include "operationcapability.h"

#include <QString>

class QWidget;

namespace OperationCapabilityMessages {

QString capabilityFailureDetail(OperationCapabilityStatus status);
QString operationCapabilityMessage(const OperationCapability &capability,
                                   const QString &operationName,
                                   const QString &itemName);
bool presentCapability(QWidget *parent,
                       const OperationCapability &capability,
                       const QString &operationName,
                       const QString &itemName);

} // namespace OperationCapabilityMessages

#endif // OPERATIONCAPABILITYMESSAGES_H

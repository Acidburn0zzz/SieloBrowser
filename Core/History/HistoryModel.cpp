﻿/***********************************************************************************
** MIT License                                                                    **
**                                                                                **
** Copyright (c) 2018 Victor DENIS (victordenis01@gmail.com)                      **
**                                                                                **
** Permission is hereby granted, free of charge, to any person obtaining a copy   **
** of this software and associated documentation files (the "Software"), to deal  **
** in the Software without restriction, including without limitation the rights   **
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      **
** copies of the Software, and to permit persons to whom the Software is          **
** furnished to do so, subject to the following conditions:                       **
**                                                                                **
** The above copyright notice and this permission notice shall be included in all **
** copies or substantial portions of the Software.                                **
**                                                                                **
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     **
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       **
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    **
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         **
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  **
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  **
** SOFTWARE.                                                                      **
***********************************************************************************/

#include "HistoryModel.hpp"

#include <ndb/query.hpp>

#include "Database/SqlDatabase.hpp"

#include "History/HistoryItem.hpp"

#include "Application.hpp"

constexpr auto& history = ndb::models::navigation.history;

namespace Sn
{
static QString dateTimeToString(const QDateTime& dateTime)
{
	const QDateTime current = QDateTime::currentDateTime();
	if (current.date() == dateTime.date()) {
		return dateTime.time().toString("hh:mm");
	}

	return dateTime.toString("dd/MM/yyyy hh:mm");
}

HistoryModel::HistoryModel(History* history) :
	QAbstractItemModel(history),
	m_rootItem(new HistoryItem(nullptr)),
	m_history(history)
{
	init();

	connect(m_history, &History::resetHistory, this, &HistoryModel::resetHistory);
	connect(m_history, &History::historyEntryAdded, this, &HistoryModel::historyEntryAdded);
	connect(m_history, &History::historyEntryEdited, this, &HistoryModel::historyEntryEdited);
	connect(m_history, &History::historyEntryDeleted, this, &HistoryModel::historyEntryDeleted);
}

QVariant HistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return tr("Title");
		case 1:
			return tr("Address");
		case 2:
			return tr("Visit Date");
		case 3:
			return tr("Visit Count");
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const
{
	HistoryItem* item{itemFromIndex(index)};

	if (index.row() < 0 || !item) {
		return QVariant();
	}

	if (item->isTopLevel()) {
		switch (role) {
		case IsTopLevelRole:
			return true;
		case TimestampStartRole:
			return item->startTimestamp();
		case TimestampEndRole:
			return item->endTimestamp();
		case Qt::DisplayRole:
		case Qt::EditRole:
			return index.column() == 0 ? item->title : QVariant();
		case Qt::DecorationRole:
			return index.column() == 0 ? Application::getAppIcon("calendar") : QVariant();
		}

		return QVariant();
	}

	const History::HistoryEntry entry{item->historyEntry};

	switch (role) {
	case IdRole:
		return entry.id;
	case TitleRole:
		return entry.title;
	case UrlRole:
		return entry.url;
	case UrlStringRole:
		return entry.urlString;
	case IconRole:
		return item->icon();
	case IsTopLevelRole:
		return false;
	case TimestampStartRole:
		return -1;
	case TimestampEndRole:
		return -1;
	case Qt::ToolTipRole:
		if (index.column() == 0) {
			return QString("%1\n%2").arg(entry.title, entry.urlString);
		}
		// fallthrough
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case 0:
			return entry.title;
		case 1:
			return entry.urlString;
		case 2:
			return dateTimeToString(entry.date);
		case 3:
			return entry.count;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == 0) {
			return item->icon().isNull() ? Application::getAppIcon("webpage") : item->icon();
		}
	}

	return QVariant();
}

bool HistoryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	HistoryItem* item{itemFromIndex(index)};

	if (index.row() < 0 || !item || item->isTopLevel())
		return false;

	if (role == IconRole) {
		item->setIcon(value.value<QIcon>());
		emit dataChanged(index, index);

		return true;
	}

	return false;
}

QModelIndex HistoryModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	HistoryItem* parentItem{itemFromIndex(parent)};
	HistoryItem* childItem{parentItem->child(row)};

	return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex HistoryModel::parent(const QModelIndex& child) const
{
	if (!child.isValid())
		return QModelIndex();

	HistoryItem* childItem{itemFromIndex(child)};
	HistoryItem* parentItem{childItem->parent()};

	if (!parentItem || parentItem == m_rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

Qt::ItemFlags HistoryModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return 0;

	return static_cast<Qt::ItemFlags>(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

int HistoryModel::rowCount(const QModelIndex& parent) const
{
	if (parent.column() > 0)
		return 0;

	HistoryItem* parentItem{itemFromIndex(parent)};
	return parentItem->childCount();
}

int HistoryModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)

	return 4;
}

bool HistoryModel::canFetchMore(const QModelIndex& parent) const
{
	HistoryItem* parentItem{itemFromIndex(parent)};

	return parentItem ? parentItem->canFetchMore : false;
}

void HistoryModel::fetchMore(const QModelIndex& parent)
{
	HistoryItem* parentItem{itemFromIndex(parent)};

	if (!parent.isValid() || !parentItem)
		return;

	parentItem->canFetchMore = false;

	QList<int> idList{};
	QVector<History::HistoryEntry> list{};

	for (int i{0}; i < parentItem->childCount(); ++i)
		idList.append(parentItem->child(i)->historyEntry.id);

	auto oquery = ndb::oquery<dbs::navigation>() <<
	(
		ndb::range(
			history.date,
			parentItem->endTimestamp(),
			parentItem->startTimestamp()
		)
	);

	for (auto& data : oquery) {
		History::HistoryEntry entry{data};

		if (!idList.contains(entry.id))
			list.append(entry);
	}


	if (list.isEmpty())
		return;

	beginInsertRows(parent, 0, list.size() - 1);

	foreach(const History::HistoryEntry& entry, list) {
		HistoryItem* newItem{new HistoryItem(parentItem)};
		newItem->historyEntry = entry;
	}

	endInsertRows();
}

bool HistoryModel::hasChildren(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return true;

	HistoryItem* item{itemFromIndex(parent)};
	return item ? item->isTopLevel() : false;
}

HistoryItem *HistoryModel::itemFromIndex(const QModelIndex& index) const
{
	if (index.isValid()) {
		HistoryItem* item = static_cast<HistoryItem*>(index.internalPointer());

		if (item)
			return item;
	}

	return m_rootItem;
}

void HistoryModel::removeTopLevelIndexes(const QList<QPersistentModelIndex>& indexes)
{
	foreach(const QPersistentModelIndex& index, indexes) {
		if (index.parent().isValid())
			continue;

		int row = index.row();
		HistoryItem* item{m_rootItem->child(row)};

		if (!item)
			return;

		beginRemoveRows(QModelIndex(), row, row);
		delete item;
		endRemoveRows();

		if (item == m_todayItem)
			m_todayItem = nullptr;
	}
}

void HistoryModel::resetHistory()
{
	beginResetModel();

	delete m_rootItem;
	m_todayItem = nullptr;

	m_rootItem = new HistoryItem();
	init();

	endResetModel();
}

void HistoryModel::historyEntryAdded(const History::HistoryEntry& entry)
{
	if (!m_todayItem) {
		beginInsertRows(QModelIndex(), 0, 0);

		m_todayItem = new HistoryItem();
		m_todayItem->setStartTimestamp(-1);
		m_todayItem->setEndTimestamp(QDateTime(QDate::currentDate()).toMSecsSinceEpoch());
		m_todayItem->title = tr("Title");

		m_rootItem->prependChild(m_todayItem);

		endInsertRows();
	}

	beginInsertRows(createIndex(0, 0, m_todayItem), 0, 0);

	HistoryItem* item{new HistoryItem()};
	item->historyEntry = entry;

	m_todayItem->prependChild(item);

	endInsertRows();
}

void HistoryModel::historyEntryDeleted(const History::HistoryEntry& entry)
{
	HistoryItem* item{findHistoryItem(entry)};

	if (!item)
		return;

	HistoryItem* parentItem{item->parent()};
	int row{item->row()};

	beginRemoveRows(createIndex(parentItem->row(), 0, parentItem), row, row);

	delete item;

	endRemoveRows();

	checkEmptyParentItem(parentItem);
}

void HistoryModel::historyEntryEdited(const History::HistoryEntry& before, const History::HistoryEntry& after)
{
	historyEntryDeleted(before);
	historyEntryAdded(after);
}

HistoryItem *HistoryModel::findHistoryItem(const History::HistoryEntry& entry)
{
	HistoryItem* parentItem{nullptr};
	qint64 timestamp = entry.date.toMSecsSinceEpoch();

	for (int i{0}; i < m_rootItem->childCount(); ++i) {
		HistoryItem* item{m_rootItem->child(i)};

		if (item->endTimestamp() < timestamp) {
			parentItem = item;
			break;
		}
	}

	if (!parentItem)
		return nullptr;

	for (int i{0}; i < parentItem->childCount(); ++i) {
		HistoryItem* item{parentItem->child(i)};

		if (item->historyEntry.id == entry.id)
			return item;
	}

	return nullptr;
}

void HistoryModel::checkEmptyParentItem(HistoryItem* item)
{
	if (item->childCount() == 0 && item->isTopLevel()) {
		int row{item->row()};

		beginRemoveRows(QModelIndex(), row, row);

		delete item;

		endRemoveRows();

		if (item == m_todayItem)
			m_todayItem = nullptr;
	}
}

void HistoryModel::init()
{
	auto& minDateQuery = ndb::query<dbs::navigation>() << (ndb::min(history.date));

	if (!minDateQuery.has_result())
		return;

	const qint64 minTimestamp = minDateQuery[0][0].get<qint64>();

	if (minTimestamp <= 0)
		return;

	const QDate today{QDate::currentDate()};
	const QDate week{today.addDays(1 - today.dayOfWeek())};
	const QDate month{QDate(today.year(), today.month(), 1)};
	const qint64 currentTimestamp{QDateTime::currentMSecsSinceEpoch()};

	qint64 timestamp{currentTimestamp};

	while (timestamp > minTimestamp) {
		QDate timestampDate{QDateTime::fromMSecsSinceEpoch(timestamp).date()};
		qint64 endTimestamp{};
		QString itemName{};

		if (timestampDate == today) {
			endTimestamp = QDateTime(today).toMSecsSinceEpoch();
			itemName = tr("Today");
		}
		else if (timestampDate >= week) {
			endTimestamp = QDateTime(week).toMSecsSinceEpoch();
			itemName = tr("This Week");
		}
		else if (timestampDate.month() == month.month() && timestampDate.year() == month.year()) {
			endTimestamp = QDateTime(month).toMSecsSinceEpoch();
			itemName = tr("This Month");
		}
		else {
			QDate startDate{timestampDate.year(), timestampDate.month(), timestampDate.daysInMonth()};
			QDate endDate{startDate.year(), startDate.month(), 1};

			timestamp = QDateTime(startDate, QTime(23, 59, 59)).toMSecsSinceEpoch();
			endTimestamp = QDateTime(endDate).toMSecsSinceEpoch();
			itemName = QString("%1 %2").arg(History::titleCaseLocalizedMonth(timestampDate.month()),
			                                timestampDate.year());
		}

		auto& query = ndb::query<dbs::navigation>() << (ndb::range(history.date, endTimestamp, timestamp));

		if (query.has_result()) {
			HistoryItem* item{new HistoryItem(m_rootItem)};
			item->setStartTimestamp(timestamp == currentTimestamp ? -1 : timestamp);
			item->setEndTimestamp(endTimestamp);
			item->title = itemName;
			item->canFetchMore = true;

			if (timestamp == currentTimestamp)
				m_todayItem = item;
		}

		timestamp = endTimestamp - 1;
	}
}
}

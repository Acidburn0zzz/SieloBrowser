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

#pragma once
#ifndef SIELOBROWSER_HISTORYMENU_HPP
#define SIELOBROWSER_HISTORYMENU_HPP

#include <QMenu>

#include <QPointer>

namespace Sn
{
class BrowserWindow;

// TODO: manage ctrl and shift
class HistoryMenu: public QMenu {
Q_OBJECT

public:
	HistoryMenu(QWidget* parent = nullptr);
	~HistoryMenu();

	void setMainWindow(BrowserWindow* window);

private slots:
	void goBack();
	void goForward();
	void goHome();
	void showHistoryManager();

	void aboutToShow();
	void aboutToHide();

	void aboutToShowMostVisited();
	void aboutToShowClosedTabs();

	void historyEntryActivated();

	void openUrl(const QUrl& url);

private:
	QPointer<BrowserWindow> m_window{};
	QMenu* m_menuMostVisited{nullptr};
	QMenu* m_menuClosedTabs{nullptr};
};
}

#endif //SIELOBROWSER_HISTORYMENU_HPP

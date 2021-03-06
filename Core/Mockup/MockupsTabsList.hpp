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
#ifndef SIELOBROWSER_MOCKUPSTABSLIST_HPP
#define SIELOBROWSER_MOCKUPSTABSLIST_HPP

#include <QListWidget>

#include <QDropEvent>

#include <QVBoxLayout>

#include <QPushButton>

#include "Mockup/MockupItem.hpp"

namespace Sn
{
class MockupsManager;

class MockupsTabsList: public QListWidget {
Q_OBJECT

public:
	MockupsTabsList(MockupsManager* manager, QWidget* parent = nullptr);
	~MockupsTabsList();

	QVBoxLayout *parentLayout() const { return m_parentLayout; }
	void setParentLayout(QVBoxLayout* layout);

	MockupItem::TabsSpace *tabsSpace();

private slots:
	void deleteItem();
	void addTab();

protected:
	void dropEvent(QDropEvent* event);
	void enterEvent(QEvent* event);
	void leaveEvent(QEvent* event);

private:
	MockupsManager* m_mockupManager{nullptr};
	QVBoxLayout* m_parentLayout{nullptr};

	QPushButton* m_deleteButton{nullptr};
	QPushButton* m_addTabButton{nullptr};

};
}

#endif //SIELOBROWSER_MOCKUPSTABSLIST_HPP

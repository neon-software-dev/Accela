/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MinHeightLayout.h"

namespace Accela
{

MinHeightLayout::MinHeightLayout(QWidget* pParent)
    : QLayout(pParent)
{

}

MinHeightLayout::~MinHeightLayout()
{
    for (QLayoutItem* pItem : m_items)
    {
        delete pItem;
    }

    m_items.clear();
}

void MinHeightLayout::addItem(QLayoutItem* pItem)
{
    m_items.append(pItem);
}

QLayoutItem *MinHeightLayout::itemAt(int index) const
{
    if (index >= (int)m_items.size())
    {
        return nullptr;
    }

    return m_items.value(index);
}

QLayoutItem *MinHeightLayout::takeAt(int index)
{
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : 0;
}

int MinHeightLayout::count() const
{
    return (int)m_items.size();
}

QSize MinHeightLayout::sizeHint() const
{
    return DoLayout(QRect(), true);
}

bool MinHeightLayout::hasHeightForWidth() const
{
    return true;
}

int MinHeightLayout::heightForWidth(int width) const
{
    return DoLayout(QRect(0,0,width,0), true).height();
}

void MinHeightLayout::setGeometry(const QRect& r)
{
    QLayout::setGeometry(r);

    (void)DoLayout(r, false);
}

QSize MinHeightLayout::DoLayout(const QRect& r, bool testOnly) const
{
    QSize totalLayoutSize{r.width(), 0};

    unsigned int curYPos = r.y();

    for (unsigned int x = 0; x < m_items.size(); ++x)
    {
        QLayoutItem* pItem = m_items.at(x);

        const int itemHeightForWidth = pItem->heightForWidth(r.width());

        QSize itemSize = {r.width(), itemHeightForWidth};
        itemSize = itemSize.expandedTo(pItem->minimumSize());

        if (!testOnly)
        {
            pItem->setGeometry(QRect(0, (int)curYPos, itemSize.width(), itemSize.height()));
        }

        curYPos += itemSize.height();
        totalLayoutSize += QSize(0, itemSize.height());
    }

    return totalLayoutSize;
}

}

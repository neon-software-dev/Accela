/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_MINHEIGHTLAYOUT_H
#define ACCELAEDITOR_VIEW_MINHEIGHTLAYOUT_H

#include <QLayout>
#include <QLayoutItem>
#include <QList>

namespace Accela
{
    /**
     * QLayout which has items expand to fill available width, but also keeps
     * items vertically shrunken to their suggested height.
     */
    class MinHeightLayout : public QLayout
    {
        public:

            explicit MinHeightLayout(QWidget* pParent = nullptr);
            ~MinHeightLayout() override;

            void addItem(QLayoutItem* pItem) override;
            [[nodiscard]] QLayoutItem* itemAt(int index) const override;
            [[nodiscard]] QLayoutItem* takeAt(int index) override;
            [[nodiscard]] int count() const override;
            [[nodiscard]] QSize sizeHint() const override;
            [[nodiscard]] bool hasHeightForWidth() const override;
            [[nodiscard]] int heightForWidth(int width) const override;
            void setGeometry(const QRect& r) override;

        private:

            [[nodiscard]] QSize DoLayout(const QRect& r, bool testOnly) const;

        private:

            QList<QLayoutItem*> m_items;
    };
}


#endif //ACCELAEDITOR_VIEW_MINHEIGHTLAYOUT_H

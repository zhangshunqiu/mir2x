/*
 * =====================================================================================
 *
 *       Filename: inventoryboard.hpp
 *        Created: 10/08/2017 19:06:52
 *  Last Modified: 11/11/2017 23:43:36
 *
 *    Description: inventory class
 *                 server won't store the item place
 *                 it only stores the item weight in total
 *
 *                 client can use different strategy to store them
 *                 player has max weight to hold only
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#pragma once
#include "widget.hpp"
#include "labelboard.hpp"
#include "tritexbutton.hpp"

class ProcessRun;
class InventoryBoard: public Widget
{
    private:
        LabelBoard m_GoldBoard;

    private:
        TritexButton m_CloseButton;

    private:
        ProcessRun *m_ProcessRun;

    public:
        InventoryBoard(int, int, ProcessRun *, Widget * = nullptr, bool = false);

    private:
        void DrawItem(int, int, const PackBin &);

    public:
        void DrawEx(int, int, int, int, int, int);

    public:
        bool ProcessEvent(const SDL_Event &, bool *);
};

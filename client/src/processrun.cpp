/*
 * =====================================================================================
 *
 *       Filename: processrun.cpp
 *        Created: 08/31/2015 03:43:46 AM
 *  Last Modified: 06/16/2017 23:34:14
 *
 *    Description: 
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

#include <memory>
#include <cstring>

#include "monster.hpp"
#include "mathfunc.hpp"
#include "sysconst.hpp"
#include "pngtexdbn.hpp"
#include "sdldevice.hpp"
#include "clientenv.hpp"
#include "processrun.hpp"

ProcessRun::ProcessRun()
    : Process()
    , m_MyHero(nullptr)
    , m_ViewX(0)
    , m_ViewY(0)
    , m_RollMap(false)
    , m_ControbBoard(0, 0, nullptr, false)
{
}

void ProcessRun::Update(double)
{
    if(m_MyHero){
        extern SDLDevice *g_SDLDevice;
        int nViewX = m_MyHero->X() * SYS_MAPGRIDXP - g_SDLDevice->WindowW(false) / 2;
        int nViewY = m_MyHero->Y() * SYS_MAPGRIDYP - g_SDLDevice->WindowH(false) / 2;

        int nDViewX = nViewX - m_ViewX;
        int nDViewY = nViewY - m_ViewY;

        if(m_RollMap
                ||  (std::abs(nDViewX) > g_SDLDevice->WindowW(false) / 4)
                ||  (std::abs(nDViewY) > g_SDLDevice->WindowH(false) / 4)){

            m_RollMap = true;

            m_ViewX += (int)(std::lround(std::copysign(std::min<int>(3, std::abs(nDViewX)), nDViewX)));
            m_ViewY += (int)(std::lround(std::copysign(std::min<int>(2, std::abs(nDViewY)), nDViewY)));

            m_ViewX = std::max<int>(0, m_ViewX);
            m_ViewY = std::max<int>(0, m_ViewY);
        }

        // stop rolling the map when
        //   1. the hero is at the required position
        //   2. the hero is not moving
        if((nDViewX == 0) && (nDViewY == 0) && !m_MyHero->Moving()){ m_RollMap = false; }
    }

    for(auto pRecord = m_CreatureRecord.begin(); pRecord != m_CreatureRecord.end();){
        if(true
                && pRecord->second
                && pRecord->second->Active()){
            pRecord->second->Update();
            ++pRecord;
        }else{
            delete pRecord->second;
            pRecord = m_CreatureRecord.erase(pRecord);
        }
    }

    // select for the pointer focus
    // need to do it outside of creatures since only one can be selected
    {
        static uint32_t nFocusUID = 0;
        if(m_CreatureRecord.find(nFocusUID) != m_CreatureRecord.end()){
            m_CreatureRecord[nFocusUID]->Focus(false);
        }

        int nPointX = -1;
        int nPointY = -1;
        SDL_GetMouseState(&nPointX, &nPointY);

        Creature *pFocus = nullptr;
        for(auto pRecord: m_CreatureRecord){
            if(true
                    && pRecord.second != m_MyHero
                    && pRecord.second->CanFocus(m_ViewX + nPointX, m_ViewY + nPointY)){
                if(false
                        || !pFocus
                        ||  pFocus->Y() < pRecord.second->Y()){
                    // 1. currently we have no candidate yet
                    // 2. we have candidate but it's not at more front location
                    pFocus = pRecord.second;
                }
            }
        }

        if(pFocus){
            pFocus->Focus(true);
            nFocusUID = pFocus->UID();
        }
    }
}

void ProcessRun::Draw()
{
    extern PNGTexDBN *g_PNGTexDBN;
    extern SDLDevice *g_SDLDevice;
    g_SDLDevice->ClearScreen();

    // 1. draw map + object
    {
        int nX0 = (m_ViewX - 2 * SYS_MAPGRIDXP - SYS_OBJMAXW) / SYS_MAPGRIDXP;
        int nY0 = (m_ViewY - 2 * SYS_MAPGRIDYP - SYS_OBJMAXH) / SYS_MAPGRIDYP;
        int nX1 = (m_ViewX + 2 * SYS_MAPGRIDXP + SYS_OBJMAXW + g_SDLDevice->WindowW(false)) / SYS_MAPGRIDXP;
        int nY1 = (m_ViewY + 2 * SYS_MAPGRIDYP + SYS_OBJMAXH + g_SDLDevice->WindowH(false)) / SYS_MAPGRIDYP;

        // tiles
        for(int nY = nY0; nY <= nY1; ++nY){
            for(int nX = nX0; nX <= nX1; ++nX){
                if(m_Mir2xMapData.ValidC(nX, nY) && !(nX % 2) && !(nY % 2)){
                    auto nParam = m_Mir2xMapData.Tile(nX, nY).Param;
                    if(nParam & 0X80000000){
                        if(auto pTexture = g_PNGTexDBN->Retrieve(nParam & 0X00FFFFFF)){
                            g_SDLDevice->DrawTexture(pTexture, nX * SYS_MAPGRIDXP - m_ViewX, nY * SYS_MAPGRIDYP - m_ViewY);
                        }
                    }
                }
            }
        }

        // ground objects
        for(int nY = nY0; nY <= nY1; ++nY){
            for(int nX = nX0; nX <= nX1; ++nX){
                if(m_Mir2xMapData.ValidC(nX, nY) && (m_Mir2xMapData.Cell(nX, nY).Param & 0X80000000)){
                    // for obj-0
                    {
                        auto nParam = m_Mir2xMapData.Cell(nX, nY).Obj[0].Param;
                        if(nParam & 0X80000000){
                            auto nObjParam = m_Mir2xMapData.Cell(nX, nY).ObjParam;
                            if(nObjParam & ((uint32_t)(1) << 22)){
                                if(auto pTexture = g_PNGTexDBN->Retrieve(nParam & 0X00FFFFFF)){
                                    int nH = 0;
                                    if(!SDL_QueryTexture(pTexture, nullptr, nullptr, nullptr, &nH)){
                                        g_SDLDevice->DrawTexture(pTexture, nX * SYS_MAPGRIDXP - m_ViewX, (nY + 1) * SYS_MAPGRIDYP - m_ViewY - nH);
                                    }
                                }
                            }
                        }
                    }

                    // for obj-1
                    {
                        auto nParam = m_Mir2xMapData.Cell(nX, nY).Obj[0].Param;
                        if(nParam & 0X80000000){
                            auto nObjParam = m_Mir2xMapData.Cell(nX, nY).ObjParam;
                            if(nObjParam & ((uint32_t)(1) << 6)){
                                if(auto pTexture = g_PNGTexDBN->Retrieve(nParam & 0X00FFFFFF)){
                                    int nH = 0;
                                    if(!SDL_QueryTexture(pTexture, nullptr, nullptr, nullptr, &nH)){
                                        g_SDLDevice->DrawTexture(pTexture, nX * SYS_MAPGRIDXP - m_ViewX, (nY + 1) * SYS_MAPGRIDYP - m_ViewY - nH);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        extern ClientEnv *g_ClientEnv;
        if(g_ClientEnv->MIR2X_DEBUG_SHOW_MAP_GRID){
            int nX0 = m_ViewX / SYS_MAPGRIDXP;
            int nY0 = m_ViewY / SYS_MAPGRIDYP;

            int nX1 = (m_ViewX + g_SDLDevice->WindowW(false)) / SYS_MAPGRIDXP;
            int nY1 = (m_ViewY + g_SDLDevice->WindowH(false)) / SYS_MAPGRIDYP;

            g_SDLDevice->PushColor(0, 255, 0, 128);
            for(int nX = nX0; nX <= nX1; ++nX){
                g_SDLDevice->DrawLine(nX * SYS_MAPGRIDXP - m_ViewX, 0, nX * SYS_MAPGRIDXP - m_ViewX, g_SDLDevice->WindowH(false));
            }
            for(int nY = nY0; nY <= nY1; ++nY){
                g_SDLDevice->DrawLine(0, nY * SYS_MAPGRIDYP - m_ViewY, g_SDLDevice->WindowW(false), nY * SYS_MAPGRIDYP - m_ViewY);
            }
            g_SDLDevice->PopColor();
        }

        // draw dead actors
        // dead actors are shown before all active actors
        for(int nY = nY0; nY <= nY1; ++nY){
            for(int nX = nX0; nX <= nX1; ++nX){
                for(auto pCreature: m_CreatureRecord){
                    if(true
                            && (pCreature.second)
                            && (pCreature.second->X() == nX)
                            && (pCreature.second->Y() == nY)
                            && (pCreature.second->StayDead())){
                        pCreature.second->Draw(m_ViewX, m_ViewY);
                    }
                }
            }
        }

        // over-ground objects
        for(int nY = nY0; nY <= nY1; ++nY){
            for(int nX = nX0; nX <= nX1; ++nX){
                if(m_Mir2xMapData.ValidC(nX, nY) && (m_Mir2xMapData.Cell(nX, nY).Param & 0X80000000)){
                    // for obj-0
                    {
                        auto nParam = m_Mir2xMapData.Cell(nX, nY).Obj[0].Param;
                        if(nParam & 0X80000000){
                            auto nObjParam = m_Mir2xMapData.Cell(nX, nY).ObjParam;
                            if(!(nObjParam & ((uint32_t)(1) << 22))){
                                if(auto pTexture = g_PNGTexDBN->Retrieve(nParam & 0X00FFFFFF)){
                                    int nH = 0;
                                    if(!SDL_QueryTexture(pTexture, nullptr, nullptr, nullptr, &nH)){
                                        g_SDLDevice->DrawTexture(pTexture, nX * SYS_MAPGRIDXP - m_ViewX, (nY + 1) * SYS_MAPGRIDYP - m_ViewY - nH);
                                    }
                                }
                            }
                        }
                    }

                    // for obj-1
                    {
                        auto nParam = m_Mir2xMapData.Cell(nX, nY).Obj[0].Param;
                        if(nParam & 0X80000000){
                            auto nObjParam = m_Mir2xMapData.Cell(nX, nY).ObjParam;
                            if(!(nObjParam & ((uint32_t)(1) << 6))){
                                if(auto pTexture = g_PNGTexDBN->Retrieve(nParam & 0X00FFFFFF)){
                                    int nH = 0;
                                    if(!SDL_QueryTexture(pTexture, nullptr, nullptr, nullptr, &nH)){
                                        g_SDLDevice->DrawTexture(pTexture, nX * SYS_MAPGRIDXP - m_ViewX, (nY + 1) * SYS_MAPGRIDYP - m_ViewY - nH);
                                    }
                                }
                            }
                        }
                    }

                    extern ClientEnv *g_ClientEnv;
                    if(g_ClientEnv->MIR2X_DEBUG_SHOW_MAP_GRID){
                        if(m_Mir2xMapData.Cell(nX, nY).Param & 0X00800000){
                            g_SDLDevice->PushColor(255, 0, 0, 128);
                            int nX0 = nX * SYS_MAPGRIDXP - m_ViewX;
                            int nY0 = nY * SYS_MAPGRIDYP - m_ViewY;
                            int nX1 = (nX + 1) * SYS_MAPGRIDXP - m_ViewX;
                            int nY1 = (nY + 1) * SYS_MAPGRIDYP - m_ViewY;
                            g_SDLDevice->DrawLine(nX0, nY0, nX1, nY0);
                            g_SDLDevice->DrawLine(nX1, nY0, nX1, nY1);
                            g_SDLDevice->DrawLine(nX1, nY1, nX0, nY1);
                            g_SDLDevice->DrawLine(nX0, nY1, nX0, nY0);
                            g_SDLDevice->DrawLine(nX0, nY0, nX1, nY1);
                            g_SDLDevice->DrawLine(nX1, nY0, nX0, nY1);
                            g_SDLDevice->PopColor();
                        }
                    }
                }

                // draw actors
                {
                    for(auto pCreature: m_CreatureRecord){
                        if(true
                                &&  (pCreature.second)
                                &&  (pCreature.second->X() == nX)
                                &&  (pCreature.second->Y() == nY)
                                && !(pCreature.second->StayDead())){
                            extern ClientEnv *g_ClientEnv;
                            if(g_ClientEnv->MIR2X_DEBUG_SHOW_CREATURE_COVER){
                                g_SDLDevice->PushColor(0, 0, 255, 30);
                                g_SDLDevice->FillRectangle(nX * SYS_MAPGRIDXP - m_ViewX, nY * SYS_MAPGRIDYP - m_ViewY, SYS_MAPGRIDXP, SYS_MAPGRIDYP);
                                g_SDLDevice->PopColor();
                            }
                            pCreature.second->Draw(m_ViewX, m_ViewY);
                        }
                    }
                }
            }
        }
    }

    m_ControbBoard.Draw();
    g_SDLDevice->Present();
}

void ProcessRun::ProcessEvent(const SDL_Event &rstEvent)
{
    switch(rstEvent.type){
        case SDL_MOUSEBUTTONDOWN:
            {
                switch(rstEvent.button.button){
                    case SDL_BUTTON_RIGHT:
                        {
                            // in mir2ei how human moves
                            // 1. client send motion request to server
                            // 2. client put motion lock to human
                            // 3. server response with "+GOOD" or "+FAIL" to client
                            // 4. if "+GOOD" client will release the motion lock
                            // 5. if "+FAIL" client will use the backup position and direction

                            int nX = -1;
                            int nY = -1;
                            if(LocatePoint(rstEvent.button.x, rstEvent.button.y, &nX, &nY)){
                                if(LDistance2(m_MyHero->CurrMotion().EndX, m_MyHero->CurrMotion().EndY, nX, nY)){
                                    m_MyHero->ParseNewAction({
                                            ACTION_MOVE,
                                            MOTION_NONE,    // need decompose
                                            1,              // by default
                                            DIR_NONE,
                                            m_MyHero->CurrMotion().EndX,
                                            m_MyHero->CurrMotion().EndY,
                                            nX,
                                            nY,
                                            MapID()}, false);
                                }
                            }

                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
                break;
            }
        case SDL_MOUSEMOTION:
            {
                break;
            }
        case SDL_KEYDOWN:
            {
                switch(rstEvent.key.keysym.sym){
                    case SDLK_e:
                        {
                            std::exit(0);
                            break;
                        }
                    case SDLK_ESCAPE:
                        {
                            extern SDLDevice *g_SDLDevice;
                            m_ViewX = std::max<int>(0, m_MyHero->X() - g_SDLDevice->WindowW(false) / 2 / SYS_MAPGRIDXP) * SYS_MAPGRIDXP;
                            m_ViewY = std::max<int>(0, m_MyHero->Y() - g_SDLDevice->WindowH(false) / 2 / SYS_MAPGRIDYP) * SYS_MAPGRIDYP;
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
                break;
            }
        default:
            {
                break;
            }
    }
}

int ProcessRun::LoadMap(uint32_t nMapID)
{
    if(nMapID){
        m_MapID = nMapID;
        if(auto pMapName = SYS_MAPFILENAME(nMapID)){
            return m_Mir2xMapData.Load(pMapName);
        }
    }
    return -1;
}

bool ProcessRun::CanMove(bool bCheckCreature, int nX, int nY){
    if(m_Mir2xMapData.ValidC(nX, nY)
            && (m_Mir2xMapData.Cell(nX, nY).Param & 0X80000000)
            && (m_Mir2xMapData.Cell(nX, nY).Param & 0X00800000)){
        if(bCheckCreature){
            for(auto pCreature: m_CreatureRecord){
                if(pCreature.second && (pCreature.second->X() == nX) && (pCreature.second->Y() == nY)){
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

bool ProcessRun::CanMove(bool bCheckCreature, int nX0, int nY0, int nX1, int nY1)
{
    if(true
            && m_Mir2xMapData.ValidC(nX0, nY0)
            && m_Mir2xMapData.ValidC(nX1, nY1)){
        switch(LDistance2(nX0, nY0, nX1, nY1)){
            case 0:
            case 1:
            case 2:
                {
                    return CanMove(bCheckCreature, nX1, nY1);
                }
            default:
                {
                    break;
                }
        }
    }
    return false;
}

bool ProcessRun::LocatePoint(int nPX, int nPY, int *pX, int *pY)
{
    if(pX){ *pX = (nPX + m_ViewX) / SYS_MAPGRIDXP; }
    if(pY){ *pY = (nPY + m_ViewY) / SYS_MAPGRIDYP; }

    return true;
}

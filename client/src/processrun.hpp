/*
 * =====================================================================================
 *
 *       Filename: processrun.hpp
 *        Created: 08/31/2015 03:42:07
 *  Last Modified: 12/14/2017 23:43:55
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
#pragma once
#include <map>
#include <list>
#include <cstdint>
#include "myhero.hpp"
#include "process.hpp"
#include "message.hpp"
#include "creature.hpp"
#include "focustype.hpp"
#include "ascendstr.hpp"
#include "grounditem.hpp"
#include "indepmagic.hpp"
#include "labelboard.hpp"
#include "mir2xmapdata.hpp"
#include "controlboard.hpp"
#include "inventoryboard.hpp"
#include "clientluamodule.hpp"
#include "linebrowserboard.hpp"

class ProcessRun: public Process
{
    private:
        struct UserCommandEntry
        {
            std::string Command;
            std::function<int(const std::vector<std::string> &)> Callback;

            UserCommandEntry(const char *szCommand, std::function<int(const std::vector<std::string> &)> rfnOP)
                : Command(szCommand ? szCommand : "")
                , Callback(rfnOP)
            {}
        };

    private:
        enum OutPortType: int
        {
            OUTPORT_NONE         = (0 << 0),
            OUTPORT_LOG          = (1 << 0),
            OUTPORT_SCREEN       = (2 << 1),
            OUTPORT_CONTROLBOARD = (3 << 1),
        };

    private:
        std::vector<UserCommandEntry> m_UserCommandGroup;

    private:
        uint32_t     m_MapID;
        Mir2xMapData m_Mir2xMapData;

    private:
        MyHero *m_MyHero;

    private:
        std::array<uint32_t, FOCUS_MAX> m_FocusTable;

    private:
        int m_ViewX;
        int m_ViewY;

    private:
        bool m_RollMap;

    private:
        ClientLuaModule m_LuaModule;

    private:
        ControlBoard m_ControbBoard;

    private:
        InventoryBoard m_InventoryBoard;

    private:
        std::vector<std::shared_ptr<IndepMagic>> m_IndepMagicList;

    private:
        std::vector<GroundItem> m_GroundItemList;
        std::map<uint32_t, Creature*> m_CreatureRecord;

    private:
        // use a tokenboard to show all in future
        LabelBoard m_MousePixlLoc;
        LabelBoard m_MouseGridLoc;

    private:
        std::list<AscendStr *> m_AscendStrRecord;

    private:
        void ScrollMap();

    private:
        int LoadMap(uint32_t);

    public:
        ProcessRun();
        virtual ~ProcessRun() = default;

    public:
        int ID() const
        {
            return PROCESSID_RUN;
        }

        uint32_t MapID() const
        {
            return m_MapID;
        }

    public:
        void Notify(int, const char *);
        void Notify(const char *, const std::map<std::string, std::function<void()>> &);

    public:
        virtual void Update(double);
        virtual void Draw();
        virtual void ProcessEvent(const SDL_Event &);

    public:
        bool ScreenPoint2Grid(int, int, int *, int *);

    public:
        bool OnMap(uint32_t, int, int) const;

    public:
        void Net_EXP(const uint8_t *, size_t);
        void Net_ACTION(const uint8_t *, size_t);
        void Net_OFFLINE(const uint8_t *, size_t);
        void Net_LOGINOK(const uint8_t *, size_t);
        void Net_PICKUPOK(const uint8_t *, size_t);
        void Net_CORECORD(const uint8_t *, size_t);
        void Net_UPDATEHP(const uint8_t *, size_t);
        void Net_FIREMAGIC(const uint8_t *, size_t);
        void Net_DEADFADEOUT(const uint8_t *, size_t);
        void Net_MONSTERGINFO(const uint8_t *, size_t);
        void Net_SHOWDROPITEM(const uint8_t *, size_t);

    public:
        bool CanMove(bool, int, int);
        bool CanMove(bool, int, int, int, int);

    public:
        double MoveCost(bool, int, int, int, int);

    private:
        uint32_t FocusUID(int);

    public:
        bool  LuaCommand(const char *);
        bool UserCommand(const char *);

    public:
        uint32_t GetFocusFaceKey();

    public:
        std::vector<int> GetPlayerList();

    public:
        bool AddOPLog(int, int, const char *, const char *, ...);

    public:
        bool RegisterUserCommand();

    public:
        bool RegisterLuaExport(ClientLuaModule *, int);

    public:
        Creature *RetrieveUID(uint32_t);
        bool LocateUID(uint32_t, int *, int *);

    private:
        bool TrackAttack(bool, uint32_t);

    public:
        void AddAscendStr(int, int, int, int);

    public:
        bool GetUIDLocation(uint32_t, bool, int *, int *);

    public:
        void CenterMyHero();

    public:
        MyHero *GetMyHero() const
        {
            return m_MyHero;
        }

    public:
        const auto &GetGroundItemList() const
        {
            return m_GroundItemList;
        }

        GroundItem GetGroundItem(int nX, int nY)
        {
            for(auto &rstItem: GetGroundItemList()){
                if(true
                        && nX == rstItem.X
                        && nY == rstItem.Y){
                    return rstItem;
                }
            }
            return {0};
        }

        void RemoveGroundItem(uint32_t nItemID, int nX, int nY)
        {
            for(size_t nIndex = 0; nIndex < m_GroundItemList.size();){
                if(true
                        && m_GroundItemList[nIndex].X  == nX
                        && m_GroundItemList[nIndex].Y  == nY){

                    if(nItemID){
                        if(m_GroundItemList[nIndex].ID == nItemID){
                            std::swap(m_GroundItemList[nIndex], m_GroundItemList.back());
                            m_GroundItemList.pop_back();

                            // could have more than one stay there
                            // we only remove one item from the location
                            return;
                        }
                    }else{
                        // zero item id
                        // we need to remove all given location
                        std::swap(m_GroundItemList[nIndex], m_GroundItemList.back());
                        m_GroundItemList.pop_back();
                        continue;
                    }
                }
                nIndex++;
            }
        }

    public:
        bool RequestSpaceMove(uint32_t, int, int);

    public:
        void ClearCreature();

    public:
        Widget *GetWidget(const char *);
};

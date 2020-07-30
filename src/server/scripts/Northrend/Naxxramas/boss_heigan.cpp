/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "naxxramas.h"
#include "Player.h"

enum Says
{
    SAY_AGGRO                   = 0,
    SAY_SLAY                    = 1,
    SAY_TAUNT                   = 2,
    SAY_DEATH                   = 3,
    EMOTE_DANCE                 = 4,
    EMOTE_DANCE_END             = 5,
    SAY_DANCE                   = 6
};

enum Spells
{
    SPELL_SPELL_DISRUPTION          = 29310,
    SPELL_DECREPIT_FEVER_10         = 29998,
    SPELL_DECREPIT_FEVER_25         = 55011,
    SPELL_PLAGUE_CLOUD              = 29350,
};

enum Events
{
    EVENT_SPELL_SPELL_DISRUPTION    = 1,
    EVENT_SPELL_DECEPIT_FEVER       = 2,
    EVENT_ERUPT_SECTION             = 3,
    EVENT_SWITCH_PHASE              = 4,
    EVENT_SAFETY_DANCE              = 5,
};

enum Misc
{
    PHASE_SLOW_DANCE                = 0,
    PHASE_FAST_DANCE                = 1,
};

class boss_heigan : public CreatureScript
{
public:
    boss_heigan() : CreatureScript("boss_heigan") { }

    CreatureAI* GetAI(Creature* pCreature) const override
    {
        return new boss_heiganAI (pCreature);
    }

    struct boss_heiganAI : public BossAI
    {
        explicit boss_heiganAI(Creature *c) : BossAI(c, BOSS_HEIGAN)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript* pInstance;
        EventMap events;
        uint8 currentPhase{};
        uint8 currentSection{};
        bool moveRight{};

        void Reset() override
        {
            BossAI::Reset();
            events.Reset();
            currentPhase = 0;
            currentSection = 3;
            moveRight = true;

            if (pInstance)
            {
                if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetData64(DATA_HEIGAN_ENTER_GATE)))
                    go->SetGoState(GO_STATE_ACTIVE);
            }
        }

        void KilledUnit(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(SAY_SLAY);

            if (pInstance)
                pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
        }

        void JustDied(Unit*  killer) override
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
        }

        void EnterCombat(Unit * who) override
        {
            BossAI::EnterCombat(who);
            me->SetInCombatWithZone();
            Talk(SAY_AGGRO);
            if (pInstance)
            {
                if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetData64(DATA_HEIGAN_ENTER_GATE)))
                    go->SetGoState(GO_STATE_READY);
            }

            StartFightPhase(PHASE_SLOW_DANCE);
        }

        void StartFightPhase(uint8 phase)
        {
            currentSection = 3;
            currentPhase = phase;
            events.Reset();
            if (phase == PHASE_SLOW_DANCE)
            {
                events.ScheduleEvent(EVENT_SPELL_SPELL_DISRUPTION, 0s);
                events.ScheduleEvent(EVENT_SPELL_DECEPIT_FEVER, 12s);
                events.ScheduleEvent(EVENT_ERUPT_SECTION, 10s);
                events.ScheduleEvent(EVENT_SWITCH_PHASE, 90s);
            }
            else // if (phase == PHASE_FAST_DANCE)
            {
                Talk(EMOTE_DANCE);
                Talk(SAY_DANCE);
                // teleport
                float x, y, z, o;
                me->GetHomePosition(x, y, z, o);
                me->NearTeleportTo(x, y, z, o);

                me->CastSpell(me, SPELL_PLAGUE_CLOUD, false);
                events.ScheduleEvent(EVENT_ERUPT_SECTION, 4s);
                events.ScheduleEvent(EVENT_SWITCH_PHASE, 45s);
            }
            events.ScheduleEvent(EVENT_SAFETY_DANCE, 5s);
        }

        bool IsInRoom(Unit* who)
        {
            if (who->GetPositionX() > 2826 || who->GetPositionX() < 2723 || who->GetPositionY() > -3641 || who->GetPositionY() < -3736)
            {
                if (who->GetGUID() == me->GetGUID())
                    EnterEvadeMode();
                return false;
            }

            return true;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!IsInRoom(me))
                return;

            if (!UpdateVictim())
                return;

            events.Update(diff);
            //if (me->HasUnitState(UNIT_STATE_CASTING))
            //  return;

            switch (events.GetEvent())
            {
                case EVENT_SPELL_SPELL_DISRUPTION:
                    me->CastSpell(me, SPELL_SPELL_DISRUPTION, false);
                    events.RepeatEvent(10s);
                    break;
                case EVENT_SPELL_DECEPIT_FEVER:
                    me->CastSpell(me, RAID_MODE(SPELL_DECREPIT_FEVER_10, SPELL_DECREPIT_FEVER_25), false);
                    events.RepeatEvent(20s);
                    break;
                case EVENT_SWITCH_PHASE:
                    if (currentPhase == PHASE_SLOW_DANCE)
                    {
                        StartFightPhase(PHASE_FAST_DANCE);
                    }
                    else
                    {
                        StartFightPhase(PHASE_SLOW_DANCE);
                        Talk(EMOTE_DANCE_END); // we put this here to avoid play the emote on aggro.
                    }
                    // no pop, there is reset in start fight
                    break;
                case EVENT_ERUPT_SECTION:
                    if (pInstance)
                    {
                        pInstance->SetData(DATA_HEIGAN_ERUPTION, currentSection);
                        if (currentSection == 3)
                            moveRight = false;
                        else if (currentSection == 0)
                            moveRight = true;

                        moveRight ? currentSection++ : currentSection--;
                    }

                    if (currentPhase == PHASE_SLOW_DANCE)
                        Talk(SAY_TAUNT);

                    events.RepeatEvent(currentPhase == PHASE_SLOW_DANCE ? 10s : 4s);
                    break;
                case EVENT_SAFETY_DANCE:
                {
                    Map::PlayerList const& pList = me->GetMap()->GetPlayers();
                    for(const auto & itr : pList)
                    {
                        if (IsInRoom(itr.GetSource()) && !itr.GetSource()->IsAlive())
                        {
                            events.PopEvent();
                            pInstance->SetData(DATA_DANCE_FAIL, 0);
                            pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
                            return;
                        }

                    }
                    events.RepeatEvent(5s);
                    return;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_heigan()
{
    new boss_heigan();
}

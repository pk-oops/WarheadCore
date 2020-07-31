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
#include "shadow_labyrinth.h"

enum BlackheartTheInciter
{
    SPELL_INCITE_CHAOS      = 33676,
    SPELL_INCITE_CHAOS_B    = 33684,                         //debuff applied to each member of party
    SPELL_CHARGE            = 33709,
    SPELL_WAR_STOMP         = 33707,

    SAY_INTRO               = 0,
    SAY_AGGRO               = 1,
    SAY_SLAY                = 2,
    SAY_HELP                = 3,
    SAY_DEATH               = 4,

    EVENT_SPELL_INCITE      = 1,
    EVENT_INCITE_WAIT       = 2,
    EVENT_SPELL_CHARGE      = 3,
    EVENT_SPELL_KNOCKBACK   = 4
};

class boss_blackheart_the_inciter : public CreatureScript
{
public:
    boss_blackheart_the_inciter() : CreatureScript("boss_blackheart_the_inciter") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_blackheart_the_inciterAI (creature);
    }

    struct boss_blackheart_the_inciterAI : public ScriptedAI
    {
        boss_blackheart_the_inciterAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;

        bool InciteChaos;

        void Reset()
        {
            InciteChaos = false;
            events.Reset();

            if (instance)
                instance->SetData(DATA_BLACKHEARTTHEINCITEREVENT, NOT_STARTED);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER && urand(0,1))
                Talk(SAY_SLAY);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);
            if (instance)
                instance->SetData(DATA_BLACKHEARTTHEINCITEREVENT, DONE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SPELL_INCITE, 20s);
            events.ScheduleEvent(EVENT_INCITE_WAIT, 15s);
            events.ScheduleEvent(EVENT_SPELL_CHARGE, 0s);
            events.ScheduleEvent(EVENT_SPELL_KNOCKBACK, 15s);

            if (instance)
                instance->SetData(DATA_BLACKHEARTTHEINCITEREVENT, IN_PROGRESS);
        }

        void EnterEvadeMode()
        {
            if (InciteChaos && SelectTargetFromPlayerList(100.0f))
                return;
            CreatureAI::EnterEvadeMode();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            switch (events.ExecuteEvent())
            {
                case EVENT_INCITE_WAIT:
                    InciteChaos = false;
                    break;
                case EVENT_SPELL_INCITE:
                {
                    me->CastSpell(me, SPELL_INCITE_CHAOS, false);

                    std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
                    for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
                    {
                        Unit* target = ObjectAccessor::GetUnit(*me, (*itr)->getUnitGuid());
                        if (target && target->GetTypeId() == TYPEID_PLAYER)
                            me->CastSpell(target, SPELL_INCITE_CHAOS_B, true);
                    }

                    DoResetThreat();
                    InciteChaos = true;
                    events.DelayEvents(15000);
                    events.Repeat(40s);
                    events.ScheduleEvent(EVENT_INCITE_WAIT, 15s);
                    break;
                }
                case EVENT_SPELL_CHARGE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        me->CastSpell(target, SPELL_CHARGE, false);
                    events.Repeat(15s, 25s);
                    break;
                case EVENT_SPELL_KNOCKBACK:
                    me->CastSpell(me, SPELL_WAR_STOMP, false);
                    events.Repeat(18s, 24s);
                    break;
            }

            if (InciteChaos)
                return;

            DoMeleeAttackIfReady();
        }
    };

};

void AddSC_boss_blackheart_the_inciter()
{
    new boss_blackheart_the_inciter();
}

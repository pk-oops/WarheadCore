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

#include "EventMap.h"

void EventMap::Reset()
{
    _eventMap.clear();
    _time = 0;
    _phase = 0;
}

void EventMap::SetPhase(uint8 phase)
{
    if (!phase)
        _phase = 0;
    else if (phase <= 8)
        _phase = (1 << (phase - 1));
}

void EventMap::AddPhase(uint8 phase)
{
    if (phase && phase <= 8)
        _phase |= (1 << (phase - 1));
}

void EventMap::RemovePhase(uint8 phase)
{
    if (phase && phase <= 8)
        _phase &= ~(1 << (phase - 1));
}

void EventMap::ScheduleEvent(uint32 eventId, Milliseconds time, uint32 group /*= 0*/, uint8 phase /*= 0*/)
{
    if (group && group <= 8)
        eventId |= (1 << (group + 15));

    if (phase && phase <= 8)
        eventId |= (1 << (phase + 23));

    _eventMap.insert(std::make_pair(_time + time.count(), eventId));
}

void EventMap::ScheduleEvent(uint32 eventId, Milliseconds minTime, Milliseconds maxTime, uint32 group /*= 0*/, uint32 phase /*= 0*/)
{
    ScheduleEvent(eventId, randtime(minTime, maxTime), group, phase);
}

void EventMap::RescheduleEvent(uint32 eventId, Milliseconds time, uint32 group /*= 0*/, uint8 phase /*= 0*/)
{
    CancelEvent(eventId);
    ScheduleEvent(eventId, time, group, phase);
}

void EventMap::RescheduleEvent(uint32 eventId, Milliseconds minTime, Milliseconds maxTime, uint32 group /*= 0*/, uint32 phase /*= 0*/)
{
    RescheduleEvent(eventId, randtime(minTime, maxTime), group, phase);
}

void EventMap::Repeat(Milliseconds time)
{
    _eventMap.insert(std::make_pair(_time + time.count(), _lastEvent));
}

void EventMap::Repeat(Milliseconds minTime, Milliseconds maxTime)
{
    Repeat(randtime(minTime, maxTime));
}

uint32 EventMap::ExecuteEvent()
{
    while (!Empty())
    {
        EventStore::iterator itr = _eventMap.begin();

        if (itr->first > _time)
            return 0;
        else if (_phase && (itr->second & 0xFF000000) && !((itr->second >> 24) & _phase))
            _eventMap.erase(itr);
        else
        {
            uint32 eventId = (itr->second & 0x0000FFFF);
            _lastEvent = itr->second; // include phase/group
            _eventMap.erase(itr);
            return eventId;
        }
    }

    return 0;
}

void EventMap::DelayEventsToMax(uint32 delay, uint32 group)
{
    for (EventStore::iterator itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (itr->first < _time + delay && (group == 0 || ((1 << (group + 15)) & itr->second)))
        {
            ScheduleEvent(itr->second, Milliseconds(delay));
            _eventMap.erase(itr);
            itr = _eventMap.begin();
        }
        else
            ++itr;
    }
}

void EventMap::DelayEvents(uint32 delay)
{
    _time = delay < _time ? _time - delay : 0;
}

void EventMap::DelayEvents(uint32 delay, uint32 group)
{
    if (!group || group > 8 || Empty())
        return;

    EventStore delayed;

    for (EventStore::iterator itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (!group || (itr->second & (1 << (group + 15))))
        {
            delayed.insert(EventStore::value_type(itr->first + delay, itr->second));
            _eventMap.erase(itr++);
        }
        else
            ++itr;
    }

    _eventMap.insert(delayed.begin(), delayed.end());
}

void EventMap::CancelEvent(uint32 eventId)
{
    if (Empty())
        return;

    for (EventStore::iterator itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (eventId == (itr->second & 0x0000FFFF))
            _eventMap.erase(itr++);
        else
            ++itr;
    }
}

void EventMap::CancelEventGroup(uint32 group)
{
    if (!group || group > 8 || Empty())
        return;

    uint32 groupMask = (1 << (group + 15));
    for (EventStore::iterator itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (itr->second & groupMask)
        {
            _eventMap.erase(itr);
            itr = _eventMap.begin();
        }
        else
            ++itr;
    }
}

uint32 EventMap::GetNextEventTime(uint32 eventId) const
{
    if (Empty())
        return 0;

    for (std::pair<uint32 const, uint32> const& itr : _eventMap)
        if (eventId == (itr.second & 0x0000FFFF))
            return itr.first;

    return 0;
}

uint32 EventMap::GetNextEventTime() const
{
    return Empty() ? 0 : _eventMap.begin()->first;
}

bool EventMap::IsInPhase(uint8 phase)
{
    return phase <= 8 && (!phase || _phase & (1 << (phase - 1)));
}

uint32 EventMap::GetTimeUntilEvent(uint32 eventId) const
{
    for (std::pair<uint32 const, uint32> const& itr : _eventMap)
        if (eventId == (itr.second & 0x0000FFFF))
            return itr.first - _time;

    return std::numeric_limits<uint32>::max();
}

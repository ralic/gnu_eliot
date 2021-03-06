/*******************************************************************
 * Eliot
 * Copyright (C) 2009-2012 Olivier Teulière
 * Authors: Olivier Teulière <ipkiss @@ gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *****************************************************************************/

#include <boost/foreach.hpp>
#include <sstream>

#include "navigation.h"
#include "turn.h"
#include "command.h"
#include "game_exception.h"
#include "debug.h"
#include "encoding.h"


INIT_LOGGER(game, Navigation);


Navigation::Navigation()
    : m_currTurn(0)
{
    // Start with an empty turn
    m_allTurns.push_back(new Turn);
}


Navigation::~Navigation()
{
    BOOST_FOREACH(Turn *c, m_allTurns)
    {
        delete c;
    }
}


void Navigation::newTurn()
{
    LOG_INFO("New turn");
    lastTurn();
    m_allTurns.push_back(new Turn);
    ++m_currTurn;
}


void Navigation::addAndExecute(Command *iCmd)
{
    if (!isLastTurn())
        throw GameException("Cannot add a command to an old turn");
    m_allTurns[m_currTurn]->addAndExecute(iCmd);
}


unsigned int Navigation::getCurrTurn() const
{
    return m_currTurn;
}


unsigned int Navigation::getNbTurns() const
{
    return m_allTurns.size();
}


bool Navigation::isFirstTurn() const
{
    return m_currTurn == 0 &&
        m_allTurns[m_currTurn]->isPartiallyExecuted() &&
        (!m_allTurns[m_currTurn]->hasNonAutoExecCmd() ||
         !m_allTurns[m_currTurn]->isFullyExecuted());
}


bool Navigation::isLastTurn() const
{
    return m_currTurn == m_allTurns.size() - 1 &&
        m_allTurns[m_currTurn]->isFullyExecuted();
}


void Navigation::prevTurn()
{
    if (isFirstTurn())
        return;

    LOG_DEBUG("Navigating to the previous turn");
    Turn *turn = m_allTurns[m_currTurn];
    if (turn->isFullyExecuted() && turn->hasNonAutoExecCmd())
    {
        ASSERT(isLastTurn(), "Unexpected turn state");
        turn->partialUndo();
        return;
    }

    ASSERT(m_currTurn > 0, "Trying to go before the first turn");
    ASSERT(turn->isPartiallyExecuted(), "Unexpected turn state");
    turn->undo();

    --m_currTurn;
    m_allTurns[m_currTurn]->partialUndo();
}


void Navigation::nextTurn()
{
    if (isLastTurn())
        return;

    LOG_DEBUG("Navigating to the next turn");
    Turn *turn = m_allTurns[m_currTurn];
    ASSERT(turn->isPartiallyExecuted(), "Unexpected turn state");

    if (m_currTurn + 1 < m_allTurns.size())
    {
        // Finish executing the current turn (if needed)
        turn->execute();

        ++m_currTurn;
        m_allTurns[m_currTurn]->partialExecute();
    }
    else
    {
        ASSERT(!turn->isFullyExecuted(), "Unexpected turn state");
        turn->execute();
    }
}


void Navigation::firstTurn()
{
    LOG_DEBUG("Navigating to the first turn");
    while (!isFirstTurn())
    {
        prevTurn();
    }
}


void Navigation::lastTurn()
{
    LOG_DEBUG("Navigating to the last turn");
    while (!isLastTurn())
    {
        nextTurn();
    }
}


void Navigation::clearFuture()
{
    LOG_INFO("Erasing all the future turns");

    // Replay the auto-execution turns (i.e. turns where only the AI was involved).
    // This is needed for a correct handling of free games
    while (!isLastTurn() && m_allTurns[m_currTurn]->isHumanIndependent())
    {
        LOG_DEBUG("Replaying a human independent turn");
        nextTurn();
    }

    // When there is no future, don't do anything
    if (isLastTurn())
        return;

    // Destroy future turns
    while (m_allTurns.size() > m_currTurn + 1)
    {
        delete m_allTurns.back();
        m_allTurns.pop_back();
    }

    Turn *turn = m_allTurns[m_currTurn];

    // Destroy non executed commands for the current turn
    ASSERT(turn->isPartiallyExecuted(), "Invalid state");
    turn->dropNonExecutedCommands();

    // Sanity checks
    ASSERT(isLastTurn(),
           "After removing the next turns, we should be at the last turn");
    ASSERT(turn->isFullyExecuted(), "Invalid final state");
}


void Navigation::dropFrom(const Command &iCmd)
{
    ASSERT(isLastTurn(), "Only possible in the last turn");
    m_allTurns.back()->dropFrom(iCmd);
}


void Navigation::dropCommand(const Command &iCmd)
{
    m_allTurns[m_currTurn]->dropCommand(iCmd);
}


void Navigation::insertCommand(Command *iCmd)
{
    m_allTurns[m_currTurn]->insertCommand(iCmd);
}


void Navigation::replaceCommand(const Command &iOldCmd,
                                Command *iNewCmd)
{
    m_allTurns[m_currTurn]->replaceCommand(iOldCmd, iNewCmd);
}


const vector<Turn *> & Navigation::getTurns() const
{
    return m_allTurns;
}


const Turn & Navigation::getCurrentTurn() const
{
    return *m_allTurns[m_currTurn];
}


void Navigation::print() const
{
#ifdef USE_LOGGING
    LOG_DEBUG("=== Commands history ===");
    LOG_DEBUG("Current position at turn " << m_currTurn);
    int index = 0;
    ostringstream oss;
    BOOST_FOREACH(const Turn *c, m_allTurns)
    {
        oss << endl << "Turn " << index << ":" << lfw(c->toString());
        ++index;
    }
    LOG_DEBUG(oss.str());
#endif
}


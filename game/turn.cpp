/*******************************************************************
 * Eliot
 * Copyright (C) 2008-2012 Olivier Teulière
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
#include <typeinfo>

#include "turn.h"
#include "command.h"
#include "player.h"
#include "debug.h"


INIT_LOGGER(game, Turn);


Turn::Turn()
    : m_firstNotExecuted(0)
{
}


Turn::~Turn()
{
    BOOST_FOREACH(Command *cmd, m_commands)
    {
        delete cmd;
    }
}


void Turn::addAndExecute(Command *iCmd)
{
    ASSERT(isFullyExecuted(), "Adding a command to a partially executed turn");
    m_commands.push_back(iCmd);
    iCmd->execute();
    ++m_firstNotExecuted;
}


void Turn::execute()
{
    execTo(m_commands.size());
    ASSERT(isFullyExecuted(), "Bug in execute()");
}


void Turn::undo()
{
    undoTo(0);
    ASSERT(isNotAtAllExecuted(), "Bug in undo()");
}


void Turn::partialExecute()
{
    execTo(findIndexFirstNaec());
    ASSERT(isPartiallyExecuted(), "Bug in partialExecute()");
}


void Turn::partialUndo()
{
    // Lazy implementation :)
    undo();
    partialExecute();
}


void Turn::dropNonExecutedCommands()
{
    if (!isFullyExecuted())
        dropFrom(m_firstNotExecuted);
}


void Turn::dropFrom(const Command &iCmd)
{
    // Find the command index
    unsigned idx = findIndex(iCmd);
    ASSERT(idx != m_commands.size(), "Cannot find command to drop");

    dropFrom(idx);
}


void Turn::dropCommand(const Command &iCmd)
{
    ASSERT(iCmd.isInsertable(), "Only insertable commands can be dropped");
    ASSERT(iCmd.isAutoExecutable(), "Non auto-executable commands cannot be dropped");

    // Find the command index
    unsigned idx = findIndex(iCmd);
    ASSERT(idx != m_commands.size(), "Cannot find command to drop");

    LOG_DEBUG("Dropping single command");

    // Undo commands after the interesting one (included)
    unsigned tmpIdx = undoTo(idx);
    ASSERT(!iCmd.isExecuted(), "Logic error");

    // Drop the command
    delete m_commands[idx];
    m_commands.erase(m_commands.begin() + idx);

    // We have deleted one command, so the index should be decreased
    --tmpIdx;

    // Re-execute the commands
    execTo(tmpIdx);
}


void Turn::insertCommand(Command *iCmd)
{
    ASSERT(iCmd->isInsertable(), "Only insertable commands can be inserted");
    ASSERT(iCmd->isAutoExecutable(), "Non auto-executable commands cannot be inserted");

    // Find the insertion index
    unsigned idx = findIndexFirstNaec();

    LOG_DEBUG("Inserting command");

    // Undo commands after the interesting one (included)
    unsigned tmpIdx = undoTo(idx);

    // Insert the command (possibly at the end, if there is no NAEC)
    if (idx == m_commands.size())
        m_commands.push_back(iCmd);
    else
        m_commands.insert(m_commands.begin() + idx, iCmd);

    // We have inserted one command, so the index should be increased
    ++tmpIdx;

    // Re-execute the commands
    execTo(tmpIdx);
}


void Turn::replaceCommand(const Command &iOldCmd,
                             Command *iNewCmd)
{
    ASSERT(string(typeid(iOldCmd).name()) == string(typeid(*iNewCmd).name()),
           "The commands should be of the same type (" +
           string(typeid(iOldCmd).name()) + " vs. " +
           string(typeid(*iNewCmd).name()));

    unsigned idx = findIndex(iOldCmd);
    ASSERT(idx != m_commands.size(), "Cannot find command");

    // Undo commands after the interesting one (included)
    unsigned tmpIdx = undoTo(idx);
    ASSERT(!iOldCmd.isExecuted(), "Logic error");

    // Replace the command
    delete m_commands[idx];
    m_commands[idx] = iNewCmd;

    // Re-execute the commands
    execTo(tmpIdx);
}


bool Turn::isFullyExecuted() const
{
    return m_firstNotExecuted == m_commands.size();
}


bool Turn::isPartiallyExecuted() const
{
    if (isFullyExecuted())
        return true;
    return !m_commands[m_firstNotExecuted]->isAutoExecutable();
}


bool Turn::isNotAtAllExecuted() const
{
    return m_firstNotExecuted == 0;
}


bool Turn::hasNonAutoExecCmd() const
{
    return findIndexFirstNaec() != m_commands.size();
}


bool Turn::isHumanIndependent() const
{
    BOOST_FOREACH(Command *cmd, m_commands)
    {
        if (!cmd->isHumanIndependent())
            return false;
    }
    return true;
}


unsigned Turn::findIndex(const Command &iCmd) const
{
    for (unsigned i = 0; i < m_commands.size(); ++i)
    {
        if (m_commands[i] == &iCmd)
            return i;
    }
    return m_commands.size();
}


unsigned Turn::findIndexFirstNaec() const
{
    for (unsigned i = 0; i < m_commands.size(); ++i)
    {
        if (!m_commands[i]->isAutoExecutable())
            return i;
    }
    return m_commands.size();
}


unsigned Turn::execTo(unsigned iNewFirstNotExec)
{
    ASSERT(iNewFirstNotExec <= m_commands.size(), "Invalid index");
    unsigned oldVal = m_firstNotExecuted;
    while (m_firstNotExecuted < iNewFirstNotExec)
    {
        Command *cmd = m_commands[m_firstNotExecuted];
        ASSERT(!cmd->isExecuted(), "Bug with m_firstNotExecuted");
        cmd->execute();
        ++m_firstNotExecuted;
    }
    return oldVal;
}


unsigned Turn::undoTo(unsigned iNewFirstNotExec)
{
    unsigned oldVal = m_firstNotExecuted;
    while (m_firstNotExecuted > iNewFirstNotExec)
    {
        --m_firstNotExecuted;
        Command *cmd = m_commands[m_firstNotExecuted];
        ASSERT(cmd->isExecuted(), "Bug with m_firstNotExecuted");
        cmd->undo();
    }
    return oldVal;
}


void Turn::dropFrom(unsigned iFirstToDrop)
{
    ASSERT(iFirstToDrop < m_commands.size(), "Invalid index");
    LOG_DEBUG("Deleting turn commands, starting from index " << iFirstToDrop);

    undoTo(iFirstToDrop);
    while (m_commands.size() > iFirstToDrop)
    {
        delete m_commands.back();
        m_commands.pop_back();
    }
    ASSERT(m_firstNotExecuted <= m_commands.size(), "Invalid state");
}


wstring Turn::toString() const
{
    wostringstream oss;
    BOOST_FOREACH(Command *cmd, m_commands)
    {
        oss << endl << L"  "
            << (cmd->isExecuted() ? L"| " : L"  " )
            << (cmd->isAutoExecutable() ? L"* " : L"  ")
            << (cmd->isHumanIndependent() ? L"  " : L"H ")
            << cmd->toString();
    }
    return oss.str();
}


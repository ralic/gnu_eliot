/*****************************************************************************
 * Eliot
 * Copyright (C) 2012 Olivier Teulière
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

#include "config.h"
#if ENABLE_NLS
#   include <libintl.h>
#   define _(String) gettext(String)
#else
#   define _(String) String
#endif

#include "topping.h"
#include "dic.h"
#include "tile.h"
#include "settings.h"
#include "rack.h"
#include "results.h"
#include "pldrack.h"
#include "player.h"
#include "turn.h"
#include "cmd/topping_move_cmd.h"
#include "cmd/player_rack_cmd.h"
#include "cmd/player_move_cmd.h"
#include "cmd/player_event_cmd.h"
#include "cmd/game_move_cmd.h"
#include "encoding.h"

#include "debug.h"


INIT_LOGGER(game, Topping);


Topping::Topping(const GameParams &iParams, const Game *iMasterGame)
    : Game(iParams, iMasterGame)
{
}


void Topping::start()
{
    ASSERT(getNPlayers(), "Cannot start a game without any player");

    // Complete the rack
    try
    {
        const PlayedRack &newRack =
            helperSetRackRandom(getHistory().getCurrentRack(), true, RACK_NEW);
        // FIXME: we assign an empty move to the player. It is not very clean,
        // but at the moment it is needed to avoid a crash in History::addPenalty()
        // when the player uses hints in the first turn.
        // The actual bug lies probably in the design of the History class.
        //
        // The unfortunate consequence is that there will be 2 PlayerMoveCmd
        // commands for the player, at every turn. Deleting the empty move is
        // not possible (it would crash in the same way), and replacing it
        // breaks the history of the game when going back to the previous turn.
        // At the moment, having 2 PlayerMoveCmd doesn't seem to cause any
        // particular problem, but it would be better to fix that
        // nevertheless...
        setGameAndPlayersRack(newRack, true);
    }
    catch (EndGameException &e)
    {
        endGame();
        return;
    }
}


void Topping::tryWord(const wstring &iWord, const wstring &iCoord, int iElapsed)
{
    m_board.removeTestRound();

    // Perform all the validity checks, and fill a move.
    // We don't really care if the move is valid or not.
    Move move;
    checkPlayedWord(iCoord, iWord, move);

    // Record the try
    LOG_INFO("Player " << m_currPlayer << " plays topping move after " <<
             iElapsed << "s: " << lfw(move.toString()));
    Command *pCmd = new ToppingMoveCmd(m_currPlayer, move, iElapsed);
    accessNavigation().addAndExecute(pCmd);

    // Find the best score
    int bestScore = getTopScore();
    LOG_DEBUG("Top score to be found: " << bestScore);
    if (bestScore < 0)
    {
        endGame();
        return;
    }
    ASSERT(move.getScore() <= bestScore, "The player found better than the top");

    if (move.getScore() < bestScore)
    {
        LOG_INFO("End of the game");
    }
    else
    {
        // End the turn
        recordPlayerMove(move, *m_players[m_currPlayer], iElapsed);

        // Next turn
        endTurn();
    }
}


void Topping::turnTimeOut(int iElapsed)
{
    LOG_INFO("Timeout reached, finishing turn automatically");

    m_board.removeTestRound();

    // Retrieve some settings
    bool giveElapsedPenalty = Settings::Instance().getBool("topping.elapsed-penalty");
    int timeoutPenalty = Settings::Instance().getInt("topping.timeout-penalty");

    // Compute the points to give to the player
    int points = timeoutPenalty;
    if (giveElapsedPenalty)
        points += iElapsed;

    // The player didn't find the move
    Command *pCmd = new PlayerMoveCmd(*m_players[m_currPlayer], Move(points));
    accessNavigation().addAndExecute(pCmd);

    // Next turn
    endTurn();
}


void Topping::addPenalty(int iPenalty)
{
    Command *pCmd = new PlayerEventCmd(*m_players[m_currPlayer],
                                       PlayerEventCmd::PENALTY, iPenalty);
    accessNavigation().addAndExecute(pCmd);
}


int Topping::play(const wstring &, const wstring &)
{
    ASSERT(false, "The play() method should not be called in topping mode");
    throw GameException("The play() method should not be called in topping mode. Please use tryWord() instead.");

    return 0;
}


void Topping::recordPlayerMove(const Move &iMove, Player &ioPlayer, int iElapsed)
{
    // Compute the penalty points to give to the player
    bool giveElapsedPenalty = Settings::Instance().getBool("topping.elapsed-penalty");
    int points = giveElapsedPenalty ? iElapsed : 0;

    ASSERT(iMove.isValid(), "Only valid rounds should be played");
    // Modify the score of the given move, to be the computed score
    Round copyRound = iMove.getRound();
    copyRound.setPoints(points);
    Move newMove(copyRound);

    // Update the rack and the score of the current player
    // PlayerMoveCmd::execute() must be called before Game::helperPlayMove()
    // (called in this class in endTurn()).
    // See the big comment in game.cpp, line 96
    Command *pCmd = new PlayerMoveCmd(ioPlayer, newMove);
    accessNavigation().addAndExecute(pCmd);
}


bool Topping::isFinished() const
{
    return !canDrawRack(m_players[0]->getHistory().getCurrentRack(), true);
}


void Topping::endTurn()
{
    // Play the top move on the board
    const Move &move = getTopMove();
    Command *pCmd = new GameMoveCmd(*this, move);
    accessNavigation().addAndExecute(pCmd);
    accessNavigation().newTurn();

    // Make sure that the player has the correct rack
    // (in case he didn't find the top, or not the same one)
    Command *pCmd2 = new PlayerRackCmd(*m_players[m_currPlayer],
                getHistory().getCurrentRack());
    accessNavigation().addAndExecute(pCmd2);

    // Start next turn...
    start();
}


void Topping::endGame()
{
    LOG_INFO("End of the game");
}


void Topping::addPlayer(Player *iPlayer)
{
    ASSERT(getNPlayers() == 0,
           "Only one player can be added in Topping mode");
    // Force the name of the player
    iPlayer->setName(wfl(_("Topping")));
    Game::addPlayer(iPlayer);
}


Move Topping::getTopMove() const
{
    // Find the most interesting top
    MasterResults results(getBag());
    results.search(getDic(), getBoard(), getHistory().getCurrentRack().getRack(),
                   getHistory().beforeFirstRound());
    ASSERT(!results.isEmpty(), "No top move found");

    return Move(results.get(0));
}


int Topping::getTopScore() const
{
    return getTopMove().getScore();
}


vector<Move> Topping::getTriedMoves() const
{
    vector<Move> results;
    const vector<const ToppingMoveCmd*> &cmdVect =
        getNavigation().getCurrentTurn().findAllMatchingCmd<ToppingMoveCmd>();
    BOOST_FOREACH(const ToppingMoveCmd * cmd, cmdVect)
    {
        results.push_back(cmd->getMove());
    }
    return results;
}



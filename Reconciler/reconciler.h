#pragma once

#include "../utility.h"
#include "../PokerLibrary/constants.h"
#include "../MersenneTwister.h"
#include "../PokerPlayerMFC/botapi.h"
#include <vector>
#include <string>
#include <poker_defs.h>
#include "MyCardMask.h"
using std::vector;
using std::string;

class Reconciler
{
public:
	Reconciler( const std::string & reconcileagent, MTRand::uint32 seed, bool forceactions )
		: m_forceactions( forceactions )
		, m_bot( reconcileagent, false, seed )
	{ }
	void setnewgame(uint64 gamenumber, Player playernum, MyCardMask cards, double sblind, double bblind, double stacksize);
	void setnextround(int gr, MyCardMask newboard, double newpot);
	void doaction(Player player, Action a, double amount);
	void endofgame( );
	void endofgame( MyCardMask cards );
private:
	const bool m_forceactions;
	Player m_me;
	BotAPI m_bot;
};

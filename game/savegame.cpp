//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name savegame.c	-	Save game. */
//
//	(c) Copyright 2001,2002 by Lutz Sammer, Andreas Arens
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; version 2 dated June, 1991.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "stratagus.h"

#include "icons.h"
#include "ui.h"
#include "construct.h"
#include "unittype.h"
#include "unit.h"
#include "upgrade.h"
#include "depend.h"
#include "interface.h"
#include "missile.h"
#include "tileset.h"
#include "map.h"
#include "player.h"
#include "ai.h"
#include "campaign.h"
#include "trigger.h"
#include "settings.h"
#include "iolib.h"
#include "spells.h"

#include "ccl.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Save a game to file.
**
**	@param filename	File name to be stored.
**
**	@note	Later we want to store in a more compact binary format.
*/
global void SaveGame(const char* filename)
{
    time_t now;
    CLFile* file;
    char* s;
    char* s1;

    file = CLopen(filename, CL_WRITE_GZ | CL_OPEN_WRITE);
    if (!file) {
	fprintf(stderr, "Can't save to `%s'\n", filename);
	return;
    }

    time(&now);
    s = ctime(&now);
    if ((s1 = strchr(s, '\n'))) {
	*s1 = '\0';
    }

    //
    //	Parseable header
    //
    CLprintf(file, ";;;(save-game\n");
    CLprintf(file, ";;;  'comment\t\"Generated by Stratagus Version " VERSION "\"\n");
    CLprintf(file, ";;;  'comment\t\"Visit http://Stratagus.Org for more informations\"\n");
    CLprintf(file, ";;;  'comment\t\"$Id$\"\n");
    CLprintf(file, ";;;  'type\t\"%s\"\n", "single-player");
    CLprintf(file, ";;;  'date\t\"%s\"\n", s);
    CLprintf(file, ";;;  'map\t\"%s\"\n", TheMap.Description);
    CLprintf(file, ";;;  'engine\t'(%d %d %d)\n",
	StratagusMajorVersion, StratagusMinorVersion, StratagusPatchLevel);
    CLprintf(file, ";;;  'savefile\t'(%d %d %d)\n",
	StratagusMajorVersion, StratagusMinorVersion, StratagusPatchLevel);
    // Save media type
    {
	SCM var;

	CLprintf(file, ";;;  'media\t'");
	var = gh_symbol2scm("media-version");
	// FIXME : guile todo
	//if (symbol_boundp(var, NIL)) {
	//    var = symbol_value(var, NIL);
	//    lprin1CL(var, file);
	//} else {
	    CLprintf(file, "nil");
	//}
    }
    CLprintf(file, "\n;;;  'preview\t\"%s.pam\"\n", filename);
    CLprintf(file, ";;;  )\n");

    // FIXME: probably not the right place for this
    CLprintf(file, "(set-game-cycle! %lu)\n", GameCycle);

    SaveCcl(file);
    SaveIcons(file);
    SaveCursors(file);
    SaveUserInterface(file);
    SaveTilesets(file);
    SaveConstructions(file);
    SaveDecorations(file);
    SaveMissileTypes(file);
    SaveUnitTypeDefs(file);
    SaveSpells(file);
    SaveUnitTypes(file);
    SaveUpgrades(file);
    SaveDependencies(file);
    SaveButtons(file);
    SavePlayers(file);
    SaveMap(file);
    SaveUnits(file);
    SaveAi(file);
    SaveSelections(file);
    SaveGroups(file);
    SaveMissiles(file);
    SaveTriggers(file);
    SaveCampaign(file);
    SaveObjectives(file);
    // FIXME: find all state information which must be saved.

    CLclose(file);
}

//@}

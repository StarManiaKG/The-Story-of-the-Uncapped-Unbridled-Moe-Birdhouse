// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2018-2020 by Kart Krew.
// Copyright (C) 2023 by StarManiaKG
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  discord.h
/// \brief Discord Rich Presence handling

#ifdef HAVE_DISCORDRPC // HAVE_DISCORDRPC

#include <time.h>

#include "i_system.h"
#include "d_clisrv.h"
#include "d_netcmd.h"
#include "i_net.h"
#include "g_game.h"
#include "p_tick.h"
#include "m_menu.h" // gametype_cons_t and discord custom string pointers and jukebox stuff and things like that
#include "r_things.h" // skins
#include "mserv.h" // cv_advertise and ms_RoomID
#include "m_cond.h" // queries about emblems
#include "p_local.h" // stplyr
#include "z_zone.h"
#include "byteptr.h"
#include "stun.h"
#include "i_tcp.h" // current_port
#include "discord.h" // duh
#include "doomdef.h"
#include "w_wad.h" // numwadfiles
#include "d_netfil.h" // nameonly
#include "doomstat.h" // savemoddata
#include "dehacked.h" // titlechanged

// Please feel free to provide your own Discord app if you're making a new custom build :)
#define DISCORD_APPID "503531144395096085"

// length of IP strings
#define IP_SIZE 21

static CV_PossibleValue_t statustype_cons_t[] = {
    {0, "All"},
    {1, "Only Characters"},
    {2, "Only Score"},
    {3, "Only Emblems"},
    {4, "Only Levels"},
    {5, "Only Statuses"},
    {6, "Only Playtime"},
    {7, "Custom"},
    {0, NULL}};
static CV_PossibleValue_t characterimagetype_cons_t[] = {{0, "CS Portrait"}, {1, "Continue Sprite"}, {0, NULL}}; //{2, "Life Icon Sprite"},

// Custom Discord Status Image Type //
static CV_PossibleValue_t customimagetype_cons_t[] = {
	{0, "CS Portraits"},
	{1, "Continue Sprites"},
	{2, "Maps"},
	{3, "Miscellaneous"},
	{4, "None"},
	{0, NULL}};
static CV_PossibleValue_t customcharacterimage_cons_t[] = { // Characters //
    // Vanilla Chars
    {0, "Default"}, //does ghost sonic count as a vanilla char? maybe.
    {1, "Sonic"},
    {2, "Tails"},
    {3, "Knuckles"},
    {4, "Metal Sonic"},
	{5, "Eggman"},
    
	//Custom Chars
    {6, "Flicky"},
	{7, "Motobug"},
	{8, "Amy"},
    {9, "Mighty"},
    {10, "Ray"},
	{11, "Espio"},
	{12, "Vector"},
	{13, "Chao"},
    {14, "Gamma"},
    {15, "Chaos"},
	{16, "Shadow"},
	{17, "Rouge"},
	{18, "Hero Chao"},
	{19, "Dark Chao"},
	{20, "Cream"},
	{21, "Omega"},
	{22, "Blze"},
	{23, "Silver"},
	{24, "Wonder Boy"},
	{25, "Arle"},
	{26, "Nights"},
	{27, "Sakura"},
	{28, "Ulala"},
	{29, "Beat"},
	{30, "Vyse"},
	{31, "Aiai"},
	{32, "Kiryu"},
	{33, "Aigis"},
	{34, "Miku"},
	{35, "Doom"},
    {0, NULL}};
static CV_PossibleValue_t custommapimage_cons_t[] = {{0, "MIN"}, {69, "MAX"}, {0, NULL}};
static CV_PossibleValue_t custommiscimage_cons_t[] = { // Miscellanious //
	{0, "Default"},
	
	// Intro Stuff
	{1, "Intro 1"},
	/*{2, "Intro 2"},
	{3, "Intro 3"},
	{4, "Intro 4"},
	{5, "Intro 5"},
	{6, "Intro 6"},
	{7, "Intro 7"},
	{8, "Intro 8"},
	
	// Alternate Images
	{9, "Alt. Sonic Image 1"},
	{10, "Alt. Sonic Image 2"},
	{11, "Alt. Sonic Image 3"},
	{12, "Alt. Sonic Image 4"},
	{13, "Alt. Tails Image 1"},
	{14, "Alt. Tails Image 2"},
	{15, "Alt. Knuckles Image 1"},
	{16, "Alt. Knuckles Image 2"},
	{17, "Alt. Amy Image 1"},
	{18, "Alt. Fang Image 1"},
	{19, "Alt. Metal Sonic Image 1"},
	{20, "Alt. Metal Sonic Image 2"},
	{21, "Alt. Eggman Image 1"},
	*/
	{0, NULL}};

                                                 ////////////////////////////
                                                //    Discord Commands    //
                                                ////////////////////////////
consvar_t cv_discordrp = {"discordrp", "On", CV_SAVE|CV_CALL, CV_OnOff, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_discordstreamer = {"discordstreamer", "Off", CV_SAVE|CV_CALL, CV_OnOff, DRPC_UpdatePresence, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_discordasks = {"discordasks", "Yes", CV_SAVE|CV_CALL, CV_OnOff, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_discordstatusmemes = {"discordstatusmemes", "Yes", CV_SAVE|CV_CALL, CV_OnOff, DRPC_UpdatePresence, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_discordshowonstatus = {"discordshowonstatus", "All", CV_SAVE|CV_CALL, statustype_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_discordcharacterimagetype = {"discordcharacterimagetype", "CS Portrait", CV_SAVE|CV_CALL, characterimagetype_cons_t, DRPC_UpdatePresence, 0, NULL, NULL, 0, 0, NULL};
//// Custom Discord Status Things ////
consvar_t cv_customdiscorddetails = {"customdiscorddetails", "I'm Feeling Good!", CV_SAVE|CV_CALL, NULL, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_customdiscordstate = {"customdiscordstate", "I'm Playing Sonic Robo Blast 2!", CV_SAVE|CV_CALL, NULL, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
// Custom Discord Status Image Type
consvar_t cv_customdiscordlargeimagetype = {"customdiscordlargeimagetype", "CS Portraits", CV_SAVE|CV_CALL, customimagetype_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_customdiscordsmallimagetype = {"customdiscordsmallimagetype", "Continue Sprites", CV_SAVE|CV_CALL, customimagetype_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
// Custom Discord Status Images
    // Characters //
consvar_t cv_customdiscordlargecharacterimage = {"customdiscordlargecharacterimage", "Sonic", CV_SAVE|CV_CALL, customcharacterimage_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_customdiscordsmallcharacterimage = {"customdiscordsmallimage", "Tails", CV_SAVE|CV_CALL, customcharacterimage_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
    // Maps //
consvar_t cv_customdiscordlargemapimage = {"customdiscordlargemapimage", "MIN", CV_SAVE|CV_CALL, custommapimage_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_customdiscordsmallmapimage = {"customdiscordsmallmapimage", "MAX", CV_SAVE|CV_CALL, custommapimage_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
    // Miscellanious //
consvar_t cv_customdiscordlargemiscimage = {"customdiscordlargemiscimage", "Default", CV_SAVE|CV_CALL, custommiscimage_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_customdiscordsmallmiscimage = {"customdiscordsmallmiscimage", "Intro 1", CV_SAVE|CV_CALL, custommiscimage_cons_t, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
    // Captions //
consvar_t cv_customdiscordlargeimagetext = {"customdiscordlargeimagetext", "My Favorite Character!", CV_SAVE|CV_CALL, NULL, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_customdiscordsmallimagetext = {"customdiscordsmallimagetext", "My Other Favorite Character!", CV_SAVE|CV_CALL, NULL, Discord_option_Onchange, 0, NULL, NULL, 0, 0, NULL};

struct discordInfo_s discordInfo;

discordRequest_t *discordRequestList = NULL;

static char self_ip[IP_SIZE];

/*--------------------------------------------------
	static char *DRPC_XORIPString(const char *input)

		Simple XOR encryption/decryption. Not complex or
		very secretive because we aren't sending anything
		that isn't easily accessible via our Master Server anyway.
--------------------------------------------------*/
static char *DRPC_XORIPString(const char *input)
{
	const UINT8 xor[IP_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
	char *output = malloc(sizeof(char) * (IP_SIZE+1));
	
	UINT8 i;

	for (i = 0; i < IP_SIZE; i++)
	{
		char xorinput;
	
		if (!input[i])
			break;

		xorinput = input[i] ^ xor[i];

		if (xorinput < 32 || xorinput > 126)
			xorinput = input[i];

		output[i] = xorinput;
	}
	output[i] = '\0';

	return output;
}

/*--------------------------------------------------
	static void DRPC_HandleReady(const DiscordUser *user)

		Callback function, ran when the game connects to Discord.

	Input Arguments:-
		user - Struct containing Discord user info.

	Return:-
		None
--------------------------------------------------*/
char discordUserName[64] = "None";
static void DRPC_HandleReady(const DiscordUser *user)
{
	snprintf(discordUserName, 64, "%s", user->username);
	
	(cv_discordstreamer.value ? CONS_Printf("Discord: connected to %s\n", user->username) : CONS_Printf("Discord: connected to %s#%s (%s)\n", user->username, user->discriminator, user->userId));
}

/*--------------------------------------------------
	static void DRPC_HandleDisconnect(int err, const char *msg)

		Callback function, ran when disconnecting from Discord.

	Input Arguments:-
		err - Error type
		msg - Error message

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleDisconnect(int err, const char *msg)
{
	snprintf(discordUserName, 5, "None");

	CONS_Printf("Discord: disconnected (%d: %s)\n", err, msg);
}

/*--------------------------------------------------
	static void DRPC_HandleError(int err, const char *msg)

		Callback function, ran when Discord outputs an error.

	Input Arguments:-
		err - Error type
		msg - Error message

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleError(int err, const char *msg)
{
	CONS_Alert(CONS_WARNING, "Discord error (%d: %s)\n", err, msg);
}

/*--------------------------------------------------
	static void DRPC_HandleJoin(const char *secret)

		Callback function, ran when Discord wants to
		connect a player to the game via a channel invite
		or a join request.

	Input Arguments:-
		secret - Value that links you to the server.

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleJoin(const char *secret)
{
	COM_BufAddText(va("connect \"%s\"\n", DRPC_XORIPString(secret)));

	M_ClearMenus(true); //Don't have menus open during connection screen
	if (demo.playback && demo.title)
		G_CheckDemoStatus(); //Stop the title demo, so that the connect command doesn't error if a demo is playing

	CONS_Printf("Connecting to %s via Discord\n", DRPC_XORIPString(secret));
}

/*--------------------------------------------------
	static boolean DRPC_InvitesAreAllowed(void)

		Determines whenever or not invites or
		ask to join requests are allowed.

	Input Arguments:-
		None

	Return:-
		true if invites are allowed, false otherwise.
--------------------------------------------------*/
static boolean DRPC_InvitesAreAllowed(void)
{
	if (!cv_discordasks.value)
		return false; // Client Doesn't Allow Ask to Join

	if (discordInfo.whoCanInvite > 1)
		return false; // Client has the CVar set to off, so never allow invites from this client.

	if (!Playing())
		return false; // We're not playing, so we should not be getting invites.
	
	if (discordInfo.joinsAllowed || cv_allownewplayer.value) //hack, since discordInfo.joinsAllowed doesn't work
	{
		if (!discordInfo.whoCanInvite && (consoleplayer == serverplayer || IsPlayerAdmin(consoleplayer)))
			return true; // Only admins are allowed!
		else if (discordInfo.whoCanInvite)
			return true; // Everyone's allowed!
	}

	return false; // Did not pass any of the checks
}

/*--------------------------------------------------
	static void DRPC_HandleJoinRequest(const DiscordUser *requestUser)

		Callback function, ran when Discord wants to
		ask the player if another Discord user can join
		or not.

	Input Arguments:-
		requestUser - DiscordUser struct for the user trying to connect.

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleJoinRequest(const DiscordUser *requestUser)
{
	discordRequest_t *append = discordRequestList;
	discordRequest_t *newRequest;

	if (!DRPC_InvitesAreAllowed())
	{
		// Something weird happened if this occurred...
		Discord_Respond(requestUser->userId, DISCORD_REPLY_IGNORE);
		return;
	}

	newRequest = Z_Calloc(sizeof(discordRequest_t), PU_STATIC, NULL);

	newRequest->username = Z_Calloc(344, PU_STATIC, NULL);
	snprintf(newRequest->username, 344, "%s", requestUser->username);

	newRequest->discriminator = Z_Calloc(8, PU_STATIC, NULL);
	snprintf(newRequest->discriminator, 8, "%s", requestUser->discriminator);

	newRequest->userID = Z_Calloc(32, PU_STATIC, NULL);
	snprintf(newRequest->userID, 32, "%s", requestUser->userId);

	if (append != NULL)
	{
		discordRequest_t *prev = NULL;

		while (append != NULL)
		{
			// CHECK FOR DUPES!! Ignore any that already exist from the same user.
			if (!strcmp(newRequest->userID, append->userID))
			{
				Discord_Respond(newRequest->userID, DISCORD_REPLY_IGNORE);
				DRPC_RemoveRequest(newRequest);
				return;
			}

			prev = append;
			append = append->next;
		}

		newRequest->prev = prev;
		prev->next = newRequest;
	}
	else
	{
		discordRequestList = newRequest;
		M_RefreshPauseMenu();
	}

	// Made it to the end, request was valid, so play the request sound :)
	S_StartSound(NULL, sfx_requst);
}

/*--------------------------------------------------
	void DRPC_RemoveRequest(discordRequest_t *removeRequest)

		See header file for description.
--------------------------------------------------*/
void DRPC_RemoveRequest(discordRequest_t *removeRequest)
{
	if (removeRequest->prev != NULL)
	{
		removeRequest->prev->next = removeRequest->next;
	}

	if (removeRequest->next != NULL)
	{
		removeRequest->next->prev = removeRequest->prev;

		if (removeRequest == discordRequestList)
		{
			discordRequestList = removeRequest->next;
		}
	}
	else
	{
		if (removeRequest == discordRequestList)
		{
			discordRequestList = NULL;
		}
	}

	Z_Free(removeRequest->username);
	Z_Free(removeRequest->userID);
	Z_Free(removeRequest);
}

/*--------------------------------------------------
	void DRPC_Init(void)

		See header file for description.
--------------------------------------------------*/
void DRPC_Init(void)
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	handlers.ready = DRPC_HandleReady;
	handlers.disconnected = DRPC_HandleDisconnect;
	handlers.errored = DRPC_HandleError;
	handlers.joinGame = DRPC_HandleJoin;
	handlers.joinRequest = DRPC_HandleJoinRequest;

	Discord_Initialize(DISCORD_APPID, &handlers, 1, NULL);
	
	I_AddExitFunc(Discord_Shutdown);
	I_AddExitFunc(DRPC_ShutDown);
	
	DRPC_UpdatePresence();
}

/*--------------------------------------------------
	static void DRPC_GotServerIP(UINT32 address)

		Callback triggered by successful STUN response.

	Input Arguments:-
		address - IPv4 address of this machine, in network byte order.

	Return:-
		None
--------------------------------------------------*/
static void DRPC_GotServerIP(UINT32 address)
{
	const unsigned char * p = (const unsigned char *)&address;
	sprintf(self_ip, "%u.%u.%u.%u:%u", p[0], p[1], p[2], p[3], current_port);
}

/*--------------------------------------------------
	static const char *DRPC_GetServerIP(void)

		Retrieves the IP address of the server that you're
		connected to. Will attempt to use STUN for getting your
		own IP address.
--------------------------------------------------*/
static const char *DRPC_GetServerIP(void)
{
	const char *address; 

	// If you're connected
	if (I_GetNodeAddress && (address = I_GetNodeAddress(servernode)) != NULL)
	{
		if (strcmp(address, "self"))
		{
			// We're not the server, so we could successfully get the IP!
			// No need to do anything else :)
			sprintf(self_ip, "%s:%u", address, current_port);
			return self_ip;
		}
	}

	if (self_ip[0])
	{
		return self_ip;
	}
	else
	{
		// There happens to be a good way to get it after all! :D
		STUN_bind(DRPC_GotServerIP);
		return NULL;
	}
}

/*--------------------------------------------------
	static void DRPC_EmptyRequests(void)

		Empties the request list. Any existing requests
		will get an ignore reply.
--------------------------------------------------*/
static void DRPC_EmptyRequests(void)
{
	while (discordRequestList != NULL)
	{
		Discord_Respond(discordRequestList->userID, DISCORD_REPLY_IGNORE);
		DRPC_RemoveRequest(discordRequestList);
	}
}

/*--------------------------------------------------
	void DRPC_UpdatePresence(void)

		See header file for description.
--------------------------------------------------*/
void DRPC_UpdatePresence(void)
{
	char detailstr[64+26+15+23] = "";
	char statestr[64+26+15+25] = "";

	char mapimg[8+1] = "";
	char mapname[5+21+21+2+1] = "";

	char charimg[4+SKINNAMESIZE+1] = "";
	char charimgS[4+SKINNAMESIZE+1] = "";
	char charname[11+SKINNAMESIZE+1] = "";
	char charnameS[11+SKINNAMESIZE+1] = "";

	char servertype[15+10] = "";

	static const char *supportedSkins[] = { // Supported Skin Pictures
		// Vanilla
		"sonic",
		"tails",
		"knuckles",
		"eggman",
		"metalsonic",
		
		// Custom
		"flicky",
		"motobug",
		"amy",
		"mighty",
		"ray",
		"espio",
		"vector",
		"chao",
		"gamma",
		"chaos",
		"shadow",
		"rouge",
		"herochao",
		"darkchao",
		"cream",
		"omega",
		"blaze",
		"silver",
		"wonderboy",
		"arle",
		"nights",
		"sakura",
		"ulala",
		"beat",
		"vyse",
		"aiai",
		"kiryu",
		"aigis",
		"miku",
		"doom",
		NULL
	};

	//nerd emoji moment
	char detailGrammar[1+2] = "";
	
	char stateGrammar[2+2] = "";
	char stateType[10+9+5] = "";

	char lifeType[9+10+2+7] = "";
	char lifeGrammar[9+10+2+3+4] = "";

	char spectatorType[9+10] = "";
	char spectatorGrammar[2+3] = "";

	char gametypeGrammar[2+3+1+9] = "";
	char gameType[2+3+8+9] = "";

	char charImageType[2+2+1] = "";

	// custom discord things from menu.c that i had to redeclare here because i do not know much about c
	static const char *customStringType[] = {
		"char",
		"cont",
		"map",
		"misc",
		NULL
	};
	
	// Iterators
	INT32 i;

	// Booleans
	boolean joinSecretSet = false;

	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	//// NO STATUS? ////
	if (!cv_discordrp.value)
	{
		// Since The User Doesn't Want To Show Their Status, This Just Shows That They're Playing SRB2. (If that's too much, then they should just disable game activity :V)
		// However, Now it also shows a few predetermined states, thanks to Star :)
		discordPresence.largeImageKey = "misctitle";
		discordPresence.largeImageText = "Sonic Robo Blast 2";
		discordPresence.details = "In Game";
		discordPresence.state = (paused ? "Currently Paused" : ((menuactive || !Playing() ? "In The Menu" : "Actively Playing")));

		DRPC_EmptyRequests();
		Discord_RunCallbacks();
		Discord_UpdatePresence(&discordPresence);
		return;
	}

#ifdef DEVELOP
	// This way, we can use the invite feature in-dev, but not have snoopers seeing any potential secrets :P
	discordPresence.largeImageKey = "miscdevelop";
	discordPresence.largeImageText = "No peeking!";
	discordPresence.details = "Developing a Masterpiece!";
	discordPresence.state = "Keep your Eyes Peeled!";
	
	DRPC_EmptyRequests();
	Discord_RunCallbacks();
	Discord_UpdatePresence(&discordPresence);
	return;
#endif // DEVELOP

	// Reset Discord Info/Presence for Clients Compiled Without HAVE_DISCORDRPC, so You Never Receieve Bad Information From Other Players!
    memset(&discordInfo, 0, sizeof(discordInfo));
	
	////////////////////////////////////////////
	////   Main Rich Presence Status Info   ////
	////////////////////////////////////////////
	if (dedicated || netgame || splitscreen || !multiplayer)
	{
		//// SERVER INFO ////
		if ((dedicated || netgame) && Playing())
		{
			if (DRPC_InvitesAreAllowed())
			{
				const char *join;

				// Grab the host's IP for joining.
				if ((join = DRPC_GetServerIP()))
				{
					discordPresence.joinSecret = DRPC_XORIPString(join);
					joinSecretSet = true;
				}
			}

			snprintf(servertype, 26, (cv_advertise.value ? "Public" : "Private"));

			if (cv_discordshowonstatus.value != 7)
				snprintf(detailstr, 60, (server ? (!dedicated ? "Hosting a %s Server" : "Hosting a Dedicated %s Server") : "In a %s Server"), servertype);
			discordPresence.partyId = server_context; // Thanks, whoever gave us Mumble support, for implementing the EXACT thing Discord wanted for this field!
			discordPresence.partySize = D_NumPlayers(); // Players in server
			discordPresence.partyMax = cv_maxplayers.value; // Max players
			discordPresence.instance = 1;
		}
		
		//// OTHER INFO ////
		//// Status Pictures ////
		if (cv_discordshowonstatus.value != 7)
		{
			if (!Playing())
			{
				discordPresence.largeImageKey = "misctitle";
				discordPresence.largeImageText = (!cv_discordshowonstatus.value ? "Title Screen" : "Sonic Robo Blast 2");
				snprintf((cv_discordshowonstatus.value == 6 ? detailstr : statestr), 25, ((!demo.playback && !demo.title) ? "Main Menu" : ((demo.playback && !demo.title) ? "Watching Replays" : ((demo.playback && demo.title) ? "Watching A Demo" : "???"))));
			}
			else if (((cv_discordshowonstatus.value == 1 || cv_discordshowonstatus.value == 5) && !Playing()) || (cv_discordshowonstatus.value != 1 && cv_discordshowonstatus.value != 5))
			{
				discordPresence.largeImageKey = "misctitle";
				discordPresence.largeImageText = "Sonic Robo Blast 2 Kart";
			}
		}
		
		//// Statuses ////
		if (cv_discordshowonstatus.value != 7 && ((Playing() && playeringame[consoleplayer] && !demo.playback) || cv_discordshowonstatus.value == 7))
		{
			//// Emblems ////
			if (!cv_discordshowonstatus.value || cv_discordshowonstatus.value == 3)
			{
				if ((!(netgame || splitscreen)) || (cv_discordshowonstatus.value))
					snprintf((!netgame ? detailstr : statestr), 128, "%d/%d Emblems", M_CountEmblems(), (numemblems + numextraemblems));
			}

			//// Score ////
			if (cv_discordshowonstatus.value == 2)
				strlcat((!netgame ? detailstr : statestr), va("Current Score: %d", players[consoleplayer].score), 128);
			
			//// SRB2 Playtime ////
			if (cv_discordshowonstatus.value == 6)
				strlcat(((Playing() && !netgame) ? detailstr : statestr), va("Total Playtime: %d Hours, %d Minutes, %d Seconds", G_TicsToHours(totalplaytime), G_TicsToMinutes(totalplaytime, false), G_TicsToSeconds(totalplaytime)), 128);

			//// Tiny Detail Things; Complete Games, etc. ////
			if (!splitscreen && !netgame)
			{
				if (cv_discordshowonstatus.value != 1 && cv_discordshowonstatus.value != 2 && cv_discordshowonstatus.value != 4 && cv_discordshowonstatus.value != 5)
					snprintf(detailGrammar, 3, ", ");

				if (gamecomplete) //You've beat the game? You Get A Special Status Then!
					strlcat(detailstr, va("%sHas Beaten the Game" , detailGrammar), 128);
			}
		}
		
		//// Apply our Info, And We're Done :) ////
		discordPresence.details = detailstr;
		discordPresence.state = statestr;
	}
	
	//// EVEN MORE INFO ////
	//// Statuses ////
	if (!cv_discordshowonstatus.value || cv_discordshowonstatus.value == 4)
	{
		if (((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && Playing() && playeringame[consoleplayer]) || (paused || menuactive || jukeboxMusicPlaying))
		{
			//// Statuses That Only Appear In-Game ////
			if (Playing())
			{
				// Modes //
				snprintf(gametypeGrammar, 20, (!ultimatemode ? "Playing " : "Taking on "));
				if (modeattacking)
					snprintf(gameType, 12, ((maptol != TOL_NIGHTS && maptol != TOL_XMAS) ? "Time Attack" : "NiGHTS Mode"));
				else
					snprintf(gameType, 24, (!splitscreen ? ((gametype == GT_COOP && !netgame) ? (!ultimatemode ? "Single-Player" : "Ultimate Mode") : "%s") : "Split-Screen"), (netgame ? gametype_cons_t[gametype].strvalue : NULL));
				
				// Mods //
				if (modifiedgame && numwadfiles > (mainwads+1))
					strlcat(gameType, ((numwadfiles - (mainwads+1) > 1) ? va(" With %d Mods", numwadfiles - (mainwads+1)) : (" With 1 Mod")), 105);
				
				// Spectators //
				if (!players[consoleplayer].spectator)
				{
					snprintf(spectatorGrammar, 4, (((displayplayers[stplyr-players] != consoleplayer) || (cv_discordstatusmemes.value && (displayplayers[stplyr-players] != consoleplayer))) ? "ing" : "er"));
					snprintf(spectatorType, 21, "View%s", spectatorGrammar);
				}
				else
				{
					snprintf(lifeGrammar, 12, ", Dead; ");
					snprintf(spectatorGrammar, 4, (((displayplayers[stplyr-players] != consoleplayer) || (cv_discordstatusmemes.value && (displayplayers[stplyr-players] == consoleplayer))) ? "ing" : "or"));
					snprintf(spectatorType, 21, "Spectat%s", spectatorGrammar);
					
					if (displayplayers[stplyr-players] == consoleplayer)
						snprintf(lifeType, 27, (!cv_discordstatusmemes.value ? "In %s Mode" : "%s Air"), spectatorType);
				}
				
				// Viewpoints //
				if (displayplayers[stplyr-players] != consoleplayer)
					snprintf(lifeType, 30, "%s %s", spectatorType, player_names[stplyr-players]);
			}

			//// Statuses That Appear Whenever ////
			// Tiny State Things; Pausing, Active Menues, etc. //
			if (paused || menuactive || jukeboxMusicPlaying)
			{
				if (!cv_discordshowonstatus.value || (cv_discordshowonstatus.value == 5 && Playing()) || !Playing())
					snprintf(stateGrammar, 3, ", ");

				snprintf(stateType, 27, (paused ? "%sCurrently Paused" : (menuactive ? "%sScrolling Through Menus" : "")), stateGrammar);
				strlcat(stateType, (jukeboxMusicPlaying ? va("%sPlaying %s in the Jukebox", stateGrammar, jukeboxMusicName) : ""), 95);
			}
			
			// Copy All Of Our Strings //
			strlcat(statestr, va("%s%s%s%s%s", gametypeGrammar, gameType, lifeGrammar, lifeType, stateType), 130);
		}
		
		// Apply String to Status //
		discordPresence.state = statestr;
	}

	//// Maps ////
	if (!cv_discordshowonstatus.value || cv_discordshowonstatus.value == 5)
	{
		if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_TITLESCREEN) // Map info
		{
			if ((gamemap >= 1 && gamemap <= 60) // Supported Race Maps
			|| (gamemap >= 136 && gamemap <= 164)) // Supported Battle Maps
			{
				snprintf(mapimg, 8, "%s", G_BuildMapName(gamemap));
				strlwr(mapimg);
				
				discordPresence.largeImageKey = mapimg; // Map image
			}
			else
				discordPresence.largeImageKey = "mapcustom";
			
			if (mapheaderinfo[gamemap-1]->menuflags & LF2_HIDEINMENU)
			{
				discordPresence.largeImageKey = "miscdice"; // Hell Map, use the method that got you here :P
				discordPresence.largeImageText = "???"; // Hell map, hide the name
			}
			else
			{
				// Find The Map Name
				snprintf(mapname, 48, (gamestate != GS_TITLESCREEN ? "%s" : "Title Screen"), (gamestate != GS_TITLESCREEN ? G_BuildMapTitle(gamemap) : 0));

				// List the Map Name
				discordPresence.largeImageText = mapname;
				
				// Display the Map's Name on our Status, Since That's What We Set
				if (cv_discordshowonstatus.value == 4)
					discordPresence.state = mapname;

				// Display The Title Screen Images, If We're on That
				if (gamestate == GS_TITLESCREEN)
					discordPresence.largeImageKey = "misctitle";
			}

			if (Playing() || paused)
			{
				const time_t currentTime = time(NULL);
				const time_t mapTimeStart = currentTime - (leveltime / TICRATE);

				discordPresence.startTimestamp = mapTimeStart;

				if (cv_timelimit.value && timelimitintics)
				{
					const time_t mapTimeEnd = mapTimeStart + ((timelimitintics + TICRATE) / TICRATE);
					discordPresence.endTimestamp = mapTimeEnd;
				}
			}
		}
		else if (gamestate == GS_VOTING) // Voting Info
		{
			discordPresence.largeImageKey = (G_BattleGametype() ? "miscredplanet" : "miscblueplanet");
			discordPresence.largeImageText = "Voting";
		}
	}

	//// Characters ////
	if ((!cv_discordshowonstatus.value || cv_discordshowonstatus.value == 1) && (Playing() && playeringame[consoleplayer]))
	{
		// Character Images/Tags //
		snprintf(charImageType, 5, (!cv_discordcharacterimagetype.value ? "char" : "cont"));
		snprintf(charimg, 11, "%scustom", charImageType);
		snprintf(charimgS, 11, ((cv_discordshowonstatus.value && ((playeringame[1] && players[1].bot) || splitscreen)) ? "%scustom" : ""), ((cv_discordshowonstatus.value && ((playeringame[1] && players[1].bot) || splitscreen)) ? charImageType : 0));
		
		// Supported Characters //
		// Main Characters //
		for (i = 0; i < 34; i++) // 34 Custom Characters Are Currently Supported :P
		{							
			if (strcmp(skins[players[consoleplayer].skin].name, supportedSkins[i]) == 0)
			{
				snprintf(charimg, 36, "%s%s", charImageType, skins[players[consoleplayer].skin].name);	
				break; // We Found Our Character!
			}
		}
		// Side Characters //
		if (cv_discordshowonstatus.value && playeringame[1] && players[1].bot)
		{
			for (i = 0; i < 23; i++) // Electric Boogalo
			{
				if (strcmp(skins[players[1].skin].name, supportedSkins[i]) == 0)
				{	
					snprintf(charimgS, 36, "%s%s", charImageType, skins[players[1].skin].name);
					break;
				}
			}
		}
		
		// Character Names //
		if (!splitscreen) // Why Would You Split My Screen
		{
			if (!players[1].bot) // No Bots //
				snprintf(charname, 75, "Playing As: %s", skins[players[consoleplayer].skin].realname);
			else // Bots //
				(!cv_discordshowonstatus.value ? snprintf(charname, 75, "Playing As: %s & %s", skins[players[consoleplayer].skin].realname, skins[players[1].skin].realname) : (snprintf(charname, 75, "Playing As: %s", skins[players[consoleplayer].skin].realname), snprintf(charnameS, 75, "& %s", skins[players[1].skin].realname)));
		}
		else // I Split my Screen
			(!cv_discordshowonstatus.value ? snprintf(charname, 50, "%s & %s", player_names[consoleplayer], player_names[1]) : (snprintf(charname, 50, "%s", player_names[consoleplayer]), snprintf(charnameS, 50, "%s", player_names[1]))); // Show Both of The Players' Names and Render Their Character Images
		
		// Apply Character Images and Names //
		(!cv_discordshowonstatus.value ? discordPresence.smallImageText = charname : (discordPresence.largeImageText = charname, discordPresence.smallImageText = charnameS)); // Character Names, And Bot Names, If They Exist
		(!cv_discordshowonstatus.value ? discordPresence.smallImageKey = charimg : (discordPresence.largeImageKey = charimg, discordPresence.smallImageKey = charimgS)); // Character images			
		
		// Also Set it On Their Status, Since They Set it To Be That Way //
		if (cv_discordshowonstatus.value)
			discordPresence.state = (strcmp(charnameS, "") != 0 ?
										// Split-Screen //
										(splitscreen ? va("%s & %s", charname, charnameS) :
										
										// No Split-Screen //
										// Bots						
										(players[2].bot ? (!players[3].bot ? va("%s %s & %s", charname, charnameS, skins[players[2].skin].realname) : va("%s %s & %s With Multiple Bots", charname, charnameS, skins[players[2].skin].realname)) : va("%s %s", charname, charnameS))) : 
										
										// No Bots
										charname);
		
	}
	
	//// Custom Statuses ////
	//// NOTE: The Main Custom Status Functions can be Found in m_menu.c! The following is just backported from there.
	if (cv_discordshowonstatus.value == 8)
	{
		discordPresence.details = cv_customdiscorddetails.string;
		discordPresence.state = cv_customdiscordstate.string;

		if (cv_customdiscordsmallimagetype.value < 2)
			discordPresence.smallImageKey = (cv_customdiscordsmallcharacterimage.value > 0 ? customSImageString : va("%scustom", customStringType[cv_customdiscordsmallimagetype.value]));
		else if (cv_customdiscordsmallimagetype.value == 2)
			discordPresence.smallImageKey = (cv_customdiscordsmallmapimage.value > 0 ? customSImageString : "map01");
		else
			discordPresence.smallImageKey = (cv_customdiscordsmallmiscimage.value > 0 ? customSImageString : "misctitle");

		if (cv_customdiscordlargeimagetype.value < 2)
			discordPresence.largeImageKey = (cv_customdiscordlargecharacterimage.value > 0 ? customLImageString : va("%scustom", customStringType[cv_customdiscordlargeimagetype.value]));
		else if (cv_customdiscordlargeimagetype.value == 2)
			discordPresence.largeImageKey = (cv_customdiscordlargemapimage.value > 0 ? customLImageString : "map01");
		else
			discordPresence.largeImageKey = (cv_customdiscordlargemiscimage.value > 0 ? customLImageString : "misctitle");

		discordPresence.smallImageText = cv_customdiscordsmallimagetext.string;
		discordPresence.largeImageText = cv_customdiscordlargeimagetext.string;
	}

	if (!joinSecretSet)
		DRPC_EmptyRequests(); // Flush the Request List, if it Exists, and We Can't Join
	
	Discord_RunCallbacks();
	Discord_UpdatePresence(&discordPresence);
}

/*--------------------------------------------------
	void DRPC_ShutDown(void)

		Clears Everything Related to Discord
		Rich Presence. Only Runs On Game Close
		or Crash.
--------------------------------------------------*/
void DRPC_ShutDown(void)
{
	// Assign a Custom Status Because We Can
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	discordPresence.details = "Currently Closing...";
	discordPresence.state = "Clearing SRB2 Discord Rich Presence...";
	
	// Empty Requests
	DRPC_EmptyRequests();

	// Close Everything Down
	Discord_ClearPresence();
	Discord_Shutdown();
}

#endif // HAVE_DISCORDRPC

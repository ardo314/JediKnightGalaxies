/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "g_local.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"
#include "ai_main.h"

char gBotChatBuffer[MAX_CLIENTS][MAX_CHAT_BUFFER_SIZE];

int GetValueGroup(char *buf, char *group, char *outbuf)
{
	char *place, *placesecond;
	int iplace;
	int failure;
	int i;
	int startpoint, startletter;
	int subg = 0;

	i = 0;

	iplace = 0;

	place = strstr(buf, group);

	if (!place)
	{
		return 0;
	}

	startpoint = place - buf + strlen(group) + 1;
	startletter = (place - buf) - 1;

	failure = 0;

	while (buf[startpoint+1] != '{' || buf[startletter] != '\n')
	{
		placesecond = strstr(place+1, group);

		if (placesecond)
		{
			startpoint += (placesecond - place);
			startletter += (placesecond - place);
			place = placesecond;
		}
		else
		{
			failure = 1;
			break;
		}
	}

	if (failure)
	{
		return 0;
	}

	//we have found the proper group name if we made it here, so find the opening brace and read into the outbuf
	//until hitting the end brace

	while (buf[startpoint] != '{')
	{
		startpoint++;
	}

	startpoint++;

	while (buf[startpoint] != '}' || subg)
	{
		if (buf[startpoint] == '{')
		{
			subg++;
		}
		else if (buf[startpoint] == '}')
		{
			subg--;
		}
		outbuf[i] = buf[startpoint];
		i++;
		startpoint++;
	}
	outbuf[i] = '\0';

	return 1;
}

int GetPairedValue(char *buf, char *key, char *outbuf)
{
	char *place, *placesecond;
	int startpoint, startletter;
	int i, found;

	if (!buf || !key || !outbuf)
	{
		return 0;
	}

	i = 0;

	while (buf[i] && buf[i] != '\0')
	{
		if (buf[i] == '/')
		{
			if (buf[i+1] && buf[i+1] != '\0' && buf[i+1] == '/')
			{
				while (buf[i] != '\n')
				{
					buf[i] = '/';
					i++;
				}
			}
		}
		i++;
	}

	place = strstr(buf, key);

	if (!place)
	{
		return 0;
	}
	//tab == 9
	startpoint = place - buf + strlen(key);
	startletter = (place - buf) - 1;

	found = 0;

	while (!found)
	{
		if (startletter == 0 || !buf[startletter] || buf[startletter] == '\0' || buf[startletter] == 9 || buf[startletter] == ' ' || buf[startletter] == '\n')
		{
			if (buf[startpoint] == '\0' || buf[startpoint] == 9 || buf[startpoint] == ' ' || buf[startpoint] == '\n')
			{
				found = 1;
				break;
			}
		}

		placesecond = strstr(place+1, key);

		if (placesecond)
		{
			startpoint += placesecond - place;
			startletter += placesecond - place;
			place = placesecond;
		}
		else
		{
			place = NULL;
			break;
		}

	}

	if (!found || !place || !buf[startpoint] || buf[startpoint] == '\0')
	{
		return 0;
	}

	while (buf[startpoint] == ' ' || buf[startpoint] == 9 || buf[startpoint] == '\n')
	{
		startpoint++;
	}

	i = 0;

	while (buf[startpoint] && buf[startpoint] != '\0' && buf[startpoint] != '\n')
	{
		outbuf[i] = buf[startpoint];
		i++;
		startpoint++;
	}

	outbuf[i] = '\0';

	return 1;
}

int BotDoChat(bot_state_t *bs, char *section, int always)
{
#ifndef __MMO__ // UQ1: Unnecessary... But we will want targetted ai to player talk on NPCs later...
	char *chatgroup;
	int rVal;
	int inc_1;
	int inc_2;
	int inc_n;
	int lines;
	int checkedline;
	int getthisline;
	gentity_t *cobject;

	if (!bs->canChat)
	{
		return 0;
	}

	if (bs->doChat)
	{ //already have a chat scheduled
		return 0;
	}

	if (trap->Cvar_VariableIntegerValue("se_language"))
	{ //no chatting unless English.
		return 0;
	}

	if (Q_irand(1, 10) > bs->chatFrequency && !always)
	{
		return 0;
	}

	bs->chatTeam = 0;

	chatgroup = (char *)malloc(MAX_CHAT_BUFFER_SIZE);

	rVal = GetValueGroup(gBotChatBuffer[bs->client], section, chatgroup);

	if (!rVal) //the bot has no group defined for the specified chat event
	{
		free(chatgroup); //chatgroup
		return 0;
	}

	inc_1 = 0;
	inc_2 = 2;

	while (chatgroup[inc_2] && chatgroup[inc_2] != '\0')
	{
		if (chatgroup[inc_2] != 13 && chatgroup[inc_2] != 9)
		{
			chatgroup[inc_1] = chatgroup[inc_2];
			inc_1++;
		}
		inc_2++;
	}
	chatgroup[inc_1] = '\0';

	inc_1 = 0;

	lines = 0;

	while (chatgroup[inc_1] && chatgroup[inc_1] != '\0')
	{
		if (chatgroup[inc_1] == '\n')
		{
			lines++;
		}
		inc_1++;
	}

	if (!lines)
	{
		free(chatgroup); //chatgroup
		return 0;
	}

	getthisline = Q_irand(0, (lines+1));

	if (getthisline < 1)
	{
		getthisline = 1;
	}
	if (getthisline > lines)
	{
		getthisline = lines;
	}

	checkedline = 1;

	inc_1 = 0;

	while (checkedline != getthisline)
	{
		if (chatgroup[inc_1] && chatgroup[inc_1] != '\0')
		{
			if (chatgroup[inc_1] == '\n')
			{
				inc_1++;
				checkedline++;
			}
		}

		if (checkedline == getthisline)
		{
			break;
		}

		inc_1++;
	}

	//we're at the starting position of the desired line here
	inc_2 = 0;

	while (chatgroup[inc_1] != '\n')
	{
		chatgroup[inc_2] = chatgroup[inc_1];
		inc_2++;
		inc_1++;
	}
	chatgroup[inc_2] = '\0';

	//trap->EA_Say(bs->client, chatgroup);
	inc_1 = 0;
	inc_2 = 0;

	if (strlen(chatgroup) > MAX_CHAT_LINE_SIZE)
	{
		free(chatgroup); //chatgroup
		return 0;
	}

	while (chatgroup[inc_1])
	{
		if (chatgroup[inc_1] == '%' && chatgroup[inc_1+1] != '%')
		{
			inc_1++;

			if (chatgroup[inc_1] == 's' && bs->chatObject)
			{
				cobject = bs->chatObject;
			}
			else if (chatgroup[inc_1] == 'a' && bs->chatAltObject)
			{
				cobject = bs->chatAltObject;
			}
			else
			{
				cobject = NULL;
			}

			if (cobject && cobject->client)
			{
				inc_n = 0;

				while (cobject->client->pers.netname[inc_n])
				{
					bs->currentChat[inc_2] = cobject->client->pers.netname[inc_n];
					inc_2++;
					inc_n++;
				}
				inc_2--; //to make up for the auto-increment below
			}
		}
		else
		{
			bs->currentChat[inc_2] = chatgroup[inc_1];
		}
		inc_2++;
		inc_1++;
	}
	bs->currentChat[inc_2] = '\0';

	if (strcmp(section, "GeneralGreetings") == 0)
	{
		bs->doChat = 2;
	}
	else
	{
		bs->doChat = 1;
	}
	bs->chatTime_stored = (strlen(bs->currentChat)*45)+Q_irand(1300, 1500);
	bs->chatTime = level.time + bs->chatTime_stored;

	free(chatgroup); //chatgroup
#endif //__MMO__

	return 1;
}

void ParseEmotionalAttachments(bot_state_t *bs, char *buf)
{
	int i = 0;
	int i_c = 0;
	char tbuf[16];

	while (buf[i] && buf[i] != '}')
	{
		while (buf[i] == ' ' || buf[i] == '{' || buf[i] == 9 || buf[i] == 13 || buf[i] == '\n')
		{
			i++;
		}

		if (buf[i] && buf[i] != '}')
		{
			i_c = 0;
			while (buf[i] != '{' && buf[i] != 9 && buf[i] != 13 && buf[i] != '\n')
			{
				bs->loved[bs->lovednum].name[i_c] = buf[i];
				i_c++;
				i++;
			}
			bs->loved[bs->lovednum].name[i_c] = '\0';

			while (buf[i] == ' ' || buf[i] == '{' || buf[i] == 9 || buf[i] == 13 || buf[i] == '\n')
			{
				i++;
			}

			i_c = 0;

			while (buf[i] != '{' && buf[i] != 9 && buf[i] != 13 && buf[i] != '\n')
			{
				tbuf[i_c] = buf[i];
				i_c++;
				i++;
			}
			tbuf[i_c] = '\0';

			bs->loved[bs->lovednum].level = atoi(tbuf);

			bs->lovednum++;
		}
		else
		{
			break;
		}

		if (bs->lovednum >= MAX_LOVED_ONES)
		{
			return;
		}

		i++;
	}
}

int ReadChatGroups(bot_state_t *bs, char *buf)
{
	char *cgroupbegin;
	int cgbplace;
	int i;

	cgroupbegin = strstr(buf, "BEGIN_CHAT_GROUPS");

	if (!cgroupbegin)
	{
		return 0;
	}

	if (strlen(cgroupbegin) >= MAX_CHAT_BUFFER_SIZE)
	{
		trap->Print(S_COLOR_RED "Error: Personality chat section exceeds max size\n");
		return 0;
	}

	cgbplace = cgroupbegin - buf+1;

	while (buf[cgbplace] != '\n')
	{
		cgbplace++;
	}

	i = 0;

	while (buf[cgbplace] && buf[cgbplace] != '\0')
	{
		gBotChatBuffer[bs->client][i] = buf[cgbplace];
		i++;
		cgbplace++;
	}

	gBotChatBuffer[bs->client][i] = '\0';

	return 1;
}

void BotUtilizePersonality(bot_state_t *bs)
{
	fileHandle_t f;
	int len, rlen;
	int failed;
	int i;
	//char buf[131072];
	char *buf = (char *)malloc(131072);
	char *readbuf, *group;

	len = trap->FS_Open(bs->settings.personalityfile, &f, FS_READ);

	failed = 0;

	if (!f)
	{
		trap->Print(S_COLOR_RED "Error: Specified personality not found\n");
		free(buf);
		return;
	}

	if (len >= 131072)
	{
		trap->Print(S_COLOR_RED "Personality file exceeds maximum length\n");
		free(buf);
		return;
	}

	trap->FS_Read(buf, len, f);

	rlen = len;

	while (len < 131072)
	{ //kill all characters after the file length, since sometimes FS_Read doesn't do that entirely (or so it seems)
		buf[len] = '\0';
		len++;
	}

	len = rlen;

	readbuf = (char *)malloc(1024);
	group = (char *)malloc(65536);

	if (!GetValueGroup(buf, "GeneralBotInfo", group))
	{
		trap->Print(S_COLOR_RED "Personality file contains no GeneralBotInfo group\n");
		failed = 1; //set failed so we know to set everything to default values
	}

	if (!failed && GetPairedValue(group, "reflex", readbuf))
	{
		bs->skills.reflex = atoi(readbuf);
	}
	else
	{
		bs->skills.reflex = 100; //default
	}

	if (!failed && GetPairedValue(group, "accuracy", readbuf))
	{
		bs->skills.accuracy = atof(readbuf);
	}
	else
	{
		bs->skills.accuracy = 10; //default
	}

	if (!failed && GetPairedValue(group, "turnspeed", readbuf))
	{
		bs->skills.turnspeed = atof(readbuf);
	}
	else
	{
		bs->skills.turnspeed = 0.01f; //default
	}

	if (!failed && GetPairedValue(group, "turnspeed_combat", readbuf))
	{
		bs->skills.turnspeed_combat = atof(readbuf);
	}
	else
	{
		bs->skills.turnspeed_combat = 0.05f; //default
	}

	if (!failed && GetPairedValue(group, "maxturn", readbuf))
	{
		bs->skills.maxturn = atof(readbuf);
	}
	else
	{
		bs->skills.maxturn = 360; //default
	}

	if (!failed && GetPairedValue(group, "perfectaim", readbuf))
	{
		bs->skills.perfectaim = atoi(readbuf);
	}
	else
	{
		bs->skills.perfectaim = 0; //default
	}

	if (!failed && GetPairedValue(group, "chatability", readbuf))
	{
		bs->canChat = atoi(readbuf);
	}
	else
	{
		bs->canChat = 0; //default
	}

	if (!failed && GetPairedValue(group, "chatfrequency", readbuf))
	{
		bs->chatFrequency = atoi(readbuf);
	}
	else
	{
		bs->chatFrequency = 5; //default
	}

	if (!failed && GetPairedValue(group, "hatelevel", readbuf))
	{
		bs->loved_death_thresh = atoi(readbuf);
	}
	else
	{
		bs->loved_death_thresh = 3; //default
	}

	if (!failed && GetPairedValue(group, "camper", readbuf))
	{
		bs->isCamper = atoi(readbuf);
	}
	else
	{
		bs->isCamper = 0; //default
	}

	if (!failed && GetPairedValue(group, "saberspecialist", readbuf))
	{
		bs->saberSpecialist = atoi(readbuf);
	}
	else
	{
		bs->saberSpecialist = 0; //default
	}

	if (!failed && GetPairedValue(group, "forceinfo", readbuf))
	{
		Com_sprintf(bs->forceinfo, sizeof(bs->forceinfo), "%s\0", readbuf);
	}
	else
	{
		Com_sprintf(bs->forceinfo, sizeof(bs->forceinfo), "%s\0", DEFAULT_FORCEPOWERS);
	}

	i = 0;

	while (i < MAX_CHAT_BUFFER_SIZE)
	{ //clear out the chat buffer for this bot
		gBotChatBuffer[bs->client][i] = '\0';
		i++;
	}

	if (bs->canChat)
	{
		if (!ReadChatGroups(bs, buf))
		{
			bs->canChat = 0;
		}
	}

	if (GetValueGroup(buf, "BotWeaponWeights", group))
	{
		if (GetPairedValue(group, "WP_STUN_BATON", readbuf))
		{
			bs->botWeaponWeights[WP_STUN_BATON] = atoi(readbuf);
			bs->botWeaponWeights[WP_MELEE] = bs->botWeaponWeights[WP_STUN_BATON];
		}

		if (GetPairedValue(group, "WP_SABER", readbuf))
		{
			bs->botWeaponWeights[WP_SABER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BRYAR_PISTOL", readbuf))
		{
			bs->botWeaponWeights[WP_BRYAR_PISTOL] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BLASTER", readbuf))
		{
			bs->botWeaponWeights[WP_BLASTER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DISRUPTOR", readbuf))
		{
			bs->botWeaponWeights[WP_DISRUPTOR] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_BOWCASTER", readbuf))
		{
			bs->botWeaponWeights[WP_BOWCASTER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_REPEATER", readbuf))
		{
			bs->botWeaponWeights[WP_REPEATER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DEMP2", readbuf))
		{
			bs->botWeaponWeights[WP_DEMP2] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_FLECHETTE", readbuf))
		{
			bs->botWeaponWeights[WP_FLECHETTE] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_ROCKET_LAUNCHER", readbuf))
		{
			bs->botWeaponWeights[WP_ROCKET_LAUNCHER] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_THERMAL", readbuf))
		{
			bs->botWeaponWeights[WP_THERMAL] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_TRIP_MINE", readbuf))
		{
			bs->botWeaponWeights[WP_TRIP_MINE] = atoi(readbuf);
		}

		if (GetPairedValue(group, "WP_DET_PACK", readbuf))
		{
			bs->botWeaponWeights[WP_DET_PACK] = atoi(readbuf);
		}
	}

	bs->lovednum = 0;

	if (GetValueGroup(buf, "EmotionalAttachments", group))
	{
		ParseEmotionalAttachments(bs, group);
	}

	free(buf);
	free(readbuf); //readbuf
	free(group); //group
	trap->FS_Close(f);
}
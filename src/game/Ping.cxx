/* bzflag
 * Copyright (c) 1993 - 2002 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <string.h>
#include <math.h>
#include <ctype.h>
#include "global.h"
#include "Ping.h"
#include "Protocol.h"
#include "TimeKeeper.h"

//
// PingPacket
//

const int				PingPacket::PacketSize = PlayerIdPLen + 52;

PingPacket::PingPacket() : gameStyle(PlainGameStyle),
								maxPlayers(1),
								maxShots(1),
								rogueCount(0),
								redCount(0),
								greenCount(0),
								blueCount(0),
								purpleCount(0),
								rogueMax(1),
								redMax(1),
								greenMax(1),
								blueMax(1),
								purpleMax(1),
								shakeWins(0),
								shakeTimeout(0),
								maxPlayerScore(0),
								maxTeamScore(0),
								maxTime(0)
{
	// do nothing
}

PingPacket::~PingPacket()
{
	// do nothing
}

void*					PingPacket::unpack(void* buf, char* version)
{
	buf = nboUnpackString(buf, version, 8);
	buf = serverId.unpack(buf);
	buf = sourceAddr.unpack(buf);
	buf = nboUnpackUShort(buf, gameStyle);
	buf = nboUnpackUShort(buf, maxPlayers);
	buf = nboUnpackUShort(buf, maxShots);
	buf = nboUnpackUShort(buf, rogueCount);
	buf = nboUnpackUShort(buf, redCount);
	buf = nboUnpackUShort(buf, greenCount);
	buf = nboUnpackUShort(buf, blueCount);
	buf = nboUnpackUShort(buf, purpleCount);
	buf = nboUnpackUShort(buf, rogueMax);
	buf = nboUnpackUShort(buf, redMax);
	buf = nboUnpackUShort(buf, greenMax);
	buf = nboUnpackUShort(buf, blueMax);
	buf = nboUnpackUShort(buf, purpleMax);
	buf = nboUnpackUShort(buf, shakeWins);
	buf = nboUnpackUShort(buf, shakeTimeout);
	buf = nboUnpackUShort(buf, maxPlayerScore);
	buf = nboUnpackUShort(buf, maxTeamScore);
	buf = nboUnpackUShort(buf, maxTime);
	return buf;
}

void*					PingPacket::pack(void* buf, const char* version) const
{
	buf = nboPackString(buf, version, 8);
	buf = serverId.pack(buf);
	buf = sourceAddr.pack(buf);
	buf = nboPackUShort(buf, gameStyle);
	buf = nboPackUShort(buf, maxPlayers);
	buf = nboPackUShort(buf, maxShots);
	buf = nboPackUShort(buf, rogueCount);
	buf = nboPackUShort(buf, redCount);
	buf = nboPackUShort(buf, greenCount);
	buf = nboPackUShort(buf, blueCount);
	buf = nboPackUShort(buf, purpleCount);
	buf = nboPackUShort(buf, rogueMax);
	buf = nboPackUShort(buf, redMax);
	buf = nboPackUShort(buf, greenMax);
	buf = nboPackUShort(buf, blueMax);
	buf = nboPackUShort(buf, purpleMax);
	buf = nboPackUShort(buf, shakeWins);
	buf = nboPackUShort(buf, shakeTimeout);	// 1/10ths of second
	buf = nboPackUShort(buf, maxPlayerScore);
	buf = nboPackUShort(buf, maxTeamScore);
	buf = nboPackUShort(buf, maxTime);
	return buf;
}

void					PingPacket::packHex(char* buf) const
{
	buf = packHex16(buf, gameStyle);
	buf = packHex16(buf, maxPlayers);
	buf = packHex16(buf, maxShots);
	buf = packHex16(buf, rogueCount);
	buf = packHex16(buf, redCount);
	buf = packHex16(buf, greenCount);
	buf = packHex16(buf, blueCount);
	buf = packHex16(buf, purpleCount);
	buf = packHex16(buf, rogueMax);
	buf = packHex16(buf, redMax);
	buf = packHex16(buf, greenMax);
	buf = packHex16(buf, blueMax);
	buf = packHex16(buf, purpleMax);
	buf = packHex16(buf, shakeWins);
	buf = packHex16(buf, shakeTimeout);
	buf = packHex16(buf, maxPlayerScore);
	buf = packHex16(buf, maxTeamScore);
	buf = packHex16(buf, maxTime);
}

void					PingPacket::unpackHex(char* buf)
{
	buf = unpackHex16(buf, gameStyle);
	buf = unpackHex16(buf, maxPlayers);
	buf = unpackHex16(buf, maxShots);
	buf = unpackHex16(buf, rogueCount);
	buf = unpackHex16(buf, redCount);
	buf = unpackHex16(buf, greenCount);
	buf = unpackHex16(buf, blueCount);
	buf = unpackHex16(buf, purpleCount);
	buf = unpackHex16(buf, rogueMax);
	buf = unpackHex16(buf, redMax);
	buf = unpackHex16(buf, greenMax);
	buf = unpackHex16(buf, blueMax);
	buf = unpackHex16(buf, purpleMax);
	buf = unpackHex16(buf, shakeWins);
	buf = unpackHex16(buf, shakeTimeout);
	buf = unpackHex16(buf, maxPlayerScore);
	buf = unpackHex16(buf, maxTeamScore);
	buf = unpackHex16(buf, maxTime);
}

void					PingPacket::repackHexPlayerCounts(
								char* buf, int* counts)
{
	buf += 2 * 2 * 3;
	buf = packHex16(buf, (uint16_t)counts[0]);
	buf = packHex16(buf, (uint16_t)counts[1]);
	buf = packHex16(buf, (uint16_t)counts[2]);
	buf = packHex16(buf, (uint16_t)counts[3]);
	buf = packHex16(buf, (uint16_t)counts[4]);
}

int						PingPacket::hex2bin(char d)
{
	switch (d) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'A':
		case 'a': return 10;
		case 'B':
		case 'b': return 11;
		case 'C':
		case 'c': return 12;
		case 'D':
		case 'd': return 13;
		case 'E':
		case 'e': return 14;
		case 'F':
		case 'f': return 15;
	}
	return 0;
}

char					PingPacket::bin2hex(int d)
{
	static const char* digit = "0123456789abcdef";
	return digit[d];
}

char*					PingPacket::packHex16(char* buf, uint16_t v)
{
	*buf++ = bin2hex((v >> 12) & 0xf);
	*buf++ = bin2hex((v >>  8) & 0xf);
	*buf++ = bin2hex((v >>  4) & 0xf);
	*buf++ = bin2hex( v        & 0xf);
	return buf;
}

char*					PingPacket::unpackHex16(char* buf, uint16_t& v)
{
	uint16_t d = 0;
	d = (d << 4) | hex2bin(*buf++);
	d = (d << 4) | hex2bin(*buf++);
	d = (d << 4) | hex2bin(*buf++);
	d = (d << 4) | hex2bin(*buf++);
	v = d;
	return buf;
}

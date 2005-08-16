/* bzflag
 * Copyright (c) 1993 - 2005 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef _MSC_VER
#pragma warning(4:4786)
#endif

/* interface header */
#include "Permissions.h"

/* system implementation headers */
#include <string>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdlib.h>

/* common implementation headers */
#include "md5.h"
#include "bzfio.h"
#include "Protocol.h"
#include "TextUtils.h"


PlayerAccessMap	groupAccess;
PlayerAccessMap	userDatabase;
PasswordMap	passwordDatabase;

uint8_t GetPlayerProperties(bool registered, bool verified, bool admin) {
  uint8_t result = 0;
  if (registered)
    result |= IsRegistered;
  if (verified)
    result |= IsVerified;
  if (admin)
    result |= IsAdmin;
  return result;
}

void setUserPassword(const std::string &nick, const std::string &pass);

PlayerAccessInfo::PlayerAccessInfo()
  : verified(false), loginTime(TimeKeeper::getCurrent()), loginAttempts (0),
    Admin(false), passwordAttempts(0)
{
  groups.push_back("EVERYONE");
}

void PlayerAccessInfo::setName(const char* callSign) {
  regName = callSign;
  makeupper(regName);
}

bool PlayerAccessInfo::gotAccessFailure() {
  bool accessFailure = loginAttempts >= 5;
  if (accessFailure)
    DEBUG1("Too Many Identifys %s\n", getName().c_str());
  return accessFailure;
}

void PlayerAccessInfo::setLoginFail() {
  loginAttempts++;
}

void PlayerAccessInfo::setPermissionRights() {
  verified = true;
  // get their real info
  PlayerAccessInfo &_info = getUserInfo(regName);
  explicitAllows = _info.explicitAllows;
  explicitDenys = _info.explicitDenys;
  groups = _info.groups;
  DEBUG1("Identify %s\n", regName.c_str());
}

void PlayerAccessInfo::reloadInfo() {
  if (verified && userExists(regName)) {
    PlayerAccessInfo accessInfo = getUserInfo(regName);
    explicitAllows = accessInfo.explicitAllows;
    explicitDenys  = accessInfo.explicitDenys;
    groups	   = accessInfo.groups;
    loginTime	   = accessInfo.loginTime;
    loginAttempts  = accessInfo.loginAttempts;
  }
}

void PlayerAccessInfo::setAdmin() {
  passwordAttempts = 0;
  Admin = true;
}

bool PlayerAccessInfo::isAdmin() const {
  return Admin;
}

bool PlayerAccessInfo::showAsAdmin() const {
  if (hasPerm(hideAdmin))
    return false;
  else if (Admin || hasPerm(ban) || hasPerm(shortBan))
    return true;
  else
    return false;
}

bool PlayerAccessInfo::passwordAttemptsMax() {
  bool maxAttempts = passwordAttempts >= 5;
  // see how many times they have tried, you only get 5
  if (!maxAttempts) {
    passwordAttempts++;
  }
  return maxAttempts;
}

std::string PlayerAccessInfo::getName() {
  return regName;
}

bool PlayerAccessInfo::hasRealPassword() {
  return checkPasswordExistence(regName.c_str());
}

bool PlayerAccessInfo::isPasswordMatching(const char* pwd) {
  return verifyUserPassword(regName.c_str(), pwd);
}

bool PlayerAccessInfo::isRegistered() const {
  return userExists(regName);
}

bool PlayerAccessInfo::isIdentifyRequired() {
  return getUserInfo(regName).hasPerm(requireIdentify);
}

bool PlayerAccessInfo::isAllowedToEnter() {
  return verified || !isRegistered() || !isIdentifyRequired();
}

bool PlayerAccessInfo::isVerified() const{
  return verified;
}

void PlayerAccessInfo::storeInfo(const char* pwd) {
  PlayerAccessInfo _info;
  _info.addGroup("VERIFIED");

  if (pwd == NULL) {
    // automatically give global users permission to use local accounts
    // since they either already have it, or there's no existing local account.
    _info.addGroup("LOCAL.GLOBAL");
    setUserPassword(regName.c_str(), "");
    DEBUG1("Global Temp Register %s\n", regName.c_str());
  } else {
    std::string pass = pwd;
    setUserPassword(regName.c_str(), pass.c_str());
    DEBUG1("Register %s %s\n", regName.c_str(), pwd);
  }
  userDatabase[regName] = _info;
  updateDatabases();
}

void PlayerAccessInfo::setPasswd(const std::string&  pwd) {
  setUserPassword(regName.c_str(), pwd.c_str());
  updateDatabases();
}

uint8_t PlayerAccessInfo::getPlayerProperties() {
  return GetPlayerProperties(isRegistered(), verified, showAsAdmin());
}

bool PlayerAccessInfo::exists() {
  return userExists(regName);
}

bool PlayerAccessInfo::hasGroup(const std::string &group)
{
  std::string str = group;
  if (!str.size())
    return false;

  makeupper(str);

  return std::find(groups.begin(), groups.end(), str) != groups.end();
}

bool PlayerAccessInfo::addGroup(const std::string &group)
{
  if (hasGroup(group))
    return false;

  std::string str = group;
  makeupper(str);

  groups.push_back(str);
  return true;
}

bool PlayerAccessInfo::removeGroup(const std::string &group)
{
  if (!hasGroup(group))
    return false;

  std::string str = group;
  makeupper(str);

  groups.erase(std::find(groups.begin(), groups.end(), str));
  return true;
}

bool PlayerAccessInfo::canSet(const std::string& group)
{
  if (hasPerm(PlayerAccessInfo::setAll))
    return true;
  return hasGroup(group) && hasPerm(PlayerAccessInfo::setPerms);
}

bool PlayerAccessInfo::hasPerm(PlayerAccessInfo::AccessPerm right) const
{
  if (Admin && (right != hideAdmin))
    return true;
  if (explicitDenys.test(right))
    return false;
  if (explicitAllows.test(right))
    return true;
	
  bool isAllowed = false;
	for (std::vector<std::string>::const_iterator itr=groups.begin(); itr!=groups.end(); ++itr) {
    PlayerAccessMap::iterator group = groupAccess.find(*itr);
    if (group != groupAccess.end()){
      if (group->second.explicitDenys.test(right))
      return false;
      else if (group->second.explicitAllows.test(right))
        isAllowed = true;
    } 
  }
  return isAllowed;
}

// grant and revoke perms used with /mute and /unmute
void PlayerAccessInfo::grantPerm(PlayerAccessInfo::AccessPerm right) 
{
  explicitAllows.set(right);
  explicitDenys.reset(right);
}

void PlayerAccessInfo::revokePerm(PlayerAccessInfo::AccessPerm right) 
{
  explicitAllows.reset(right);
  explicitDenys.set(right);
}


// custom perms are ONLY on groups
bool	PlayerAccessInfo::hasCustomPerm(const char* right) const
{
	if (Admin)
		return true;

	std::string perm = TextUtils::toupper(std::string(right));

	for (std::vector<std::string>::const_iterator itr=groups.begin(); itr!=groups.end(); ++itr) 
	{
		PlayerAccessMap::iterator group = groupAccess.find(*itr);
		if (group != groupAccess.end())
		{
			for(unsigned int i = 0; i < group->second.customPerms.size(); i++)
			{
				if ( perm == TextUtils::toupper(group->second.customPerms[i]) )
					return true;
			}
		} 
	}
	return false;
}

bool userExists(const std::string &nick)
{
  std::string str = nick;
  makeupper(str);
  PlayerAccessMap::iterator itr = userDatabase.find(str);
  if (itr == userDatabase.end())
    return false;
  return true;
}

//FIXME - check for non-existing user (throw?)
PlayerAccessInfo &PlayerAccessInfo::getUserInfo(const std::string &nick)
{
  //  if (!userExists(nick))
  //    return false;
  std::string str = nick;
  makeupper(str);
  PlayerAccessMap::iterator itr = userDatabase.find(str);
  //  if (itr == userDatabase.end())
  //    return false;
  return itr->second;
}

bool checkPasswordExistence(const std::string &nick)
{
  std::string str1 = nick;
  makeupper(str1);
  PasswordMap::iterator itr = passwordDatabase.find(str1);
  if (itr == passwordDatabase.end())
    return false;
  if (itr->second == "*" || itr->second == "")
    return false;
  return true;
}

bool verifyUserPassword(const std::string &nick, const std::string &pass)
{
  std::string str1 = nick;
  makeupper(str1);
  PasswordMap::iterator itr = passwordDatabase.find(str1);
  if (itr == passwordDatabase.end())
    return false;
  return itr->second == MD5(pass).hexdigest();
}

void setUserPassword(const std::string &nick, const std::string &pass)
{
  std::string str1 = nick;
  makeupper(str1);
  if (pass.size() == 0) {
    passwordDatabase[str1] = "*";
  } else if (pass == "*") {
    // never encode *, this would change the person's password from NULL to *.
    passwordDatabase[str1] = "*";
  } else {
    // assume it's already a hash when length is 32 (FIXME?)
    passwordDatabase[str1] = pass.size()==32 ? pass : MD5(pass).hexdigest();
  }
}

std::string nameFromPerm(PlayerAccessInfo::AccessPerm perm)
{
  switch (perm) {
    case PlayerAccessInfo::actionMessage: return "actionMessage";
    case PlayerAccessInfo::adminMessageReceive: return "adminMessageReceive";
    case PlayerAccessInfo::adminMessageSend: return "adminMessageSend";
    case PlayerAccessInfo::antiban : return "antiban";
    case PlayerAccessInfo::antideregister : return "antideregister";
    case PlayerAccessInfo::antikick : return "antikick";
    case PlayerAccessInfo::antikill : return "antikill";
    case PlayerAccessInfo::antipoll : return "antipoll";
    case PlayerAccessInfo::antipollban : return "antipollban";
    case PlayerAccessInfo::antipollkick : return "antipollkick";
    case PlayerAccessInfo::antipollkill : return "antipollkill";
    case PlayerAccessInfo::ban: return "ban";
    case PlayerAccessInfo::banlist: return "banlist";
    case PlayerAccessInfo::countdown: return "countdown";
    case PlayerAccessInfo::date: return "date";
    case PlayerAccessInfo::endGame: return "endGame";
    case PlayerAccessInfo::flagHistory: return "flagHistory";
    case PlayerAccessInfo::flagMod: return "flagMod";
    case PlayerAccessInfo::hideAdmin: return "hideAdmin";
    case PlayerAccessInfo::idleStats: return "idleStats";
    case PlayerAccessInfo::info: return "info";
    case PlayerAccessInfo::kick: return "kick";
    case PlayerAccessInfo::kill: return "kill";
    case PlayerAccessInfo::lagStats: return "lagStats";
    case PlayerAccessInfo::lagwarn: return "lagwarn";
    case PlayerAccessInfo::listPerms: return "listPerms";
    case PlayerAccessInfo::masterBan: return "masterban";
    case PlayerAccessInfo::mute: return "mute";
    case PlayerAccessInfo::playerList: return "playerList";
    case PlayerAccessInfo::poll: return "poll";
    case PlayerAccessInfo::pollBan: return "pollBan";
    case PlayerAccessInfo::pollKick: return "pollKick";
    case PlayerAccessInfo::pollKill: return "pollKill";
    case PlayerAccessInfo::pollSet: return "pollSet";
    case PlayerAccessInfo::pollFlagReset: return "pollFlagReset";
    case PlayerAccessInfo::privateMessage: return "privateMessage";
    case PlayerAccessInfo::record: return "record";
    case PlayerAccessInfo::rejoin: return "rejoin";
    case PlayerAccessInfo::removePerms: return "removePerms";
    case PlayerAccessInfo::replay: return "replay";
    case PlayerAccessInfo::requireIdentify: return "requireIdentify";
    case PlayerAccessInfo::say: return "say";
    case PlayerAccessInfo::setAll: return "setAll";
    case PlayerAccessInfo::setPassword: return "setPassword";
    case PlayerAccessInfo::setPerms: return "setPerms";
    case PlayerAccessInfo::setVar: return "setVar";
    case PlayerAccessInfo::showOthers: return "showOthers";
    case PlayerAccessInfo::shortBan: return "shortBan";
    case PlayerAccessInfo::shutdownServer: return "shutdownServer";
    case PlayerAccessInfo::spawn: return "spawn";
    case PlayerAccessInfo::superKill: return "superKill";
    case PlayerAccessInfo::talk: return "talk";
    case PlayerAccessInfo::unban: return "unban";
    case PlayerAccessInfo::unmute: return "unmute";
    case PlayerAccessInfo::veto: return "veto";
    case PlayerAccessInfo::viewReports: return "viewReports";
    case PlayerAccessInfo::vote: return "vote";
    default: return TextUtils::format("UNKNOWN_PERMISSION: %d", (int)perm).c_str();
  };
}

PlayerAccessInfo::AccessPerm permFromName(const std::string &name)
{
  if (name == "ACTIONMESSAGE") return PlayerAccessInfo::actionMessage;
  if (name == "ADMINMESSAGERECEIVE") return PlayerAccessInfo::adminMessageReceive;
  if (name == "ADMINMESSAGESEND") return PlayerAccessInfo::adminMessageSend;
  if (name == "ANTIBAN") return PlayerAccessInfo::antiban;
  if (name == "ANTIDEREGISTER") return PlayerAccessInfo::antideregister;
  if (name == "ANTIKICK") return PlayerAccessInfo::antikick;
  if (name == "ANTIKILL") return PlayerAccessInfo::antikill;
  if (name == "ANTIPOLL") return PlayerAccessInfo::antipoll;
  if (name == "ANTIPOLLBAN") return PlayerAccessInfo::antipollban;
  if (name == "ANTIPOLLKICK") return PlayerAccessInfo::antipollkick;
  if (name == "ANTIPOLLKILL") return PlayerAccessInfo::antipollkill;
  if (name == "BAN") return PlayerAccessInfo::ban;
  if (name == "BANLIST") return PlayerAccessInfo::banlist;
  if (name == "COUNTDOWN") return PlayerAccessInfo::countdown;
  if (name == "DATE") return PlayerAccessInfo::date;
  if (name == "ENDGAME") return PlayerAccessInfo::endGame;
  if (name == "FLAGHISTORY") return PlayerAccessInfo::flagHistory;
  if (name == "FLAGMOD") return PlayerAccessInfo::flagMod;
  if (name == "HIDEADMIN") return PlayerAccessInfo::hideAdmin;
  if (name == "IDLESTATS") return PlayerAccessInfo::idleStats;
  if (name == "INFO") return PlayerAccessInfo::info;
  if (name == "KICK") return PlayerAccessInfo::kick;
  if (name == "KILL") return PlayerAccessInfo::kill;
  if (name == "LAGSTATS") return PlayerAccessInfo::lagStats;
  if (name == "LAGWARN") return PlayerAccessInfo::lagwarn;
  if (name == "LISTPERMS") return PlayerAccessInfo::listPerms;
  if (name == "MASTERBAN") return PlayerAccessInfo::masterBan;
  if (name == "MUTE") return PlayerAccessInfo::mute;
  if (name == "PLAYERLIST") return PlayerAccessInfo::playerList;
  if (name == "POLL") return PlayerAccessInfo::poll;
  if (name == "POLLBAN") return PlayerAccessInfo::pollBan;
  if (name == "POLLKICK") return PlayerAccessInfo::pollKick;
  if (name == "POLLKILL") return PlayerAccessInfo::pollKill;
  if (name == "POLLSET") return PlayerAccessInfo::pollSet;
  if (name == "POLLFLAGRESET") return PlayerAccessInfo::pollFlagReset;
  if (name == "PRIVATEMESSAGE") return PlayerAccessInfo::privateMessage;
  if (name == "RECORD") return PlayerAccessInfo::record;
  if (name == "REJOIN") return PlayerAccessInfo::rejoin;
  if (name == "REMOVEPERMS") return PlayerAccessInfo::removePerms;
  if (name == "REPLAY") return PlayerAccessInfo::replay;
  if (name == "REQUIREIDENTIFY") return PlayerAccessInfo::requireIdentify;
  if (name == "SAY") return PlayerAccessInfo::say;
  if (name == "SETALL") return PlayerAccessInfo::setAll;
  if (name == "SETPASSWORD") return PlayerAccessInfo::setPassword;
  if (name == "SETPERMS") return PlayerAccessInfo::setPerms;
  if (name == "SETVAR") return PlayerAccessInfo::setVar;
  if (name == "SHORTBAN") return PlayerAccessInfo::shortBan;
  if (name == "SHOWOTHERS") return PlayerAccessInfo::showOthers;
  if (name == "SHUTDOWNSERVER") return PlayerAccessInfo::shutdownServer;
  if (name == "SPAWN") return PlayerAccessInfo::spawn;
  if (name == "SUPERKILL") return PlayerAccessInfo::superKill;
  if (name == "TALK") return PlayerAccessInfo::talk;
  if (name == "UNBAN") return PlayerAccessInfo::unban;
  if (name == "UNMUTE") return PlayerAccessInfo::unmute;
  if (name == "VETO") return PlayerAccessInfo::veto;
  if (name == "VIEWREPORTS") return PlayerAccessInfo::viewReports;
  if (name == "VOTE") return PlayerAccessInfo::vote;
  return PlayerAccessInfo::lastPerm;
}

void parsePermissionString(const std::string &permissionString, PlayerAccessInfo &info)
{
  if (permissionString.length() < 1)
    return;

  std::istringstream permStream(permissionString);
  std::string word;

  while (permStream >> word) {
    makeupper(word);
 	
    // do we have an operator? check for a leading, non alphabetic character
    char first = NULL;
    if (!TextUtils::isAlphabetic(word[0])) {
      first = word[0];
      word.erase(0,1);
    }

    // Operators are not allowed for userdb
    if (!info.groupState.test(PlayerAccessInfo::isGroup) && first != NULL){
      DEBUG1("userdb: illegal permission string, operators are not allowed in userdb\n");
      return;
    }

    // if we have an operator, lets handle it
    // operators are only allowed for groups, they make no sense for userdb
    if (info.groupState.test(PlayerAccessInfo::isGroup) && first != NULL) {
      switch (first) {
	case '*': {
	  // referenced group
	  // don't copy setThis, groups have to be named explicitly for every group
	  // to prevent unexpected side effects
	  PlayerAccessMap::iterator refgroup = groupAccess.find(word);
	  if (refgroup != groupAccess.end()) {
	    info.explicitAllows |= refgroup->second.explicitAllows;
	    info.explicitDenys |= refgroup->second.explicitDenys;
	    refgroup->second.groupState.set(PlayerAccessInfo::isReferenced);
	  } else {
	    DEBUG1("groupdb: unknown group \"%s\" was referenced\n", word.c_str());
	  }

	  continue;
	} 
            
      case '!': {
	// forbid a permission
	PlayerAccessInfo::AccessPerm perm = permFromName(word);
	if (perm != PlayerAccessInfo::lastPerm) 
	  info.explicitDenys.set(perm);
	else
	  DEBUG1("groupdb: Cannot forbid unknown permission %s\n", word.c_str());

	continue;
      }

      case '-': {
	// remove a permission
	PlayerAccessInfo::AccessPerm perm = permFromName(word);
	if (perm != PlayerAccessInfo::lastPerm) {
	  info.explicitAllows.reset(perm);
	} else {
	  if (word == "ALL")
	    info.explicitAllows.reset();
	  else
	    DEBUG1("groupdb: Cannot remove unknown permission %s\n", word.c_str());
	}

	continue;
      }
            
      // + is like no operator, just let it pass trough
      case '+': break;
            
      default:
	DEBUG1("groupdb: ignoring unknown operator type %c\n", first);
      }
    }

    // regular permission
    PlayerAccessInfo::AccessPerm perm = permFromName(word);
    if (perm != PlayerAccessInfo::lastPerm) {
      info.explicitAllows.set(perm);
    } else {
      if (word == "ALL") {
	info.explicitAllows.set();  
	info.explicitAllows[PlayerAccessInfo::lastPerm] = false;
      } else {
	//DEBUG1("groupdb: Cannot set unknown permission %s\n", word.c_str());
				info.customPerms.push_back(word);
      }
    }
  }
}


bool readPassFile(const std::string &filename)
{
  std::ifstream in(filename.c_str());
  if (!in)
    return false;

  std::string line;
  while (std::getline(in, line)) {
    // Should look at an unescaped ':'
    int colonpos = TextUtils::unescape_lookup(line, '\\', ':');
    if (colonpos == -1)
      continue;
    {
      std::string name = TextUtils::unescape(line.substr(0, colonpos), '\\');
      std::string pass = line.substr(colonpos + 1);
      makeupper(name);
      setUserPassword(name.c_str(), pass.c_str());
    }
  }

  return (passwordDatabase.size() > 0);
}

bool writePassFile(const std::string &filename)
{
  std::ofstream out(filename.c_str());
  if (!out)
    return false;
  PasswordMap::iterator itr = passwordDatabase.begin();
  while (itr != passwordDatabase.end()) {
    if (itr->second != "*") {
      out << TextUtils::escape(itr->first, '\\') << ':' << itr->second << std::endl;
    }
    itr++;
  }
  out.close();
  return true;
}

bool PlayerAccessInfo::readGroupsFile(const std::string &filename)
{
  std::ifstream in(filename.c_str());
  if (!in)
    return false;

  int linenum = 0;
  std::string line;
  while (std::getline(in, line)) {
    linenum++;

    // check for a comment string
    bool skip = true;
    for (std::string::size_type pos = 0; pos < line.size(); pos++) {
      const char c = line[pos];
      if (!TextUtils::isWhitespace(c)) {
	if (c != '#') {
	  skip = false;
	}
	break;
      }
    }
    if (skip) continue;
	
    makeupper(line);

    std::string::size_type colonpos = line.find(':');
    if (colonpos != std::string::npos) {
      std::string name = line.substr(0, colonpos);
      std::string perm = line.substr(colonpos + 1);
	  
      // check if we already have this group, else make a new
      PlayerAccessInfo info;
      PlayerAccessMap::iterator oldgroup = groupAccess.find(name);
      if (oldgroup != groupAccess.end())
	info = oldgroup->second;
      else
	info.groupState[PlayerAccessInfo::isGroup] = true;

      // don't allow changing permissions for a group 
      // that was used as a reference before
      if (info.groupState.test(isReferenced)) {
	DEBUG1("groupdb: skipping groupdb line (%i), group was used as reference before\n", linenum);
	continue;
      }
      parsePermissionString(perm, info);
      info.verified = true;
      groupAccess[name] = info;
    } else {
      DEBUG1("WARNING: bad groupdb line (%i)\n", linenum);
    }

  }

  return true;
}

bool PlayerAccessInfo::readPermsFile(const std::string &filename)
{
  std::ifstream in(filename.c_str());
  if (!in)
    return false;

  for (;;) {
    // 1st line - name
    std::string name;
    if (!std::getline(in, name))
      break;

    PlayerAccessInfo info;

    // 2nd line - groups
    std::string groupline;
    std::getline(in, groupline); // FIXME -it's an error when line cannot be read
    std::istringstream groupstream(groupline);
    std::string group;
    while (groupstream >> group) {
      info.addGroup(group);
    }

    // 3rd line - allows
    std::string perms;
    std::getline(in, perms);
    parsePermissionString(perms, info);

    // 4th line - denies
    // FIXME: not nice ... any ideas how to make it better?
    std::getline(in, perms);
    PlayerAccessInfo dummy;
    parsePermissionString(perms, dummy);
    info.explicitDenys = dummy.explicitAllows;

    userDatabase[name] = info;
  }

  return true;
}

bool PlayerAccessInfo::writePermsFile(const std::string &filename)
{
  int i;
  std::ofstream out(filename.c_str());
  if (!out)
    return false;
  PlayerAccessMap::iterator itr = userDatabase.begin();
  std::vector<std::string>::iterator group;
  while (itr != userDatabase.end()) {
    out << itr->first << std::endl;
    group = itr->second.groups.begin();
    while (group != itr->second.groups.end()) {
      out << (*group) << ' ';
      group++;
    }
    out << std::endl;
    // allows
    for (i = 0; i < PlayerAccessInfo::lastPerm; i++)
      if (itr->second.explicitAllows.test(i))
	out << nameFromPerm((PlayerAccessInfo::AccessPerm) i);
    out << std::endl;
    // denys
    for (i = 0; i < PlayerAccessInfo::lastPerm; i++)
      if (itr->second.explicitDenys.test(i))
	out << nameFromPerm((PlayerAccessInfo::AccessPerm) i);
    out << std::endl;
    itr++;
  }
  out.close();
  return true;
}

std::string		groupsFile;
std::string		passFile;
std::string		userDatabaseFile;

void PlayerAccessInfo::updateDatabases()
{
  if (passFile.size())
    writePassFile(passFile);
  if (userDatabaseFile.size())
    writePermsFile(userDatabaseFile);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

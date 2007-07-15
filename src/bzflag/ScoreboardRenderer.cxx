/* bzflag
 * Copyright (c) 1993 - 2007 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Id$
 */


/* interface header */
#include "ScoreboardRenderer.h"

/* common implementation headers */
#include "Bundle.h"
#include "Team.h"
#include "FontManager.h"
#include "BZDBCache.h"
#include "OpenGLGState.h"
#include "TextUtils.h"

/* local implementation headers */
#include "LocalPlayer.h"
#include "AutoHunt.h"
#include "World.h"
#include "sound.h"

#define DEBUG_SHOWRATIOS 1

std::string		ScoreboardRenderer::scoreSpacingLabel("88% 8888 888-888 [88]");
std::string		ScoreboardRenderer::scoreLabel("Score");
std::string		ScoreboardRenderer::killSpacingLabel("888~888 Hunt->");
std::string		ScoreboardRenderer::killLabel(" Kills");
std::string		ScoreboardRenderer::teamScoreSpacingLabel("88 (888-888) 88");
std::string		ScoreboardRenderer::teamCountSpacingLabel("888");
std::string		ScoreboardRenderer::playerLabel("Player");

// NOTE: order of sort labels must match Sort enums
const char* ScoreboardRenderer::sortLabels[SortTypeCount + 1] = {
  "[Score]",
  "[Normalized Score]",
  "[Callsign]",
  "[Team Kills]",
  "[TK ratio]",
  "[Team]",
  "[1on1]",
  "[HuntLevel]",
  NULL
};
int ScoreboardRenderer::sortMode = 0;
bool ScoreboardRenderer::alwaysShowTeamScore = 0;

ScoreboardRenderer::ScoreboardRenderer() :
  winWidth (0.0),
  teamScoreYVal(0.0f),
  roaming(false),
  dim(false),
  huntIndicator(false),
  huntPosition(0),
  huntSelectEvent(false),
  huntPositionEvent(0),
  huntState(HUNT_NONE),
  huntAddMode(false),
  numHunted(0)
{
  // initialize message color (white)
  messageColor[0] = 1.0f;
  messageColor[1] = 1.0f;
  messageColor[2] = 1.0f;
  sortMode = BZDB.getIntClamped("scoreboardSort", 0, SortTypeCount - 1);
  alwaysShowTeamScore = (BZDB.getIntClamped("alwaysShowTeamScore", 0, 1) != 0);
}


/*  Set window size and location to be used to render the scoreboard.
 *  Values are relative to .... when render() is invoked.
 */
void  ScoreboardRenderer::setWindowSize (float x, float y, float width, float height)
{
  winX = x;
  winY = y;
  winWidth = width;
  winHeight = height;
  setMinorFontSize();
  setLabelsFontSize(winHeight);
}


ScoreboardRenderer::~ScoreboardRenderer()
{
  // no destruction needed
}


const char** ScoreboardRenderer::getSortLabels ()
{
  return sortLabels;
}


void		ScoreboardRenderer::setSort (int _sortby)
{
  sortMode = _sortby;
  BZDB.setInt ("scoreboardSort", sortMode);
}

int			ScoreboardRenderer::getSort ()
{
  return sortMode;
}

void    ScoreboardRenderer::setAlwaysTeamScore (bool _onoff)
{
  alwaysShowTeamScore = _onoff;
  BZDB.set ("alwaysShowTeamScores", _onoff ? "1" : "0");
}

bool    ScoreboardRenderer::getAlwaysTeamScore ()
{
  return alwaysShowTeamScore;
}

void		ScoreboardRenderer::setMinorFontSize()
{
  FontManager &fm = FontManager::instance();
  minorFontFace = fm.getFaceID(BZDB.get("consoleFont"));

  switch (static_cast<int>(BZDB.eval("scorefontsize"))) {
    case 0: { // auto
      for (minorFontSize = 40.0f; minorFontSize > 8.0f; minorFontSize -= 8.0f) {
	const float fontheight = fm.getStringHeight(minorFontFace, minorFontSize);
	const float fontwidth = fm.getStringWidth(minorFontFace, minorFontSize, "X");

	if ((winWidth / fontwidth) < 120) {
	  continue;
	}
	if ((winHeight / fontheight) > 50) {
	  break;
	}
      }
      break;
    }
    case 1: // tiny
      minorFontSize = 8;
      break;
    case 2: // small
      minorFontSize = 16;
      break;
    case 3: // medium
      minorFontSize = 24;
      break;
    case 4: // big
      minorFontSize = 32;
      break;
  }

  huntArrowWidth = fm.getStringWidth(minorFontFace, minorFontSize, "->");
  huntPlusesWidth = fm.getStringWidth(minorFontFace, minorFontSize, "@>");
  huntedArrowWidth = fm.getStringWidth(minorFontFace, minorFontSize, "Hunt->");
  scoreLabelWidth = fm.getStringWidth(minorFontFace, minorFontSize, scoreSpacingLabel);
  killsLabelWidth = fm.getStringWidth(minorFontFace, minorFontSize, killSpacingLabel);
  teamScoreLabelWidth = fm.getStringWidth(minorFontFace, minorFontSize, teamScoreSpacingLabel);
  teamCountLabelWidth = fm.getStringWidth(minorFontFace, minorFontSize, teamCountSpacingLabel);
  const float spacing = fm.getStringWidth(minorFontFace, minorFontSize, " ");
  scoreLabelWidth += spacing;
  killsLabelWidth += spacing;
}


void			ScoreboardRenderer::setLabelsFontSize(float height)
{
  const float s = height / 75.0f;
  FontManager &fm = FontManager::instance();
  labelsFontFace = fm.getFaceID(BZDB.get("consoleFont"));
  labelsFontSize = floorf(s);
}


void			ScoreboardRenderer::setDim(bool _dim)
{
  dim = _dim;
}


static const float dimFactor = 0.2f;

void			ScoreboardRenderer::hudColor3fv(const GLfloat* c)
{
  if (dim)
    glColor3f(dimFactor * c[0], dimFactor * c[1], dimFactor * c[2]);
  else
    glColor3fv(c);
}


void    ScoreboardRenderer::exitSelectState (void){
  playLocalSound(SFX_HUNT_SELECT);
  if (numHunted > 0) {
    setHuntState(HUNT_ENABLED);
  } else {
    setHuntState(HUNT_NONE);
  }
}


void	ScoreboardRenderer::render(bool forceDisplay)
{
  FontManager &fm = FontManager::instance();

  if (dim) {
    fm.setOpacity(dimFactor);
  }

  if (BZDB.isTrue("displayScore") || forceDisplay){
    OpenGLGState::resetState();
    renderScoreboard();
  } else {
    if (getHuntState() == HUNT_SELECTING){      // 'S' pressed while selecting ...
      exitSelectState ();
    }
    World *world = World::getWorld();
    if (world && BZDB.isTrue("alwaysShowTeamScores") && world->allowTeams()){
      OpenGLGState::resetState();
      renderTeamScores(winWidth, winY, FontManager::instance().getStringHeight(minorFontFace, minorFontSize));
    }
  }

  if (dim) {
    fm.setOpacity(1.0f);
  }
}


int ScoreboardRenderer::teamScoreCompare(const void* _c, const void* _d)
{
  World *world = World::getWorld();
  if (!world) {
    return 0;
  }
  Team* c = world->getTeams()+*(int*)_c;
  Team* d = world->getTeams()+*(int*)_d;
  return (d->won-d->lost) - (c->won-c->lost);
}

// invoked by playing.cxx when 'prev' is pressed
void		ScoreboardRenderer::setHuntPrevEvent()
{
  huntPositionEvent = -1;
  --huntPosition;
}

// invoked by playing.cxx when 'next' is pressed
void		ScoreboardRenderer::setHuntNextEvent()
{
  huntPositionEvent = 1;
  ++huntPosition;
}

// invoked by playing.cxx when select (fire) is pressed
void			ScoreboardRenderer::setHuntSelectEvent ()
{
  huntSelectEvent = true;
}

// invoked by clientCommands.cxx when '7' or 'U' is pressed
void			ScoreboardRenderer::huntKeyEvent (bool isAdd)
{
  if (getHuntState() == HUNT_ENABLED) {
    if (isAdd) {
      setHuntState(HUNT_SELECTING);
      playLocalSound(SFX_HUNT_SELECT);
    } else {
      setHuntState(HUNT_NONE);
      playLocalSound(SFX_HUNT);
    }
    huntAddMode = isAdd;

  } else if (getHuntState() == HUNT_SELECTING) {
    exitSelectState ();

  } else {
    setHuntState(HUNT_SELECTING);
    playLocalSound(SFX_HUNT_SELECT);
    huntAddMode = isAdd;
    if (!BZDB.isTrue("displayScore"))
      BZDB.set("displayScore", "1");
  }
}


void		ScoreboardRenderer::setHuntState (int _huntState)
{
  if (huntState == _huntState)
    return;
  if (_huntState != HUNT_SELECTING)
    huntAddMode = false;
  if (_huntState==HUNT_NONE){
    clearHuntedTanks();
  } else if (_huntState==HUNT_SELECTING) {
    huntPosition = 0;
  }
  huntState = _huntState;
}


int			ScoreboardRenderer::getHuntState() const
{
  return huntState;
}


// invoked when joining a server
void ScoreboardRenderer::huntReset()
{
  huntState = HUNT_NONE;
  numHunted = 0;
}

void			ScoreboardRenderer::renderTeamScores (float x, float y, float dy){
  // print teams sorted by score
  int teams[NumTeams];
  int teamCount = 0;
  int i;
  float xn, xl;
  std::string label;
  World *world = World::getWorld();

  if (!world || world->allowRabbit()) {
    return;
  }

  Bundle *bdl = BundleMgr::getCurrentBundle();
  FontManager &fm = FontManager::instance();
  hudColor3fv(messageColor);

  label = bdl->getLocalString("Team Score");
  xl = xn = x - teamScoreLabelWidth;
  fm.drawString(xl, y, 0, minorFontFace, minorFontSize, label);

  for (i = RedTeam; i < NumTeams; i++) {
    if (!Team::isColorTeam(TeamColor(i))) continue;
    const Team* team = world->getTeams() + i;
    if (team->size == 0) continue;
    teams[teamCount++] = i;
  }
  qsort(teams, teamCount, sizeof(int), teamScoreCompare);
  y -= dy;

  char score[44];
  for (i = 0 ; i < teamCount; i++){
    Team& team = world->getTeam(teams[i]);
    sprintf(score, "%3d (%3d-%-3d) %3d", team.won - team.lost, team.won, team.lost, team.size);
    hudColor3fv(Team::getRadarColor((TeamColor)teams[i]));
    fm.drawString(xn, y, 0, minorFontFace, minorFontSize, score);
    y -= dy;
  }
}


// not used yet - new feature coming
void			ScoreboardRenderer::renderCtfFlags (){
  int i;
  RemotePlayer* player = NULL;
  char flagColor[200] = {0};
  World *world = World::getWorld();

  /* no world yet, nothing to render */
  if (!world) {
    return;
  }

  const int curMaxPlayers = world->getCurMaxPlayers();
  FontManager &fm = FontManager::instance();
  const float x = winX;
  const float y = winY;
  const float dy = fm.getStringHeight(minorFontFace, minorFontSize);
  float y0 = y - dy;


  hudColor3fv(messageColor);
  fm.drawString(x, y, 0, minorFontFace, minorFontSize, "Team Flags");

  fm.setDimFactor(dimFactor);

  for (i=0; i < curMaxPlayers; i++) {
    if ((player = world->getPlayer(i))) {
      FlagType* flagd = player->getFlag();
      TeamColor teamIndex = player->getTeam();
      if (flagd!=Flags::Null && flagd->flagTeam != NoTeam) {   // if player has team flag ...
	std::string playerInfo = ColorStrings[flagd->flagTeam];
	sprintf (flagColor, "%-12s", flagd->flagName);
	playerInfo += flagColor;
	playerInfo += ColorStrings[teamIndex];
	playerInfo += player->getCallSign();

	fm.drawString(x, y0, 0, minorFontFace, minorFontSize, playerInfo);
	y0 -= dy;
      }
    }
  }
  renderTeamScores (winWidth, y, dy);
}


void     ScoreboardRenderer::clearHuntedTanks ()
{
  World *world = World::getWorld();

  if (!world) {
    return;
  }

  const int curMaxPlayers = world->getCurMaxPlayers();
  Player *p;
  for (int i=0; i<curMaxPlayers; i++) {
    if ((p = world->getPlayer(i)))
      p->setHunted (false);
  }
  numHunted = 0;
}


void			ScoreboardRenderer::renderScoreboard(void)
{
  int i=0;
  int numPlayers;
  int emailLen;
  Player** players;
  Player*  player;
  bool haveObs = false;

  if ( (players = newSortedList (sortMode, true, &numPlayers)) == NULL)
    return;

  LocalPlayer* myTank = LocalPlayer::getMyTank();
  Bundle *bdl = BundleMgr::getCurrentBundle();
  FontManager &fm = FontManager::instance();

  const float x1 = winX;
  const float x2 = x1 + scoreLabelWidth;
  const float x3 = x2 + killsLabelWidth;
  const float y0 = winY;
  hudColor3fv(messageColor);

  std::string psLabel = bdl->getLocalString(playerLabel);
  if (sortMode != SortScore){
    psLabel += "  ";
    psLabel += sortLabels[sortMode];
  }
  fm.drawString(x1, y0, 0, minorFontFace, minorFontSize, bdl->getLocalString(scoreLabel));
  fm.drawString(x2, y0, 0, minorFontFace, minorFontSize, bdl->getLocalString(killLabel));
  fm.drawString(x3, y0, 0, minorFontFace, minorFontSize, psLabel);
  const float dy = fm.getStringHeight(minorFontFace, minorFontSize);
  float y = y0 - dy;

  // make room for the status marker
  const float xs = x3 - fm.getStringWidth(minorFontFace, minorFontSize, "+|");

  if (huntState == HUNT_SELECTING){
    std::string huntStr = ColorStrings[YellowColor];
    huntStr += ColorStrings[PulsatingColor];
    huntStr += " *SEL*";
    fm.drawString(xs - huntedArrowWidth, y0, 0, minorFontFace, minorFontSize, huntStr);
  }

  // grab the tk warning ratio
  tkWarnRatio = BZDB.eval("tkwarnratio");

  if (huntState == HUNT_SELECTING){
    if (numPlayers<1 || (numPlayers==1 && players[0]==myTank)){
      setHuntState (HUNT_NONE);
    } else {
      if (players[huntPosition] == myTank){
	if (huntPositionEvent < 0)
	  --huntPosition;
	else
	  ++huntPosition;
      }
      if (huntPosition>=numPlayers)
	huntPosition = 0;
      if (huntPosition<0)
	huntPosition = numPlayers-1;
      if (huntSelectEvent){	     // if 'fire' was pressed ...
	if (!huntAddMode)
	  clearHuntedTanks ();
	if (huntAddMode && players[huntPosition]->isHunted()) {   // UNselect
	  players[huntPosition]->setHunted(false);
	  if (--numHunted != 0)
	    playLocalSound(SFX_HUNT_SELECT);
	} else {						  // else select
	  players[huntPosition]->setHunted(true);
	  if (++numHunted == 1)
	    playLocalSound(SFX_HUNT);
	  else
	    playLocalSound(SFX_HUNT_SELECT);
	}
	huntState = HUNT_ENABLED;
      }
    }
  }


  emailLen = BZDB.getIntClamped ("emailDispLen", 0, 128);
  huntSelectEvent = false;
  huntPositionEvent = 0;
  numHunted = 0;

  while ( (player = players[i]) != NULL){
    if (player->isHunted())
      ++numHunted;
    if (player->getTeam()==ObserverTeam && !haveObs){
      y -= dy;
      haveObs = true;
    }
    if (huntState==HUNT_SELECTING && i==huntPosition)
      drawPlayerScore(player, x1, x2, x3, xs, (float)y, emailLen, true);
    else
      drawPlayerScore(player, x1, x2, x3, xs, (float)y, emailLen, false);
    y -= dy;
    ++i;
  }

  if (huntState==HUNT_ENABLED && !numHunted) {
    huntState = HUNT_NONE;	// last hunted player must have left the game
    huntAddMode = false;
    playLocalSound(SFX_HUNT);
  }

  delete[] players;

  World *world = World::getWorld();
  if (world && world->allowTeams()) {
    renderTeamScores(winWidth, y0, dy);
  }
}


void      ScoreboardRenderer::stringAppendNormalized (std::string *s, float n)
{
  char fmtbuf[10];
  sprintf (fmtbuf, "  [%4.2f]", n);
  *s += fmtbuf;
}


void			ScoreboardRenderer::drawPlayerScore(const Player* player,
							    float x1, float x2, float x3, float xs, float y,
							    int emailLen, bool huntCursor)
{
  // score
  char score[40], kills[40];

  bool highlightTKratio = false;
  if (tkWarnRatio > 0.0) {
    if (((player->getWins() > 0) && (player->getTKRatio() > tkWarnRatio)) ||
	((player->getWins() == 0) && (player->getTeamKills() >= 3))) {
      highlightTKratio = true;
    }
  }

  World *world = World::getWorld();
  if (world && world->allowRabbit()) {
    sprintf(score, "%2d%% %4d %3d-%-3d%s[%2d]", player->getRabbitScore(),
	    player->getScore(), player->getWins(), player->getLosses(),
	    highlightTKratio ? ColorStrings[CyanColor].c_str() : "",
	    player->getTeamKills());
  } else {
    sprintf(score, "%4d %4d-%-4d%s[%2d]", player->getScore(),
	    player->getWins(), player->getLosses(),
	    highlightTKratio ? ColorStrings[CyanColor].c_str() : "",
	    player->getTeamKills());
  }

  // kills
  if (LocalPlayer::getMyTank() != player) {
    sprintf(kills, "%3d~%-3d", player->getLocalWins(), player->getLocalLosses());
  } else {
    kills[0] = '\0';
  }


  // team color
  TeamColor teamIndex = player->getTeam();
  if (teamIndex < RogueTeam) {
    teamIndex = RogueTeam;
  }
  std::string teamColor;
  if (player->getId() < 200) {
    teamColor = ColorStrings[teamIndex];
  } else {
    teamColor = ColorStrings[CyanColor]; // replay observers
  }

  // authentication status
  std::string statusInfo;
  if (BZDBCache::colorful) {
    statusInfo += ColorStrings[CyanColor];
  } else {
    statusInfo += teamColor;
  }
  if (player->isAdmin()) {
    statusInfo += '@';
  } else if (player->isVerified()) {
    statusInfo += '+';
  } else if (player->isRegistered()) {
    statusInfo += '-';
  } else {
    statusInfo = ""; // don't print
  }

  std::string playerInfo;
  // team color
  playerInfo += teamColor;
  //Slot number only for admins
  LocalPlayer* localPlayer = LocalPlayer::getMyTank();
  if (localPlayer->isAdmin()){
    char slot[10];
    sprintf(slot, "%3d",player->getId());
    playerInfo += slot;
    playerInfo += " - ";
  }

  if (roaming && BZDB.isTrue("showVelocities"))
    {
      float vel[3] = {0};
      memcpy(vel,player->getVelocity(),sizeof(float)*3);

      float linSpeed = sqrt(vel[0]*vel[0]+vel[1]*vel[1]);

      float badFactor = 1.5f;
      if (linSpeed > player->getMaxSpeed()*badFactor)
	playerInfo += ColorStrings[RedColor];
      if (linSpeed > player->getMaxSpeed())
	playerInfo += ColorStrings[YellowColor];
      else if (linSpeed < 0.0001f)
	playerInfo += ColorStrings[GreyColor];
      else
	playerInfo += ColorStrings[WhiteColor];
      playerInfo += TextUtils::format("%5.2f",linSpeed);
      playerInfo += teamColor;
    }
  // callsign
  playerInfo += player->getCallSign();

  // email in parenthesis
  if (player->getEmailAddress()[0] != '\0' && emailLen>0) {
    playerInfo += " (";
    playerInfo += TextUtils::str_trunc_continued (player->getEmailAddress(), emailLen);
    playerInfo += ")";
  }
  // carried flag
  bool coloredFlag = false;
  FlagType* flagd = player->getFlag();
  if (flagd != Flags::Null) {
    // color special flags
    if (BZDBCache::colorful) {
      if ((flagd == Flags::ShockWave)
	  ||  (flagd == Flags::Genocide)
	  ||  (flagd == Flags::Laser)
	  ||  (flagd == Flags::GuidedMissile)) {
	playerInfo += ColorStrings[WhiteColor];
      } else if (flagd->flagTeam != NoTeam) {
	// use team color for team flags
	playerInfo += ColorStrings[flagd->flagTeam];
      }
      coloredFlag = true;
    }
    playerInfo += "/";
    playerInfo += (flagd->endurance == FlagNormal ? flagd->flagName : flagd->flagAbbv);
    // back to original color
    if (coloredFlag) {
      playerInfo += teamColor;
    }
  }
  // status
  if (player->isPaused())
    playerInfo += "[p]";
  else if (player->isNotResponding())
    playerInfo += "[nr]";
  else if (player->isAutoPilot())
    playerInfo += "[auto]";

#if DEBUG_SHOWRATIOS
  if (player->getTeam() != ObserverTeam) {
    if (sortMode == SortNormalized)
      stringAppendNormalized (&playerInfo, player->getNormalizedScore());
    else if (sortMode == SortMyRatio)
      stringAppendNormalized (&playerInfo, player->getLocalNormalizedScore());
    else if (sortMode == SortTkRatio)
      stringAppendNormalized (&playerInfo, player->getTKRatio());
  }
#endif

  FontManager &fm = FontManager::instance();
  fm.setDimFactor(dimFactor);

  // draw
  if (player->getTeam() != ObserverTeam) {
    hudColor3fv(Team::getRadarColor(teamIndex));
    fm.drawString(x1, y, 0, minorFontFace, minorFontSize, score);
    hudColor3fv(Team::getRadarColor(teamIndex));
    fm.drawString(x2, y, 0, minorFontFace, minorFontSize, kills);
  }
  fm.drawString(x3, y, 0, minorFontFace, minorFontSize, playerInfo);
  if (statusInfo.size() > 0) {
    fm.drawString(xs, y, 0, minorFontFace, minorFontSize, statusInfo);
  }
  if (BZDB.isTrue("debugHud")) {
    printf ("playerInfo: %s\n", playerInfo.c_str()); //FIXME
  }

  // draw huntEnabled status
  if (player->isHunted()) {
    std::string huntStr = ColorStrings[WhiteColor];
    huntStr += "Hunt->";
    fm.drawString(xs - huntedArrowWidth, y, 0, minorFontFace, minorFontSize,
		  huntStr.c_str());
  }
  else if (player->getAutoHuntLevel()) {
    std::string huntStr = AutoHunt::getColorString(player->getAutoHuntLevel());
    huntStr += "Auto->";
    fm.drawString(xs - huntedArrowWidth, y, 0, minorFontFace, minorFontSize,
		  huntStr.c_str());
  }

  if (huntCursor && huntAddMode) {
    std::string huntStr = ColorStrings[WhiteColor];
    huntStr += ColorStrings[PulsatingColor];
    huntStr += "@>";
    fm.drawString(xs - huntPlusesWidth, y, 0, minorFontFace, minorFontSize,
		  huntStr.c_str());
  }

  if (huntCursor && !huntAddMode) {
    std::string huntStr = ColorStrings[WhiteColor];
    huntStr += ColorStrings[PulsatingColor];
    huntStr += "->";
    fm.drawString(xs - huntArrowWidth, y, 0, minorFontFace, minorFontSize,
		  huntStr.c_str());
  }
}


// get current 'leader' (NULL if no player)
Player* ScoreboardRenderer::getLeader(std::string *label)
{
  int sortType=sortMode;

  if (sortMode==SortCallsign || sortMode==SortMyRatio || sortMode==SortTeam){
    sortType = SortScore;
  }
  if (label != NULL){
    if (sortMode==SortTKs)
      *label = "TK Leader ";
    else if (sortMode==SortTkRatio)
      *label = "TK Ratio Leader ";
    else
      *label = "Leader ";
  }

  Player** list = newSortedList (sortType, true);

  Player* top;
  if (!list)
    top = NULL;
  else
    top = list[0];

  delete[] list;

  if (top==NULL || top->getTeam()==ObserverTeam)
    return NULL;
  return top;
}

/************************ Sort logic follows .... **************************/

struct st_playersort{
  Player *player;
  int i1;
  int i2;
  const char *cp;
};
typedef struct st_playersort sortEntry;


int       ScoreboardRenderer::sortCompareCp(const void* _a, const void* _b)
{
  sortEntry *a = (sortEntry *)_a;
  sortEntry *b = (sortEntry *)_b;
  return strcasecmp (a->cp, b->cp);
}


int       ScoreboardRenderer::sortCompareI2(const void* _a, const void* _b)
{
  sortEntry *a = (sortEntry *)_a;
  sortEntry *b = (sortEntry *)_b;
  if (a->i1 != b->i1 )
    return b->i1 - a->i1;
  return b->i2 - a->i2;
}


// creates (allocates) a null-terminated array of Player*
Player **  ScoreboardRenderer::newSortedList (int sortType, bool obsLast, int *_numPlayers)
{
  LocalPlayer *myTank = LocalPlayer::getMyTank();
  World *world = World::getWorld();

  if (!myTank || !world) {
    return NULL;
  }

  const int curMaxPlayers = world->getCurMaxPlayers() +1;
  int i,j;
  int numPlayers=0;
  int numObs=0;
  Player* p;
  sortEntry* sorter = new sortEntry [curMaxPlayers];

  // fill the array with remote players
  for (i=0; i<curMaxPlayers-1; i++) {
    if ((p = world->getPlayer(i))){
      if (obsLast && p->getTeam()==ObserverTeam)
	sorter[curMaxPlayers - (++numObs)].player = p;
      else
	sorter[numPlayers++].player = p;
    }
  }
  // add my tank
  if (obsLast && myTank->getTeam()==ObserverTeam)
    sorter[curMaxPlayers - (++numObs)].player = myTank;
  else
    sorter[numPlayers++].player = myTank;

  // sort players ...
  if (numPlayers > 0){
    for (i=0; i<numPlayers; i++){
      p = sorter[i].player;
      switch (sortType){
	case SortTKs:
	  sorter[i].i1 = p->getTeamKills();
	  sorter[i].i2 = 0 - (int)(p->getNormalizedScore() * 100000);
	  break;
	case SortTkRatio:
	  sorter[i].i1 = (int)(p->getTKRatio() * 1000);
	  sorter[i].i2 = 0 - (int)(p->getNormalizedScore() * 100000);
	  break;
	case SortTeam:
	  sorter[i].i1 = p->getTeam();
	  sorter[i].i2 = (int)(p->getNormalizedScore() * 100000);
	  break;
	case SortMyRatio:
	  if (p == myTank)
	    sorter[i].i1 = -100001;
	  else
	    sorter[i].i1 = 0 - (int)(p->getLocalNormalizedScore() * 100000);
	  sorter[i].i2 = (int)(p->getNormalizedScore() * 100000);
	  break;
	case SortNormalized:
	  sorter[i].i1 = (int)(p->getNormalizedScore() * 100000);
	  sorter[i].i2 = 0;
	  break;
	case SortCallsign:
	  sorter[i].cp = p->getCallSign();
	  break;
	case SortHuntLevel: {
	  const int huntLevel = p->getAutoHuntLevel();
	  sorter[i].i1 = p->isHunted() ? 10 : huntLevel;
	  sorter[i].i2 = p->getScore();
	  break;
	}
	default: {
	  if (world->allowRabbit())
	    sorter[i].i1 = p->getRabbitScore();
	  else
	    sorter[i].i1 = p->getScore();
	  sorter[i].i2 = 0;
	}
      }
    }
    if (sortType == SortCallsign)
      qsort (sorter, numPlayers, sizeof(sortEntry), sortCompareCp);
    else
      qsort (sorter, numPlayers, sizeof(sortEntry), sortCompareI2);
  }

  // TODO: Sort obs here (by time joined, when that info is available)

  Player** players = new Player *[numPlayers + numObs + 1];
  for (i=0; i<numPlayers; i++)
    players[i] = sorter[i].player;
  for (j=curMaxPlayers-numObs; j<curMaxPlayers; j++)
    players[i++] = sorter[j].player;
  players[i] = NULL;

  if (_numPlayers != NULL)
    *_numPlayers = numPlayers;

  delete[] sorter;
  return players;
}


// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

/* bzflag
 * Copyright (c) 1993 - 2004 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* interface header */
#include "KeyboardMapMenu.h"

/* system implementation headers */
#include <vector>

/* common implementation headers */
#include "KeyManager.h"

/* local implementation headers */
#include "ActionBinding.h"
#include "HUDDialogStack.h"
#include "MainMenu.h"

/* FIXME -- from playing.h */
void notifyBzfKeyMapChanged();


KeyboardMapMenuDefaultKey::KeyboardMapMenuDefaultKey(KeyboardMapMenu* _menu) :
  menu(_menu)
{
  // do nothing
}

bool KeyboardMapMenuDefaultKey::keyPress(const BzfKeyEvent& key)
{
  // escape key has usual effect
  if (key.ascii == 27)
    return MenuDefaultKey::keyPress(key);

  // keys have normal effect if not editing
  if (!menu->isEditing())
    return MenuDefaultKey::keyPress(key);

  // ignore keys we don't know
  if (key.ascii != 0 && isspace(key.ascii)) {
    if (key.ascii != ' ' && key.ascii != '\t' && key.ascii != '\r')
      return true;
  }

  // all other keys modify mapping
  menu->setKey(key);
  return true;
}

bool KeyboardMapMenuDefaultKey::keyRelease(const BzfKeyEvent&)
{
  // ignore key releases
  return true;
}

KeyboardMapMenu::KeyboardMapMenu() : defaultKey(this), editing(-1), quickKeysMenu(NULL)
{
  // add controls
  std::vector<HUDuiControl*>& controls = getControls();

  controls.push_back(createLabel("Key Mapping"));
  controls.push_back(createLabel("Use up/down arrows to navigate, enter key to enter edit mode"));
  controls.push_back(reset = createLabel(NULL, "Reset Defaults"));
  controls.push_back(createLabel("fire", "Fire shot:"));
  controls.push_back(createLabel(NULL, "Drop flag:"));
  controls.push_back(createLabel(NULL, "Identify/Lock On:"));
  controls.push_back(createLabel(NULL, "Radar Short:"));
  controls.push_back(createLabel(NULL, "Radar Medium:"));
  controls.push_back(createLabel(NULL, "Radar Long:"));
  controls.push_back(createLabel(NULL, "Send to All:"));
  controls.push_back(createLabel(NULL, "Send to Teammates:"));
  controls.push_back(createLabel(NULL, "Send to Nemesis:"));
  controls.push_back(createLabel(NULL, "Send to Recipient:"));
	controls.push_back(createLabel(NULL, "Send to Admin:"));
  controls.push_back(createLabel(NULL, "Jump:"));
  controls.push_back(createLabel(NULL, "Binoculars:"));
  controls.push_back(createLabel(NULL, "Toggle Score:"));
  controls.push_back(createLabel(NULL, "Tank Labels:"));
  controls.push_back(createLabel(NULL, "Flag Help:"));
  controls.push_back(createLabel(NULL, "Time Forward:"));
  controls.push_back(createLabel(NULL, "Time Backward:"));
  controls.push_back(createLabel(NULL, "Pause/Resume:"));
  controls.push_back(createLabel(NULL, "Self Destruct/Cancel:"));
  controls.push_back(createLabel(NULL, "Fast Quit:"));
  controls.push_back(createLabel(NULL, "Scroll Backward:"));
  controls.push_back(createLabel(NULL, "Scroll Forward:"));
  controls.push_back(createLabel(NULL, "Slow Keyboard Motion:"));
  controls.push_back(createLabel(NULL, "Toggle Flags On Radar:"));
  controls.push_back(createLabel(NULL, "Toggle Flags On Field:"));
  controls.push_back(createLabel(NULL, "Silence/UnSilence Key:"));
  controls.push_back(createLabel(NULL, "Server Command Key:"));
  controls.push_back(createLabel(NULL, "Hunt Key:"));
  controls.push_back(createLabel(NULL, "AutoPilot Key: "));
  controls.push_back(createLabel(NULL, "Restart:"));
  controls.push_back(createLabel(NULL, "Iconify:"));
  controls.push_back(createLabel(NULL, "Fullscreen:"));
  controls.push_back(quickKeys = createLabel(NULL, "Define Quick Keys"));

  initNavigation(controls, 2, controls.size()-1);

  initkeymap("fire", 3);
  initkeymap("drop", 4);
  initkeymap("identify", 5);
  initkeymap("set displayRadarRange 0.25", 6);
  initkeymap("set displayRadarRange 0.5", 7);
  initkeymap("set displayRadarRange 1.0", 8);
  initkeymap("send all", 9);
  initkeymap("send team", 10);
  initkeymap("send nemesis", 11);
  initkeymap("send recipient", 12);
  initkeymap("jump", 13);
  initkeymap("toggle displayBinoculars", 14);
  initkeymap("toggle displayScore", 15);
  initkeymap("toggle displayLabels", 16);
  initkeymap("toggle displayFlagHelp", 17);
  initkeymap("time forward", 18);
  initkeymap("time backward", 19);
  initkeymap("pause", 20);
  initkeymap("destruct", 21);
  initkeymap("quit", 22);
  initkeymap("scrollpanel up", 23);
  initkeymap("scrollpanel down", 24);
  initkeymap("toggle slowKeyboard", 25);
  initkeymap("toggleFlags radar", 26);
  initkeymap("toggleFlags main", 27);
  initkeymap("silence", 28);
  initkeymap("servercommand", 29);
  initkeymap("hunt", 30);
  initkeymap("autopilot", 31);
  initkeymap("restart", 32);
  initkeymap("iconify", 33);
  initkeymap("fullscreen", 34);
	initkeymap("send admin",35);
}

void KeyboardMapMenu::initkeymap(const std::string& name, int index)
{
  mappable[name].key1 = "";
  mappable[name].key2 = "";
  mappable[name].index = index;
}

bool KeyboardMapMenu::isEditing() const
{
  return editing != -1;
}

void KeyboardMapMenu::setKey(const BzfKeyEvent& event)
{
  if (editing == -1)
    return;
  KeyKeyMap::iterator it;
  for (it = mappable.begin(); it != mappable.end(); it++)
    if (it->second.index == editing)
      break;
  if ((KEYMGR.keyEventToString(event) == it->second.key1 && it->second.key2.empty()) || (KEYMGR.keyEventToString(event) == it->second.key2))
    return;
  ActionBinding::instance().associate(KEYMGR.keyEventToString(event),
				      it->first);
  editing = -1;
  update();
}

void KeyboardMapMenu::execute()
{
  const HUDuiControl* const focus = HUDui::getFocus();
  if (focus == reset) {
    ActionBinding::instance().resetBindings();
    update();
  }
  else if (focus == quickKeys) {
    if (!quickKeysMenu) quickKeysMenu = new QuickKeysMenu;
    HUDDialogStack::get()->push(quickKeysMenu);
  }
  else {
    // start editing
    std::vector<HUDuiControl*>& list = getControls();
    KeyKeyMap::iterator it;
    for (it = mappable.begin(); it != mappable.end(); it++) {
      if (list[it->second.index] == focus) {
	editing = it->second.index;
	if (!it->second.key1.empty() && !it->second.key2.empty()) {
	  ActionBinding::instance().deassociate(it->first);
	}
      }
    }
  }
  update();
}

void KeyboardMapMenu::dismiss()
{
  editing = -1;
  notifyBzfKeyMapChanged();
}

void KeyboardMapMenu::resize(int width, int height)
{
  HUDDialog::resize(width, height);

  int i;
  // use a big font for title, smaller font for the rest
  const float titleFontWidth = (float)height / 10.0f;
  const float titleFontHeight = (float)height / 10.0f;
  const float bigFontWidth = (float)height / 28.0f;
  const float bigFontHeight = (float)height / 28.0f;

  const float fontWidth = (float)height / 38.0f;
  const float fontHeight = (float)height / 38.0f;

  // reposition title
  std::vector<HUDuiControl*>& list = getControls();
  HUDuiLabel* title = (HUDuiLabel*)list[0];
  title->setFontSize(titleFontWidth, titleFontHeight);
  const OpenGLTexFont& titleFont = title->getFont();
  const float titleWidth = titleFont.getWidth(title->getString());
  float x = 0.5f * ((float)width - titleWidth);
  float y = (float)height - titleFont.getHeight();
  title->setPosition(x, y);
  // reposition help
  HUDuiLabel*help = (HUDuiLabel*)list[1];
  help->setFontSize( bigFontWidth, bigFontHeight);
  const OpenGLTexFont& helpFont = help->getFont();
  const float helpWidth = helpFont.getWidth(help->getString());
  x = 0.5f * ((float)width - helpWidth);
  y -= 1.1f * helpFont.getHeight();
  help->setPosition(x, y);


  // reposition options in two columns
  x = 0.30f * (float)width;
  const float topY = y - (0.6f * titleFont.getHeight());
  y = topY;
  list[2]->setFontSize(fontWidth, fontHeight);
  const float h = list[2]->getFont().getHeight();
  const int count = list.size() - 2;
  const int mid = (count / 2);

  for (i = 2; i <= mid+1; i++) {
    list[i]->setFontSize(fontWidth, fontHeight);
    list[i]->setPosition(x, y);
    y -= 1.0f * h;
  }

  x = 0.80f * (float)width;
  y = topY;
  for (i = mid+2; i < count+2; i++) {
    list[i]->setFontSize(fontWidth, fontHeight);
    list[i]->setPosition(x, y);
    y -= 1.0f * h;
  }

  update();
}

void KeyboardMapMenu::update()
{
  KeyKeyMap::iterator it;
  // clear
  for (it = mappable.begin(); it != mappable.end(); it++) {
    it->second.key1 = "";
    it->second.key2 = "";
  }
  // load current settings
  KEYMGR.iterate(&onScanCB, this);
  std::vector<HUDuiControl*>& list = getControls();
  for (it = mappable.begin(); it != mappable.end(); it++) {
    std::string value = "";
    if (it->second.key1.empty()) {
      if (isEditing() && (it->second.index == editing))
	value = "???";
      else
	value = "<not mapped>";
    } else {
      value += it->second.key1;
      if (!it->second.key2.empty()) {
	value += " or " + it->second.key2;
      } else if (isEditing() && (it->second.index == editing)) {
	value += " or ???";
      }
    }
    ((HUDuiLabel*)list[it->second.index])->setString(value);
  }
}

void KeyboardMapMenu::onScan(const std::string& name, bool press,
			     const std::string& cmd)
{
  if (!press && cmd == "fire")
    return;
  KeyKeyMap::iterator it = mappable.find(cmd);
  if (it == mappable.end())
    return;
  if (it->second.key1.empty())
    it->second.key1 = name;
  else if (it->second.key2.empty())
    it->second.key2 = name;
}

void KeyboardMapMenu::onScanCB(const std::string& name, bool press,
			       const std::string& cmd, void* userData)
{
  reinterpret_cast<KeyboardMapMenu*>(userData)->onScan(name, press, cmd);
}

HUDuiLabel* KeyboardMapMenu::createLabel(const char* str, const char* _label)
{
  HUDuiLabel* label = new HUDuiLabel;
  label->setFont(MainMenu::getFont());
  if (str) label->setString(str);
  if (_label) label->setLabel(_label);
  return label;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

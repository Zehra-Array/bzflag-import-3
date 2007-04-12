/* bzflag
 * Copyright (c) 1993 - 2007 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef BZF_CLIENT_AUTHENTICATION_H
#define BZF_CLIENT_AUTHENTICATION_H

/* bzflag special common - 1st one */
#include "common.h"


/* system implementation headers */

/* local implementation headers */
#include "ServerLink.h"

class ClientAuthentication {
 public:
  static void login(const char *username, const char *password);
  static void logout();
  static void sendCredential(ServerLink &serverLink);

private:
  static char	      principalName[128];
  static bool	      authentication;
};

#endif

// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

/* bzflag
 * Copyright (c) 1993 - 2004 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __BZWERROR_H__
#define __BZWERROR_H__

// implementation-specific system header
#include <string>

class BZWError {

public:
  BZWError(std::string _location);
  ~BZWError();

  /* return false if error reporting failed, true otherwise */
  bool fatalError(std::string errorMsg, int line);
  bool warning(std::string warningMsg, int line);

  /* status */
  bool hasHadError() {return hadError;};
  bool hasHadWarning() {return hadWarning;};

private:
  /* data */
  bool hadError;
  bool hadWarning;
  std::string location;

  /* no public default constructor */
  BZWError();
};

#endif //__BZWERROR_H__


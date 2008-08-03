/* bzflag
 * Copyright (c) 1993 - 2008 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __BZW_WORLD_H__
#define __BZW_WORLD_H__

#include <iostream>

/* bzflag common headers */

namespace BZW
{

  /**
   * World class
   */
  class World
  {
    public:
      /// Default constructor
      World();
      /// Read/Create a world from an stream
      World(std::istream& input);
      /// Default destructor
      ~World();

      /// Read/Create a world from a stream
      /// Write current world to a stream
      void write(std::ostream& output);

    private:

  };

}

#endif // __BZW_WORLD_H__

// Local Variables: ***
// Mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

/******************************************************************************
 *
 * 
 *
 * Copyright (C) 1997-2005 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby 
 * granted. No representations are made about the suitability of this software 
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#ifndef SCANNER_H
#define SCANNER_H

#include "qtbc.h"

class OutputList;
class Entry;

// Public interface provided by the language scanner
void parseMain(Entry *);
void parseMain(Entry *,const char *fileName);

// Internal callback interface for comment block scanner
void parsePrototype(const QCString &text);
void handleGroupStartCommand(const char *header);
void handleGroupEndCommand();

#endif

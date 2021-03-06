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
 * The code is this file is largely based on a contribution from
 * Harm van der Heijden <H.v.d.Heijden@phys.tue.nl>
 * Please send thanks to him and bug reports to me :-)
 */

#ifndef HTMLHELP_H
#define HTMLHELP_H

#include "qtbc.h"
#include <qtextstream.h>
#include <qstrlist.h>

class QFile;
class HtmlHelpIndex;

/*! A class that generated the HTML Help specific files.
 *  These files can be used with the Microsoft HTML Help workshop
 *  to generate compressed HTML files (.chm).
 */
class HtmlHelp 
{
    /*! used in imageNumber param of HTMLHelp::addContentsItem() function 
        to specify document icon in tree view.  
        Writes \<param name="ImageNumber" value="xx"\> in .HHC file. */
    enum ImageNumber { 
      BOOK_CLOSED=1,    BOOK_OPEN,
      BOOK_CLOSED_NEW,  BOOK_OPEN_NEW,
      FOLDER_CLOSED,    FOLDER_OPEN,
      FOLDER_CLOSED_NEW,FOLDER_OPEN_NEW,
      QUERY,            QUERY_NEW,
      TEXT,             TEXT_NEW,
      WEB_DOC,          WEB_DOC_NEW,
      WEB_LINK,         WEB_LINK_NEW,
      INFO,             INFO_NEW,
      LINK,             LINK_NEW,
      BOOKLET,          BOOKLET_NEW,
      EMAIL,            EMAIL_NEW,
      EMAIL2,           EMAIL2_NEW,
      IMAGE,            IMAGE_NEW,
      AUDIO,            AUDIO_NEW,
      MUSIC,            MUSIC_NEW,
      VIDEO,            VIDEO_NEW,
      INDEX,            INDEX_NEW,
      IDEA,             IDEA_NEW,
      NOTE,             NOTE_NEW,
      TOOL,             TOOL_NEW
    };
  public:
    static HtmlHelp *getInstance();
    void initialize();
    void finalize();
    int  incContentsDepth();
    int  decContentsDepth();
    /*! return the current depth of the contents tree */ 
    int  contentsDepth() { return dc; }
    // added imageNumber - KPW
    void addContentsItem(bool isDir,
                         const char *name, 
                         const char *ref = 0, 
                         const char *anchor = 0);
    void addIndexItem(const char *level1, const char *level2, 
                      const char *ref, const char *anchor);
    void addIndexFile(const char *name);


  private:
    void createProjectFile();

    HtmlHelp();
    QFile *cf,*kf; 
    QTextStream cts,kts;
    HtmlHelpIndex *index;
    int dc;
    QStrList indexFiles;
    QDict<void> indexFileDict;
    static HtmlHelp *theInstance;
};

#endif /* HTMLHELP_H */


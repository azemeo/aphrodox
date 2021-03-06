#include "dirdef.h"
#include "filename.h"
#include "doxygen.h"
#include "util.h"
#include "outputlist.h"
#include "language.h"
#include "message.h"
#include "dot.h"

//----------------------------------------------------------------------
// method implementation

static int g_dirCount=0;

DirDef::DirDef(const char *path) : Definition(path,1,path)
{
  // get display name (stipping the paths mentioned in STRIP_FROM_PATH)
  m_dispName = stripFromPath(path);
  // get short name (last part of path)
  m_shortName = path;
  if (m_shortName.at(m_shortName.length()-1)=='/')
  { // strip trailing /
    m_shortName = m_shortName.left(m_shortName.length()-1);
  }
  int pi=m_shortName.findRev('/');
  if (pi!=-1) 
  { // remove everything till the last /
    m_shortName = m_shortName.mid(pi+1);
  }
  setLocalName(m_shortName);
  
  m_fileList   = new FileList;
  m_usedDirs   = new QDict<UsedDir>(257);
  m_usedDirs->setAutoDelete(TRUE);
  m_dirCount   = g_dirCount++;
  m_level=-1;
  m_parent=0;
}

DirDef::~DirDef()
{
  delete m_fileList;
  delete m_usedDirs;
}

bool DirDef::isLinkableInProject() const 
{ 
  return !isReference() && Config_getBool("SHOW_DIRECTORIES"); 
}

bool DirDef::isLinkable() const 
{ 
  return isReference() || isLinkableInProject(); 
}

void DirDef::addSubDir(DirDef *subdir)
{
  m_subdirs.inSort(subdir);
  subdir->setOuterScope(this);
  subdir->m_parent=this;
}

void DirDef::addFile(FileDef *fd)
{
  m_fileList->inSort(fd);
  fd->setDirDef(this);
}

QCString DirDef::getOutputFileBase() const
{
  //return "dir_"+convertNameToFile(name());
  return QCString().sprintf("dir_%06d",m_dirCount);
}

void DirDef::writeDetailedDocumentation(OutputList &ol)
{
  if (!briefDescription().isEmpty() || !documentation().isEmpty())
  {
    ol.writeRuler();
    ol.pushGeneratorState();
      ol.disable(OutputGenerator::Latex);
      ol.disable(OutputGenerator::RTF);
    ol.writeAnchor(0,"_details");
    ol.popGeneratorState();
    ol.startGroupHeader();
    ol.parseText(theTranslator->trDetailedDescription());
    ol.endGroupHeader();

    // repeat brief description
    if (!briefDescription().isEmpty() && Config_getBool("REPEAT_BRIEF"))
    {
      ol.parseDoc(briefFile(),briefLine(),this,0,briefDescription(),FALSE,FALSE);
      ol.newParagraph();
    }

    // write documentation
    if (!documentation().isEmpty())
    {
      ol.parseDoc(docFile(),docLine(),this,0,documentation()+"\n",TRUE,FALSE);
    }
  }
}

void DirDef::writeDocumentation(OutputList &ol)
{
  ol.pushGeneratorState();
  
  QCString shortTitle=theTranslator->trDirReference(m_shortName);
  QCString title=theTranslator->trDirReference(m_dispName);
  startFile(ol,getOutputFileBase(),name(),title);

  // write navigation path
  writeNavigationPath(ol);

  startTitle(ol,getOutputFileBase());
  ol.pushGeneratorState();
    ol.disableAllBut(OutputGenerator::Html);
    ol.parseText(shortTitle);
    ol.enableAll();
    ol.disable(OutputGenerator::Html);
    ol.parseText(title);
  ol.popGeneratorState();
  endTitle(ol,getOutputFileBase(),title);

  // write brief or details (if DETAILS_AT_TOP)
  if (Config_getBool("DETAILS_AT_TOP"))
  {
    writeDetailedDocumentation(ol);
    ol.newParagraph();
  }
  else if (!briefDescription().isEmpty())
  {
    ol.parseDoc(briefFile(),briefLine(),this,0,briefDescription(),TRUE,FALSE);
    ol.writeString(" \n");
    ol.pushGeneratorState();
    ol.disable(OutputGenerator::Latex);
    ol.disable(OutputGenerator::RTF);
    ol.disable(OutputGenerator::Man);
    ol.startTextLink(0,"_details");
    ol.parseText(theTranslator->trMore());
    ol.endTextLink();
    ol.enableAll();
    ol.disableAllBut(OutputGenerator::Man);
    ol.newParagraph();
    ol.popGeneratorState();
  }

  if (!Config_getString("GENERATE_TAGFILE").isEmpty()) 
  {
    Doxygen::tagFile << "  <compound kind=\"dir\">" << endl;
    Doxygen::tagFile << "    <name>" << convertToXML(displayName()) << "</name>" << endl;
    Doxygen::tagFile << "    <path>" << convertToXML(name()) << "</path>" << endl;
    Doxygen::tagFile << "    <filename>" << convertToXML(getOutputFileBase()) << Doxygen::htmlFileExtension << "</filename>" << endl;
  }
  
  // write graph dependency graph
  if (Config_getBool("DIRECTORY_GRAPH") && Config_getBool("HAVE_DOT"))
  {
    DotDirDeps dirDep(this);
    if (!dirDep.isTrivial())
    {
      msg("Generating dependency graph for directory %s\n",displayName().data());
      ol.disable(OutputGenerator::Man);
      ol.newParagraph();
      ol.startDirDepGraph();
      //TODO: ol.parseText(theTranslator->trDirDepGraph());
      ol.endDirDepGraph(dirDep);
      ol.enableAll();
    }
  }

  ol.startMemberSections();
  // write subdir list
  if (m_subdirs.count()>0)
  {
    ol.startMemberHeader();
    ol.parseText(theTranslator->trDir(TRUE,FALSE));
    ol.endMemberHeader();
    ol.startMemberList();
    DirDef *dd=m_subdirs.first();
    while (dd)
    {
      ol.startMemberItem(0);
      ol.parseText(theTranslator->trDir(FALSE,TRUE)+" ");
      ol.insertMemberAlign();
      ol.writeObjectLink(dd->getReference(),dd->getOutputFileBase(),0,dd->shortName());
      ol.endMemberItem();
      if (!Config_getString("GENERATE_TAGFILE").isEmpty()) 
      {
        Doxygen::tagFile << "    <dir>" << convertToXML(dd->displayName()) << "</dir>" << endl;
      }
      if (!dd->briefDescription().isEmpty() && Config_getBool("BRIEF_MEMBER_DESC"))
      {
        ol.startMemberDescription();
        ol.parseDoc(briefFile(),briefLine(),dd,0,dd->briefDescription(),FALSE,FALSE);
        ol.endMemberDescription();
        ol.newParagraph();
      }
      dd=m_subdirs.next();
    }

    ol.endMemberList();
  }
  
  // write file list
  if (m_fileList->count()>0)
  {
    ol.startMemberHeader();
    ol.parseText(theTranslator->trFile(TRUE,FALSE));
    ol.endMemberHeader();
    ol.startMemberList();
    FileDef *fd=m_fileList->first();
    while (fd)
    {
      ol.startMemberItem(0);
      ol.docify(theTranslator->trFile(FALSE,TRUE)+" ");
      ol.insertMemberAlign();
      if (fd->isLinkable())
      {
        ol.writeObjectLink(fd->getReference(),fd->getOutputFileBase(),0,fd->name());
      }
      else
      {
        ol.startBold();
        ol.docify(fd->name()); 
        ol.endBold();
      }
      if (fd->generateSourceFile())
      {
        ol.pushGeneratorState();
        ol.disableAllBut(OutputGenerator::Html);
        ol.docify(" ");
        ol.startTextLink(fd->includeName(),0);
        ol.docify("[");
        ol.parseText(theTranslator->trCode());
        ol.docify("]");
        ol.endTextLink();
        ol.popGeneratorState();
      }
      if (!Config_getString("GENERATE_TAGFILE").isEmpty()) 
      {
        Doxygen::tagFile << "    <file>" << convertToXML(fd->name()) << "</file>" << endl;
      }
      ol.endMemberItem();
      if (!fd->briefDescription().isEmpty() && Config_getBool("BRIEF_MEMBER_DESC"))
      {
        ol.startMemberDescription();
        ol.parseDoc(briefFile(),briefLine(),fd,0,fd->briefDescription(),FALSE,FALSE);
        ol.endMemberDescription();
        ol.newParagraph();
      }
      fd=m_fileList->next();
    }
    ol.endMemberList();
  }
  ol.endMemberSections();

  if (!Config_getString("GENERATE_TAGFILE").isEmpty()) 
  {
    writeDocAnchorsToTagFile();
    Doxygen::tagFile << "  </compound>" << endl;
  }


  if (!Config_getBool("DETAILS_AT_TOP"))
  {
    writeDetailedDocumentation(ol);
  }
  
  endFile(ol); 
  ol.popGeneratorState();
}

#if 0
void DirDef::writePathFragment(OutputList &ol) const
{
  if (m_parent)
  {
    m_parent->writePathFragment(ol);
    ol.writeString("&nbsp;/&nbsp;");
  }
  ol.writeObjectLink(getReference(),getOutputFileBase(),0,shortName());
}

void DirDef::writeNavigationPath(OutputList &ol)
{
  ol.pushGeneratorState();
  ol.disableAllBut(OutputGenerator::Html);

  ol.writeString("<div class=\"nav\">\n");
  writePathFragment(ol);
  ol.writeString("</div>\n");

  ol.popGeneratorState();
}
#endif

void DirDef::setLevel()
{
  if (m_level==-1) // level not set before
  {
    DirDef *p = parent();
    if (p)
    {
      p->setLevel();
      m_level = p->level()+1;
    }
    else
    {
      m_level = 0;
    }
  }
}

/** Add as "uses" dependency between \a this dir and \a dir,
 *  that was caused by a dependency on file \a fd.
 */ 
void DirDef::addUsesDependency(DirDef *dir,FileDef *srcFd,
                               FileDef *dstFd,bool inherited)
{
  if (this==dir) return; // do not add self-dependencies
  //printf("  > add dependency %s->%s due to %s\n",shortName().data(),
  //    dir->shortName().data(),fd->name().data());

  // levels match => add direct dependency
  bool added=FALSE;
  UsedDir *usedDir = m_usedDirs->find(dir->getOutputFileBase());
  if (usedDir) // dir dependency already present
  {
     FilePair *usedPair = usedDir->findFilePair(
         srcFd->getOutputFileBase()+dstFd->getOutputFileBase());
     if (usedPair==0) // new file dependency
     {
       //printf("  => new file\n");
       usedDir->addFileDep(srcFd,dstFd); 
       added=TRUE;
     }
     else
     {
       // dir & file dependency already added
     }
  }
  else // new directory dependency
  {
    //printf("  => new file\n");
    usedDir = new UsedDir(dir,inherited);
    usedDir->addFileDep(srcFd,dstFd); 
    m_usedDirs->insert(dir->getOutputFileBase(),usedDir);
    added=TRUE;
  }
  if (added && dir->parent())
  {
    // add relation to parent of used dir
    addUsesDependency(dir->parent(),srcFd,dstFd,inherited);
  }
  if (parent())
  {
    // add relation for the parent of this dir as well
    parent()->addUsesDependency(dir,srcFd,dstFd,TRUE);
  }
}

/** Computes the dependencies between directories
 */
void DirDef::computeDependencies()
{
  FileList *fl = m_fileList;
  if (fl) 
  {
    QListIterator<FileDef> fli(*fl);
    FileDef *fd;
    for (fli.toFirst();(fd=fli.current());++fli) // foreach file in dir dd
    {
      //printf("** dir=%s file=%s\n",shortName().data(),fd->name().data());
      QList<IncludeInfo> *ifl = fd->includeFileList();
      if (ifl)
      {
        QListIterator<IncludeInfo> ifli(*ifl); 
        IncludeInfo *ii;
        for (ifli.toFirst();(ii=ifli.current());++ifli) // foreach include file
        {
          //printf("  > %s\n",ii->includeName.data());
          if (ii->fileDef && ii->fileDef->isLinkable()) // linkable file
          {
            DirDef *usedDir = ii->fileDef->getDirDef();
            if (usedDir)
            {
              // add dependency: thisDir->usedDir
              addUsesDependency(usedDir,fd,ii->fileDef,FALSE);
            }
          } 
        }
      }
    }
  }
}

bool DirDef::isParentOf(DirDef *dir) const
{
  if (dir->parent()==this) // this is a parent of dir 
    return TRUE;
  else if (dir->parent()) // repeat for the parent of dir
    return isParentOf(dir->parent()); 
  else
    return FALSE;
}

bool DirDef::depGraphIsTrivial() const
{
  return FALSE;
}

//----------------------------------------------------------------------

int FilePairDict::compareItems(GCI item1,GCI item2)
{
  FilePair *left  = (FilePair*)item1;
  FilePair *right = (FilePair*)item2;
  int orderHi = stricmp(left->source()->name(),right->source()->name());
  int orderLo = stricmp(left->destination()->name(),right->destination()->name());
  return orderHi==0 ? orderLo : orderHi;
}

//----------------------------------------------------------------------

UsedDir::UsedDir(DirDef *dir,bool inherited) :
   m_dir(dir), m_filePairs(7), m_inherited(inherited)
{
  m_filePairs.setAutoDelete(TRUE);
}

UsedDir::~UsedDir()
{
}


void UsedDir::addFileDep(FileDef *srcFd,FileDef *dstFd)
{
  m_filePairs.inSort(srcFd->getOutputFileBase()+dstFd->getOutputFileBase(),
                     new FilePair(srcFd,dstFd));
}

FilePair *UsedDir::findFilePair(const char *name)
{
  QCString n=name;
  return n.isEmpty() ? 0 : m_filePairs.find(n);
}

DirDef *DirDef::createNewDir(const char *path)
{
  ASSERT(path!=0);
  DirDef *dir = Doxygen::directories.find(path);
  if (dir==0) // new dir
  {
    //printf("Adding new dir %s\n",path);
    dir = new DirDef(path);
    //printf("createNewDir %s short=%s\n",path,dir->shortName().data());
    Doxygen::directories.inSort(path,dir);
  }
  return dir;
}

bool DirDef::matchPath(const QCString &path,QStrList &l)
{
  const char *s=l.first();
  while (s)
  {
    QCString prefix = s;
    if (stricmp(prefix.left(path.length()),path)==0) // case insensitive compare
    {
      return TRUE;
    }
    s = l.next();
  }
  return FALSE;
}

/*! strip part of \a path if it matches
 *  one of the paths in the Config_getList("STRIP_FROM_PATH") list
 */
DirDef *DirDef::mergeDirectoryInTree(const QCString &path)
{
  //printf("DirDef::mergeDirectoryInTree(%s)\n",path.data());
  int p=0,i=0;
  DirDef *dir=0;
  while ((i=path.find('/',p))!=-1)
  {
    QCString part=path.left(i+1);
    if (!matchPath(part,Config_getList("STRIP_FROM_PATH")) && part!="/")
    {
      dir=createNewDir(part); 
    }
    p=i+1;
  }
  return dir;
}

void DirDef::writeDepGraph(QTextStream &t)
{
    t << "digraph G {\n";
    if (Config_getBool("DOT_TRANSPARENT"))
    {
      t << "  bgcolor=transparent;\n";
    }
    t << "  compound=true\n";
    t << "  node [ fontsize=10, fontname=\"Helvetica\"];\n";
    t << "  edge [ labelfontsize=9, labelfontname=\"Helvetica\"];\n";

    QDict<DirDef> dirsInGraph(257);
    
    dirsInGraph.insert(getOutputFileBase(),this);
    if (parent())
    {
      t << "  subgraph cluster" << parent()->getOutputFileBase() << " {\n";
      t << "    graph [ bgcolor=\"#ddddee\", pencolor=\"black\", label=\"" 
        << parent()->shortName() 
        << "\" fontname=\"Helvetica\", fontsize=10, URL=\"";
      t << parent()->getOutputFileBase() << Doxygen::htmlFileExtension;
      t << "\"]\n";
    }
    if (isCluster())
    {
      t << "  subgraph cluster" << getOutputFileBase() << " {\n";
      t << "    graph [ bgcolor=\"#eeeeff\", pencolor=\"black\", label=\"\""
        << " URL=\"" << getOutputFileBase() << Doxygen::htmlFileExtension 
        << "\"];\n";
      t << "    " << getOutputFileBase() << " [shape=plaintext label=\"" 
        << shortName() << "\"];\n";

      // add nodes for sub directories
      QListIterator<DirDef> sdi(m_subdirs);
      DirDef *sdir;
      for (sdi.toFirst();(sdir=sdi.current());++sdi)
      {
        t << "    " << sdir->getOutputFileBase() << " [shape=box label=\"" 
          << sdir->shortName() << "\"";
        if (sdir->isCluster())
        {
          t << " color=\"red\"";
        }
        else
        {
          t << " color=\"black\"";
        }
        t << " fillcolor=\"white\" style=\"filled\"";
        t << " URL=\"" << sdir->getOutputFileBase() 
          << Doxygen::htmlFileExtension << "\"";
        t << "];\n";
        dirsInGraph.insert(sdir->getOutputFileBase(),sdir);
      }
      t << "  }\n";
    }
    else
    {
      t << "  " << getOutputFileBase() << " [shape=box, label=\"" 
        << shortName() << "\", style=\"filled\", fillcolor=\"#eeeeff\","
        << " pencolor=\"black\", URL=\"" << getOutputFileBase() 
        << Doxygen::htmlFileExtension << "\"];\n";
    }
    if (parent())
    {
      t << "  }\n";
    }

    // add nodes for other used directories
    QDictIterator<UsedDir> udi(*m_usedDirs);
    UsedDir *udir;
    //printf("*** For dir %s\n",shortName().data());
    for (udi.toFirst();(udir=udi.current());++udi) 
      // for each used dir (=directly used or a parent of a directly used dir)
    {
      const DirDef *usedDir=udir->dir();
      DirDef *dir=this;
      while (dir)
      {
        //printf("*** check relation %s->%s same_parent=%d !%s->isParentOf(%s)=%d\n",
        //    dir->shortName().data(),usedDir->shortName().data(),
        //    dir->parent()==usedDir->parent(),
        //    usedDir->shortName().data(),
        //    shortName().data(),
        //    !usedDir->isParentOf(this)
        //    );
        if (dir!=usedDir && dir->parent()==usedDir->parent() && !usedDir->isParentOf(this))
          // include if both have the same parent (or no parent)
        {
          t << "  " << usedDir->getOutputFileBase() << " [shape=box label=\"" 
            << usedDir->shortName() << "\"";
          if (usedDir->isCluster())
          {
            if (!Config_getBool("DOT_TRANSPARENT"))
            {
              t << " fillcolor=\"white\" style=\"filled\"";
            }
            t << " color=\"red\"";
          }
          t << " URL=\"" << usedDir->getOutputFileBase() 
            << Doxygen::htmlFileExtension << "\"];\n";
          dirsInGraph.insert(usedDir->getOutputFileBase(),usedDir);
          break;
        }
        dir=dir->parent();
      }
    }

    // add relations between all selected directories
    DirDef *dir;
    QDictIterator<DirDef> di(dirsInGraph);
    for (di.toFirst();(dir=di.current());++di) // foreach dir in the graph
    {
      QDictIterator<UsedDir> udi(*dir->usedDirs());
      UsedDir *udir;
      for (udi.toFirst();(udir=udi.current());++udi) // foreach used dir
      {
        const DirDef *usedDir=udir->dir();
        if ((dir!=this || !udir->inherited()) &&     // only show direct dependendies for this dir
            (usedDir!=this || !udir->inherited()) && // only show direct dependendies for this dir
            !usedDir->isParentOf(dir) &&             // don't point to own parent
            dirsInGraph.find(usedDir->getOutputFileBase())) // only point to nodes that are in the graph
        {
          QCString relationName;
          relationName.sprintf("dir_%06d_%06d",dir->m_dirCount,usedDir->m_dirCount);
          if (Doxygen::dirRelations.find(relationName)==0)
          {
            // new relation
            Doxygen::dirRelations.append(relationName,
                new DirRelation(relationName,dir,udir));
          }
          int nrefs = udir->filePairs().count();
          t << "  " << dir->getOutputFileBase() << "->" 
                    << usedDir->getOutputFileBase();
          t << " [headlabel=\"" << nrefs << "\", labeldistance=1.5";
          t << " headhref=\"" << relationName << Doxygen::htmlFileExtension 
            << "\"];\n";
        }
      }
    }

    t << "}\n";
}

//----------------------------------------------------------------------

static void writePartialDirPath(OutputList &ol,const DirDef *root,const DirDef *target)
{
  if (target->parent()!=root) 
  {
    writePartialDirPath(ol,root,target->parent());
    ol.writeString("&nbsp;/&nbsp;");
  }
  ol.writeObjectLink(target->getReference(),target->getOutputFileBase(),0,target->shortName());
}

static void writePartialFilePath(OutputList &ol,const DirDef *root,const FileDef *fd)
{
  if (fd->getDirDef() && fd->getDirDef()!=root)
  {
    writePartialDirPath(ol,root,fd->getDirDef());
    ol.writeString("&nbsp;/&nbsp;");
  }
  if (fd->isLinkable())
  {
    ol.writeObjectLink(fd->getReference(),fd->getOutputFileBase(),0,fd->name());
  }
  else
  {
    ol.startBold();
    ol.docify(fd->name());
    ol.endBold();
  }
}

void DirRelation::writeDocumentation(OutputList &ol)
{
  ol.pushGeneratorState();
  ol.disableAllBut(OutputGenerator::Html);

  QCString shortTitle=m_src->shortName()+" &rarr; "+
                      m_dst->dir()->shortName()+" Relation";//theTranslator->trDirRelation(m_shortName);
  QCString title=m_src->displayName()+" -> "+
                 m_dst->dir()->shortName()+" Relation";//theTranslator->trDirRelation(m_dispName);
  startFile(ol,getOutputFileBase(),getOutputFileBase(),title);

  // write navigation path
  m_src->writeNavigationPath(ol);

  //startTitle(ol,getOutputFileBase());
  //  ol.parseText(shortTitle);
  //endTitle(ol,getOutputFileBase(),title);
  ol.writeString("<h3>"+shortTitle+"</h3>");
  
  ol.writeString("<table class=\"dirtab\">");
  ol.writeString("<tr class=\"dirtab\">");
  ol.writeString("<th class=\"dirtab\">File in ");
  m_src->writePathFragment(ol);
  ol.writeString("</th>");
  ol.writeString("<th class=\"dirtab\">Includes file in ");
  m_dst->dir()->writePathFragment(ol);
  ol.writeString("</th>");
  ol.writeString("</tr>");

  SDict<FilePair>::Iterator fpi(m_dst->filePairs());
  FilePair *fp;
  for (fpi.toFirst();(fp=fpi.current());++fpi)
  {
    ol.writeString("<tr class=\"dirtab\">");
    ol.writeString("<td class=\"dirtab\">");
    writePartialFilePath(ol,m_src,fp->source());
    ol.writeString("</td>");
    ol.writeString("<td class=\"dirtab\">");
    writePartialFilePath(ol,m_dst->dir(),fp->destination());
    ol.writeString("</td>");
    ol.writeString("</tr>");
  }
  ol.writeString("</table>");
  
  endFile(ol); 
  ol.popGeneratorState();
}

//----------------------------------------------------------------------
// external functions

void buildDirectories()
{
  // for each input file
  FileNameListIterator fnli(Doxygen::inputNameList); 
  FileName *fn;
  for (fnli.toFirst();(fn=fnli.current());++fnli)
  {
    FileNameIterator fni(*fn);
    FileDef *fd;
    for (;(fd=fni.current());++fni)
    {
      //printf("buildDirectories %s\n",fd->name().data());
      if (fd->getReference().isEmpty() && !fd->isDocumentationFile())
      {
        DirDef *dir;
        if ((dir=Doxygen::directories.find(fd->getPath()))==0) // new directory
        {
          dir = DirDef::mergeDirectoryInTree(fd->getPath());
        }
        if (dir) dir->addFile(fd);
      }
      else
      {
        // do something for file imported via tag files.
      }
    }
  }

  //DirDef *root = new DirDef("root:");
  // compute relations between directories => introduce container dirs.
  DirDef *dir;
  DirSDict::Iterator sdi(Doxygen::directories);
  for (sdi.toFirst();(dir=sdi.current());++sdi)
  {
    //printf("New dir %s\n",dir->displayName().data());
    QCString name = dir->name();
    int i=name.findRev('/',name.length()-2);
    if (i>0)
    {
      DirDef *parent = Doxygen::directories.find(name.left(i+1));
      //if (parent==0) parent=root;
      if (parent) 
      {
        parent->addSubDir(dir); 
        //printf("DirDef::addSubdir(): Adding subdir\n%s to\n%s\n",
        //  dir->displayName().data(), parent->displayName().data());
      }
    }
  }
}

void computeDirDependencies()
{
  DirDef *dir;
  DirSDict::Iterator sdi(Doxygen::directories);
  // compute nesting level for each directory
  for (sdi.toFirst();(dir=sdi.current());++sdi)
  {
    dir->setLevel();
  }
  // compute uses dependencies between directories
  for (sdi.toFirst();(dir=sdi.current());++sdi)
  {
    dir->computeDependencies();
  }

#if 0
  printf("-------------------------------------------------------------\n");
  // print dependencies (for debugging)
  for (sdi.toFirst();(dir=sdi.current());++sdi)
  {
    if (dir->usedDirs())
    {
      QDictIterator<UsedDir> udi(*dir->usedDirs());
      UsedDir *usedDir;
      for (udi.toFirst();(usedDir=udi.current());++udi)
      {
        printf("%s depends on %s due to ",
            dir->shortName().data(),usedDir->dir()->shortName().data());
        QDictIterator<FileDef> fdi(usedDir->files());
        FileDef *fd;
        for (fdi.toFirst();(fd=fdi.current());++fdi)
        {
          printf("%s ",fd->name().data());
        }
        printf("\n");
      }
    }
  }
  printf("^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^\n");
#endif
}


void writeDirDependencyGraph(const char *dirName)
{
  QString path;
  DirDef *dir;
  DirSDict::Iterator sdi(Doxygen::directories);
  QFile htmlPage(QCString(dirName)+"/dirdeps.html");
  if (htmlPage.open(IO_WriteOnly))
  {
    QTextStream out(&htmlPage);
    out << "<html><body>";
    for (sdi.toFirst();(dir=sdi.current());++sdi)
    {
      path=dirName;
      path+="/";
      path+=dir->getOutputFileBase();
      path+="_dep.dot";
      out << "<h4>" << dir->displayName() << "</h4>" << endl;
      out << "<img src=\"" << dir->getOutputFileBase() << "_dep.gif\">" << endl;
      QFile f(path);
      if (f.open(IO_WriteOnly))
      {
        QTextStream t(&f);
        dir->writeDepGraph(t);
      }
      f.close();

      QCString imgExt = Config_getEnum("DOT_IMAGE_FORMAT");
      QCString outFile = QCString(dirName)+"/"+
                         dir->getOutputFileBase()+"_dep."+imgExt; 
      DotRunner dotRun(path);
      dotRun.addJob(imgExt,outFile);
      dotRun.run();
      
      //QCString dotArgs(4096);
      //dotArgs.sprintf("%s -Tgif -o %s",path.data(),outFile.data());
      //if (iSystem(Config_getString("DOT_PATH")+"dot",dotArgs)!=0)
      //{
      //  err("Problems running dot. Check your installation!\n");
      //}
    }
    out << "</body></html>";
  }
  htmlPage.close();
}

void generateDirDocs(OutputList &ol)
{
  DirDef *dir;
  DirSDict::Iterator sdi(Doxygen::directories);
  for (sdi.toFirst();(dir=sdi.current());++sdi)
  {
    dir->writeDocumentation(ol);
  }
  if (Config_getBool("DIRECTORY_GRAPH"))
  {
    SDict<DirRelation>::Iterator rdi(Doxygen::dirRelations);
    DirRelation *dr;
    for (rdi.toFirst();(dr=rdi.current());++rdi)
    {
      dr->writeDocumentation(ol);
    }
  }
}


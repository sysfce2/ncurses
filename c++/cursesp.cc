// * this is for making emacs happy: -*-Mode: C++;-*-
/*
  written and
  Copyright (C) 1993 by Anatoly Ivasyuk (anatoly@nick.csh.rit.edu)

  Modified by Juergen Pfeifer, April 1997
*/

#include "internal.h"

MODULE_ID("$Id: cursesp.cc,v 1.8 1997/07/13 20:33:05 juergen Exp $")

#ifdef __GNUG__
#  pragma  implementation
#endif

#include "cursesp.h"

NCursesPanel::NCursesPanel(int lines,
			   int cols,
			   int begin_y,
			   int begin_x)
  : NCursesWindow(lines, cols, begin_y, begin_x) {
    
    p = ::new_panel(w);
    if (!p)
      OnError(ERR);
    
    UserHook* hook = new UserHook;
    hook->m_user  = NULL;
    hook->m_back  = this;
    hook->m_owner = p;
    ::set_panel_userptr(p, (void *)hook);
}


NCursesPanel::~NCursesPanel() {
  UserHook* hook = (UserHook*)::panel_userptr(p);
  assert(hook && hook->m_back==this && hook->m_owner==p);
  delete hook;
  ::del_panel(p);
  ::update_panels();
  ::doupdate();
}

void
NCursesPanel::redraw() {
  PANEL *pan;
  
  pan = ::panel_above(NULL);
  while (pan) {
    ::touchwin(panel_window(pan));
    pan = ::panel_above(pan);
  }
  ::update_panels();
  ::doupdate();
}

void
NCursesPanel::refresh() {
  ::update_panels();
  ::doupdate();
}

void
NCursesPanel::boldframe(const char *title, const char* btitle) {
  standout();
  frame(title, btitle);
  standend();
}

void
NCursesPanel::frame(const char *title,const char *btitle) {
  int err = OK;
  if (!title && !btitle) {
    err = box();
  } 
  else {
    err = box();
    if (err==OK)
      label(title,btitle); 
  }
  OnError(err);
}

void
NCursesPanel::label(const char *tLabel, const char *bLabel) {
  if (tLabel) 
    centertext(0,tLabel);
  if (bLabel) 
    centertext(maxy(),bLabel);
}

void
NCursesPanel::centertext(int row,const char *label) {
  if (label) 
    OnError(addstr(row,(maxx() - strlen(label)) / 2, label));
}

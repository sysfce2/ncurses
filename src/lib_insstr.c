

/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992-1995                        *
*                          by Zeyd M. Ben-Halim                            *
*                          zmbenhal@netcom.com                             *
*                                                                          *
*        Permission is hereby granted to reproduce and distribute ncurses  *
*        by any means and for any fee, whether alone or as part of a       *
*        larger distribution, in source or in binary form, PROVIDED        *
*        this notice is included with any such distribution, not removed   *
*        from header files, and is reproduced in any documentation         *
*        accompanying it or the applications linked with it.               *
*                                                                          *
*        ncurses comes AS IS with no warranty, implied or expressed.       *
*                                                                          *
***************************************************************************/


/*
**	lib_insstr.c
**
**	The routine winsnstr().
**
*/

#include "curses.priv.h"
#include <ctype.h>

int winsnstr(WINDOW *win, char *const str, int n)
{
int oy = win->_cury;
int ox = win->_curx;
char	*cp;

	T(("winsstr(%p,'%s',%d) called", win, str, n));

	for (cp = str; *cp && (n > 0 || (cp - str) >= n); cp++)
	{
		if (*cp == '\n' || *cp == '\r' || *cp == '\t' || *cp == '\b')
			addch(*cp);
		else if (iscntrl(*cp))
		{
			winsch(win, ' ' + *cp);
			winsch(win, '^');
			win->_curx += 2;
		}
		else
		{
			winsch(win, *cp);
			win->_curx++;
		}

		if (win->_curx > win->_maxx)
			win->_curx = win->_maxx;
	}
	
	win->_curx = ox;
	win->_cury = oy;
	return OK;
}


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
**	lib_newwin.c
**
**	The routines newwin(), subwin() and their dependent
**
*/

#include <stdlib.h>
#include "curses.priv.h"

WINDOW * newwin(int num_lines, int num_columns, int begy, int begx)
{
WINDOW	*win;
chtype	*ptr;
int	i, j;

	T(("newwin(%d,%d,%d,%d) called", num_lines, num_columns, begy, begx));

	if (num_lines == 0)
	    num_lines = screen_lines - begy;
	if (num_columns == 0)
	    num_columns = screen_columns - begx;

	if (num_columns + begx > SP->_columns || num_lines + begy > SP->_lines)
		return NULL;

	if ((win = makenew(num_lines, num_columns, begy, begx)) == NULL)
		return NULL;

	for (i = 0; i < num_lines; i++) {
	    if ((win->_line[i].text = TypeAllocN(chtype, num_columns)) == NULL) {
			for (j = 0; j < i; j++)
			    free(win->_line[j].text);

			free(win->_line);
			free(win);

			return NULL;
	    }
	    else
		for (ptr = win->_line[i].text; ptr < win->_line[i].text + num_columns; )
		    *ptr++ = ' ';
	}

	T(("newwin: returned window is %p", win));

	return(win);
}

WINDOW * derwin(WINDOW *orig, int num_lines, int num_columns, int begy, int begx)
{
WINDOW	*win;
int	i;

	T(("derwin(%p, %d,%d,%d,%d) called", orig, num_lines, num_columns, begy, begx));

	/*
	** make sure window fits inside the original one
	*/
	if ( begy < 0 || begx < 0) 
		return NULL;
	if ( begy + num_lines > orig->_maxy + 1
		|| begx + num_columns > orig->_maxx + 1)
	    return NULL;

	if (num_lines == 0)
	    num_lines = orig->_maxy - orig->_begy - begy;

	if (num_columns == 0)
	    num_columns = orig->_maxx - orig->_begx - begx;

	if ((win = makenew(num_lines, num_columns, orig->_begy + begy, orig->_begx + begx)) == NULL)
	    return NULL;

	win->_pary = begy;
	win->_parx = begx;
	win->_attrs = orig->_attrs;
	win->_bkgd = orig->_bkgd;

	for (i = 0; i < num_lines; i++)
	    win->_line[i].text = &orig->_line[begy++].text[begx];

	win->_flags = _SUBWIN;
	win->_parent = orig;

	T(("derwin: returned window is %p", win));

	return(win);
}


WINDOW *subwin(WINDOW *w, int l, int c, int y, int x)
{
	T(("subwin(%p, %d, %d, %d, %d) called", w, l, c, y, x));
	T(("parent has begy = %d, begx = %d", w->_begy, w->_begx));

	return derwin(w, l, c, y - w->_begy, x - w->_begx); 
}

WINDOW *
makenew(int num_lines, int num_columns, int begy, int begx)
{
int	i;
WINDOW	*win;

	T(("makenew(%d,%d,%d,%d)", num_lines, num_columns, begy, begx));

	if (num_lines <= 0
	 || num_columns <= 0)
	 	return NULL;

	if ((win = TypeAllocN(WINDOW, 1)) == NULL)
		return NULL;            

	if ((win->_line = TypeAllocN(struct ldat, num_lines)) == NULL) {
		free(win);
		return NULL;               
	}

	win->_curx       = 0;
	win->_cury       = 0;
	win->_maxy       = num_lines - 1;
	win->_maxx       = num_columns - 1;
	win->_begy       = begy;
	win->_begx       = begx;

	win->_flags      = 0;
	win->_attrs      = A_NORMAL;
	win->_bkgd	 = ' ';

	win->_clear      = (num_lines == screen_lines  &&  num_columns == screen_columns);
	win->_idlok      = FALSE;
	win->_idcok      = TRUE;
	win->_scroll     = FALSE;
	win->_leave      = FALSE;
	win->_use_keypad = FALSE;
	win->_delay    	 = -1;   
	win->_immed	 = FALSE;
	win->_sync	 = 0;
	win->_parx	 = -1;
	win->_pary	 = -1;
	win->_parent	 = (WINDOW *)NULL;

	win->_regtop     = 0;
	win->_regbottom  = num_lines - 1;

	for (i = 0; i < num_lines; i++)
	{
	    win->_line[i].firstchar = win->_line[i].lastchar = _NOCHANGE;
	    win->_line[i].oldindex = i;
	}

	if (begx + num_columns == screen_columns) {
		win->_flags |= _ENDLINE;

		if (begx == 0  &&  num_lines == screen_lines  &&  begy == 0)
			win->_flags |= _FULLWIN;

		if (begy + num_lines == screen_lines)
			win->_flags |= _SCROLLWIN;
	}

	return(win);
}
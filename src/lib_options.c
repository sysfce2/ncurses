

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
**	lib_options.c
**
**	The routines to handle option setting.
**
*/

#include <stdlib.h>
#include "terminfo.h"
#include "curses.priv.h"

int idlok(WINDOW *win,  bool flag)
{
	T(("idlok(%p,%d) called", win, flag));

   	win->_idlok = flag && (has_il() || change_scroll_region);
	return OK; 
}


int idcok(WINDOW *win, bool flag)
{
	T(("idcok(%p,%d) called", win, flag));

	win->_idcok = flag && has_ic();

	return OK; 
}


int clearok(WINDOW *win, bool flag)
{
	T(("clearok(%p,%d) called", win, flag));

   	if (win == curscr)
	    newscr->_clear = flag;
	else
	    win->_clear = flag;
	return OK; 
}


int immedok(WINDOW *win, bool flag)
{
	T(("immedok(%p,%d) called", win, flag));

   	win->_immed = flag;
	return OK; 
}

int leaveok(WINDOW *win, bool flag)
{
	T(("leaveok(%p,%d) called", win, flag));

   	win->_leave = flag;
   	if (flag == TRUE)
   		curs_set(0);
   	else
   		curs_set(1);
	return OK; 
}


int scrollok(WINDOW *win, bool flag)
{
	T(("scrollok(%p,%d) called", win, flag));

   	win->_scroll = flag;
	return OK; 
}

int halfdelay(int t)
{
	T(("halfdelay(%d) called", t));

	if (t < 1 || t > 255)
		return ERR;

	cbreak();
	SP->_cbreak = t+1;
	return OK;
}

int nodelay(WINDOW *win, bool flag)
{
	T(("nodelay(%p,%d) called", win, flag));

   	if (flag == TRUE)
		win->_delay = 0;
	else win->_delay = -1;
	return OK;
}

int notimeout(WINDOW *win, bool f)
{
	T(("notimout(%p,%d) called", win, f));

	win->_notimeout = f;
	return OK;
}

int wtimeout(WINDOW *win, int delay)
{
	T(("wtimeout(%p,%d) called", win, delay));

	win->_delay = delay;
	return OK;
}

static void init_keytry(void);
static void add_to_try(char *, short);

int keypad(WINDOW *win, bool flag)
{
	T(("keypad(%p,%d) called", win, flag));

   	win->_use_keypad = flag;

	if (flag  &&  keypad_xmit)
	    putp(keypad_xmit);
	else if (! flag  &&  keypad_local)
	    putp(keypad_local);
	    
	if (SP->_keytry == UNINITIALISED)
	    init_keytry();
	return OK; 
}



int meta(WINDOW *win, bool flag)
{
	T(("meta(%p,%d) called", win, flag));

	SP->_use_meta = flag;

	if (flag  &&  meta_on)
	    putp(meta_on);
	else if (! flag  &&  meta_off)
	    putp(meta_off);
	return OK; 
}

/*
**      init_keytry()
**
**      Construct the try for the current terminal's keypad keys.
**
*/


static struct  try *newtry;

static void init_keytry(void)
{
    newtry = NULL;
	
#include "keys.tries"

	SP->_keytry = newtry;
}


static void add_to_try(char *str, short code)
{
static bool     out_of_memory = FALSE;
struct try      *ptr, *savedptr;

	if (! str  ||  out_of_memory)
	    	return;
	
	if (newtry != NULL)    {
    	ptr = savedptr = newtry;
	    
       	for (;;) {
	       	while (ptr->ch != (unsigned char) *str
		       &&  ptr->sibling != NULL)
	       		ptr = ptr->sibling;
	    
	       	if (ptr->ch == (unsigned char) *str) {
	    		if (*(++str)) {
	           		if (ptr->child != NULL)
	           			ptr = ptr->child;
               		else
	           			break;
	    		} else {
	        		ptr->value = code;
					return;
	   			}
			} else {
	    		if ((ptr->sibling = TypeAllocN(struct try, 1)) == NULL) {
	        		out_of_memory = TRUE;
					return;
	    		}
		    
	    		savedptr = ptr = ptr->sibling;
	    		ptr->child = ptr->sibling = NULL;
	    		ptr->ch = *str++;
	    		ptr->value = (short) NULL;
	    
           		break;
	       	}
	   	} /* end for (;;) */  
	} else {   /* newtry == NULL :: First sequence to be added */
	    	savedptr = ptr = newtry = TypeAllocN(struct try, 1);
	    
	    	if (ptr == NULL) {
	        	out_of_memory = TRUE;
				return;
	    	}
	    
	    	ptr->child = ptr->sibling = NULL;
	    	ptr->ch = *(str++);
	    	ptr->value = (short) NULL;
	}
	
	    /* at this point, we are adding to the try.  ptr->child == NULL */
	    
	while (*str) {
	   	ptr->child = TypeAllocN(struct try, 1);
	    
	   	ptr = ptr->child;
	   
	   	if (ptr == NULL) {
	       	out_of_memory = TRUE;
		
			ptr = savedptr;
			while (ptr != NULL) {
		    	savedptr = ptr->child;
		    	free(ptr);
		    	ptr = savedptr;
			}
		
			return;
		}
	    
	   	ptr->child = ptr->sibling = NULL;
	   	ptr->ch = *(str++);
	   	ptr->value = (short) NULL;
	}
	
	ptr->value = code;
	return;
}

int typeahead(int fd)
{

	T(("typeahead(%d) called", fd));
	SP->_checkfd = fd;
	return OK;
}


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
 *	vidputs(newmode, outc)
 *
 *	newmode is taken to be the logical 'or' of the symbols in curses.h
 *	representing graphic renditions.  The teminal is set to be in all of
 *	the given modes, if possible.
 *
 *	if set-attributes exists
 *		use it to set exactly what you want
 *	else
 *		if exit-attribute-mode exists
 *			turn off everything
 *		else
 *			turn off those which can be turned off and aren't in
 *			newmode.
 *		turn on each mode which should be on and isn't, one by one
 *
 *	NOTE that this algorithm won't achieve the desired mix of attributes
 *	in some cases, but those are probably just those cases in which it is
 *	actually impossible, anyway, so...
 *
 */

#include <string.h>
#include <termcap.h>
#include "curses.priv.h"
#include "terminfo.h"

static void do_color(int pair, int  (*outc)(char))
{
int fg, bg;

	if ( pair == 0 ) {
		tputs(orig_pair, 1, outc);
	} else {
		fg = FG(color_pairs[pair]);
		bg = BG(color_pairs[pair]);

		T(("setting colors: pair = %d, fg = %d, bg = %d\n", pair, fg, bg));

		if (set_a_foreground)
		    tputs(tparm(set_a_foreground, fg), 1, outc);
		else
		    tputs(tparm(set_foreground, fg), 1, outc);
		if (set_a_background)
		    tputs(tparm(set_a_background, bg), 1, outc);
		else
		    tputs(tparm(set_background, bg), 1, outc);
	}
}

#define previous_attr SP->_current_attr

#define SetAttr(flag, s) if ((flag) && (s != 0)) tputs(s, 1, outc)

int vidputs(attr_t newmode, int  (*outc)(char))
{
chtype	turn_off = (~newmode & previous_attr) & (chtype)(~A_COLOR);
chtype	turn_on  = (newmode & ~previous_attr) & (chtype)(~A_COLOR);

	T(("vidputs(%lx) called %s", newmode, _traceattr(newmode)));
	T(("previous attribute was %s", _traceattr(previous_attr)));

	if (newmode == previous_attr)
		return OK;
	if (newmode == A_NORMAL && exit_attribute_mode) {
		if((previous_attr & A_ALTCHARSET) && exit_alt_charset_mode) {
 			tputs(exit_alt_charset_mode, 1, outc);
 			previous_attr &= ~A_ALTCHARSET;
 		}
 		if (previous_attr) 
 			tputs(exit_attribute_mode, 1, outc);
 	
	} else if (set_attributes) {
		if (turn_on || turn_off) {
	    		tputs(tparm(set_attributes,
				(newmode & A_STANDOUT) != 0,
				(newmode & A_UNDERLINE) != 0,
				(newmode & A_REVERSE) != 0,
				(newmode & A_BLINK) != 0,
				(newmode & A_DIM) != 0,
				(newmode & A_BOLD) != 0,
				(newmode & A_INVIS) != 0,
				(newmode & A_PROTECT) != 0,
				(newmode & A_ALTCHARSET) != 0), 1, outc);
		}
	} else {

		T(("turning %s off", _traceattr(turn_off)));
			
		SetAttr(turn_off & A_ALTCHARSET, exit_alt_charset_mode);
		SetAttr(turn_off & A_BOLD,       exit_standout_mode);
		SetAttr(turn_off & A_DIM,        exit_standout_mode);
		SetAttr(turn_off & A_BLINK,      exit_standout_mode);
		SetAttr(turn_off & A_INVIS,      exit_standout_mode);
		SetAttr(turn_off & A_PROTECT,    exit_standout_mode);
		SetAttr(turn_off & A_UNDERLINE,  exit_underline_mode);
   	 	SetAttr(turn_off & A_REVERSE,    exit_standout_mode);
		SetAttr(turn_off & A_STANDOUT,   exit_standout_mode);

#ifdef A_PCCHARSET
		/*
		 * exit_pc_charset_mode is a defined SVr4 terminfo
		 * A_PCCHARSET is an ncurses invention to invoke it
		 */
		SetAttr(turn_off & A_PCCHARSET,  exit_pc_charset_mode);
#endif /* A_PCCHARSET */

		T(("turning %s on", _traceattr(turn_on)));

		SetAttr(turn_on & A_ALTCHARSET,  enter_alt_charset_mode); 
		SetAttr(turn_on & A_BLINK,       enter_blink_mode);
		SetAttr(turn_on & A_BOLD,        enter_bold_mode);
		SetAttr(turn_on & A_DIM,         enter_dim_mode);
		SetAttr(turn_on & A_REVERSE,     enter_reverse_mode);
		SetAttr(turn_on & A_STANDOUT,    enter_standout_mode);
		SetAttr(turn_on & A_PROTECT,     enter_protected_mode);
		SetAttr(turn_on & A_INVIS,       enter_secure_mode);
		SetAttr(turn_on & A_UNDERLINE,   enter_underline_mode);

#ifdef A_PCCHARSET
		/*
		 * enter_pc_charset_mode is a defined SVr4 terminfo
		 * A_PCCHARSET is an ncurses invention to invoke it
		 */
		SetAttr(turn_on & A_PCCHARSET,   enter_pc_charset_mode);
#endif /* A_PCCHARSET */

	}

	if (SP->_coloron) {
	int pair = PAIR_NUMBER(newmode);
	int current_pair = PAIR_NUMBER(previous_attr);

   		T(("old pair = %d -- new pair = %d", current_pair, pair));
   		if (pair != current_pair || turn_off) {
			do_color(pair, outc);
		}
   	}

	previous_attr = newmode;

	T(("vidputs finished"));
	return OK;
}

int vidattr(attr_t newmode)
{

	T(("vidattr(%lx) called", newmode));

	return(vidputs(newmode, _outch));
}

attr_t termattrs(void)
{
	int attrs = A_NORMAL;

	if (enter_alt_charset_mode)
		attrs |= A_ALTCHARSET;

	if (enter_blink_mode)
		attrs |= A_BLINK;

	if (enter_bold_mode)
		attrs |= A_BOLD;

	if (enter_dim_mode)
		attrs |= A_DIM;

	if (enter_reverse_mode)
		attrs |= A_REVERSE;

	if (enter_standout_mode)
		attrs |= A_STANDOUT;

	if (enter_protected_mode)
		attrs |= A_PROTECT;

	if (enter_secure_mode)
		attrs |= A_INVIS;

	if (enter_underline_mode)
		attrs |= A_UNDERLINE;

	if (SP->_coloron)
		attrs |= A_COLOR;

	return(attrs);
}

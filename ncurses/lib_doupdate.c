/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992-1995                        *
*                          Zeyd M. Ben-Halim                               *
*                          zmbenhal@netcom.com                             *
*                          Eric S. Raymond                                 *
*                          esr@snark.thyrsus.com                           *
*                                                                          *
*        Permission is hereby granted to reproduce and distribute ncurses  *
*        by any means and for any fee, whether alone or as part of a       *
*        larger distribution, in source or in binary form, PROVIDED        *
*        this notice is included with any such distribution, and is not    *
*        removed from any of its header files. Mention of ncurses in any   *
*        applications linked with it is highly appreciated.                *
*                                                                          *
*        ncurses comes AS IS with no warranty, implied or expressed.       *
*                                                                          *
***************************************************************************/


/*-----------------------------------------------------------------
 *
 *	lib_doupdate.c
 *
 *	The routine doupdate() and its dependents.  Also _nc_outstr(),
 *	so all physical output is concentrated here (except _nc_outch()
 *	in lib_tputs.c).
 *
 *-----------------------------------------------------------------*/

#include <curses.priv.h>

#if defined(TRACE) && HAVE_SYS_TIMES_H && HAVE_TIMES
#define USE_TRACE_TIMES 1
#else
#define USE_TRACE_TIMES 0
#endif

#if HAVE_SYS_TIME_H && ! SYSTEM_LOOKS_LIKE_SCO
#include <sys/time.h>
#endif

#if USE_TRACE_TIMES
#include <sys/times.h>
#endif

#if USE_FUNC_POLL
#include <stropts.h>
#include <poll.h>
#elif HAVE_SELECT
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif

#include <term.h>

MODULE_ID("$Id: lib_doupdate.c,v 1.79 1997/09/03 15:27:09 Alexander.V.Lukyanov Exp $")

/*
 * This define controls the line-breakout optimization.  Every once in a
 * while during screen refresh, we want to check for input and abort the
 * update if there's some waiting.  CHECK_INTERVAL controls the number of
 * changed lines to be emitted between input checks.
 *
 * Note: Input-check-and-abort is no longer done if the screen is being
 * updated from scratch.  This is a feature, not a bug.
 */
#define CHECK_INTERVAL	5

/*
 * Enable checking to see if doupdate and friends are tracking the true
 * cursor position correctly.  NOTE: this is a debugging hack which will
 * work ONLY on ANSI-compatible terminals!
 */
/* #define POSITION_DEBUG */

static inline chtype ClrBlank ( WINDOW *win );
static int ClrBottom(int total);
static int InsStr( chtype *line, int count );
static void ClearScreen( chtype blank );
static void ClrUpdate( WINDOW *win );
static void DelChar( int count );
static void TransformLine( int const lineno );

#ifdef POSITION_DEBUG
/****************************************************************************
 *
 * Debugging code.  Only works on ANSI-standard terminals.
 *
 ****************************************************************************/

void position_check(int expected_y, int expected_x, char *legend)
/* check to see if the real cursor position matches the virtual */
{
    static char  buf[9];
    int y, x;

    if (_nc_tracing)
	return;

    memset(buf, '\0', sizeof(buf));
    (void) write(1, "\033[6n", 4);	/* only works on ANSI-compatibles */
    (void) read(0, (void *)buf, 8);
    _tracef("probe returned %s", _nc_visbuf(buf));

    /* try to interpret as a position report */
    if (sscanf(buf, "\033[%d;%dR", &y, &x) != 2)
	_tracef("position probe failed in %s", legend);
    else if (y - 1 != expected_y || x - 1 != expected_x)
	_tracef("position seen (%d, %d) doesn't match expected one (%d, %d) in %s",
		y-1, x-1, expected_y, expected_x, legend);
    else
	_tracef("position matches OK in %s", legend);
}
#endif /* POSITION_DEBUG */

/****************************************************************************
 *
 * Optimized update code
 *
 ****************************************************************************/

static inline void GoTo(int const row, int const col)
{
	chtype	oldattr = SP->_current_attr;

	TR(TRACE_MOVE, ("GoTo(%d, %d) from (%d, %d)",
			row, col, SP->_cursrow, SP->_curscol));

#ifdef POSITION_DEBUG
	position_check(SP->_cursrow, SP->_curscol, "GoTo");
#endif /* POSITION_DEBUG */

	/*
	 * Force restore even if msgr is on when we're in an alternate
	 * character set -- these have a strong tendency to screw up the
	 * CR & LF used for local character motions!
	 */
	if ((oldattr & A_ALTCHARSET)
	    || (oldattr && !move_standout_mode))
	{
		TR(TRACE_CHARPUT, ("turning off (%#lx) %s before move",
		   oldattr, _traceattr(oldattr)));
		vidattr(A_NORMAL);
	}

	mvcur(SP->_cursrow, SP->_curscol, row, col);
	SP->_cursrow = row;
	SP->_curscol = col;
}

static inline void PutAttrChar(chtype ch)
{
	if (tilde_glitch && (TextOf(ch) == '~'))
		ch = ('`' | AttrOf(ch));

	TR(TRACE_CHARPUT, ("PutAttrChar(%s) at (%d, %d)",
			  _tracechtype(ch),
			   SP->_cursrow, SP->_curscol));
	UpdateAttrs(ch);
	putc((int)TextOf(ch), SP->_ofp);
#ifdef TRACE
	_nc_outchars++;
#endif /* TRACE */
	SP->_curscol++;
	if (char_padding) {
		TPUTS_TRACE("char_padding");
		putp(char_padding);
	}
}

static bool check_pending(void)
/* check for pending input */
{
	bool have_pending = FALSE;

	/*
	 * Only carry out this check when the flag is zero, otherwise we'll
	 * have the refreshing slow down drastically (or stop) if there's an
	 * unread character available.
	 */
	if(SP->_fifohold != 0)
		return FALSE;

	if (SP->_checkfd >= 0) {
#if USE_FUNC_POLL
		struct pollfd fds[1];
		fds[0].fd = SP->_checkfd;
		fds[0].events = POLLIN;
		if (poll(fds, 1, 0) > 0)
		{
			have_pending = TRUE;
		}
#elif HAVE_SELECT
		fd_set fdset;
		struct timeval ktimeout;

		ktimeout.tv_sec =
		ktimeout.tv_usec = 0;

		FD_ZERO(&fdset);
		FD_SET(SP->_checkfd, &fdset);
		if (select(SP->_checkfd+1, &fdset, NULL, NULL, &ktimeout) > 0)
		{
			have_pending = TRUE;
		}
#endif
	}
	if (have_pending) {
		SP->_fifohold = 5;
		fflush(SP->_ofp);
	}
	return FALSE;
}

/*
 * No one supports recursive inline functions.  However, gcc is quieter if we
 * instantiate the recursive part separately.
 */
#if CC_HAS_INLINE_FUNCS
static void callPutChar(chtype const);
#else
#define callPutChar(ch) PutChar(ch)
#endif

static inline void PutChar(chtype const ch)
/* insert character, handling automargin stuff */
{
    if (!(SP->_cursrow == screen_lines-1 && SP->_curscol == screen_columns-1
		&& auto_right_margin && !eat_newline_glitch))
    {
	PutAttrChar(ch);	/* normal case */
    }
    else if (!auto_right_margin	/* maybe we can suppress automargin */
	     || (enter_am_mode && exit_am_mode))
    {
	bool old_am = auto_right_margin;

	if (old_am)
	{
	    TPUTS_TRACE("exit_am_mode");
	    putp(exit_am_mode);
	}
	PutAttrChar(ch);
	if (old_am)
	{
	    TPUTS_TRACE("enter_am_mode");
	    putp(enter_am_mode);
	}
    }
    else
    {
	GoTo(screen_lines-1,screen_columns-2);
	callPutChar(ch);
	GoTo(screen_lines-1,screen_columns-2);
	if (InsStr(newscr->_line[screen_lines-1].text+screen_columns-2,1)==ERR)
	    return;
    }

    if (SP->_curscol >= screen_columns)
    {
	if (eat_newline_glitch)
	{
	    /*
	     * xenl can manifest two different ways.  The vt100
	     * way is that, when you'd expect the cursor to wrap,
	     * it stays hung at the right margin (on top of the
	     * character just emitted) and doesn't wrap until the
	     * *next* graphic char is emitted.  The c100 way is
	     * to ignore LF received just after an am wrap.
	     *
	     * An aggressive way to handle this would be to
	     * emit CR/LF after the char and then assume the wrap
	     * is done, you're on the first position of the next
	     * line, and the terminal out of its weird state.
	     * Here it's safe to just tell the code that the
	     * cursor is in hyperspace and let the next mvcur()
	     * call straighten things out.
	     */
	    SP->_curscol = -1;
	    SP->_cursrow = -1;
	}
	else if (auto_right_margin)
	{
	    SP->_curscol = 0;
	    SP->_cursrow++;
	}
	else
	{
	    SP->_curscol--;
	}
    }
#ifdef POSITION_DEBUG
    position_check(SP->_cursrow, SP->_curscol, "PutChar");
#endif /* POSITION_DEBUG */
}

/*
 * Issue a given span of characters from an array.
 * Must be functionally equivalent to:
 *	for (i = 0; i < num; i++)
 *	    PutChar(ntext[i]);
 * but can leave the cursor positioned at the middle of the interval.
 *
 * Returns: 0 - cursor is at the end of interval
 *	    1 - cursor is somewhere in the middle
 *
 * This code is optimized using ech and rep.
 */
static inline int EmitRange(const chtype *ntext, int num)
{
    int	i;

    if (erase_chars || repeat_char)
    {
	while (num > 0)
	{
	    int	runcount;
	    chtype ntext0;

	    while (num>1 && ntext[0]!=ntext[1])
	    {
		PutChar(ntext[0]);
		ntext++;
		num--;
	    }
	    ntext0 = ntext[0];
	    if (num==1)
	    {
		PutChar(ntext0);
		return 0;
	    }
	    runcount = 2;

	    while (runcount < num && ntext[runcount] == ntext0)
		runcount++;

	    /*
	     * The cost expression in the middle isn't exactly right.
	     * _cup_cost is an upper bound on the cost for moving to the
	     * end of the erased area, but not the cost itself (which we
	     * can't compute without emitting the move).  This may result
	     * in erase_chars not getting used in some situations for
	     * which it would be marginally advantageous.
	     */
	    if (erase_chars
		&& runcount > SP->_ech_cost + SP->_cup_cost
		&& can_clear_with(ntext0))
	    {
		UpdateAttrs(ntext0);
		putp(tparm(erase_chars, runcount));

		/*
		 * If this is the last part of the given interval,
		 * don't bother moving cursor, since it can be the
		 * last update on the line.
		 */
		if (runcount < num)
		    GoTo(SP->_cursrow, SP->_curscol + runcount);
		else
		    return 1;	/* cursor stays in the middle */
	    }
	    else if (repeat_char && runcount > SP->_rep_cost)
	    {
		bool wrap_possible = (SP->_curscol + runcount >= screen_columns);
		int rep_count = runcount;

		if (wrap_possible)
		    rep_count--;

		UpdateAttrs(ntext0);
		putp(tparm(repeat_char, TextOf(ntext0), rep_count));
		SP->_curscol += rep_count;

		if (wrap_possible)
		    PutChar(ntext0);
	    }
	    else
	    {
		for (i = 0; i < runcount; i++)
		    PutChar(ntext[i]);
	    }
	    ntext += runcount;
	    num -= runcount;
	}
	return 0;
    }

    for (i = 0; i < num; i++)
	PutChar(ntext[i]);
    return 0;
}

/*
 * Output the line in the given range [first .. last]
 *
 * If there's a run of identical characters that's long enough to justify
 * cursor movement, use that also.
 *
 * Returns: same as EmitRange
 */
static int PutRange(
	const chtype *otext,
	const chtype *ntext,
	int row,
	int first, int last)
{
	int j, run;
	int cost = min(SP->_cup_ch_cost, SP->_hpa_ch_cost);

	TR(TRACE_CHARPUT, ("PutRange(%p, %p, %d, %d, %d)",
			 otext, ntext, row, first, last));

	if (otext != ntext
	 && (last-first+1) > cost) {
		for (j = first, run = 0; j <= last; j++) {
			if (otext[j] == ntext[j]) {
				run++;
			} else {
				if (run > cost) {
					int before_run = (j - run);
					EmitRange(ntext+first, before_run-first);
					GoTo(row, first = j);
				}
				run = 0;
			}
		}
	}
	return EmitRange(ntext + first, last-first+1);
}

#if CC_HAS_INLINE_FUNCS
static void callPutChar(chtype const ch)
{
	PutChar(ch);
}
#endif

#define MARK_NOCHANGE(win,row) \
	{ \
		win->_line[row].firstchar = _NOCHANGE; \
		win->_line[row].lastchar = _NOCHANGE; \
		if_USE_SCROLL_HINTS(win->_line[row].oldindex = row); \
	}

int doupdate(void)
{
int	i;
int	nonempty;
#if USE_TRACE_TIMES
struct tms before, after;
#endif /* USE_TRACE_TIMES */

	T((T_CALLED("doupdate()")));

#ifdef TRACE
	if (_nc_tracing & TRACE_UPDATE)
	{
	    if (curscr->_clear)
		_tracef("curscr is clear");
	    else
		_tracedump("curscr", curscr);
	    _tracedump("newscr", newscr);
	}
#endif /* TRACE */

	_nc_signal_handler(FALSE);

	if (SP->_fifohold)
		SP->_fifohold--;

	if (SP->_endwin || SP->_sig_winch)
	{
		/*
		 * This is a transparent extension:  XSI does not address it,
		 * and applications need not know that ncurses can do it.
		 *
		 * Check if the terminal size has changed while curses was off
		 * (this can happen in an xterm, for example), and resize the
		 * ncurses data structures accordingly.
		 */
		_nc_get_screensize();
		resizeterm(LINES, COLS);
	}

	if (SP->_endwin) {

		T(("coming back from shell mode"));
		reset_prog_mode();

		_nc_mvcur_resume();
		_nc_screen_resume();
		_nc_mouse_resume(SP);

		SP->_endwin = FALSE;
	}

#if USE_TRACE_TIMES
	/* zero the metering machinery */
	_nc_outchars = 0;
	(void) times(&before);
#endif /* USE_TRACE_TIMES */

	/*
	 * This is the support for magic-cookie terminals.  The
	 * theory: we scan the virtual screen looking for attribute
	 * turnons.  Where we find one, check to make sure it's
	 * realizable by seeing if the required number of
	 * un-attributed blanks are present before and after the
	 * attributed range; try to shift the range boundaries over
	 * blanks (not changing the screen display) so this becomes
	 * true.  If it is, shift the beginning attribute change
	 * appropriately (the end one, if we've gotten this far, is
	 * guaranteed room for its cookie). If not, nuke the added
	 * attributes out of the span.
	 */
#if USE_XMC_SUPPORT
	if (magic_cookie_glitch > 0) {
	    int	j, k;
	    attr_t rattr = A_NORMAL;

	    for (i = 0; i < screen_lines; i++)
		for (j = 0; j < screen_columns; j++)
		{
		    bool failed = FALSE;
		    chtype turnon = AttrOf(newscr->_line[i].text[j]) & ~rattr;

		    /* is an attribute turned on here? */
		    if (turnon == 0) {
			rattr = AttrOf(newscr->_line[i].text[j]);
			continue;
		    }

		    T(("At (%d, %d): from %s...", i, j, _traceattr(rattr)));
		    T(("...to %s",_traceattr(turnon)));

		    /*
		     * If the attribute change location is a blank with a
		     * "safe" attribute, undo the attribute turnon.  This may
		     * ensure there's enough room to set the attribute before
		     * the first non-blank in the run.
		     */
#define SAFE(a)	!((a) & ~NONBLANK_ATTR)
		    if (TextOf(newscr->_line[i].text[j])==' ' && SAFE(turnon))
		    {
			newscr->_line[i].text[j] &= ~turnon;
			continue;
		    }

		    /* check that there's enough room at start of span */
		    for (k = 1; k <= magic_cookie_glitch; k++)
			if (j-k < 0
				|| TextOf(newscr->_line[i].text[j-k]) != ' '
				|| !SAFE(AttrOf(newscr->_line[i].text[j-k])))
			    failed = TRUE;
		    if (!failed)
		    {
			bool	end_onscreen = FALSE;
			int	m, n = j;

			/* find end of span, if it's onscreen */
			for (m = i; m < screen_lines; m++)
			{
			    for ( ; n < screen_columns; n++)
			    {
				if (AttrOf(newscr->_line[m].text[n]) == rattr)
				{
				    end_onscreen = TRUE;
				    T(("Range attributed with %s ends at (%d, %d)",
				       _traceattr(turnon),m,n));
				    goto foundit;
				}
			    }
			    n = 0;
			}
			T(("Range attributed with %s ends offscreen",
			    _traceattr(turnon)));
		    foundit:;

			if (end_onscreen)
			{
			    chtype	*lastline = newscr->_line[m].text;

			    /*
			     * If there are safely-attributed blanks at the
			     * end of the range, shorten the range.  This will
			     * help ensure that there is enough room at end
			     * of span.
			     */
			    while (n >= 0
				   && TextOf(lastline[n]) == ' '
				   && SAFE(AttrOf(lastline[n])))
				lastline[n--] &= ~turnon;

			    /* check that there's enough room at end of span */
			    for (k = 1; k <= magic_cookie_glitch; k++)
				if (n + k >= screen_columns
					|| TextOf(lastline[n + k]) != ' '
					|| !SAFE(AttrOf(lastline[n+k])))
				    failed = TRUE;
			}
		    }

		    if (failed)
		    {
			int p, q = j;

			T(("Clearing %s beginning at (%d, %d)",
						_traceattr(turnon), i, j));

			/* turn off new attributes over span */
			for (p = i; p < screen_lines; p++)
			{
			    for ( ; q < screen_columns; q++)
			    {
				if (AttrOf(newscr->_line[p].text[q]) == rattr)
				    goto foundend;
				newscr->_line[p].text[q] &= ~turnon;
			    }
			    q = 0;
			}
		    foundend:;
		    }
		    else
		    {
			T(("Cookie space for %s found before (%d, %d)",
						_traceattr(turnon), i, j));

			/*
			 * back up the start of range so there's room
			 * for cookies before the first nonblank character
			 */
			for (k = 1; k <= magic_cookie_glitch; k++)
			    newscr->_line[i].text[j-k] |= turnon;
		    }

		    rattr = AttrOf(newscr->_line[i].text[j]);
		}

#ifdef TRACE
	    /* show altered highlights after magic-cookie check */
	    if (_nc_tracing & TRACE_UPDATE)
	    {
		_tracef("After magic-cookie check...");
		_tracedump("newscr", newscr);
	    }
#endif /* TRACE */
	}
#endif	/* USE_XMC_SUPPORT */

	nonempty = 0;
	if (curscr->_clear) {		/* force refresh ? */
		/* yes, clear all & update */
		T(("clearing and updating curscr"));
		if (is_wintouched(newscr))
			ClrUpdate(newscr);
		else
			ClrUpdate(curscr);
		curscr->_clear = FALSE;	/* reset flag */
		newscr->_clear = FALSE;	/* reset flag */
	} else if (newscr->_clear) {
		T(("clearing and updating newscr"));
		ClrUpdate(newscr);
		newscr->_clear = FALSE;
	} else {
		int changedlines = CHECK_INTERVAL;

		if(check_pending())
		    goto cleanup;

		nonempty = min(screen_lines, newscr->_maxy+1);
#if USE_HASHMAP		/* still 5% slower 960928 */
#if defined(TRACE) || defined(NCURSES_TEST)
		if (_nc_optimize_enable & OPTIMIZE_HASHMAP)
#endif /*TRACE */
			_nc_hash_map();
#elif !USE_SCROLL_HINTS
		_nc_setup_scroll();
#endif
#if defined(TRACE) || defined(NCURSES_TEST)
		if (_nc_optimize_enable & OPTIMIZE_SCROLL)
#endif /*TRACE */
#if USE_SCROLL_HINTS || USE_HASHMAP
			_nc_scroll_optimize();
#else
			_nc_perform_scroll();
#endif

		if (clr_eos)
			nonempty = ClrBottom(nonempty);

		T(("Transforming lines, nonempty %d", nonempty));
		for (i = 0; i < nonempty; i++) {
			/*
			 * Here is our line-breakout optimization.
			 */
			if (changedlines == CHECK_INTERVAL)
			{
			    if (check_pending())
				goto cleanup;
			    changedlines = 0;
			}

			/*
			 * newscr->line[i].firstchar is normally set
			 * by wnoutrefresh.  curscr->line[i].firstchar
			 * is normally set by _nc_scroll_window in the
			 * vertical-movement optimization code,
			 */
			if (newscr->_line[i].firstchar != _NOCHANGE
			 || curscr->_line[i].firstchar != _NOCHANGE)
			{
				TransformLine(i);
				changedlines++;
			}

			/* mark line changed successfully */
			if (i <= newscr->_maxy)
				MARK_NOCHANGE(newscr,i)
			if (i <= curscr->_maxy)
				MARK_NOCHANGE(curscr,i)
		}
	}

	/* put everything back in sync */
	for (i = nonempty; i <= newscr->_maxy; i++)
		MARK_NOCHANGE(newscr,i)
	for (i = nonempty; i <= curscr->_maxy; i++)
		MARK_NOCHANGE(curscr,i)

	curscr->_curx = newscr->_curx;
	curscr->_cury = newscr->_cury;

	GoTo(curscr->_cury, curscr->_curx);

    cleanup:
	/*
	 * Keep the physical screen in normal mode in case we get other
	 * processes writing to the screen.
	 */
	UpdateAttrs(A_NORMAL);

	fflush(SP->_ofp);
	curscr->_attrs = newscr->_attrs;
/*	curscr->_bkgd  = newscr->_bkgd; */

#if USE_TRACE_TIMES
	(void) times(&after);
	TR(TRACE_TIMES, ("Update cost: %ld chars, %ld clocks system time, %ld clocks user time",
	    _nc_outchars,
	    after.tms_stime-before.tms_stime,
	    after.tms_utime-before.tms_utime));
#endif /* USE_TRACE_TIMES */

	_nc_signal_handler(TRUE);

	returnCode(OK);
}

/*
 *	ClrBlank(win)
 *
 *	Returns the attributed character that corresponds to the "cleared"
 *	screen.  If the terminal has the back-color-erase feature, this will be
 *	colored according to the wbkgd() call.
 *
 *	We treat 'curscr' specially because it isn't supposed to be set directly
 *	in the wbkgd() call.  Assume 'stdscr' for this case.
 */
#define BCE_ATTRS (A_NORMAL|A_COLOR)
#define BCE_BKGD(win) (((win) == curscr ? stdscr : (win))->_bkgd)

static inline chtype ClrBlank (WINDOW *win)
{
chtype	blank = BLANK;
	if (back_color_erase)
		blank |= (BCE_BKGD(win) & BCE_ATTRS);
	return blank;
}

/*
**	ClrUpdate(win)
**
**	Update by clearing and redrawing the entire screen.
**
*/

static void ClrUpdate(WINDOW *win)
{
	int i;
	chtype blank = ClrBlank(win);
	int nonempty = min(screen_lines, newscr->_maxy+1);

	T(("ClrUpdate() called"));

	if (win == curscr) {
		/* discard updates */
		for (i = 0; i < screen_lines ; i++) {
			memcpy( newscr->_line[i].text,
				curscr->_line[i].text,
				screen_columns * sizeof(chtype));
		}
	}

	ClearScreen(blank);

	T(("updating screen from scratch"));

	if (clr_eos)
		nonempty = ClrBottom(nonempty);

	for (i = 0; i < nonempty; i++)
		TransformLine(i);
}

/*
**	ClrToEOL(blank)
**
**	Clear to end of current line, starting at the cursor position
*/

static void ClrToEOL(chtype blank)
{
int	j;
bool	needclear = FALSE;

	for (j = SP->_curscol; j < screen_columns; j++)
	{
	    chtype *cp = &(curscr->_line[SP->_cursrow].text[j]);

	    if (*cp != blank)
	    {
		*cp = blank;
		needclear = TRUE;
	    }
	}

	if (needclear)
	{
	    UpdateAttrs(blank);
	    TPUTS_TRACE("clr_eol");
	    if (SP->_el_cost > (screen_columns - SP->_curscol))
	    {
		int count = (screen_columns - SP->_curscol);
		while (count-- > 0)
			PutChar(blank);
	    }
	    else
		putp(clr_eol);
	}
}

/*
**	ClrToEOS(blank)
**
**	Clear to end of screen, starting at the cursor position
*/

static void ClrToEOS(chtype blank)
{
int row, col;

	UpdateAttrs(blank);
	TPUTS_TRACE("clr_eos");
	row = SP->_cursrow;
	tputs(clr_eos, screen_lines-row, _nc_outch);

	for (col = SP->_curscol; col < screen_columns; col++)
		curscr->_line[row].text[col] = blank;

	for (row++; row < screen_lines; row++)
	{
		for (col = 0; col < screen_columns; col++)
			curscr->_line[row].text[col] = blank;
	}
}

/*
 *	ClrBottom(total)
 *
 *	Test if clearing the end of the screen would satisfy part of the
 *	screen-update.  Do this by scanning backwards through the lines in the
 *	screen, checking if each is blank, and one or more are changed.
 */
static int ClrBottom(int total)
{
static	chtype	*tstLine;
static	size_t	lenLine;

int	row, col;
int	top    = total;
int	last   = min(screen_columns, newscr->_maxx+1);
size_t	length = sizeof(chtype) * last;
chtype	blank  = newscr->_line[total-1].text[last-1]; /* lower right char */

	if(!can_clear_with(blank))
		return total;

	if (tstLine == 0)
		tstLine = (chtype *)malloc(length);
	else if (length > lenLine)
		tstLine = (chtype *)realloc(tstLine, length);

	if (tstLine != 0) {
		lenLine = length;
		for (col = 0; col < last; col++)
			tstLine[col] = blank;

		for (row = total-1; row >= 0; row--) {
			if (memcmp(tstLine, newscr->_line[row].text, length))
				break;
			if (memcmp(tstLine, curscr->_line[row].text, length))
				top = row;
		}

		if (top < total) {
			GoTo(top,0);
			ClrToEOS(blank);
			total = top;
		}
	}
#if NO_LEAKS
	FreeAndNull(tstLine);
#endif
	return total;
}


/*
**	TransformLine(lineno)
**
**	Transform the given line in curscr to the one in newscr, using
**	Insert/Delete Character if _nc_idcok && has_ic().
**
**		firstChar = position of first different character in line
**		oLastChar = position of last different character in old line
**		nLastChar = position of last different character in new line
**
**		move to firstChar
**		overwrite chars up to min(oLastChar, nLastChar)
**		if oLastChar < nLastChar
**			insert newLine[oLastChar+1..nLastChar]
**		else
**			delete oLastChar - nLastChar spaces
*/

static void TransformLine(int const lineno)
{
int	firstChar, oLastChar, nLastChar;
chtype	*newLine = newscr->_line[lineno].text;
chtype	*oldLine = curscr->_line[lineno].text;
int	n;
bool	attrchanged = FALSE;

	T(("TransformLine(%d) called", lineno));

	if(ceol_standout_glitch && clr_eol) {
		firstChar = 0;
		while(firstChar < screen_columns) {
			if(AttrOf(newLine[firstChar]) != AttrOf(oldLine[firstChar]))
				attrchanged = TRUE;
			firstChar++;
		}
	}

	firstChar = 0;

	if (attrchanged) {	/* we may have to disregard the whole line */
		GoTo(lineno, firstChar);
		ClrToEOL(ClrBlank(curscr));
		PutRange(oldLine, newLine, lineno, 0, (screen_columns-1));
	} else {
		chtype blank;

		/* find the first differing character */
		while (firstChar < screen_columns  &&
				newLine[firstChar] == oldLine[firstChar])
			firstChar++;

		/* if there wasn't one, we're done */
		if (firstChar >= screen_columns)
			return;

		/* it may be cheap to clear leading whitespace with clr_bol */
		if (clr_bol && can_clear_with(blank=newLine[0]))
		{
			int oFirstChar, nFirstChar;

			for (oFirstChar = 0; oFirstChar < screen_columns; oFirstChar++)
				if (oldLine[oFirstChar] != blank)
					break;
			for (nFirstChar = 0; nFirstChar < screen_columns; nFirstChar++)
				if (newLine[nFirstChar] != blank)
					break;

			if (nFirstChar > oFirstChar + SP->_el1_cost)
			{
			    if (nFirstChar >= screen_columns && SP->_el_cost <= SP->_el1_cost)
			    {
				GoTo(lineno, 0);
				UpdateAttrs(blank);
				TPUTS_TRACE("clr_eol");
				putp(clr_eol);
			    }
			    else
			    {
				GoTo(lineno, nFirstChar - 1);
				UpdateAttrs(blank);
				TPUTS_TRACE("clr_bol");
				putp(clr_bol);
			    }
			    
			    while (firstChar < nFirstChar)
				oldLine[firstChar++] = blank;

			    if (firstChar >= screen_columns)
				return;
			}
		}

		blank = newLine[screen_columns-1];

		if(!can_clear_with(blank))
		{
			/* find the last differing character */
			nLastChar = screen_columns - 1;

			while (nLastChar > firstChar
			 && newLine[nLastChar] == oldLine[nLastChar])
				nLastChar--;

			if (nLastChar >= firstChar) {
				GoTo(lineno, firstChar);
				PutRange(oldLine, newLine, lineno, firstChar, nLastChar);
				memcpy( oldLine + firstChar,
					newLine + firstChar,
					(nLastChar - firstChar + 1) * sizeof(chtype));
			}
			return;
		}

		/* find last non-blank character on old line */
		oLastChar = screen_columns - 1;
		while (oLastChar > firstChar  &&  oldLine[oLastChar] == blank)
			oLastChar--;

		/* find last non-blank character on new line */
		nLastChar = screen_columns - 1;
		while (nLastChar > firstChar  &&  newLine[nLastChar] == blank)
			nLastChar--;

		if((nLastChar == firstChar)
		 && (SP->_el_cost < (oLastChar - nLastChar))) {
			GoTo(lineno, firstChar);
			if(newLine[firstChar] != blank )
				PutChar(newLine[firstChar]);
			ClrToEOL(blank);
		} else if( newLine[nLastChar] != oldLine[oLastChar]
				|| !(_nc_idcok && has_ic()) ) {
			GoTo(lineno, firstChar);
			if ((oLastChar - nLastChar) > SP->_el_cost) {
				if(PutRange(oldLine, newLine, lineno, firstChar, nLastChar))
				    GoTo(lineno, nLastChar+1);
				ClrToEOL(blank);
			} else {
				n = max( nLastChar , oLastChar );
				PutRange(oldLine, newLine, lineno, firstChar, n);
			}
		} else {
			int nLastNonblank = nLastChar;
			int oLastNonblank = oLastChar;

			/* find the last characters that really differ */
			while (newLine[nLastChar] == oldLine[oLastChar]) {
				if (nLastChar != 0
				 && oLastChar != 0) {
					nLastChar--;
					oLastChar--;
				 } else {
					break;
				 }
			}

			n = min(oLastChar, nLastChar);
			if (n >= firstChar) {
				GoTo(lineno, firstChar);
				PutRange(oldLine, newLine, lineno, firstChar, n);
			}
			GoTo(lineno, n+1);

			if (oLastChar < nLastChar) {
				int m = max(nLastNonblank, oLastNonblank);
				if (InsCharCost(nLastChar - oLastChar)
				 > (m - n)) {
					PutRange(oldLine, newLine, lineno, n+1, m);
				} else {
					InsStr(&newLine[n+1], nLastChar - oLastChar);
				}
			} else if (oLastChar > nLastChar ) {
				if (DelCharCost(oLastChar - nLastChar)
				    > SP->_el_cost + nLastNonblank - (n+1)) {
					if(PutRange(oldLine, newLine, lineno,
							n+1, nLastNonblank))
						GoTo(lineno, nLastNonblank+1);
					ClrToEOL(blank);
				} else {
					/*
					 * The delete-char sequence will
					 * effectively shift in blanks from the
					 * right margin of the screen.  Ensure
					 * that they are the right color by
					 * setting the video attributes from
					 * the last character on the row.
					 */
					UpdateAttrs(blank);
					DelChar(oLastChar - nLastChar);
				}
			}
		}
	}

	/* update the code's internal representation */
	if (screen_columns > firstChar)
		memcpy( oldLine + firstChar,
			newLine + firstChar,
			(screen_columns - firstChar) * sizeof(chtype));
}

/*
**	ClearScreen(blank)
**
**	Clear the physical screen and put cursor at home
**
*/

static void ClearScreen(chtype blank)
{
	int	i, j;

	T(("ClearScreen() called"));

	if (clear_screen) {
		UpdateAttrs(blank);
		TPUTS_TRACE("clear_screen");
		putp(clear_screen);
		SP->_cursrow = SP->_curscol = 0;
#ifdef POSITION_DEBUG
		position_check(SP->_cursrow, SP->_curscol, "ClearScreen");
#endif /* POSITION_DEBUG */
	} else if (clr_eos) {
		SP->_cursrow = SP->_curscol = -1;
		GoTo(0,0);

		UpdateAttrs(blank);
		TPUTS_TRACE("clr_eos");
		putp(clr_eos);
	} else if (clr_eol) {
		SP->_cursrow = SP->_curscol = -1;

		for (i = 0; i < screen_lines; i++) {
			GoTo(i, 0);
			UpdateAttrs(blank);
			TPUTS_TRACE("clr_eol");
			putp(clr_eol);
		}
		GoTo(0,0);
	} else {
		T(("cannot clear screen"));
		return;
	}

	for (i = 0; i < screen_lines; i++) {
		for (j = 0; j < screen_columns; j++)
			curscr->_line[i].text[j] = blank;
	}

	T(("screen cleared"));
}

/*
**	InsStr(line, count)
**
**	Insert the count characters pointed to by line.
**
*/

static int InsStr(chtype *line, int count)
{
	T(("InsStr(%p,%d) called", line, count));

	if (enter_insert_mode  &&  exit_insert_mode) {
		TPUTS_TRACE("enter_insert_mode");
		putp(enter_insert_mode);
		while (count) {
			PutAttrChar(*line);
			line++;
			count--;
		}
		TPUTS_TRACE("exit_insert_mode");
		putp(exit_insert_mode);
		return(OK);
	} else if (parm_ich) {
		TPUTS_TRACE("parm_ich");
		tputs(tparm(parm_ich, count), count, _nc_outch);
		while (count) {
			PutAttrChar(*line);
			line++;
			count--;
		}
		return(OK);
	} else {
		while (count) {
			TPUTS_TRACE("insert_character");
			putp(insert_character);
			PutAttrChar(*line);
			if (insert_padding)
			{
				TPUTS_TRACE("insert_padding");
				putp(insert_padding);
			}
			line++;
			count--;
		}
		return(OK);
	}
}

/*
**	DelChar(count)
**
**	Delete count characters at current position
**
*/

static void DelChar(int count)
{
	T(("DelChar(%d) called, position = (%d,%d)", count, newscr->_cury, newscr->_curx));

	if (parm_dch) {
		TPUTS_TRACE("parm_dch");
		tputs(tparm(parm_dch, count), count, _nc_outch);
	} else {
		while (count--)
		{
			TPUTS_TRACE("delete_character");
			putp(delete_character);
		}
	}
}

/*
**	_nc_outstr(char *str)
**
**	Emit a string without waiting for update.
*/

void _nc_outstr(const char *str)
{
    FILE *ofp = SP ? SP->_ofp : stdout;

    (void) fputs(str, ofp);
    (void) fflush(ofp);

#ifdef TRACE
    _nc_outchars += strlen(str);
#endif /* TRACE */
}

/****************************************************************************
 *
 * Physical-scrolling support
 *
 ****************************************************************************/

int _nc_scrolln(int n, int top, int bot, int maxy)
/* scroll region from top to bot by n lines */
{
    chtype blank=ClrBlank(stdscr);
    int i;
    bool cursor_saved=FALSE;

    TR(TRACE_MOVE, ("mvcur_scrolln(%d, %d, %d, %d)", n, top, bot, maxy));

    /*
     * This code was adapted from Keith Bostic's hardware scrolling
     * support for 4.4BSD curses.  I (esr) translated it to use terminfo
     * capabilities, narrowed the call interface slightly, and cleaned
     * up some convoluted tests.  I also added support for the memory_above
     * memory_below, and non_dest_scroll_region capabilities.
     *
     * For this code to work, we must have either
     * change_scroll_region and scroll forward/reverse commands, or
     * insert and delete line capabilities.
     * When the scrolling region has been set, the cursor has to
     * be at the last line of the region to make the scroll
     * happen.
     *
     * This code makes one aesthetic decision in the opposite way from
     * BSD curses.  BSD curses preferred pairs of il/dl operations
     * over scrolls, allegedly because il/dl looked faster.  We, on
     * the other hand, prefer scrolls because (a) they're just as fast
     * on many terminals and (b) using them avoids bouncing an
     * unchanged bottom section of the screen up and down, which is
     * visually nasty.
     */
    if (n > 0) /* scroll up (forward) */
    {
	/*
	 * Explicitly clear if stuff pushed off top of region might
	 * be saved by the terminal.
	 */
	if (non_dest_scroll_region || (memory_above && top == 0)) {
	    for (i = 0; i < n; i++)
	    {
		GoTo(i, 0);
		ClrToEOL(BLANK);
	    }
	}

	if (n == 1 && scroll_forward && top == 0 && bot == maxy)
	{
	    GoTo(bot, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("scroll_forward");
	    tputs(scroll_forward, 0, _nc_outch);
	}
	else if (n == 1 && delete_line && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("delete_line");
	    tputs(delete_line, 0, _nc_outch);
	}
	else if (parm_index && top == 0 && bot == maxy)
	{
	    GoTo(bot, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("parm_index");
	    tputs(tparm(parm_index, n, 0), n, _nc_outch);
	}
	else if (parm_delete_line && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("parm_delete_line");
	    tputs(tparm(parm_delete_line, n, 0), n, _nc_outch);
	}
	else if (scroll_forward && top == 0 && bot == maxy)
	{
	    GoTo(bot, 0);
	    UpdateAttrs(blank);
	    for (i = 0; i < n; i++)
	    {
		TPUTS_TRACE("scroll_forward");
		tputs(scroll_forward, 0, _nc_outch);
	    }
	}
	else if (delete_line && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    for (i = 0; i < n; i++)
	    {
		TPUTS_TRACE("delete_line");
		tputs(delete_line, 0, _nc_outch);
	    }
	}
	else if (change_scroll_region && (scroll_forward || parm_index))
	{
	    if ((SP->_cursrow == bot || SP->_cursrow == bot-1)
	        && save_cursor && restore_cursor)
	    {
		cursor_saved=TRUE;
	    	TPUTS_TRACE("save_cursor");
		tputs(save_cursor, 0, _nc_outch);
	    }
	    TPUTS_TRACE("change_scroll_region");
	    tputs(tparm(change_scroll_region, top, bot), 0, _nc_outch);
	    if (cursor_saved)
	    {
	    	TPUTS_TRACE("restore_cursor");
		tputs(restore_cursor, 0, _nc_outch);
	    }
	    else
	    {
		SP->_cursrow = SP->_curscol = -1;
	    }

	    GoTo(bot, 0);
	    UpdateAttrs(blank);
	    if (n == 1 && scroll_forward)
	    {
		TPUTS_TRACE("scroll_forward");
		tputs(scroll_forward, 0, _nc_outch);
	    }
	    else if (parm_index)
	    {
		TPUTS_TRACE("parm_index");
		tputs(tparm(parm_index, n, 0), n, _nc_outch);
	    }
	    else
	    {
		for (i = 0; i < n; i++)
		{
		    TPUTS_TRACE("scroll_forward");
		    tputs(scroll_forward, 0, _nc_outch);
		}
	    }
	    TPUTS_TRACE("change_scroll_region");
	    tputs(tparm(change_scroll_region, 0, maxy), 0, _nc_outch);
	    SP->_cursrow = SP->_curscol = -1;
	}
	else if (_nc_idlok
	 && (parm_delete_line || delete_line)
	 && (parm_insert_line || insert_line))
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    if (n == 1 && delete_line)
	    {
		TPUTS_TRACE("delete_line");
		tputs(delete_line, 0, _nc_outch);
	    }
	    else if (parm_delete_line)
	    {
		TPUTS_TRACE("parm_delete_line");
		tputs(tparm(parm_delete_line, n, 0), n, _nc_outch);
	    }
	    else
	    {
		for (i = 0; i < n; i++)
		{
		    TPUTS_TRACE("delete_line");
		    tputs(delete_line, 0, _nc_outch);
		}
	    }

	    GoTo(bot - n + 1, 0);

	    /* Push down the bottom region. */
	    UpdateAttrs(blank);
	    if (n == 1 && insert_line)
	    {
		TPUTS_TRACE("insert_line");
		tputs(insert_line, 0, _nc_outch);
	    }
	    else if (parm_insert_line)
	    {
		TPUTS_TRACE("parm_insert_line");
		tputs(tparm(parm_insert_line, n, 0), n, _nc_outch);
	    }
	    else
	    {
		for (i = 0; i < n; i++)
		{
		    TPUTS_TRACE("insert_line");
		    tputs(insert_line, 0, _nc_outch);
		}
	    }
	}
	else
	    return(ERR);
    }
    else /* (n < 0) - scroll down (backward) */
    {
	/*
	 * Do explicit clear to end of region if it's possible that the
	 * terminal might hold on to stuff we push off the end.
	 */
	if (non_dest_scroll_region || (memory_below && bot == maxy))
	{
	    if (bot == maxy && clr_eos)
	    {
		GoTo(maxy + n, 0);
		ClrToEOS(BLANK);
	    }
	    else if (clr_eol)
	    {
		for (i = 0; i < -n; i++)
		{
		    GoTo(maxy + n + i, 0);
		    ClrToEOL(BLANK);
		}
	    }
	}

	if (n == -1 && scroll_reverse && top == 0 && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("scroll_reverse");
	    tputs(scroll_reverse, 0, _nc_outch);
	}
	else if (n == -1 && insert_line && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("insert_line");
	    tputs(insert_line, 0, _nc_outch);
	}
	else if (parm_rindex && top == 0 && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("parm_rindex");
	    tputs(tparm(parm_rindex, -n, 0), -n, _nc_outch);
	}
	else if (parm_insert_line && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    TPUTS_TRACE("parm_insert_line");
	    tputs(tparm(parm_insert_line, -n, 0), -n, _nc_outch);
	}
	else if (scroll_reverse && top == 0 && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    for (i = n; i < 0; i++)
	    {
		TPUTS_TRACE("scroll_reverse");
		tputs(scroll_reverse, 0, _nc_outch);
	    }
	}
	else if (insert_line && bot == maxy)
	{
	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    for (i = n; i < 0; i++)
	    {
		TPUTS_TRACE("insert_line");
		tputs(insert_line, 0, _nc_outch);
	    }
	}
	else if (change_scroll_region && (scroll_reverse || parm_rindex))
	{
	    if (top != 0 && (SP->_cursrow == top || SP->_cursrow == top-1)
	        && save_cursor && restore_cursor)
	    {
		cursor_saved=TRUE;
	    	TPUTS_TRACE("save_cursor");
		tputs(save_cursor, 0, _nc_outch);
	    }
	    TPUTS_TRACE("change_scroll_region");
	    tputs(tparm(change_scroll_region, top, bot), 0, _nc_outch);
	    if (cursor_saved)
	    {
	    	TPUTS_TRACE("restore_cursor");
		tputs(restore_cursor, 0, _nc_outch);
	    }
	    else
	    {
		SP->_cursrow = SP->_curscol = -1;
	    }

	    GoTo(top, 0);
	    UpdateAttrs(blank);
	    if (n == -1 && scroll_reverse)
	    {
		TPUTS_TRACE("scroll_reverse");
		tputs(scroll_reverse, 0, _nc_outch);
	    }
	    else if (parm_rindex)
	    {
		TPUTS_TRACE("parm_rindex");
		tputs(tparm(parm_rindex, -n, 0), -n, _nc_outch);
	    }
	    else
	    {
		for (i = n; i < 0; i++)
		{
		    TPUTS_TRACE("scroll_reverse");
		    tputs(scroll_reverse, 0, _nc_outch);
		}
	    }
	    TPUTS_TRACE("change_scroll_region");
	    tputs(tparm(change_scroll_region, 0, maxy), 0, _nc_outch);
	    SP->_cursrow = SP->_curscol = -1;
	}
	else if (_nc_idlok
	 && (parm_delete_line || delete_line)
	 && (parm_insert_line || insert_line))
	{
	    GoTo(bot + n + 1, 0);
	    UpdateAttrs(blank);
	    if (n == -1 && delete_line)
	    {
		TPUTS_TRACE("delete_line");
		tputs(delete_line, 0, _nc_outch);
	    }
	    else if (parm_delete_line)
	    {
		TPUTS_TRACE("parm_delete_line");
		tputs(tparm(parm_delete_line, -n, 0), -n, _nc_outch);
	    }
	    else
	    {
		for (i = n; i < 0; i++)
		{
		    TPUTS_TRACE("delete_line");
		    tputs(delete_line, 0, _nc_outch);
		}
	    }

	    GoTo(top, 0);

	    /* Scroll the block down. */
	    UpdateAttrs(blank);
	    if (n == -1 && insert_line)
	    {
		TPUTS_TRACE("insert_line");
		tputs(insert_line, 0, _nc_outch);
	    }
	    else if (parm_insert_line)
	    {
		TPUTS_TRACE("parm_insert_line");
		tputs(tparm(parm_insert_line, -n, 0), -n, _nc_outch);
	    }
	    else
	    {
		for (i = n; i < 0; i++)
		{
		    TPUTS_TRACE("insert_line");
		    tputs(insert_line, 0, _nc_outch);
		}
	    }
	}
	else
	    return(ERR);
    }

    /* this is mainly for doupdate not to skip those lines */
    for (i = top; i <= bot; i++)
    {
	curscr->_line[i].firstchar = 0;
	curscr->_line[i].lastchar = curscr->_maxx;
    }

    _nc_scroll_window(curscr, n, top, bot, blank);

    return(OK);
}


void _nc_screen_resume()
{
    /* make sure terminal is in a sane known state */
    SP->_current_attr = A_NORMAL;
    newscr->_clear = TRUE;

    if (SP->_coloron == TRUE && orig_pair)
	putp(orig_pair);
    if (exit_attribute_mode)
	putp(exit_attribute_mode);
    else
    {
	/* turn off attributes */
	if (exit_alt_charset_mode)
	    putp(exit_alt_charset_mode);
	if (exit_standout_mode)
	    putp(exit_standout_mode);
	if (exit_underline_mode)
	    putp(exit_underline_mode);
    }
    if (exit_insert_mode)
	putp(exit_insert_mode);
}

void _nc_screen_init()
{
    _nc_screen_resume();
}

/* wrap up screen handling */
void _nc_screen_wrap()
{
    UpdateAttrs(A_NORMAL);
}

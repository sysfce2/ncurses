
/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992, 1993, 1994                 *
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
**	lib_tstp.c
**
**	The routine tstp().
**
*/

#include "curses.priv.h"
#ifdef SVR4_ACTION
#define _POSIX_SOURCE
#endif
#include <signal.h>

static void tstp(int dummy)
{
	sigset_t mask, omask;
	sigaction_t act, oact;

	T(("tstp() called"));

	/*
	 * The user may have changed the prog_mode tty bits, so save them.
	 */
	def_prog_mode();

	/*
	 * Block window change and timer signals.  The latter
	 * is because applications use timers to decide when
	 * to repaint the screen.
	 */
	(void)sigemptyset(&mask);
	(void)sigaddset(&mask, SIGALRM);
	(void)sigaddset(&mask, SIGWINCH);
	(void)sigprocmask(SIG_BLOCK, &mask, &omask);
	
	/*
	 * End window mode, which also resets the terminal state to the
	 * original (pre-curses) modes.
	 */
	endwin();

	/* Unblock SIGTSTP. */
	(void)sigemptyset(&mask);
	(void)sigaddset(&mask, SIGTSTP);
	(void)sigprocmask(SIG_UNBLOCK, &mask, NULL);

	/* Now we want to resend SIGSTP to this process and suspend it */ 
	act.sa_handler = SIG_DFL;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGTSTP, &act, &oact);
	kill(getpid(), SIGTSTP);

	/* Process gets suspended...time passes...process resumes */

	T(("SIGCONT received"));
	sigaction(SIGTSTP, &oact, NULL);
	flushinp();

	/*
	 * If the user modified the tty state while suspended, he wants
	 * those changes to stick.  So save the new "default" terminal state.
	 */
	def_shell_mode();

	/*
	 * This relies on the fact that doupdate() will restore the 
	 * program-mode tty state, and issue enter_ca_mode if need be.
	 */
	doupdate();

	/* Reset the signals. */
	(void)sigprocmask(SIG_SETMASK, &omask, NULL);
}

static void cleanup(int sig)
{
	if (sig == SIGSEGV)
		fprintf(stderr, "Got a segmentation violation signal, cleaning up and exiting\n");
	endwin();
	exit(1);
}

void curses_signal_handler(bool enable)
{
static sigaction_t act, oact;

	if (!enable)
	{
		act.sa_handler = SIG_IGN;
		sigaction(SIGTSTP, &act, &oact);
	}
	else if (act.sa_handler)
	{
		sigaction(SIGTSTP, &oact, NULL);
	}
	else	/*initialize */
	{
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;

		act.sa_handler = cleanup;
		sigaction(SIGINT, &act, NULL);
		sigaction(SIGTERM, &act, NULL);
#if 0
		sigaction(SIGSEGV, &act, NULL);
#endif

		act.sa_handler = tstp;
		sigaction(SIGTSTP, &act, NULL);
	}
}

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

/*
 *	toe.c --- table of entries report generator
 *
 */

#include <progs.priv.h>

#include <string.h>

#include "tic.h"
#include "term.h"
#include "dump_entry.h"
#include "term_entry.h"

extern char	*getenv(const char *);

char	*_nc_progname;

static void typelist(int eargc, char *eargv[], bool,
		     void (*)(const char *, TERMTYPE *));
static void deschook(const char *, TERMTYPE *);

int main (int argc, char *argv[])
{
    bool	direct_dependencies = FALSE;
    bool	invert_dependencies = FALSE;
    bool	header = FALSE;
    int		i, c, debug_level = 0;

    if ((_nc_progname = strrchr(argv[0], '/')) == NULL)
	_nc_progname = argv[0];
    else
	_nc_progname++;

    while ((c = getopt(argc, argv, "huv:UV")) != EOF)
	switch (c)
	{
	case 'h':
	    header = TRUE;
	    break;
	case 'u':
	    direct_dependencies = TRUE;
	    break;
	case 'v':
	    debug_level = atoi(optarg);
	    _nc_tracing = (1 << debug_level) - 1;
	    break;
	case 'U':
	    invert_dependencies = TRUE;
	    break;
	case 'V':
	    (void) fputs(NCURSES_VERSION, stdout);
	    putchar('\n');
	    exit(EXIT_SUCCESS);
	default:
	    (void) fprintf (stderr, "usage: toe [-huUV] [-v n] [file...]\n");
	    exit(EXIT_FAILURE);
	}

    if (direct_dependencies || invert_dependencies)
    {
	if (freopen(argv[optind], "r", stdin) == NULL)
	{
	    fprintf(stderr, "%s: can't open %s\n", _nc_progname, argv[optind]);
	    exit(EXIT_FAILURE);
	}

	/* parse entries out of the source file */
	_nc_set_source(argv[optind]);
	_nc_read_entry_source(stdin, (char *)NULL,
			      FALSE, FALSE,
			      NULLHOOK);
    }

    /* maybe we want a direct-dependency listing? */
    if (direct_dependencies)
    {
	ENTRY	*qp;

	for_entry_list(qp)
	    if (qp->nuses)
	    {
		int		j;

		(void) printf("%s:", _nc_first_name(qp->tterm.term_names));
		for (j = 0; j < qp->nuses; j++)
		    (void) printf(" %s", (char *)(qp->uses[j].parent));
		putchar('\n');
	    }

	exit(EXIT_SUCCESS);
    }

    /* maybe we want a reverse-dependency listing? */
    if (invert_dependencies)
    {
	ENTRY	*qp, *rp;
	int		matchcount;

	for_entry_list(qp)
	{
	    matchcount = 0;
	    for_entry_list(rp)
	    {
		if (rp->nuses == 0)
		    continue;

		for (i = 0; i < rp->nuses; i++)
		    if (_nc_name_match(qp->tterm.term_names,(char*)rp->uses[i].parent, "|"))
		    {
			if (matchcount++ == 0)
			    (void) printf("%s:",
					  _nc_first_name(qp->tterm.term_names));
			(void) printf(" %s", 
				      _nc_first_name(rp->tterm.term_names));
		    }
	    }
	    if (matchcount)
		putchar('\n');
	}

	exit(EXIT_SUCCESS);
    }

    /*
     * If we get this far, user wants a simple terminal type listing.
     */
    if (optind < argc)
	typelist(argc-optind, argv+optind, header, deschook);
    else
    {
	char	*explicit, *home, *eargv[3];
	int	j;

	j = 0;
	if ((explicit = getenv("TERMINFO")) != (char *)NULL)
	    eargv[j++] = explicit;
	else
	{
	    if ((home = getenv("HOME")) != (char *)NULL)
	    {
		char	personal[PATH_MAX];

		(void) sprintf(personal, PRIVATE_INFO, home);
		if (access(personal, F_OK) == 0)
		    eargv[j++] = personal;
	    }
	    eargv[j++] = TERMINFO;
	}
	eargv[j] = (char *)NULL;

	typelist(j, eargv, header, deschook);
    }

    exit(EXIT_SUCCESS);
}

static void deschook(const char *cn, TERMTYPE *tp)
/* display a description for the type */
{
    char	*desc;

    if ((desc = strrchr(tp->term_names, '|')) == (char *)NULL)
	desc = "(No description)";
    else
	++desc;

    (void) printf("%-10s\t%s\n", cn, desc);
}

static void typelist(int eargc, char *eargv[],
		     bool verbosity,
		     void  (*hook)(const char *, TERMTYPE *tp))
/* apply a function to each entry in given terminfo directories */
{
    int	i;

    for (i = 0; i < eargc; i++)
    {
	DIR	*termdir;
	struct dirent *subdir;

	if ((termdir = opendir(eargv[i])) == (DIR *)NULL)
	{
	    (void) fprintf(stderr,
			   "%s: can't open terminfo directory %s\n",
			   _nc_progname, eargv[i]);
	    exit(EXIT_FAILURE);
	}
	else if (verbosity)
	    (void) printf("#\n#%s:\n#\n", eargv[i]);

	while ((subdir = readdir(termdir)) != NULL)
	{
	    size_t	len = NAMLEN(subdir);
	    char	buf[PATH_MAX];
	    char	name_1[PATH_MAX];
	    DIR	*entrydir;
	    struct dirent *entry;

	    strncpy(name_1, subdir->d_name, len)[len] = '\0';
	    if (!strcmp(name_1, ".")
		|| !strcmp(name_1, ".."))
		continue;

	    (void) strcpy(buf, eargv[i]);
	    (void) strcat(buf, "/");
	    (void) strcat(buf, name_1);
	    (void) strcat(buf, "/");
	    chdir(buf);
	    entrydir = opendir(".");
	    while ((entry = readdir(entrydir)) != NULL)
	    {
		char		name_2[PATH_MAX];
		TERMTYPE	lterm;
		char		*cn;
		int		status;

		len = NAMLEN(entry);
		strncpy(name_2, entry->d_name, len)[len] = '\0';
		if (!strcmp(name_2, ".")
		    || !strcmp(name_2, ".."))
		    continue;

		status = _nc_read_file_entry(name_2, &lterm);
		if (status == -1)
		{
		    (void) fprintf(stderr,
				   "infocmp: couldn't open terminfo directory %s.\n",
				   eargv[i]);
		    return;
		}
		else if (status == 0)
		{
		    (void) fprintf(stderr,
				   "infocmp: couldn't open terminfo file %s.\n",
				   name_2);
		    return;
		}

		/* only visit things once, by primary name */
		cn = _nc_first_name(lterm.term_names);
		if (!strcmp(cn, name_2))
		{
		    /* apply the selected hook function */
		    (*hook)(cn, &lterm);
		}
		if (lterm.term_names)
	    	    free(lterm.term_names);
		if (lterm.str_table)
	    	    free(lterm.str_table);
	    }
	    closedir(entrydir);
	}
	closedir(termdir);
    }

    exit(EXIT_SUCCESS);
}

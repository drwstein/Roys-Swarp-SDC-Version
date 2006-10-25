 /*
 				main.c

*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
*	Part of:	SWarp
*
*	Author:		E.BERTIN (IAP)
*
*	Contents:	Parsing of the command line.
*
*	Last modify:	25/10/2006
*
*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#ifdef HAVE_CONFIG_H
#include	"config.h"
#endif

#include	<ctype.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#ifdef HAVE_MPI
#include	<mpi.h>
#endif

#include	"define.h"
#include	"globals.h"
#include	"fits/fitscat.h"
#include	"prefs.h"

#define		SYNTAX \
BANNER " image1 [image2 ...][-c <configuration_file>][-<keyword> <value>]\n" \
">\t or " \
BANNER " @image_list [-c <configuration_file>][-<keyword> <value>]\n" \
"> to dump a default configuration file: " BANNER " -d \n" \
"> to dump a default extended configuration file: " BANNER " -dd \n"

extern const char	notokstr[];

/********************************** main ************************************/
int	main(int argc, char *argv[])

  {
   FILE         *fp;
   char		**argkey, **argval, *str, *list;
   int		a, narg, nim, opt, opt2;

#ifdef HAVE_MPI
  MPI_Init (&argc,&argv);
#endif

  if (argc<2)
    {
    fprintf(OUTPUT, "\n		%s  version %s (%s)\n", BANNER,MYVERSION,DATE);
    fprintf(OUTPUT, "\nby %s\n", COPYRIGHT);
    fprintf(OUTPUT, "visit %s\n", WEBSITE);
    error(EXIT_SUCCESS, "SYNTAX: ", SYNTAX);
    }
  QMALLOC(argkey, char *, argc);
  QMALLOC(argval, char *, argc);

/*default parameters */
  prefs.command_line = argv;
  prefs.ncommand_line = argc;
  prefs.ninfield = 1;
  prefs.infield_name[0] = "image";
  strcpy(prefs.prefs_name, "default.swarp");
  narg = nim = 0;

  for (a=1; a<argc; a++)
    {
    if (*(argv[a]) == '-')
      {
      opt = (int)argv[a][1];
      if (strlen(argv[a])<4 || opt == '-')
        {
        opt2 = (int)tolower((int)argv[a][2]);
        if (opt == '-')
          {
          opt = opt2;
          opt2 = (int)tolower((int)argv[a][3]);
          }
        switch(opt)
          {
          case 'c':
            if (a<(argc-1))
              strcpy(prefs.prefs_name, argv[++a]);
            break;
          case 'd':
            dumpprefs(opt2=='d' ? 1 : 0);
            exit(EXIT_SUCCESS);
            break;
          case 'v':
            printf("%s version %s (%s)\n", BANNER,MYVERSION,DATE);
            exit(EXIT_SUCCESS);
            break;
          case 'h':
          default:
            error(EXIT_SUCCESS,"SYNTAX: ", SYNTAX);
          }
        }
      else
        {
        argkey[narg] = &argv[a][1];
        argval[narg++] = argv[++a];
        }       
      }
    else
      {
/*---- The input image list filename */
      if (*(argv[a]) == '@')
        {
        QMALLOC(list, char, MAXCHAR);
        for(*argv[a]++; (a<argc) && (*argv[a]!='-'); a++)
          strcpy(list, argv[a]);        
        if (!(fopen(list,"r")==NULL))
          {
          QMALLOC(str, char, MAXCHAR);
          fp = fopen(list,"r");
          for (nim=0; fgets(str,MAXCHAR,fp); nim++)
            if (nim<MAXINFIELD)
              {
              QMALLOC(prefs.infield_name[nim], char, strlen(str)-1);
              strncpy(prefs.infield_name[nim],str,strlen(str)-1);        
              }
            else
              error(EXIT_FAILURE, "*Error*: Too many input images: ", str);
          fclose(fp);
          free(str);
          }
        else
          error(EXIT_FAILURE, "*Error*: Cannot open file ", list);
        free(list);
        }
/*---- The input image filename(s) */
      for(; (a<argc) && (*argv[a]!='-'); a++)
        for (str=NULL;(str=strtok(str?NULL:argv[a], notokstr)); nim++)
          if (nim<MAXINFIELD)
            prefs.infield_name[nim] = str;
          else
            error(EXIT_FAILURE, "*Error*: Too many input images: ", str);
      prefs.ninfield = nim;
      a--;
      }
    }

  readprefs(prefs.prefs_name, argkey, argval, narg);
  useprefs();

  free(argkey);
  free(argval);

#ifdef HAVE_MPI
  MPI_Comm_size(MPI_COMM_WORLD,&prefs.nnodes);
  MPI_Comm_rank(MPI_COMM_WORLD,&prefs.node_index);
#endif

  makeit();

  NFPRINTF(OUTPUT, "");
  NPRINTF(OUTPUT, "> All done (in %.0f s)\n", prefs.time_diff);

#ifdef HAVE_MPI
  MPI_Finalize();
#endif
  exit(EXIT_SUCCESS);
  }


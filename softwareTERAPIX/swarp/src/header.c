 /*
 				header.c

*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
*	Part of:	SWarp
*
*	Author:		E.BERTIN (IAP)
*
*	Contents:	Provide additional FITS header management.
*
*	Last modify:	18/11/2004
*
*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#ifdef HAVE_CONFIG_H
#include	"config.h"
#endif

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "define.h"
#include "globals.h"
#include "data.h"
#include "fits/fitscat.h"
#include "fitswcs.h"
#include "field.h"
#include "header.h"
#include "key.h"
#include "prefs.h"
#include "wcs/wcs.h"


/****** read_aschead ********************************************************
PROTO	int	read_aschead(char *filename, int frameno, tabstruct *tab)
PURPOSE	Read a ASCII header file and update the current field's tab
INPUT	Name of the ASCII file,
	Frame number (if extensions),
	Tab structure.
OUTPUT	RETURN_OK if the file was found, RETURN_ERROR otherwise.
NOTES	-.
AUTHOR	E. Bertin (IAP)
VERSION	18/11/2004
 ***/
int	read_aschead(char *filename, int frameno, tabstruct *tab)
  {
   static char	keyword[MAXCHAR],data[MAXCHAR],comment[MAXCHAR];
   FILE		*file;
   h_type	htype;
   t_type	ttype;
   char		*gstrt;
   int		i, pos;

  if ((file=fopen(filename, "r")))
    {
/*- Skip previous ENDs in multi-FITS extension headers */
    for (i=(frameno?(frameno-1):0); i--;)
      while (fgets(gstr, MAXCHAR, file)
		&& strncmp(gstr,"END ",4)
		&& strncmp(gstr,"END\n",4));
    pos = fitsfind(tab->headbuf, "END     ");
    memset(gstr, ' ', 80);
    while (fgets(gstr, 81, file) && strncmp(gstr,"END ",4)
				&& strncmp(gstr,"END\n",4))
      {
/*---- Remove possible junk within the 80 first chars (linefeeds, etc.) */
      gstrt = gstr;
      for (i=80; i--; gstrt++)
        if (*gstrt < ' ' || *gstrt >'~')
          *gstrt = ' ';
/*---- Skip non-FITS parts */
      if (gstr[0]<=' ' || (gstr[8]!=' ' && gstr[8]!='='))
        continue;
      fitspick(gstr, keyword, data, &htype, &ttype, comment);
/*---- Block critical keywords */
      if (!wstrncmp(keyword, "SIMPLE  ", 8)
	||!wstrncmp(keyword, "BITPIX  ", 8)
	||!wstrncmp(keyword, "NAXIS   ", 8)
	||!wstrncmp(keyword, "BSCALE  ", 8)
	||!wstrncmp(keyword, "BZERO   ", 8))
        continue;
/*---- Always keep a one-line margin */
      if ((pos+1)*80>=tab->headnblock*FBSIZE)
        {
        tab->headnblock++;
        QREALLOC(tab->headbuf, char, tab->headnblock*FBSIZE);
        memset(tab->headbuf+(tab->headnblock-1)*FBSIZE, ' ', FBSIZE);
        }
      fitsadd(tab->headbuf, keyword, comment);
      if (fitsfind(tab->headbuf, "END     ")>=pos)
        pos++;
      fitswrite(tab->headbuf, keyword, data, htype, ttype);
      memset(gstr, ' ', 80);
      }
    fclose(file);
/*-- Update the tab data */
    readbasic_head(tab);
    return RETURN_OK;
    }
  else
    return RETURN_ERROR;
  }


/****** readfitsinfo_field ****************************************************
PROTO	void	readfitsinfo_field(fieldstruct *field, tabstruct *tab)
PURPOSE	Read additional information from a FITS header and update the current
	field's tab
INPUT	Pointer to the field structure,
	Tab structure.
OUTPUT	-.
NOTES	-.
AUTHOR	E. Bertin (IAP)
VERSION	17/08/2001
 ***/
void	readfitsinfo_field(fieldstruct *field, tabstruct *tab)
  {
#define	FITSREADF(buf, k, val, def) \
		{if (fitsread(buf,k, &val, H_FLOAT,T_DOUBLE) != RETURN_OK) \
		   val = def; \
		}

#define	FITSREADI(buf, k, val, def) \
		{if (fitsread(buf,k, &val, H_INT,T_LONG) != RETURN_OK) \
		   val = def; \
		}

#define	FITSREADS(buf, k, str, def) \
		{if (fitsread(buf,k,str, H_STRING,T_STRING) != RETURN_OK) \
		   strcpy(str, (def)); \
		}
   char		*buf;
   wcsstruct	*wcs;
   int		l;

  buf = tab->headbuf;
  wcs = field->wcs;
  if (!buf || !wcs)
    return;
  for (l=0; l<tab->naxis; l++)
    {
    sprintf(gstr, "COMIN%-3d", l+1);
    FITSREADI(buf, gstr, wcs->outmin[l], 1);
    wcs->outmax[l] = wcs->outmin[l] + wcs->naxisn[l] - 1;
    }
  FITSREADF(buf, "BACKMEAN", field->backmean, 0.0);
  FITSREADF(buf, "BACKSIG ", field->backsig, 0.0);
/* Set the flux scale */
  FITSREADF(buf,prefs.fscale_keyword, field->fscale, field->fscale);
/* Set the conversion factor */
  FITSREADF(buf, prefs.gain_keyword, field->gain, field->gain);

  return;

#undef FITSREADF
#undef FITSREADI
#undef FITSREADS
  }


/****** writefitsinfo_outfield ************************************************
PROTO	void writefitsinfo_outfield(fieldstruct *field, fieldstruct *infield)
PURPOSE	Add relevant information to the output field FITS header.
INPUT	Pointer to the field.
OUTPUT	-.
NOTES	Global preferences are used.
AUTHOR	E. Bertin (IAP)
VERSION	16/08/2003
 ***/
void	writefitsinfo_outfield(fieldstruct *field, fieldstruct *infield)

  {
   struct tm		*tm;
   time_t		thetime;
   tabstruct		*tab, *intab;
   extern pkeystruct	key[];
   h_type		htype;
   t_type		ttype;
   extern char		keylist[][16];
   char			comment[80],
			*pstr;
   int			d, n, k, l;

  tab = field->tab;
  addkeywordto_head(tab, "COMMENT ", "");
  addkeywordto_head(tab, "SOFTNAME", "The software that processed those data");
  fitswrite(tab->headbuf, "SOFTNAME", BANNER, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTVERS", "Version of the software");
  fitswrite(tab->headbuf, "SOFTVERS", VERSION, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTDATE", "Release date of the software");
  fitswrite(tab->headbuf, "SOFTDATE", DATE, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTAUTH", "Maintainer of the software");
  fitswrite(tab->headbuf, "SOFTAUTH", COPYRIGHT, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTINST", "Institute");
  fitswrite(tab->headbuf, "SOFTINST", INSTITUTE, H_STRING, T_STRING);
  addkeywordto_head(tab, "COMMENT ", "");

/* User name, we don't use getpwuid() to allow for static linking with glibc 2.3*/
  addkeywordto_head(tab, "AUTHOR", "Who ran the software");
#ifdef HAVE_GETENV
  if (!(pstr=getenv("USERNAME")))	/* Cygwin,... */
    if ((pstr=getenv("LOGNAME")))	/* Linux,... */
      fitswrite(tab->headbuf, "AUTHOR", pstr, H_STRING, T_STRING);
  if (!pstr)
#endif
    fitswrite(tab->headbuf, "AUTHOR", "unknown", H_STRING, T_STRING);

/* Host name */
  if (!gethostname(gstr, 80))
    {
    addkeywordto_head(tab, "ORIGIN", "Where it was done");
    fitswrite(tab->headbuf, "ORIGIN", gstr, H_STRING, T_STRING);
    }

/* Obs dates */
  thetime = time(NULL);
  tm = gmtime(&thetime);
  sprintf(gstr,"%04d-%02d-%02dT%02d:%02d:%02d",
                        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
                        tm->tm_hour, tm->tm_min, tm->tm_sec);
  addkeywordto_head(tab, "DATE    ", "When it was started (GMT)");
  fitswrite(tab->headbuf, "DATE    ", gstr, H_STRING, T_STRING);

/* Config parameters */
/* COMBINE_TYPE */
  addkeywordto_head(tab, "COMBINET","COMBINE_TYPE config parameter for " \
			BANNER);
  fitswrite(tab->headbuf, "COMBINET", key[findkeys("COMBINE_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.coadd_type], H_STRING, T_STRING);

  if (infield && (intab=infield->tab) && prefs.ncopy_keywords)
    {
    addkeywordto_head(tab, "COMMENT ", "");
    addkeywordto_head(tab, "COMMENT ", "Propagated FITS keywords");
    for (k=0; k<prefs.ncopy_keywords; k++)
      {
      if ((l=fitsfind(intab->headbuf, prefs.copy_keywords[k])) != RETURN_ERROR)
        {
        fitspick(intab->headbuf+l*80, prefs.copy_keywords[k], gstr,
		&htype, &ttype, comment);
        addkeywordto_head(tab, prefs.copy_keywords[k], comment);
        fitswrite(tab->headbuf, prefs.copy_keywords[k], gstr, htype, ttype);
        }
      }
    }

/* Axis-dependent config parameters */
  addkeywordto_head(tab, "COMMENT ", "");
  addkeywordto_head(tab, "COMMENT ", "Axis-dependent config parameters");
  for (d=0; d<tab->naxis; d++)
    {
/*-- RESAMPLING_TYPE */
    sprintf(gstr, "RESAMPT%1d", d+1);
    addkeywordto_head(tab, gstr, "RESAMPLING_TYPE config parameter");
    fitswrite(tab->headbuf, gstr, key[findkeys("RESAMPLING_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.resamp_type[d]], H_STRING, T_STRING);
/*-- CENTER_TYPE */
    sprintf(gstr, "CENTERT%1d", d+1);
    addkeywordto_head(tab, gstr, "CENTER_TYPE config parameter");
    fitswrite(tab->headbuf, gstr, key[findkeys("CENTER_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.center_type[d]], H_STRING, T_STRING);
/*-- PIXELSCALE_TYPE */
    sprintf(gstr, "PSCALET%1d", d+1);
    addkeywordto_head(tab, gstr, "PIXELSCALE_TYPE config parameter");
    fitswrite(tab->headbuf, gstr, key[findkeys("PIXELSCALE_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.pixscale_type[d]], H_STRING, T_STRING);
    }

/* Image-dependent config parameters */
  if (prefs.writefileinfo_flag)
    {
    addkeywordto_head(tab, "COMMENT ", "");
    addkeywordto_head(tab, "COMMENT ", "File-dependent config parameters");
    for (n=0; n<prefs.ninfield; n++)
      {
/*---- Input images */
      sprintf(gstr, "FILE%04d", n+1);
      addkeywordto_head(tab, gstr, "Input filename");
/*---- A short, "relative" version of the filename */
      if (!(pstr = strrchr(prefs.infield_name[n], '/')))
        pstr = prefs.infield_name[n];
      else
        pstr++;
      fitswrite(tab->headbuf, gstr, pstr, H_STRING, T_STRING);
/*---- Input weights */
      if (n<prefs.ninwfield)
        {
        sprintf(gstr, "WGHT%04d", n+1);
        addkeywordto_head(tab, gstr, "Input weight-map");
/*------ A short, "relative" version of the filename */
        if (!(pstr = strrchr(prefs.inwfield_name[n], '/')))
          pstr = prefs.inwfield_name[n];
        else
          pstr++;
        fitswrite(tab->headbuf, gstr, pstr, H_STRING, T_STRING);
/*------ WEIGHT_TYPE */
        sprintf(gstr, "WGTT%04d", n+1);
        addkeywordto_head(tab, gstr, "WEIGHT_TYPE config parameter");
        fitswrite(tab->headbuf, gstr, key[findkeys("WEIGHT_TYPE", keylist,
	  FIND_STRICT)].keylist[prefs.weight_type[n]], H_STRING, T_STRING);
        }
/*---- Interpolation flag */
      sprintf(gstr, "INTF%04d", n+1);
      addkeywordto_head(tab, gstr, "INTERPOLATE config flag");
      fitswrite(tab->headbuf, gstr, &prefs.interp_flag[n], H_BOOL, T_LONG);
/*---- Background-subtraction flag */
      sprintf(gstr, "SUBF%04d", n+1);
      addkeywordto_head(tab, gstr, "SUBTRACT_BACK config flag");
      fitswrite(tab->headbuf, gstr, &prefs.subback_flag[n], H_BOOL, T_LONG);
/*---- BACK_TYPE */
      sprintf(gstr, "BCKT%04d", n+1);
      addkeywordto_head(tab, gstr, "BACK_TYPE config parameter");
      fitswrite(tab->headbuf, gstr, key[findkeys("BACK_TYPE", keylist,
        FIND_STRICT)].keylist[prefs.back_type[n]], H_STRING, T_STRING);
/*---- BACK_DEFAULT */
      if (prefs.back_type[n]==BACK_ABSOLUTE)
        {
        sprintf(gstr, "BCKD%04d", n+1);
        addkeywordto_head(tab, gstr,"BACK_DEFAULT config parameter");
        fitswrite(tab->headbuf, gstr, &prefs.back_default[n], H_EXPO,T_DOUBLE);
        }
/*---- BACK_SIZE */
      sprintf(gstr, "BCKS%04d", n+1);
      addkeywordto_head(tab, gstr,"BACK_SIZE config parameter");
      fitswrite(tab->headbuf, gstr, &prefs.back_size[n], H_INT, T_LONG);
/*---- BACK_FILTERSIZE */
      sprintf(gstr, "BKFS%04d", n+1);
      addkeywordto_head(tab, gstr, "BACK_FILTERSIZE config parameter");
      fitswrite(tab->headbuf, gstr, &prefs.back_fsize[n], H_INT, T_LONG);
      }
    }

  return;
  }


/****** writefitsinfo_field **************************************************
PROTO	void writefitsinfo_field(fieldstruct *field, fieldstruct *infield)
PURPOSE	Add and copy relevant information to a field FITS header.
INPUT	Pointer to the field.
OUTPUT	-.
NOTES	Global preferences are used.
AUTHOR	E. Bertin (IAP)
VERSION	16/08/2003
 ***/
void	writefitsinfo_field(fieldstruct *field, fieldstruct *infield)

  {
   struct tm		*tm;
   time_t		thetime;
   tabstruct		*tab, *intab;
   wcsstruct		*wcs;
   extern pkeystruct	key[];
   h_type		htype;
   t_type		ttype;
   extern char		keylist[][16];
   char			comment[80],
			*pstr;
   int			d, k, l, nf;

  nf = field->fieldno;
  tab = field->tab;
/* Keep some margin */
  addkeywordto_head(tab, "COMMENT ", "");
  addkeywordto_head(tab, "SOFTNAME", "The software that processed those data");
  fitswrite(tab->headbuf, "SOFTNAME", BANNER, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTVERS", "Version of the software");
  fitswrite(tab->headbuf, "SOFTVERS", VERSION, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTDATE", "Release date of the software");
  fitswrite(tab->headbuf, "SOFTDATE", DATE, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTAUTH", "Maintainer of the software");
  fitswrite(tab->headbuf, "SOFTAUTH", COPYRIGHT, H_STRING, T_STRING);
  addkeywordto_head(tab, "SOFTINST", "Institute");
  fitswrite(tab->headbuf, "SOFTINST", INSTITUTE, H_STRING, T_STRING);
  addkeywordto_head(tab, "COMMENT ", "");

/* User name, we don't use getpwuid() to allow for static linking with glibc 2.3*/
  addkeywordto_head(tab, "AUTHOR", "Who ran the software");
#ifdef HAVE_GETENV
  if (!(pstr=getenv("USERNAME")))	/* Cygwin,... */
    if ((pstr=getenv("LOGNAME")))	/* Linux,... */
      fitswrite(tab->headbuf, "AUTHOR", pstr, H_STRING, T_STRING);
  if (!pstr)
#endif
    fitswrite(tab->headbuf, "AUTHOR", "unknown", H_STRING, T_STRING);

/* Host name */
  if (!gethostname(gstr, 80))
    {
    addkeywordto_head(tab, "ORIGIN", "Where it was done");
    fitswrite(tab->headbuf, "ORIGIN", gstr, H_STRING, T_STRING);
    }

/* Obs dates */
  thetime = time(NULL);
  tm = gmtime(&thetime);
  sprintf(gstr,"%04d-%02d-%02dT%02d:%02d:%02d",
                        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
                        tm->tm_hour, tm->tm_min, tm->tm_sec);
  addkeywordto_head(tab, "DATE    ", "When it was started (GMT)");
  fitswrite(tab->headbuf, "DATE    ", gstr, H_STRING, T_STRING);

/* User-selected keywords */
  if (infield && (intab=infield->tab))
    for (k=0; k<prefs.ncopy_keywords; k++)
      {
      if ((l=fitsfind(intab->headbuf, prefs.copy_keywords[k])) != RETURN_ERROR)
        {
        fitspick(intab->headbuf+l*80, prefs.copy_keywords[k], gstr,
		&htype, &ttype, comment);
        addkeywordto_head(tab, prefs.copy_keywords[k], comment);
        fitswrite(tab->headbuf, prefs.copy_keywords[k], gstr, htype, ttype);
        }
      }

/* Config parameters */
/* COMBINE_TYPE */
  addkeywordto_head(tab, "COMBINET","COMBINE_TYPE config parameter for " \
			BANNER);
  fitswrite(tab->headbuf, "COMBINET", key[findkeys("COMBINE_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.coadd_type], H_STRING, T_STRING);

/* Axis-dependent config parameters */
  addkeywordto_head(tab, "COMMENT ", "");
  addkeywordto_head(tab, "COMMENT ", "Axis-dependent SWarp parameters");
  for (d=0; d<tab->naxis; d++)
    {
    if ((wcs=field->wcs) && wcs->outmax[d])
      {
      sprintf(gstr, "COMIN%-3d", d+1);
      addkeywordto_head(tab, gstr, "Output minimum position of image");
      fitswrite(tab->headbuf, gstr, &wcs->outmin[d], H_INT, T_LONG);
      }
/*-- RESAMPLING_TYPE */
    sprintf(gstr, "RESAMPT%1d", d+1);
    addkeywordto_head(tab, gstr,
		"RESAMPLING_TYPE config parameter");
    fitswrite(tab->headbuf, gstr, key[findkeys("RESAMPLING_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.resamp_type[d]], H_STRING, T_STRING);
/*-- CENTER_TYPE */
    sprintf(gstr, "CENTERT%1d", d+1);
    addkeywordto_head(tab, gstr, "CENTER_TYPE config parameter");
    fitswrite(tab->headbuf, gstr, key[findkeys("CENTER_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.center_type[d]], H_STRING, T_STRING);
/*-- PIXELSCALE_TYPE */
    sprintf(gstr, "PSCALET%1d", d+1);
    addkeywordto_head(tab, gstr, "PIXELSCALE_TYPE config parameter");
    fitswrite(tab->headbuf, gstr, key[findkeys("PIXELSCALE_TYPE", keylist,
	FIND_STRICT)].keylist[prefs.pixscale_type[d]], H_STRING, T_STRING);
    }

/* Image-dependent config parameters */
  addkeywordto_head(tab, "COMMENT ", "");
  addkeywordto_head(tab, "COMMENT ", "Image-dependent SWarp parameters");
/* Flux scales */
  addkeywordto_head(tab, prefs.fscale_keyword,
	"Relative flux scaling from photometry");
  fitswrite(tab->headbuf,prefs.fscale_keyword, &field->fscale,H_EXPO,T_DOUBLE);
  addkeywordto_head(tab, "FLASCALE",
	"Relative flux scaling from astrometry");
  fitswrite(tab->headbuf, "FLASCALE", &field->fascale, H_EXPO,T_DOUBLE);
/* Gain (conversion factor) */
  addkeywordto_head(tab, prefs.gain_keyword,
	"Effective conversion factor in e-/ADU");
  fitswrite(tab->headbuf,prefs.gain_keyword, &field->gain, H_EXPO,T_DOUBLE);

/* Background */
  addkeywordto_head(tab, "BACKMEAN", "Measured background level");
  fitswrite(tab->headbuf, "BACKMEAN", &field->backmean, H_EXPO,T_DOUBLE);
  addkeywordto_head(tab, "BACKSIG ", "Measured background RMS");
  fitswrite(tab->headbuf, "BACKSIG ", &field->backsig, H_EXPO,T_DOUBLE);
/* Input image */
  addkeywordto_head(tab, "ORIGFILE", "Input filename");
  fitswrite(tab->headbuf, "ORIGFILE", field->rfilename, H_STRING, T_STRING);
/* Interpolation flag */
  addkeywordto_head(tab, "INTERPF ", "INTERPOLATE config flag");
  fitswrite(tab->headbuf, "INTERPF ", &prefs.interp_flag[nf], H_BOOL, T_LONG);
/* Background-subtraction flag */
  addkeywordto_head(tab, "BACKSUBF", "SUBTRACT_BACK config flag");
  fitswrite(tab->headbuf, "BACKSUBF", &prefs.subback_flag[nf], H_BOOL, T_LONG);
/* BACK_TYPE */
  addkeywordto_head(tab, "BACKTYPE", "BACK_TYPE config parameter");
  fitswrite(tab->headbuf, "BACKTYPE", key[findkeys("BACK_TYPE", keylist,
        FIND_STRICT)].keylist[prefs.back_type[nf]], H_STRING, T_STRING);
/* BACK_DEFAULT */
  if (field->back_type==BACK_ABSOLUTE)
    {
    addkeywordto_head(tab, "BACKDEF ", "BACK_DEFAULT config parameter");
    fitswrite(tab->headbuf, "BACKDEF ", &prefs.back_default[nf],
		H_EXPO,T_FLOAT);
    }
/* BACK_SIZE */
  addkeywordto_head(tab, "BACKSIZE", "BACK_SIZE config parameter");
  fitswrite(tab->headbuf, "BACKSIZE", &prefs.back_size[nf], H_INT, T_LONG);
/* BACK_FILTERSIZE */
  addkeywordto_head(tab, "BACKFSIZ", "BACK_FSIZE config parameter");
  fitswrite(tab->headbuf, "BACKFSIZ", &prefs.back_fsize[nf], H_INT, T_LONG);

  return;
  }


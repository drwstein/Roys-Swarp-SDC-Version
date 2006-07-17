/*
 				fitswcs.h

*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
*	Part of:	LDACTools+
*
*	Author:		E.BERTIN (IAP)
*
*	Contents:	Include file for fitswcs.c
*
*	Last modify:	17/07/2006
*
*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#ifndef _FITSWCS_H_
#define _FITSWCS_H_

/*-------------------------------- macros -----------------------------------*/

/*----------------------------- Internal constants --------------------------*/

#define		NAXIS	2		/* Max number of FITS axes */

#define		DEG	(PI/180.0)	/* 1 deg in radians */
#define		ARCMIN	(DEG/60.0)	/* 1 arcsec in radians */
#define		ARCSEC	(DEG/3600.0)	/* 1 arcsec in radians */
#define		MAS	(ARCSEC/1000.0)	/* 1 mas in radians */
#define		MJD2000	51544.50000	/* Modified Julian date for J2000.0 */
#define		MJD1950	33281.92346	/* Modified Julian date for B1950.0 */
#define		JU2TROP	1.0000214	/* 1 Julian century in tropical units*/
#define		WCS_NOCOORD	1e31	/* Code for non-existing coordinates */

#define		WCS_NGRIDPOINTS	12	/* Number of WCS grid points / axis */
#define		WCS_NGRIDPOINTS2	(WCS_NGRIDPOINTS*WCS_NGRIDPOINTS)
#define		WCS_INVMAXDEG	9	/* Maximum inversion polynom degree */
#define		WCS_INVACCURACY	0.04	/* Maximum inversion error (pixels) */
#define		WCS_NRANGEPOINTS 32	/* Number of WCS range points / axis */

/*------------------------------- structures --------------------------------*/

typedef struct wcs
  {
  int		naxis;			/* Number of image axes */
  int		naxisn[NAXIS];		/* FITS NAXISx parameters */
  char		ctype[NAXIS][9];	/* FITS CTYPE strings */
  char		cunit[NAXIS][32];	/* FITS CUNIT strings */
  double	crval[NAXIS];		/* FITS CRVAL parameters */
  double	cdelt[NAXIS];		/* FITS CDELT parameters */
  double	crpix[NAXIS];		/* FITS CRPIX parameters */
  double	crder[NAXIS];		/* FITS CRDER parameters */
  double	csyer[NAXIS];		/* FITS CSYER parameters */
  double	cd[NAXIS*NAXIS];	/* FITS CD matrix */
  double	*projp;			/* FITS PV/PROJP mapping parameters */
  int		nprojp;			/* number of useful projp parameters */
  double	longpole,latpole;	/* FITS LONGPOLE and LATPOLE */
  double	wcsmin[NAXIS];		/* minimum values of WCS coords */
  double	wcsmax[NAXIS];		/* maximum values of WCS coords */
  double	wcsscale[NAXIS];	/* typical pixel scale at center */
  double	wcsscalepos[NAXIS];	/* WCS coordinates of scaling point */
  double	wcsmaxradius;		/* Maximum distance to wcsscalepos */
  int		outmin[NAXIS];		/* minimum output pixel coordinate */
  int		outmax[NAXIS];		/* maximum output pixel coordinate */
  int		lat,lng;		/* longitude and latitude axes # */
  double	r0;			/* projection "radius" */
  double	lindet;			/* Determinant of the local matrix */
  double	pixscale;		/* (Local) pixel scale */
  double	ap2000,dp2000;		/* J2000 coordinates of pole */
  double	ap1950,dp1950;		/* B1950 coordinates of pole */
  double	obsdate;		/* Date of observations */
  double	equinox;		/* Equinox of observations */
  double	epoch;			/* Epoch of observations (deprec.) */
  enum {RDSYS_ICRS, RDSYS_FK5, RDSYS_FK4, RDSYS_FK4_NO_E, RDSYS_GAPPT}
		radecsys;		/* FITS RADECSYS reference frame */
  enum	celsys {CSYS_EQU, CSYS_GAL, CSYS_ECL}
		celsys;			/* Celestial coordinate system */
  double	celsysmat[4];		/* Equ. <=> Cel. system parameters */
  int		celsysconvflag;		/* Equ. <=> Cel. conversion needed? */
  struct wcsprm	*wcsprm;		/* WCSLIB's wcsprm structure */
  struct linprm	*lin;			/* WCSLIB's linprm structure */
  struct celprm	*cel;			/* WCSLIB's celprm structure */
  struct prjprm *prj;			/* WCSLIB's prjprm structure */
  struct tnxaxis *tnx_latcor;		/* IRAF's TNX latitude corrections */
  struct tnxaxis *tnx_lngcor;		/* IRAF's TNX longitude corrections */
  struct poly	*inv_x;			/* Proj. correction polynom in x */
  struct poly	*inv_y;			/* Proj. correction polynom in y */
  }	wcsstruct;

/*------------------------------- functions ---------------------------------*/

extern wcsstruct	*create_wcs(char **ctype, double *crval, double *crpix,
				double *cdelt, int *naxisn, int naxis),
			*copy_wcs(wcsstruct *wcsin),
			*read_wcs(tabstruct *tab);

extern double		sextodegal(char *hms),
			sextodegde(char *dms),
			wcs_dist(wcsstruct *wcs,
				double *wcspos1, double *wcspos2),
			wcs_scale(wcsstruct *wcs, double *pixpos);

extern int		raw_to_red(wcsstruct *wcs,
				double *pixpos, double *redpos),
			raw_to_wcs(wcsstruct *wcs,
				double *pixpos, double *wcspos),
			reaxe_wcs(wcsstruct *wcs, int lng, int lat),
			red_to_raw(wcsstruct *wcs,
				double *redpos, double *pixpos),
			wcs_supproj(char *name),
			wcs_to_raw(wcsstruct *wcs,
				double *wcspos, double *pixpos);

extern char		*degtosexal(double alpha, char *str),
			*degtosexde(double delta, char *str);

extern void		end_wcs(wcsstruct *wcs),
			init_wcs(wcsstruct *wcs),
			init_wcscelsys(wcsstruct *wcs),
			invert_wcs(wcsstruct *wcs),
			frame_wcs(wcsstruct *wcsin, wcsstruct *wcsout),
			j2b(double yearobs, double alphain, double deltain,
				double *alphaout, double *deltaout),
			precess(double yearin, double alphain, double deltain,
				double yearout,
				double *alphaout, double *deltaout),
			precess_wcs(wcsstruct *wcs, double yearin,
				double yearout),
			range_wcs(wcsstruct *wcs),
			write_wcs(tabstruct *tab, wcsstruct *wcs);

#endif

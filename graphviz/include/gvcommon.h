/* $Id: gvcommon.h,v 1.7 2006/12/07 22:49:36 erg Exp $ $Revision: 1.7 $ */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      This software is part of the graphviz package      *
*                http://www.graphviz.org/                 *
*                                                         *
*            Copyright (c) 1994-2004 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
**********************************************************/

#ifndef GVCOMMON_H
#define GVCOMMON_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct GVCOMMON_s {
	char *user;
	char **info;
	char *cmdname;
	int verbose;
	boolean config, auto_outfile_names;
        void (*errorfn) (char *fmt, ...);
	char **show_boxes; /* emit code for correct box coordinates */
	char **lib; 

	/* rendering state */
	int viewNum;     /* current view - 1 based count of views,
			    all pages in all layers */
    } GVCOMMON_t;

#ifdef __cplusplus
}
#endif
#endif				/* GVCOMMON_H */

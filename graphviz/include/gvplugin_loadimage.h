/* $Id: gvplugin_loadimage.h,v 1.5 2008/04/18 14:10:43 glenlow Exp $ $Revision: 1.5 $ */
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

#ifndef GVPLUGIN_IMAGELOAD_H
#define GVPLUGIN_IMAGELOAD_H

#include "types.h"
#include "gvplugin.h"
#include "gvcjob.h"

extern void gvdevice_fputs(GVJ_t * job, char *s);
extern void gvdevice_printf(GVJ_t * job, const char *format, ...);

#ifdef __cplusplus
extern "C" {
#endif

extern boolean gvusershape_file_access(usershape_t *us);
extern void gvusershape_file_release(usershape_t *us);

    struct gvloadimage_engine_s {
	void (*loadimage) (GVJ_t *job, usershape_t *us, boxf b, boolean filled);
    };

#ifdef __cplusplus
}
#endif
#endif				/* GVPLUGIN_IMAGELOAD_H */

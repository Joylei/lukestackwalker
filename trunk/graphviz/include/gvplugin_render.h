/* $Id: gvplugin_render.h,v 1.29 2007/12/19 22:03:36 ellson Exp $ $Revision: 1.29 $ */
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

#ifndef GVPLUGIN_RENDER_H
#define GVPLUGIN_RENDER_H

#include "types.h"
#include "gvplugin.h"
#include "gvcjob.h"

extern void gvdevice_fputs(GVJ_t * job, char *s);
extern void gvdevice_printf(GVJ_t * job, const char *format, ...);
extern void gvdevice_printnum(GVJ_t * job, double num);
extern void gvdevice_printpointf(GVJ_t * job, pointf p);
extern void gvdevice_printpointflist(GVJ_t * job, pointf *p, int n);

#ifdef __cplusplus
extern "C" {
#endif

    struct gvrender_engine_s {
	void (*begin_job) (GVJ_t * job);
	void (*end_job) (GVJ_t * job);
	void (*begin_graph) (GVJ_t * job);
	void (*end_graph) (GVJ_t * job);
	void (*begin_layer) (GVJ_t * job, char *layername,
			     int layerNum, int numLayers);
	void (*end_layer) (GVJ_t * job);
	void (*begin_page) (GVJ_t * job);
	void (*end_page) (GVJ_t * job);
	void (*begin_cluster) (GVJ_t * job);
	void (*end_cluster) (GVJ_t * job);
	void (*begin_nodes) (GVJ_t * job);
	void (*end_nodes) (GVJ_t * job);
	void (*begin_edges) (GVJ_t * job);
	void (*end_edges) (GVJ_t * job);
	void (*begin_node) (GVJ_t * job);
	void (*end_node) (GVJ_t * job);
	void (*begin_edge) (GVJ_t * job);
	void (*end_edge) (GVJ_t * job);
	void (*begin_anchor) (GVJ_t * job, char *href, char *tooltip,
			      char *target);
	void (*end_anchor) (GVJ_t * job);
	void (*textpara) (GVJ_t * job, pointf p, textpara_t * str);
	void (*resolve_color) (GVJ_t * job, gvcolor_t * color);
	void (*ellipse) (GVJ_t * job, pointf * A, int filled);
	void (*polygon) (GVJ_t * job, pointf * A, int n, int filled);
	void (*beziercurve) (GVJ_t * job, pointf * A, int n,
			     int arrow_at_start, int arrow_at_end, int);
	void (*polyline) (GVJ_t * job, pointf * A, int n);
	void (*comment) (GVJ_t * job, char *comment);
	void (*library_shape) (GVJ_t * job, char *name, pointf * A, int n, int filled);
    };

#ifdef __cplusplus
}
#endif
#endif				/* GVPLUGIN_RENDER_H */

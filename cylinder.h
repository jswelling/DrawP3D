/****************************************************************************
 * cylinder.h
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/
/*
This header file provides coordinates for a cylinder mesh.  The end plates
need normal vectors which point along the axis of the cylinder, while the
cylinder walls must have radial normals, so each vertex appears twice-
once as an end cap vertex and the other time as a wall vertex.  Walls
are specified first.
*/

#define cylinder_vertices 32
#define cylinder_facets 10

#define S 0.710678

static float cylinder_coords[cylinder_vertices][3]= {
	{ 1.0, 0.0, 0.0 },
	{ S, S, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ -S, S, 0.0 },
	{ -1.0, 0.0, 0.0 },
	{ -S, -S, 0.0 },
	{ 0.0, -1.0, 0.0 },
	{ S, -S, 0.0 },
	{ 1.0, 0.0, 1.0 },
	{ S, S, 1.0 },
	{ 0.0, 1.0, 1.0 },
	{ -S, S, 1.0 },
	{ -1.0, 0.0, 1.0 },
	{ -S, -S, 1.0 },
	{ 0.0, -1.0, 1.0 },
	{ S, -S, 1.0 },
	{ S, -S, 0.0 },
	{ 0.0, -1.0, 0.0 },
	{ -S, -S, 0.0 },
	{ -1.0, 0.0, 0.0 },
	{ -S, S, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ S, S, 0.0 },
	{ 1.0, 0.0, 0.0 },
	{ 1.0, 0.0, 1.0 },
	{ S, S, 1.0 },
	{ 0.0, 1.0, 1.0 },
	{ -S, S, 1.0 },
	{ -1.0, 0.0, 1.0 },
	{ -S, -S, 1.0 },
	{ 0.0, -1.0, 1.0 },
	{ S, -S, 1.0 },
	};

static float cylinder_normals[cylinder_vertices][3]= {
	{ 1.0, 0.0, 0.0 },
	{ S, S, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ -S, S, 0.0 },
	{ -1.0, 0.0, 0.0 },
	{ -S, -S, 0.0 },
	{ 0.0, -1.0, 0.0 },
	{ S, -S, 0.0 },
	{ 1.0, 0.0, 0.0 },
	{ S, S, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ -S, S, 0.0 },
	{ -1.0, 0.0, 0.0 },
	{ -S, -S, 0.0 },
	{ 0.0, -1.0, 0.0 },
	{ S, -S, 0.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, -1.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0 },
	};

#undef S

static int cylinder_v_counts[cylinder_facets]= {
	4, 4, 4, 4, 4, 4, 4, 4, 8, 8 };

static int cylinder_connect[]= {
	0, 1, 9, 8,
	1, 2, 10, 9,
	2, 3, 11, 10,
	3, 4, 12, 11,
	4, 5, 13, 12,
	5, 6, 14, 13,
	6, 7, 15, 14,
	7, 0, 8, 15,
	24, 25, 26, 27, 28, 29, 30, 31,
	16, 17, 18, 19, 20, 21, 22, 23
	};

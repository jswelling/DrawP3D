/****************************************************************************
 * sphere.h
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
This header file provides coordinates for a sphere mesh.
*/

#define sphere_vertices 32
#define sphere_facets 60

static float sphere_coords[sphere_vertices][3]= {
	{ 0.000000, 0.000000, 1.000000 },
	{ 0.894427, 0.000000, 0.447214 },
	{ 0.276393, 0.850651, 0.447214 },
	{ -0.723607, 0.525731, 0.447214 },
	{ -0.723607, -0.525731, 0.447214 },
	{ 0.276393, -0.850651, 0.447214 },
	{ 0.723607, 0.525731, -0.447214 },
	{ -0.276393, 0.850651, -0.447214 },
	{ -0.894427, 0.000000, -0.447214 },
	{ -0.276393, -0.850651, -0.447214 },
	{ 0.723607, -0.525731, -0.447214 },
	{ 0.000000, 0.000000, -1.000000 },
	{ 0.491123, 0.356822, 0.794655 },
	{ -0.187592, 0.577350, 0.794655 },
	{ -0.607062, 0.000000, 0.794654 },
	{ -0.187592, -0.577350, 0.794655 },
	{ 0.491123, -0.356822, 0.794655 },
	{ 0.794655, 0.577350, 0.187592 },
	{ -0.303531, 0.934172, 0.187592 },
	{ -0.982247, 0.000000, 0.187592 },
	{ -0.303531, -0.934172, 0.187592 },
	{ 0.794655, -0.577350, 0.187592 },
	{ 0.303531, 0.934172, -0.187592 },
	{ -0.794655, 0.577350, -0.187592 },
	{ -0.794655, -0.577350, -0.187592 },
	{ 0.303531, -0.934172, -0.187592 },
	{ 0.982247, 0.000000, -0.187592 },
	{ 0.607062, 0.000000, -0.794654 },
	{ 0.187592, 0.577350, -0.794655 },
	{ -0.491123, 0.356822, -0.794655 },
	{ -0.491123, -0.356822, -0.794655 },
	{ 0.187592, -0.577350, -0.794655 },
	};

static int sphere_v_counts[sphere_facets]= {
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

static int sphere_connect[]= {
	0, 12, 13, 0, 13, 14, 0, 14, 15, 0, 15, 16,
	0, 16, 12, 1, 12, 16, 1, 16, 21, 1, 21, 26,
	1, 26, 17, 1, 17, 12, 2, 12, 17, 2, 17, 22,
	2, 22, 18, 2, 18, 13, 2, 13, 12, 3, 13, 18,
	3, 18, 23, 3, 23, 19, 3, 19, 14, 3, 14, 13,
	4, 14, 19, 4, 19, 24, 4, 24, 20, 4, 20, 15,
	4, 15, 14, 5, 15, 20, 5, 20, 25, 5, 25, 21,
	5, 21, 16, 5, 16, 15, 6, 17, 26, 6, 26, 27,
	6, 27, 28, 6, 28, 22, 6, 22, 17, 7, 18, 22,
	7, 22, 28, 7, 28, 29, 7, 29, 23, 7, 23, 18,
	8, 19, 23, 8, 23, 29, 8, 29, 30, 8, 30, 24,
	8, 24, 19, 9, 20, 24, 9, 24, 30, 9, 30, 31,
	9, 31, 25, 9, 25, 20, 10, 21, 25, 10, 25, 31,
	10, 31, 27, 10, 27, 26, 10, 26, 21, 11, 27, 31,
	11, 31, 30, 11, 30, 29, 11, 29, 28, 11, 28, 27
	};

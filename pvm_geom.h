/****************************************************************************
 * pvm_geom.h
 * Copyright 1995, Pittsburgh Supercomputing Center, Carnegie Mellon University
 * Author Joel Welling
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
/* This module provides includes for the PVM-mediated distributed renderer */

#define CLIENT_SYNC_MSG 10101037 /* random and hopefully unused */
#define SERVER_GROUP_NAME "P3D_PVM_RENSERVER"
#define CLIENT_GROUP_NAME "P3D_PVM_CLIENT"

/* Message syntax elements:
 *
 * type_length_word:   { int ((vertexcount<<8) | (vertextype&255)) }
 *
 * vertex_list_record: { type_length_word, { float }* }
 *     (with appropriate number of floats for type_length_word)
 *
 * mesh_record:        { int polycount, int total_index_count,
 *                       { int vertex_count }*, 
 *                       { int index }*, vertex_list_record }
 *     (with polycount vertex_count elements, and as many index
 *      values as the sum of all the vertex_count values)
 * string_record:      { int length, string tstring }
 *     (with length= strlen(tstring), and tstring null terminated)
 * transform_record:   { {float val}*12 }
 *     (with the 12 values giving the first 12 transform matrix elements;
 *      rest are (0,0,0,1)
 *
 */

typedef enum {
  PVM3D_CONNECT,    /* message is { int tid, string_record instance } */
  PVM3D_DRAW,       /* message is { string_record instance, int framenumber, */
                    /*      { pvm3d_submsgtype, submsg }* } */
  PVM3D_DISCONNECT, /* message is { int tid, string_record instance } */
  PVM3D_DRAW_AND_RESET, /* message is { string_record instance } */
  PVM3D_DEMO_HACK,  /* demo hacks; to give quick update msgs safe id space */
  PVM3D_LAST } pvm3d_msgtype;

typedef enum {
  PVM3D_BACKCULL,   /* msg record is { int boolean_value } */
  PVM3D_COLOR,      /* msg record is { float r, float g, float b, float a } */
  PVM3D_MATERIAL,   /* msg record is { int material_id } */
  PVM3D_TEXT_HEIGHT,/* msg record is { float text_height } */
  PVM3D_SPHERE,     /* msg record is { transform_record } */
  PVM3D_CYLINDER,   /* msg record is { transform_record } */
  PVM3D_TORUS,      /* msg record is { float major, float minor, */
                    /*                 transform_record } */
  PVM3D_POLYMARKER, /* msg record is { vertex_list_record } */
  PVM3D_POLYLINE,   /* msg record is { vertex_list_record } */
  PVM3D_POLYGON,    /* msg record is { vertex_list_record } */
  PVM3D_TRISTRIP,   /* msg record is { vertex_list_record } */
  PVM3D_BEZIER,     /* msg record is { vertex_list_record } */
  PVM3D_MESH,       /* msg record is { mesh_record } */
  PVM3D_TEXT,       /* msg record is { string_record tstring, float loc_x,   */
                    /*                 float loc_y, float loc_z, float u_x,  */
                    /*                 float u_y, float u_z, float v_x,      */
                    /*                 float v_y, float v_z,                 */
                    /*                 transform_record }                    */
  PVM3D_CAMERA,     /* msg record is { float fm_x, float fm_y, float fm_z,   */
                    /*                 float at_x, float at_y, float at_z,   */
                    /*                 float up_x, float up_y, float up_z,   */
                    /*                 float fovea, float hither, float yon, */
                    /*                 float bg_r, float bg_g, float bg_b,   */
                    /*                 float bg_a } */
  PVM3D_ENDFRAME,   /* msg record is {} */
  PVM3D_SM_LAST } pvm3d_submsgtype;


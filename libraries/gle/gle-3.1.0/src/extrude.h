/*
 * FILE:
 * extrude.h
 *
 * FUNCTION:
 * Prototypes for privately used subroutines for the tubing library.
 * Applications should not include this file.
 *
 * HISTORY:
 * Copyright (C) 1991,2003 Linas Vepstas <linas@linas.org>
 */

#ifndef GLE_EXTRUDE_H_
#define GLE_EXTRUDE_H_

/* ============================================================ */
/* 
 * Provides choice of calling subroutine, vs. invoking macro.
 * Basically, inlines the source, or not.
 * Trades performance for executable size.
 */

#define INLINE_INTERSECT
#ifdef INLINE_INTERSECT
#define INNERSECT(sect,p,n,v1,v2) { int retval; INTERSECT(retval,sect,p,n,v1,v2); }
#else
#define INNERSECT(sect,p,n,v1,v2) intersect(sect,p,n,v1,v2)
#endif /* INLINE_INTERSECT */

/* ============================================================ */
/* The folowing defines give a kludgy way of accessing the qmesh primitive */

/*
#define bgntmesh _emu_qmesh_bgnqmesh
#define endtmesh _emu_qmesh_endqmesh
#define c3f _emu_qmesh_c3f
#define n3f _emu_qmesh_n3f
#define v3f _emu_qmesh_v3f
*/

/* ============================================================ */
/* Mid-level  drawing routines.  Note that some of these differ
 * only due to the different color array argument 
 */

extern void 
up_sanity_check (gleDouble up[3],      /* up vector for contour */
                    int npoints,              /* numpoints in poly-line */
                    gleDouble point_array[][3]);   /* polyline */


extern void 
draw_raw_style_end_cap (int ncp,             /* number of contour points */
                    gleDouble contour[][2],  /* 2D contour */
                    gleDouble zval,          /* where to draw cap */
                    int frontwards);         /* front or back cap */

extern void 
draw_round_style_cap_callback (int iloop,
                    double cap[][3],
                    gleColor face_color,
                    gleDouble cut_vector[3],
                    gleDouble bisect_vector[3],
                    double norms[][3],
                    int frontwards);

extern void 
draw_round_style_cap_callback_c4f (int iloop,
                    double cap[][3],
                    gleColor4f face_color,
                    gleDouble cut_vector[3],
                    gleDouble bisect_vector[3],
                    double norms[][3],
                    int frontwards);

extern void 
extrusion_raw_join (int ncp,                   /* number of contour points */
                    gleDouble contour[][2],    /* 2D contour */
                    gleDouble cont_normal[][2],/* 2D contour normal vecs */
                    gleDouble up[3],           /* up vector for contour */
                    int npoints,               /* numpoints in poly-line */
                    gleDouble point_array[][3], /* polyline */
                    gleColor color_array[],     /* color of polyline */
                    gleDouble xform_array[][2][3]);  /* 2D contour xforms */


extern void 
extrusion_raw_join_c4f (int ncp,               /* number of contour points */
                    gleDouble contour[][2],    /* 2D contour */
                    gleDouble cont_normal[][2],/* 2D contour normal vecs */
                    gleDouble up[3],           /* up vector for contour */
                    int npoints,               /* numpoints in poly-line */
                    gleDouble point_array[][3],      /* polyline */
                    gleColor4f color_array[],        /* color of polyline */
                    gleDouble xform_array[][2][3]);  /* 2D contour xforms */


extern void 
extrusion_round_or_cut_join (int ncp,          /* number of contour points */
                    gleDouble contour[][2],    /* 2D contour */
                    gleDouble cont_normal[][2],/* 2D contour normal vecs */
                    gleDouble up[3],           /* up vector for contour */
                    int npoints,               /* numpoints in poly-line */
                    gleDouble point_array[][3], /* polyline */
                    gleColor color_array[],     /* color of polyline */
                    gleDouble xform_array[][2][3]);  /* 2D contour xforms */


extern void 
extrusion_round_or_cut_join_c4f (int ncp, /* number of contour points */
                    gleDouble contour[][2],    /* 2D contour */
                    gleDouble cont_normal[][2],/* 2D contour normal vecs */
                    gleDouble up[3],           /* up vector for contour */
                    int npoints,               /* numpoints in poly-line */
                    gleDouble point_array[][3],    /* polyline */
                    gleColor4f color_array[],      /* color of polyline */
                    gleDouble xform_array[][2][3]);  /* 2D contour xforms */


extern void 
extrusion_angle_join (int ncp,      /* number of contour points */
                    gleDouble contour[][2],    /* 2D contour */
                    gleDouble cont_normal[][2],/* 2D contour normal vecs */
                    gleDouble up[3],           /* up vector for contour */
                    int npoints,               /* numpoints in poly-line */
                    gleDouble point_array[][3],  /* polyline */
                    gleColor color_array[],      /* color of polyline */
                    gleDouble xform_array[][2][3]);  /* 2D contour xforms */

extern void 
extrusion_angle_join_c4f (int ncp,      /* number of contour points */
                    gleDouble contour[][2],    /* 2D contour */
                    gleDouble cont_normal[][2],/* 2D contour normal vecs */
                    gleDouble up[3],           /* up vector for contour */
                    int npoints,               /* numpoints in poly-line */
                    gleDouble point_array[][3],      /* polyline */
                    gleColor4f color_array[],        /* color of polyline */
                    gleDouble xform_array[][2][3]);  /* 2D contour xforms */

#endif /* GLE_EXTRUDE_H_ */
/* -------------------------- end of file -------------------------------- */

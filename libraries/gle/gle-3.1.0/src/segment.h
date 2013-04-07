
/*
 * MODULE: segment.h
 *
 * FUNCTION:
 * Contains function prototypes for segment drawing subroutines.
 * These are the lowest-level drawing routines in the system.
 *
 * These are used only internally, and are not to be exported to
 * the user.
 *
 * HISTORY:
 * Copyright (c) 1991,2003 Linas Vepstas <linas@linas.org>
 */

/* ============================================================ */

extern void draw_segment_plain (int ncp,       /* number of contour points */
                           gleVector front_contour[],
                           gleVector back_contour[],
                           int inext, double len);

extern void draw_segment_color (int ncp,       /* number of contour points */
                           gleVector front_contour[],
                           gleVector back_contour[],
                           gleColor color_last,
                           gleColor color_next,
                           int inext, double len);

extern void draw_segment_color_c4f (int ncp,   /* number of contour points */
                           gleVector front_contour[],
                           gleVector back_contour[],
                           gleColor4f color_last,
                           gleColor4f color_next,
                           int inext, double len);

extern void draw_segment_edge_n (int ncp,      /* number of contour points */
                           gleVector front_contour[],
                           gleVector back_contour[],
                           double norm_cont[][3],
                           int inext, double len);

extern void draw_segment_c_and_edge_n (int ncp,   
                           gleVector front_contour[],
                           gleVector back_contour[],
                           double norm_cont[][3],
                           gleColor color_last,
                           gleColor color_next,
                           int inext, double len);

extern void draw_segment_c_and_edge_n_c4f (int ncp,   
                           gleVector front_contour[],
                           gleVector back_contour[],
                           double norm_cont[][3],
                           gleColor4f color_last,
                           gleColor4f color_next,
                           int inext, double len);

extern void draw_segment_facet_n (int ncp,     
                           gleVector front_contour[],
                           gleVector back_contour[],
                           double norm_cont[][3],
                           int inext, double len);

extern void draw_segment_c_and_facet_n (int ncp,    
                           gleVector front_contour[],
                           gleVector back_contour[],
                           double norm_cont[][3],
                           gleColor color_last,
                           gleColor color_next,
                           int inext, double len);

extern void draw_segment_c_and_facet_n_c4f (int ncp,    
                           gleVector front_contour[],
                           gleVector back_contour[],
                           double norm_cont[][3],
                           gleColor4f color_last,
                           gleColor4f color_next,
                           int inext, double len);

/* ============================================================ */

extern void draw_binorm_segment_edge_n (int ncp,  
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           int inext, double len);

extern void draw_binorm_segment_c_and_edge_n (int ncp,   
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           gleColor color_last,
                           gleColor color_next,
                           int inext, double len);

extern void draw_binorm_segment_c_and_edge_n_c4f (int ncp,   
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           gleColor4f color_last,
                           gleColor4f color_next,
                           int inext, double len);

extern void draw_binorm_segment_facet_n (int ncp, 
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           int inext, double len);

extern void draw_binorm_segment_c_and_facet_n (int ncp,    
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           gleColor color_last,
                           gleColor color_next,
                           int inext, double len);

extern void draw_binorm_segment_c_and_facet_n_c4f (int ncp,    
                           double front_contour[][3],
                           double back_contour[][3],
                           double front_norm[][3],
                           double back_norm[][3],
                           gleColor4f color_last,
                           gleColor4f color_next,
                           int inext, double len);

extern void draw_angle_style_front_cap (int ncp,       /* num of contour pts */
                           gleDouble bi[3],             /* biscetor */
                           gleVector point_array[]); /* polyline */

extern void draw_angle_style_back_cap (int ncp,        /* num of contour pts */
                           gleDouble bi[3],             /* biscetor */
                           gleVector point_array[]); /* polyline */


/* -------------------------- end of file -------------------------------- */

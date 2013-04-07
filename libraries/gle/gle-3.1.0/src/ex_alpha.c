/*
 * FILE:
 * ex_alpha.c
 *
 * FUNCTION:
 * Framework file to auto-generate subroutines with different 
 * signature for color argument.  The *_c4f routines take a 
 * 3D RGBA color so that alpha transparency can be supported.
 *
 * HISTORY:
 * Copyright (c) 2003 Linas Vepstas <linas@linas.org>
 */

#include "config.h"
#include "gle.h"
#include "port.h"

/* Autogen the missing c4f routines by redefining C3F to be 4F, 
 * changeing the subroutine signature, and changing the function names.
 */

#undef   C3F
#define  C3F(x)      glColor4fv(x)
#define  gleColor    gleColor4f
#define  COLOR_SIGNATURE 1

#define draw_raw_segment_color         draw_raw_segment_color_c4f
#define draw_raw_segment_c_and_edge_n  draw_raw_segment_c_and_edge_n_c4f
#define draw_raw_segment_c_and_facet_n draw_raw_segment_c_and_facet_n_c4f

#define draw_round_style_cap_callback  draw_round_style_cap_callback_c4f
#define draw_segment_color             draw_segment_color_c4f
#define draw_segment_c_and_edge_n      draw_segment_c_and_edge_n_c4f
#define draw_segment_c_and_facet_n     draw_segment_c_and_facet_n_c4f
#define draw_binorm_segment_c_and_edge_n draw_binorm_segment_c_and_edge_n_c4f
#define draw_binorm_segment_c_and_facet_n draw_binorm_segment_c_and_facet_n_c4f

#define extrusion_raw_join             extrusion_raw_join_c4f
#define extrusion_angle_join           extrusion_angle_join_c4f
#define extrusion_round_or_cut_join    extrusion_round_or_cut_join_c4f

#define gen_polycone                   gen_polycone_c4f

#define gleSuperExtrusion              gleSuperExtrusion_c4f
#define gleExtrusion                   gleExtrusion_c4f
#define glePolyCylinder                glePolyCylinder_c4f
#define glePolyCone                    glePolyCone_c4f
#define gleTwistExtrusion              gleTwistExtrusion_c4f

/* Now include the source files ! */

#include "ex_angle.c"
#include "ex_cut_round.c"
#include "ex_raw.c"
#include "extrude.c"
#include "round_cap.c"
#include "segment.c"

/* ===================== END OF FILE ======================== */

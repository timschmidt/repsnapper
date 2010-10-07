#pragma once

/*
* 3DS AutoCAD 3D Studio Max binary files;
* ASE AutoCAD ASCII export files;
* BYU Movie.BYU surface geometry files;
* DXF AutoCAD DXF files;
* GMOD Golgotha GMOD files;
* HRC SoftImage hierarchical files;
* IV SGI Inventor files;
* OBJ - a file format from Alias ( http://www.alias.com/eng/index.shtml)
* OFF GEOMVIEW OFF files;
* SMF Michael Garland's format for his QSLIM program;
* STL/STLA ASCII Stereolithography files;
* STLB binary Stereolithography files;
* TRI/TRIA a simple ASCII triangle format requested by Greg Hood;
* TRIB a simple binary triangle format requested by Greg Hood;
* VLA Evans and Sutherland Digistar II VLA files for planetariums;
---------------------------------------------------------------------------------
* POV Persistence of Vision files (output only);
* TEC TECPLOT files (output only);
* TXT a text dump (output only);
* UCD Advanced Visual Systems (AVS) Unstructured Cell Data (output only);
* WRL WRL/VRML (Virtual Reality Modeling Language) files (output only).
* XGL the XGL format, based on the XML language and OpenGl graphics (output only).
*/


//****************************************************************************80
//
//  FUNCTION PROTOTYPES 
//
//****************************************************************************80
extern int ivconmain ( int argc, char *argv[] );
int ase_read ( FILE *filein );
int ase_write ( FILE *fileout );
int byu_read ( FILE *filein );
int byu_write ( FILE *fileout );
char ch_cap ( char c );
bool ch_eqi ( char c1, char c2 );
int ch_index_last ( char* string, char c );
bool ch_is_space ( char c );
int ch_pad ( int *char_index, int *null_index, char *s, int max_string );
char ch_read ( FILE *filein );
int ch_to_digit ( char c );
int ch_write ( FILE *fileout, char c );
int command_line ( char **argv );
void cor3_normal_set ( );
void cor3_range ( );
void data_check ( );
void data_init ( );
bool data_read ( );
void data_report ( );
int data_write ( );
int dxf_read ( FILE *filein );
int dxf_write ( FILE *fileout );
int edge_count ( );
void edge_null_delete ( );
void face_area_set ( );
void face_normal_ave ( );
void face_null_delete ( );
int face_print ( int iface );
void face_reverse_order ( );
int face_subset ( );
void face_to_line ( );
void face_to_vertex_material ( );
char *file_ext ( char *file_name );
float float_read ( FILE *filein );
float float_reverse_bytes ( float x );
int float_write ( FILE *fileout, float float_val );
bool gmod_arch_check ( );
int gmod_read ( FILE *filein );
float gmod_read_float ( FILE *filein );
unsigned short gmod_read_w16 ( FILE *filein );
unsigned long gmod_read_w32 ( FILE *filein );
int gmod_write ( FILE *fileout );
void gmod_write_float ( float Val, FILE *fileout );
void gmod_write_w16 ( unsigned short Val, FILE *fileout );
void gmod_write_w32 ( unsigned long Val, FILE *fileout );
void hello ( );
void help ( );
int hrc_read ( FILE *filein );
int hrc_write ( FILE *fileout );
int i4_max ( int i1, int i2 );
int i4_min ( int i1, int i2 );
int i4_modp ( int i, int j );
int i4_wrap ( int ival, int ilo, int ihi );
void init_program_data ( );
int interact ( );
int iv_read ( FILE *filein );
int iv_write ( FILE *fileout );
int i4vec_max ( int n, int *a );
long int long_int_read ( FILE *filein );
int long_int_write ( FILE *fileout, long int int_val );
void news ( );
void node_to_vertex_material ( );
int obj_read ( FILE *filein );
int obj_write ( FILE *fileout );
int off_read ( ifstream &file_in );
int off_write ( FILE *fileout );
int pov_write ( FILE *fileout );
int rcol_find ( float a[][200000], int m, int n, float r[] );	// the 200000 is the define COR3_MAX in incon.cpp
float rgb_to_hue ( float r, float g, float b );
bool s_eqi ( const char* string1, const char* string2 );
int s_len_trim ( char *s );
int s_to_i4 ( char *s, int *last, bool *error );
bool s_to_i4vec ( char *s, int n, int ivec[] );
float s_to_r4 ( char *s, int *lchar, bool *error );
bool s_to_r4vec ( char *s, int n, float rvec[] );
short int short_int_read ( FILE *filein );
int short_int_write ( FILE *fileout, short int int_val );
int smf_read ( FILE *filein );
int smf_write ( FILE *fileout );
void sort_heap_external ( int n, int *indx, int *i, int *j, int isgn );
int stla_read ( FILE *filein );
int stla_write ( FILE *fileout );
int stlb_read ( FILE *filein );
int stlb_write ( FILE *fileout );
void tds_pre_process ( );
int tds_read ( FILE *filein );
unsigned long int tds_read_ambient_section ( FILE *filein );
unsigned long int tds_read_background_section ( FILE *filein );
unsigned long int tds_read_boolean ( unsigned char *boolean, FILE *filein );
unsigned long int tds_read_camera_section ( FILE *filein );
unsigned long int tds_read_edit_section ( FILE *filein, int *views_read );
unsigned long int tds_read_keyframe_section ( FILE *filein, int *views_read );
unsigned long int tds_read_keyframe_objdes_section ( FILE *filein );
unsigned long int tds_read_light_section ( FILE *filein );
unsigned long int tds_read_u_long_int ( FILE *filein );
int tds_read_long_name ( FILE *filein );
unsigned long int tds_read_matdef_section ( FILE *filein );
unsigned long int tds_read_material_section ( FILE *filein );
int tds_read_name ( FILE *filein );
unsigned long int tds_read_obj_section ( FILE *filein );
unsigned long int tds_read_object_section ( FILE *filein );
unsigned long int tds_read_tex_verts_section ( FILE *filein );
unsigned long int tds_read_texmap_section ( FILE *filein );
unsigned short int tds_read_u_short_int ( FILE *filein );
unsigned long int tds_read_spot_section ( FILE *filein );
unsigned long int tds_read_unknown_section ( FILE *filein );
unsigned long int tds_read_view_section ( FILE *filein, int *views_read );
unsigned long int tds_read_vp_section ( FILE *filein, int *views_read );
int tds_write ( FILE *fileout );
int tds_write_string ( FILE *fileout, const char *string );
int tds_write_u_short_int ( FILE *fileout, unsigned short int int_val );
int tec_write ( FILE *fileout );
void tmat_init ( float a[4][4] );
void tmat_mxm ( float a[4][4], float b[4][4], float c[4][4] );
void tmat_mxp ( float a[4][4], float x[4], float y[4] );
void tmat_mxp2 ( float a[4][4], float x[][3], float y[][3], int n );
void tmat_mxv ( float a[4][4], float x[4], float y[4] );
void tmat_rot_axis ( float a[4][4], float b[4][4], float angle, char axis );
void tmat_rot_vector ( float a[4][4], float b[4][4], float angle,  float v1, float v2, float v3 );
void tmat_scale ( float a[4][4], float b[4][4], float sx, float sy, float sz );
void tmat_shear ( float a[4][4], float b[4][4], char *axis, float s );
void tmat_trans ( float a[4][4], float b[4][4], float x, float y, float z );
int tria_read ( FILE *filein );
int tria_write ( FILE *fileout );
int trib_read ( FILE *filein );
int trib_write ( FILE *fileout );
int txt_write ( FILE *fileout );
int ucd_write ( FILE *fileout );
void vertex_normal_set ( );
void vertex_to_face_material ( );
void vertex_to_node_material ( );
int vla_read ( FILE *filein );
int vla_write ( FILE *fileout );
int wrl_write ( FILE *filout );
int xgl_write ( FILE *fileout );

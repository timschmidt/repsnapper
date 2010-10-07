#include "stdafx.h"

// This class was snagged from the opengl tutorial at http://www.gamedev.net/reference/articles/article1172.asp

#include "glutils.h"

CGL_Light3D::CGL_Light3D ( ) {
	static GLint lightNr = GL_LIGHT0;
	on_off_state = 0;
	Init ( lightNr );
	lightNr++;
}

CGL_Light3D::~CGL_Light3D ( ) {
}

void CGL_Light3D::Init ( int light ) {
	LIGHT = light;

	// set all members to OpenGL defaults
	// all values comply with the OpenGL 1.2.1 Specification
	ambient[0] = 0.0f;
	ambient[1] = 0.0f;
	ambient[2] = 0.0f;
	ambient[3] = 1.0f;

	if ( light == GL_LIGHT0 ) {
		diffuse[0] = 1.0f;
		diffuse[1] = 1.0f;
		diffuse[2] = 1.0f;
		diffuse[3] = 1.0f;
	} else {
		diffuse[0] = 0.0f;
		diffuse[1] = 0.0f;
		diffuse[2] = 0.0f;
		diffuse[3] = 1.0f;
	}

	if ( light == GL_LIGHT0 ) {
		specular[0] = 1.0f;
		specular[1] = 1.0f;
		specular[2] = 1.0f;
		specular[3] = 1.0f;
	} else {
		specular[0] = 0.0f;
		specular[1] = 0.0f;
		specular[2] = 0.0f;
		specular[3] = 1.0f;
	}

	position[0] = 0.0f;
	position[1] = 1.0f;
	position[2] = 0.0f;
	position[3] = 0.0f;

	spot_direction[0] =  0.0f;
	spot_direction[1] =  0.0f;
	spot_direction[2] = -1.0f;

	spot_exponent = 0.0f;
	spot_cutoff = 180.0f;

	constant_attenuation  = 1.0f;
	linear_attenuation    = 0.0f;
	quadratic_attenuation = 0.0f;
}

void CGL_Light3D::SetValues ( ) {
	glLightfv ( LIGHT, GL_AMBIENT, ambient );
	glLightfv ( LIGHT, GL_DIFFUSE, diffuse );
	glLightfv ( LIGHT, GL_SPECULAR, specular );
	glLightfv ( LIGHT, GL_POSITION, position );
	glLightfv ( LIGHT, GL_SPOT_DIRECTION, spot_direction );
	glLightfv ( LIGHT, GL_SPOT_EXPONENT, &spot_exponent );
	glLightfv ( LIGHT, GL_CONSTANT_ATTENUATION, &constant_attenuation );
	glLightfv ( LIGHT, GL_LINEAR_ATTENUATION, &linear_attenuation );
	glLightfv ( LIGHT, GL_QUADRATIC_ATTENUATION, &quadratic_attenuation );
}

void CGL_Light3D::TurnOn ( ) {
	on_off_state = 1;
	glEnable ( LIGHT );
}

void CGL_Light3D::TurnOff ( ) {
	on_off_state = 0;
	glDisable ( LIGHT );
}

void CGL_Light3D::Toggle ( ) {
	on_off_state = 1 - on_off_state;

	if ( on_off_state )
		glEnable ( LIGHT );
	else
		glDisable ( LIGHT );
}

void CGL_Light3D::SetAmbientColor ( float r, float g, float b, float a ) {
	ambient[0] = r;
	ambient[1] = g;
	ambient[2] = b;
	ambient[3] = a;
}

void CGL_Light3D::GetAmbientColor ( float *r, float *g, float *b, float *a ) {
	*r = ambient[0];
	*g = ambient[0];
	*b = ambient[0];
	*a = ambient[0];
}

void CGL_Light3D::SetAmbientColor ( float c[4] ) {
	ambient[0] = c[0];
	ambient[1] = c[1];
	ambient[2] = c[2];
	ambient[3] = c[3];
}

void CGL_Light3D::GetAmbientColor ( float *c[4] ) {
	*c[0] = ambient[0];
	*c[1] = ambient[1];
	*c[2] = ambient[2];
	*c[3] = ambient[3];
}

void CGL_Light3D::SetDiffuseColor ( float r, float g, float b, float a ) {
	diffuse[0] = r;
	diffuse[1] = g;
	diffuse[2] = b;
	diffuse[3] = a;
}

void CGL_Light3D::GetDiffuseColor ( float *r, float *g, float *b, float *a ) {
	*r = diffuse[0];
	*g = diffuse[1];
	*b = diffuse[2];
	*a = diffuse[3];
}

void CGL_Light3D::SetDiffuseColor ( float c[4] ) {
	diffuse[0] = c[0];
	diffuse[1] = c[1];
	diffuse[2] = c[2];
	diffuse[3] = c[3];
}

void CGL_Light3D::GetDiffuseColor ( float *c[4] ) {
	*c[0] = diffuse[0];
	*c[1] = diffuse[1];
	*c[2] = diffuse[2];
	*c[3] = diffuse[3];
}

void CGL_Light3D::SetPosition ( float x, float y, float z, float w ) {
	position[0] = x;
	position[1] = y;
	position[2] = z;
	position[3] = w;
}

void CGL_Light3D::GetPosition ( float *x, float *y, float *z, float *w ) {
	*x = position[0];
	*y = position[1];
	*z = position[2];
	*w = position[3];
}

void CGL_Light3D::SetPosition ( float p[4] ) {
	position[0] = p[0];
	position[1] = p[1];
	position[2] = p[2];
	position[3] = p[3];
}

void CGL_Light3D::GetPosition ( float *p[4] ) {
	*p[0] = position[0];
	*p[1] = position[1];
	*p[2] = position[2];
	*p[3] = position[3];
}

void CGL_Light3D::SetSpecular ( float r, float g, float b, float a ) {
	specular[0] = r;
	specular[1] = g;
	specular[2] = b;
	specular[3] = a;
}

void CGL_Light3D::GetSpecular ( float *r, float *g, float *b, float *a ) {
	*r = specular[0];
	*g = specular[1];
	*b = specular[2];
	*a = specular[3];
}

void CGL_Light3D::SetSpecular ( float s[4] ) {
	specular[0] = s[0];
	specular[1] = s[1];
	specular[2] = s[2];
	specular[3] = s[3];
}

void CGL_Light3D::GetSpecular ( float *s[4] ) {
	*s[0] = specular[0];
	*s[1] = specular[1];
	*s[2] = specular[2];
	*s[3] = specular[3];
}

void CGL_Light3D::SetSpotDirection ( float x, float y, float z ) {
	spot_direction[0] = x;
	spot_direction[1] = y;
	spot_direction[2] = z;
}

void CGL_Light3D::GetSpotDirection ( float *x, float *y, float *z ) {
	*x = spot_direction[0];
	*y = spot_direction[1];
	*z = spot_direction[2];
}

void CGL_Light3D::SetSpotDirection ( float s[3] ) {
	spot_direction[0] = s[0];
	spot_direction[1] = s[1];
	spot_direction[2] = s[2];
}

void CGL_Light3D::GetSpotDirection ( float *s[3] ) {
	*s[0] = spot_direction[0];
	*s[1] = spot_direction[1];
	*s[2] = spot_direction[2];
}

void CGL_Light3D::SetSpotExponent ( float exponent ) {
	spot_exponent = exponent;
}

float CGL_Light3D::GetSpotExponent ( ) {
	return spot_exponent;
}

void CGL_Light3D::SetSpotCutoff ( float cutoff ) {
	spot_cutoff = cutoff;
}

float CGL_Light3D::GetSpotCutoff ( ) {
	return spot_cutoff;
}

void CGL_Light3D::SetConstantAtt ( float constant ) {
	constant_attenuation = constant;
}

float CGL_Light3D::GetConstantAtt ( ) {
	return constant_attenuation;
}

void CGL_Light3D::SetLinearAtt ( float linear ) {
	linear_attenuation = linear;
}

float CGL_Light3D::GetLinearAtt ( ) {
	return linear_attenuation;
}

void CGL_Light3D::SetQuadraticAtt ( float quadratic ) {
	quadratic_attenuation = quadratic;
}

float CGL_Light3D::GetQuadraticAtt ( ) {
	return quadratic_attenuation;
}

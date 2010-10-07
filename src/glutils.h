#ifndef OPENGL_UTILITIES_H
#define OPENGL_UTILITIES_H

// This class was snagged from the opengl tutorial at http://www.gamedev.net/reference/articles/article1172.asp

class CGL_Light3D {
public:
	CGL_Light3D ( );
	~CGL_Light3D ( );

	void Init ( int light );   		// Resets all light information and sets current 
                              		// light to a OpenGL light GL_LIGHT0, GL_LIGHT1, etc.
                               		// specified by the user
    void SetLight ( int light );	// Similar to Init (light) except that current
									// light settings in the class are set to defaults.
	void SetValues ( );        		// Completely Sets Light Up. Must be called if any changes
                                 	// are made to the light.

	void TurnOn ( );           		// Turns on light
	void TurnOff ( );          		// Turns off light
	void Toggle ( );           		// Toggles light on or off
	bool GetOnOffState ( ) { return on_off_state;}	   		// Returns current on/off state

	int GetLightNr() { return LIGHT;}

	void SetAmbientColor ( float r, float g, float b, float a );
	void GetAmbientColor ( float *r, float *g, float *b, float *a );
	void SetAmbientColor ( float c[4] );
	void GetAmbientColor ( float *c[4] );

	void SetDiffuseColor ( float r, float g, float b, float a );
	void GetDiffuseColor ( float *r, float *g, float *b, float *a );
	void SetDiffuseColor ( float c[4] );
	void GetDiffuseColor ( float *c[4] );

	void SetPosition ( float x, float y, float z, float w );
	void GetPosition ( float *x, float *y, float *z, float *w );
	void SetPosition ( float p[4] );
	void GetPosition ( float *p[4] );

	void SetSpecular ( float r, float g, float b, float a );
	void GetSpecular ( float *r, float *g, float *b, float *a );
	void SetSpecular ( float s[4] );
	void GetSpecular ( float *s[4] );

	/* Spot Light Functions */
	void SetSpotDirection ( float x, float y, float z );
	void GetSpotDirection ( float *x, float *y, float *z );
	void SetSpotDirection ( float s[3] );
	void GetSpotDirection ( float *s[3] );

	void SetSpotExponent ( float exponent );
	float GetSpotExponent ( );

	void SetSpotCutoff ( float cutoff );
	float GetSpotCutoff ( );

	/* Attenuation factors */
	void SetConstantAtt ( float constant );
	float GetConstantAtt ( );
	void SetLinearAtt ( float linear );
	float GetLinearAtt ( );
	void SetQuadraticAtt ( float quadratic );
	float GetQuadraticAtt ( );
private:
	int LIGHT;

	float ambient[4];
	float diffuse[4];
	float specular[4];
	float position[4];
	float spot_direction[3];
	float spot_exponent;
	float spot_cutoff;
	float constant_attenuation;
	float linear_attenuation;
	float quadratic_attenuation;

	int on_off_state;
};

#endif
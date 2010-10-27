#pragma once
#include "stdafx.h"
class gllight
{
	public:
		gllight();
		~gllight();
	
		void Init (int position);
		void On();
		void Off();
	
		void SetAmbient(float r, float g, float b, float a);
		void SetDiffuse(float r, float g, float b, float a);
		void SetLocation(float x, float y, float z, float w);
		void SetSpecular(float r, float g, float b, float a);

	private:
		int offset;
};

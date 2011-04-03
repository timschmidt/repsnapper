/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

// stdafx.cpp : source file that includes just the standard includes
// RepSnapper.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "stl.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

ModelViewController *MVC;

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define LERP(a, b, t) ( a + t * (b - a) )

bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2)
{
	return d1.d < d2.d;
}

float TeRound(float val)
{       
	if (val>=0)
		return (int)(val+.5);
	else
		return (int)(val-.5);
}

void HSVtoRGB (const Vector3f &hsv, float &r,float &g,float &b)
{
  HSVtoRGB (hsv.x, hsv.y, hsv.z, r, g, b);
}

void HSVtoRGB (const Vector3f &hsv, Vector3f &rgb)
{
  HSVtoRGB (hsv, rgb.r, rgb.g, rgb.b);
}


void HSVtoRGB 	(const float &h, const float &s, const float &v, float &r,float &g,float &b) 			
{
	int i;
	float f, p, q, t, hh = h*360.0f;

	if( s == 0 || h == -1) {
		// achromatic (grey)
		r = g = b = v;
		return;
	}

	hh /= 60;                       // sector 0 to 5
	i = TeRound(floor(hh));
	f = hh - i;                     // factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
case 0:
	r = v;
	g = t;
	b = p;
	break;
case 1:
	r = q;
	g = v;
	b = p;
	break;
case 2:
	r = p;
	g = v;
	b = t;
	break;
case 3:
	r = p;
	g = q;
	b = v;
	break;
case 4:
	r = t;
	g = p;
	b = v;
	break;
default:                // case 5:
	r = v;
	g = p;
	b = q;
	break;
	}
}

void RGBtoHSV 	(const float &r, const float &g, const float &b, float &h, float &s, float &v) 			
{
	if(r == g && g == b) // achromatic (grey)
	{
		h = - 1;
		s = 0;
		v = r;
		return;
	}

	float min, max, delta;

	min = MIN(r, g);
	min = MIN(min, b);
	max = MAX(r, g);
	max = MAX(max, b);
	v = max;                                // v

	delta = max - min;

	if( max != 0 )
		s = delta / max;                // s
	else {
		// r = g = b = 0                // s = 0, v is undefined
		s = 0;
		h = -1;
		return;
	}

	if( r == max )
		h = ( g - b ) / delta;          // between yellow & magenta
	else if( g == max )
		h = 2 + ( b - r ) / delta;      // between cyan & yellow
	else
		h = 4 + ( r - g ) / delta;      // between magenta & cyan

	h *= 60;                                // degrees
	if( h < 0 )
		h += 360;

}


void RGBTOHSL(float red, float green, float blue, float &hue, float &sat, float &lightness)
{
	double min, max;
	double delta;
	int maxval;

	if (red > green) {
		if (red > blue) {
			max = red;
			maxval = 0;
		} else {
			max = blue;
			maxval = 2;
		}
		min = MIN(green, blue);
	} else {
		if (green > blue) {
			max = green;
			maxval = 1;
		} else {
			max = blue;
			maxval = 2;
		}
		min = MIN(red, blue);
	}

	lightness = (max + min) / 2.0;
	delta = max - min;

	if (delta < .000001) { /* Suggested by Eugene Anikin <eugene@anikin.com> */
		sat = 0.0;
		hue = 0.0;
	} else {
		if (lightness <= .5)
			sat = delta / (max + min);
		else
			sat = delta / (2 - max - min);

		if (maxval == 0)
			hue = (green - blue) / delta;
		else if (maxval == 1)
			hue = 2 + (blue - red) / delta;
		else
			hue = 4 + (red - green) / delta;

		if (hue < 0.0)
			hue += 6.0;
		else if (hue > 6.0)
			hue -= 6.0;
	}

	hue *= 60;                                // degrees
}

void RGBTOYUV(float r, float g, float b, float &y, float &u, float &v)
{
	y = (0.299*r + 0.587*g + 0.114*b);
	u = (-0.147*r - 0.289*g + 0.436*b)+0.5;
	v = (0.615*r - 0.515*g - 0.100*b)+0.5;
}

void YUVTORGB(float y, float u, float v, float &r, float &g, float &b)
{
	r=y+1.140*v;
	g=y-0.395*u-0.581*v;
	b=y+2.032*u;
}

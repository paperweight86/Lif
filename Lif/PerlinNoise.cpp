#include "stdafx.h"

#include "PerlinNoise.h"

#define _USE_MATH_DEFINES
#include <math.h>

float Perlin::Noise2D( __int32 x, __int32 y )
{
	__int32 n = x + y * 57;
	n = ( n<<13 ) ^ n;
	return ( 1.0f - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f );
}

float Perlin::SmoothNoise2D( float x, float y)
{
	float corners = ( Noise2D( (__int32)( x-1.0f ), (__int32)( y-1.0f ) ) + Noise2D( (__int32)( x+1.0f ), (__int32)( y-1.0f ) ) + Noise2D( (__int32)( x-1.0f ), (__int32)( y+1.0f ) )+ Noise2D( (__int32)( x+1.0f ), (__int32)( y+1.0f ) ) ) / 16.0f;
	float sides = ( Noise2D( (__int32)( x-1.0f ), (__int32)( y ) )  + Noise2D( (__int32)( x+1.0f ), (__int32)( y ) ) + Noise2D( (__int32)( x ), (__int32)( y-1.0f ) ) + Noise2D( (__int32)( x ), (__int32)( y+1.0f ) ) ) /  8.0f;
	float center =  Noise2D( (__int32)( x ), (__int32)( y ) ) / 4.0f;
	return ( corners + sides + center );
}

static float CosInterpolate(float a, float b, float s)
{
	float ft = s * (float)M_PI;
	float f = (1 - cosf(ft)) * 0.5f;
	return  a * (1 - f) + (b * f);
}

float Perlin::InterpolatedNoise2D(float x, float y)
{
	//calculate integer and fractional parts of the input co-ords
	__int32 xInt = (__int32)x;
	float xFract = x - (float)xInt;

	__int32 yInt = (__int32)y;
	float yFract = y - (float)yInt;

	//retrieve 4 smoothed noise samples
	float v1 = SmoothNoise2D( xInt,     yInt );
	float v2 = SmoothNoise2D( xInt + 1, yInt );
	float v3 = SmoothNoise2D( xInt,     yInt + 1 );
	float v4 = SmoothNoise2D( xInt + 1, yInt + 1 );

	//interpolate between these samples using cosine interpolation
	float _1 = CosInterpolate( v1, v2, xFract );
	float _2 = CosInterpolate( v3, v4, xFract );
	return CosInterpolate( _1 , _2 , yFract );
}

float Perlin::Get2DNoise( float x, float y, float persistence, unsigned __int32 nOctaves )
{
	float total = 0.0f;

	for( unsigned __int32 i = 0; i < nOctaves; i++ )
	{
		float frequency = powf( 2, (float)i );
		float amplitude = powf( persistence, (float)i );
		total = total + InterpolatedNoise2D( x * frequency, y * frequency ) * amplitude;
	}

	return total;
}

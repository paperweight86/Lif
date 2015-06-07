#pragma once

/*
	Author: Daniel Jennings
	Date: 02/03/08
	Referances: Derived From an explaination on http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
*/

namespace Perlin // all functions return noise in the range -1.0f - 1.0f
{
	//base noise function
	float Noise2D( __int32 x, __int32 y );
	//smothed noise using the base function and several samples
	float SmoothNoise2D( float x, float y);
	//interpolated noise using both of the above functions and interpolating between noise values
	float InterpolatedNoise2D(float x, float y);
	//uses interpolated noise function to generate perlin noise with a number of octaves
	//and a level of persistance
	float Get2DNoise( float x, float y, float persistence, unsigned __int32 nOctaves );
}

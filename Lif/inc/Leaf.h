#pragma once

#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

#include "float2.h"

namespace lif
{
	//struct LeafTrunk
	//{
	//	float2* trunk_pts;
	//	u32 trunk_pt_count;
	//	/*     T  B
	//	     \ | /
	//	      \|/
	//	       |
	//	      \|/
	//         |
	//	T - trunk points
	//	B - branch point
	//	*/
	//	float2* branch_pts;
	//};

	struct Leaf
	{
		//LeafTrunk* trunks;
		//u32		   trunk_count;
		std::vector<float2> geometry;
	};

	struct LeafConfig
	{
		r32 trunk_len;
		u32 trunk_resolution;
		r32 trunk_stem_ratio;
		u32 branch_count;
		r32 branch_angle;
		r32 branch_max_len;
		r32 border_distance;
	};

	void GenerateLeaf( Leaf* leaf, LeafConfig& cfg )
	{
		// Skeleton (trunk and branches)
		auto trunk_pts = new float2[cfg.trunk_resolution];
		std::vector<std::vector<float2>> trunk_branches;
		rv2 pos(0.0f, 0.0f);
		r32 gap = -(r32)cfg.trunk_len / (r32)cfg.trunk_resolution;
		rv2 avgPos (0.0f,0.0f);
		float2 top, bottom;
		std::vector<float2> left;
		std::vector<float2> right;
		r32 angle = cfg.branch_angle;
		for (int i = cfg.trunk_resolution - 1; i >= 0; --i)
		{
			pos = rv2(0.0f, (r32)i*gap);
			if (i == 0)
				bottom = pos;
			else if( i + 1 == cfg.trunk_resolution )
				top = pos;
			avgPos = avgPos + pos;
			trunk_pts[i] = pos;
			auto branches = std::vector<float2>();
			trunk_branches.push_back(branches);
			
			if(i != 0 && i + 1 != cfg.trunk_resolution)
			{
				real coeff = (real)(i) / (real)(cfg.trunk_resolution - 2);
				real branch_len_scale = sin(M_PI * pow(coeff, 0.7f));

				// Left
				auto endPoint = float2(pos.x + cosf(angle) * cfg.branch_max_len*branch_len_scale,
									   pos.y - sinf(angle) * cfg.branch_max_len*branch_len_scale);
				trunk_branches.back().push_back(endPoint);
				avgPos = avgPos + endPoint;
				left.push_back(endPoint);

				// Right
				endPoint = float2(pos.x + -cosf(angle) * cfg.branch_max_len*branch_len_scale,
								  pos.y -  sinf(angle) * cfg.branch_max_len*branch_len_scale);
				trunk_branches.back().push_back(endPoint);
				avgPos = avgPos + endPoint;
				right.push_back(endPoint);
			}
		}

		//TODO: [DanJ] Not sure we need this at all
		//rv2 avg = avg / (cfg.trunk_resolution * cfg.branch_count * 2.0f);

		// Geometry (skin)
		// Note [DanJ]: geometry is basically a border around the extemeties of the skeleton
		leaf->geometry.push_back(top);
		for (auto iter = right.cbegin(); iter != right.cend(); ++iter)
			leaf->geometry.push_back((*iter));
		leaf->geometry.push_back(bottom);
		for (auto iter = left.crbegin(); iter != left.crend(); ++iter)
			leaf->geometry.push_back((*iter));
	}
}

#pragma once

#include <vector>

namespace lif
{
	struct SBranch
	{
		float2 start;
		float2 end;
		float startThickness;
		float endThickness;
		float2 v0;
		float2 v1;
		float2 v2;
		float2 v3;
	};

	class CLSystemTree
	{
	public:
		float m_seed;
		uint8 m_maxTrunks;
		float m_maxTrunkDev;
		float m_maxTrunkLen;
		float m_branchPow;
		float m_trunkScale;
		uint8 m_maxDepth;
		float m_maxBranchAngle;
		float m_interval;
		float m_scale;
	private:
		std::vector<float3> m_vertices;
		std::vector<int>	m_leafIndexes;
	private:
		virtual void Branch( const SBranch& last, float angle, int& depth );
		virtual void Trunk( float2 start, int& trunk );
		virtual bool TestIntersection( float2 lineA1, float2 lineA2, float2 lineB1, float2 lineB2, float2& result );
		virtual void CalcBranchStartPoints( const float2& startPoint, const float2& endPoint,
											float2& startPointA, float2& startPointB, float thickEnd );
		virtual void CalcBranchEndPoints( const float2& startPoint, const float2& endPoint,
										  float2& endPointA, float2& endPointB, float thickEnd );
	public:
		CLSystemTree();
		virtual ~CLSystemTree();
		void Generate( );
		const std::vector<float3>& GetVerts() { return m_vertices; }
		const std::vector<int>& GetLeaves() { return m_leafIndexes; }
	};
}
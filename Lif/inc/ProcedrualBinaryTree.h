#pragma once

#include <vector>

namespace lif
{
	class CProcedrualBinaryTree
	{
	private:
		enum BranchType
		{
			eBranchType_Trunk,
			eBranchType_Left,
			eBranchType_Right
		};
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
			float2 center;
			BranchType type;
		};
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
		float m_branchThickMul;
	private:
		std::vector<float3> m_vertices;
		std::vector<int>	m_leafIndexes;
		std::vector<float>	m_leafRotations;
		std::vector<float2>	m_debugPositions;
	private:
		virtual bool Branch(SBranch& last, float angle, int& depth);
		virtual void Trunk(float2 start, int& trunk);
		virtual bool TestIntersection(float2 lineA1, float2 lineA2, float2 lineB1, float2 lineB2, float2& result);
		virtual void CalcBranchStartPoints(const float2& startPoint, const float2& endPoint,
			float2& startPointA, float2& startPointB, float thickEnd);
		virtual void CalcBranchEndPoints(const float2& startPoint, const float2& endPoint,
			float2& endPointA, float2& endPointB, float thickEnd);
	public:
		CProcedrualBinaryTree();
		virtual ~CProcedrualBinaryTree();
		void Generate();
		void ConsumeVertexBuffer(std::vector<float3>& out) { out = std::move(m_vertices); }
		void ConsumeAuxIndexBuffer(const tchar* name, std::vector<int>& out)
		{ 
			if (_tcscmp(name, _T("leaves")) == 0)
				out = std::move(m_leafIndexes);
		}
		void ConsumeAuxBuffer(const tchar* name, std::vector<float>& out)
		{
			if (_tcscmp(name, _T("leafRotations")) == 0)
				out = std::move(m_leafRotations);
		}
		void ConsumeAuxBuffer(const tchar* name, std::vector<float2>& out)
		{
			if (_tcscmp(name, _T("debugPositions")) == 0)
				out = std::move(m_debugPositions);
		}
	};
}
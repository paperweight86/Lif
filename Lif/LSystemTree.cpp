#include "stdafx.h"
#include "LSystemTree.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <algorithm>

#define M_PI_(x) x
#define M_PI_F M_PI_(M_PI) ## f
#define M_PI_2_F M_PI_(M_PI_2) ## f

using namespace lif;

CLSystemTree::CLSystemTree()
{
	// Angle of the trunk
	m_maxTrunkDev = M_PI_F / 12.0f;
	/// Length of the trunk
	m_maxTrunkLen = 50.0f;
	// Maximum times to split each branch
	m_maxDepth = 10;
	// How long the trunk should be (scaled from default...)
	m_trunkScale = 1.0f;
	// How many trunks to have - essentially stacks trees
	m_maxTrunks = 1;
	// The base angle for new branches relative to the old branch
	m_interval = M_PI_F / 10.0f;
	// The power to raise the branch length to when creating a new branch
	m_branchPow = 20.0f;
	// The overall scale of the tree from 0-1 in x & y
	m_scale = 45.0f;
	// The maximum absolute angle of a branch
	m_maxBranchAngle = M_PI_F / 12.0f;
}

CLSystemTree::~CLSystemTree()
{
}

void CLSystemTree::CalcBranchEndPoints( const float2& startPoint, const float2& endPoint,
								  	    float2& endPointA, float2& endPointB, float thickEnd)
{
	float2 line = float2(startPoint.x - endPoint.x, startPoint.y - endPoint.y);

	float2 perp = float2(-line.y, line.x);
	float perpLen = sqrtf(powf(perp.x, 2.0f) + powf(perp.y, 2.0f));
	if (perpLen == 0)
		return;

	// Normalise
	perp = float2(perp.x / perpLen, perp.y / perpLen);

	float topThickness = thickEnd / 2.0f;

	endPointA = float2(endPoint.x - perp.x * topThickness,
		endPoint.y - perp.y * topThickness);

	endPointB = float2(endPoint.x + perp.x * topThickness,
		endPoint.y + perp.y * topThickness);
}

void CLSystemTree::CalcBranchStartPoints( const float2& startPoint, const float2& endPoint,
										  float2& startPointA, float2& startPointB, float thickEnd)
{
	float2 line = float2(startPoint.x - endPoint.x, startPoint.y - endPoint.y);

	float2 perp = float2(-line.y, line.x);
	float perpLen = sqrtf(powf(perp.x, 2.0f) + powf(perp.y, 2.0f));
	if (perpLen == 0)
		return;

	// Normalise
	perp = float2(perp.x / perpLen, perp.y / perpLen);

	float topThickness = thickEnd / 2.0f;

	startPointA = float2(startPoint.x - perp.x * topThickness,
		startPoint.y - perp.y * topThickness);

	startPointB = float2(startPoint.x + perp.x * topThickness,
		startPoint.y + perp.y * topThickness);
}

bool CLSystemTree::TestIntersection(float2 lineA1, float2 lineA2, float2 lineB1, float2 lineB2, float2& result)
{
	float divisor = (lineA1.x - lineA2.x) * (lineB1.y - lineB2.y) - (lineA1.y - lineA2.y) * (lineB1.x - lineB2.x);
	if (fabs(divisor) < float_epsilon)
	{
		return false;
	}
	else
	{
		float x = (lineA1.x * lineA2.y - lineA1.y * lineA2.x) * (lineB1.x - lineB2.x) - (lineA1.x - lineA2.x) * (lineB1.x * lineB2.y - lineB1.y * lineB2.x);
		float y = (lineA1.x * lineA2.y - lineA1.y * lineA2.x) * (lineB1.y - lineB2.y) - (lineA1.y - lineA2.y) * (lineB1.x * lineB2.y - lineB1.y * lineB2.x);
		result = float2(x / divisor, y / divisor);
		return true;
	}
}

void CLSystemTree::Branch(const SBranch& last, float angle, int& depth)
{
	if (depth > m_maxDepth || angle > M_PI - m_maxBranchAngle || angle < m_maxBranchAngle)
	{
		m_leafIndexes.push_back(m_vertices.size()-1);
		return;
	}

	SBranch leftBranch, rightBranch;

	float fdepth = (float)(depth + 1);
	fdepth = (float)(m_maxDepth + 1) - fdepth;
	float thickPow = 1.0f;
	float thickness = std::max<float>(1.0f, powf(fdepth, thickPow) * m_trunkScale);
	if (depth == 0)
		thickPow = 1.0f;
	float lastThickness = std::max<float>(1.0f, powf(std::max(fdepth, 1.0f), thickPow) * m_trunkScale);

	float angleRight = (float)(rand() % 360) / 360.0f * m_interval * 2.0f;
	float angleLeft = (float)(rand() % 360) / 360.0f * m_interval * 2.0f;
	float scaleRnd = (float)(rand() % 100);
	float lScale = 1.0f - powf(((float)depth / (float)m_maxDepth), m_branchPow) * (scaleRnd / 100.0f);
	lScale = std::max<float>(0.001f, lScale);
	scaleRnd = (float)(rand() % 100);
	float rScale = 1.0f - powf(((float)depth / (float)m_maxDepth), m_branchPow) * (scaleRnd / 100.0f);
	rScale = std::max<float>(0.001f, rScale);

	rightBranch.start = leftBranch.start = last.end;
	rightBranch.v0 = leftBranch.v0 = last.v3;
	rightBranch.v1 = leftBranch.v1 = last.v2;
	rightBranch.startThickness = leftBranch.startThickness = last.endThickness;
	rightBranch.endThickness = leftBranch.endThickness = thickness;
	leftBranch.end = float2(leftBranch.start.x + cosf(angle + angleLeft)*(m_scale*lScale),
		leftBranch.start.y - sinf(angle + angleLeft)*(m_scale*lScale));
	rightBranch.end = float2(rightBranch.start.x + cosf(angle - angleRight)*(m_scale*rScale),
		rightBranch.start.y - sinf(angle - angleRight)*(m_scale*rScale));
	CalcBranchEndPoints(  leftBranch.start,  leftBranch.end,  leftBranch.v3,  leftBranch.v2, thickness );
	CalcBranchEndPoints( rightBranch.start, rightBranch.end, rightBranch.v3, rightBranch.v2, thickness );

	/*
		\  L\    /  R/
	 	 \   \  /   /
		  \   \/   /  <---- calculate the point at which a branch vert can be moved to to optimise out the other vert
		   \  /\  /
		    \/  \/
			|  T |
			|    |
	*/
	float2 intersection(0.0f, 0.0f);
	if (TestIntersection(  leftBranch.v0, 
						   leftBranch.v2, 
						  rightBranch.v1, 
						  rightBranch.v3, 
						  intersection))
	{
		// 0 - bottom right
		// 1 - bottom left
		// 2 - top left
		// 3 - top right
		 leftBranch.v0 = intersection;
		rightBranch.v1 = intersection;
	}

	++depth;
	int depthLeft = depth;
	int depthRight = depth;

	m_vertices.push_back(leftBranch.v2);
	Branch(leftBranch, angle + angleLeft, depthLeft);
	m_vertices.push_back( leftBranch.v3);
	m_vertices.push_back(rightBranch.v1);
	m_vertices.push_back(rightBranch.v2);
	Branch(rightBranch, angle - angleRight, depthRight);
	m_vertices.push_back(rightBranch.v3);
}

void CLSystemTree::Trunk(float2 start, int& trunk)
{
	float fTrunkAngle = M_PI_2_F; // half pi
	// TODO: rand() is a poor rand and something in <random> would probably be better
	// http://www.cplusplus.com/reference/random/
	float fTrunkAngleRnd = ((float)(rand() % 1000) / 1000.0f) * m_maxTrunkDev;

	if (rand() % 2 == 1)
		fTrunkAngle += -fTrunkAngleRnd;
	else
		fTrunkAngle += fTrunkAngleRnd;

	float2 trunkEnd = float2(start.x + (cosf(fTrunkAngle)*m_maxTrunkLen),
							 start.y - (sinf(fTrunkAngle)*m_maxTrunkLen));
	float scaleRnd = (float)(rand() % 100);
	float trunkScale = 1.0f - powf(((float)trunk / (float)m_maxTrunks), m_branchPow) * (scaleRnd / 100.0f);
	float fdepth = (float)(m_maxDepth + 1);
	float trunkThickness = std::max<float>(1.0f, powf(std::max(fdepth, 1.0f), 1.0f) * m_trunkScale);

	float2 branchTrunkPos[4];
	CalcBranchStartPoints(float2(0.0f, 0.0f), trunkEnd, branchTrunkPos[0], branchTrunkPos[1], trunkThickness);
	CalcBranchEndPoints(float2(0.0f, 0.0f), trunkEnd, branchTrunkPos[3], branchTrunkPos[2], trunkThickness);

	m_vertices.push_back(branchTrunkPos[1]);
	m_vertices.push_back(branchTrunkPos[2]);

	SBranch trunkBranch;
	trunkBranch.v0 = branchTrunkPos[0];
	trunkBranch.v1 = branchTrunkPos[1];
	trunkBranch.v2 = branchTrunkPos[2];
	trunkBranch.v3 = branchTrunkPos[3];
	trunkBranch.start = float2(0.0f, 0.0f);
	trunkBranch.end = trunkEnd;
	trunkBranch.startThickness = trunkThickness;
	trunkBranch.endThickness = trunkThickness;
	int depth = 0;
	Branch(trunkBranch, fTrunkAngle, depth);
	m_vertices.push_back(branchTrunkPos[3]);
	m_vertices.push_back(branchTrunkPos[0]);

	++trunk;
	if (trunk < m_maxTrunks)
		Trunk( trunkEnd, trunk );
}

void CLSystemTree::Generate()
{
	srand((int)m_seed + m_maxTrunks);
	int depth = 0;
	int trunk = 0;
	Trunk( float2(0.0f, 0.0f), trunk );
}

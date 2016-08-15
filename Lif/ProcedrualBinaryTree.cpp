#include "stdafx.h"
#include "inc\ProcedrualBinaryTree.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <algorithm>

#define M_PI_(x) x
#define M_PI_F M_PI_(M_PI) ## f
#define M_PI_2_F M_PI_(M_PI_2) ## f

#include "float2.h"
#include "float3.h"

using namespace lif;

CProcedrualBinaryTree::CProcedrualBinaryTree()
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
	// Multiplied by the previous thickness to achieve a branch thickness reduction
	m_branchThickMul = 0.75f;
}

CProcedrualBinaryTree::~CProcedrualBinaryTree()
{
}

void CProcedrualBinaryTree::CalcBranchEndPoints( const float2& startPoint, const float2& endPoint,
								  					   float2& endPointA,		 float2& endPointB, 
													   float thickEnd )
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

void CProcedrualBinaryTree::CalcBranchStartPoints( const float2& startPoint, const float2& endPoint,
													     float2& startPointA, float2& startPointB, 
														 float thickEnd)
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

bool CProcedrualBinaryTree::TestIntersection(float2 lineA1, float2 lineA2, float2 lineB1, float2 lineB2, float2& result)
{
	float divisor = (lineA1.x - lineA2.x) * (lineB1.y - lineB2.y) 
				  - (lineA1.y - lineA2.y) * (lineB1.x - lineB2.x);
	if (fabs(divisor) <= float_epsilon)
	{
		return false;
	}
	else
	{
		float x = (lineA1.x * lineA2.y - lineA1.y * lineA2.x) 
				* (lineB1.x - lineB2.x) - (lineA1.x - lineA2.x) 
				* (lineB1.x * lineB2.y - lineB1.y * lineB2.x);
		float y = (lineA1.x * lineA2.y - lineA1.y * lineA2.x) 
			    * (lineB1.y - lineB2.y) - (lineA1.y - lineA2.y) 
				* (lineB1.x * lineB2.y - lineB1.y * lineB2.x);
		result = float2(x / divisor, y / divisor);
		return true;
	}
}

bool CProcedrualBinaryTree::Branch(SBranch& last, float angle, int& depth, bool isBushel)
{
	if (!isBushel)
	{
		if (depth > m_maxDepth || angle > M_PI - m_maxBranchAngle || angle < m_maxBranchAngle)
		{
			//if(last.type == BranchType::eBranchType_Right)
			//{
			//	m_vertices.push_back(last.v2);
			//	m_vertices.push_back(last.v3);
			//}
			//else if(last.type == BranchType::eBranchType_Left)
			{
				//m_vertices.push_back(last.v2);
				//m_vertices.push_back(last.v3);
				//m_vertices.push_back(last.center);
			}
			// here we should also write out the rotation 

			// TODO: Move this to the section which figures out if we branched to remove double leaves and other shit

			//m_leafIndexes.push_back(m_vertices.size() - 1);
			//float2 branchV = last.v0 - last.v3;
			//branchV = branchV / sqrt(pow(branchV.x,2.0f) + pow(branchV.y,2.0f));
			//float2 res = uti::dot(float2(0.0f, -1.0f), branchV);
			//float angle = acos(res.x);
			//angle = angle * 57.295779513082320876798154814105f - 180.0f;
			//
			//m_leafRotations.push_back(angle);
			//m_debugPositions.push_back(last.v0);

			return false;
		}
	}
	else if(depth - m_maxDepth >= 4)
	{
		return false;
	}

	SBranch leftBranch, rightBranch;

	float fdepth = (float)(depth + 1);
	fdepth = (float)(m_maxDepth + 1) - fdepth;
	float thickPow = 1.0f;
	float thickness = std::max<float>(1.0f, powf(std::max(fdepth, 1.0f), thickPow) * m_trunkScale);
	if (depth == 0)
		thickPow = 1.0f;
	float lastThickness = last.endThickness; 
	//thickness = lastThickness / 2.0f;
	float rThick = m_branchThickMul;//(float)(rand() % 1000) / 1000.0f;
	float lThick = m_branchThickMul;//(float)(rand() % 1000) / 1000.0f;// 1.0f - rThick;

	//float ptDist = uti::dist(last.v3, last.v2);
	//assert(fabs(ptDist - lastThickness) < 0.00001f);
	assert(rThick*lastThickness <= lastThickness);
	assert(lThick*lastThickness <= lastThickness);

	float angleRight = (float)(rand() % 360) / 360.0f * m_interval * 2.0f;
	float angleLeft = (float)(rand() % 360) / 360.0f * m_interval * 2.0f;
	float scaleRnd = (float)(rand() % 100);
	float lScale = 1.0f - powf(((float)depth / (float)m_maxDepth), m_branchPow) * (scaleRnd / 100.0f);
	if (isBushel)
		lScale *= 0.2f;
	//lScale *= lScale;
	lScale = std::max<float>(0.001f, lScale);
	scaleRnd = (float)(rand() % 100);
	float rScale = 1.0f - powf(((float)depth / (float)m_maxDepth), m_branchPow) * (scaleRnd / 100.0f);
	if (isBushel)
		rScale *= 0.2f;
	//rScale *= rScale;
	rScale = std::max<float>(0.001f, rScale);

	rightBranch.start = leftBranch.start = last.end;
	rightBranch.v0 = leftBranch.v0 = last.v3;
	rightBranch.v1 = leftBranch.v1 = last.v2;
	rightBranch.startThickness = rThick*lastThickness;
	rightBranch.endThickness   = rThick*lastThickness;
	rightBranch.type = BranchType::eBranchType_Right;

	leftBranch.startThickness = lThick*lastThickness;
	leftBranch.endThickness =	lThick*lastThickness;
	leftBranch.type = BranchType::eBranchType_Left;
	leftBranch.end = float2( leftBranch.start.x  + cosf(angle + angleLeft) *(m_scale*lScale),
						     leftBranch.start.y  - sinf(angle + angleLeft) *(m_scale*lScale));
	rightBranch.end = float2(rightBranch.start.x + cosf(angle - angleRight)*(m_scale*rScale),
						     rightBranch.start.y - sinf(angle - angleRight)*(m_scale*rScale));
	CalcBranchEndPoints(  leftBranch.start,  leftBranch.end,  leftBranch.v3,  leftBranch.v2, lThick*lastThickness );
	CalcBranchEndPoints( rightBranch.start, rightBranch.end, rightBranch.v3, rightBranch.v2, rThick*lastThickness );

	//Logger.Info(_T("Target: %f"), thickness);
	//Logger.Info(_T("Left: %f"), uti::dist(leftBranch.v3, leftBranch.v2));
	//Logger.Info(_T("Right: %f"), uti::dist(rightBranch.v3, rightBranch.v2));

	/*
		\  L\    /  R/
	 	 \   \  /   /
		  \   \/   /  <---- calculate the point at which a branch vert can be moved to to optimise out the other vert
		   \  /\  /
		    \/  \/
			|  T |
			|    |
	*/

	/*

		Unfortunately the previous explaination doesn't work nicely!

		The intersection actually needs to be calculated between the "root" lines and each branch end points to construct the branch
		start points as they aren't necessarily the same.

				   r2  r3
				   |   |
	  l3__________I|___|rI
	  l2___________|___|
			   lI/1|   |0
				   |   |


		2_______3
		|       |
		|       |
		|_______|
		1       0
	*/

	// TODO: DELAY TESSELATION!?!?!
	// TODO: extend lines to force intersection!??!

	float2 intersectionLeft(0.0f, 0.0f);
	// previous left vs new left left
	if( TestIntersection(leftBranch.v1, leftBranch.v2, last.v1, last.v2, intersectionLeft) )
	{ 
		leftBranch.v1 = intersectionLeft;
		last.v2 = intersectionLeft;
	}
	else
	{
		Logger.Error(_T("No left vs new left left intersection"));
	}

	float2 intersectionRight(0.0f, 0.0f);
	// previous right vs new right right
	if (TestIntersection(rightBranch.v0, rightBranch.v3, last.v0, last.v3, intersectionRight))
	{ 
		rightBranch.v0 = intersectionRight;
		last.v3 = intersectionRight;
	}
	else
	{ 
		Logger.Error(_T("No right vs new right right intersection"));
	}

	float2 intersectionTopCenter(0.0f, 0.0f);
	// new left right vs new right left
	if (TestIntersection(leftBranch.v0, leftBranch.v3, rightBranch.v1, rightBranch.v2, intersectionTopCenter))
	{ 
		leftBranch.v0 = intersectionTopCenter;
		rightBranch.v1 = intersectionTopCenter;

		leftBranch.center = intersectionTopCenter;
		rightBranch.center = intersectionTopCenter;
	}
	else
	{ 
		Logger.Error(_T("No left vs new right left intersection"));
	}

	++depth;
	int depthLeft = depth;
	int depthRight = depth;

	m_vertices.push_back(last.v2);
	
	bool right = true;
	bool left = true;

	if (Branch(leftBranch, angle + angleLeft, depthLeft, isBushel))
		m_vertices.push_back(intersectionTopCenter);
	else
		left = false;
	
	if (Branch(rightBranch, angle - angleRight, depthRight, isBushel))
		m_vertices.push_back(last.v3);
	else
		right = false;

	if (!left && !right)
	{
		if (!isBushel)
		{
			isBushel = true;
			//m_vertices.push_back(last.v2);
			//m_vertices.push_back(last.v3);

			//m_leafIndexes.push_back(m_vertices.size() - 1);

			//float2 halfV = (last.v3 - last.v2) / 2.0f;
			//float2 toLeaf = (last.v1 - last.v2);
			//float len = sqrtf(uti::dot(toLeaf, toLeaf).x);
			//toLeaf = toLeaf / len;
			//angle = acosf(uti::dot(float2(1.0f, 0.0f), toLeaf).x) - M_PI_2;
			////TODO: This also works but it's slower?
			////angle = atan2f(toLeaf.y, toLeaf.x) - M_PI_2;
			//angle = angle * 57.295779513082320876798154814105f;

			//m_debugPositions.push_back(last.v2 + halfV);
			//m_leafRotations.push_back(angle);
		}
		//else
		{
			right = true;
			left = true;

			if (Branch(leftBranch, angle + angleLeft, depthLeft, true))
				m_vertices.push_back(intersectionTopCenter);
			else
				left = false;

			if (Branch(rightBranch, angle - angleRight, depthRight, true))
				m_vertices.push_back(last.v3);
			else
				right = false;

			if (!left && !right)
			{
				m_vertices.push_back(last.v2);
				m_vertices.push_back(last.v3);

				m_leafIndexes.push_back(m_vertices.size() - 1);

				float2 halfV = (last.v3 - last.v2) / 2.0f;
				float2 toLeaf = (last.v1 - last.v2);
				float len = sqrtf(uti::dot(toLeaf, toLeaf).x);
				toLeaf = toLeaf / len;
				angle = acosf(uti::dot(float2(1.0f, 0.0f), toLeaf).x) - M_PI_2;
				//TODO: This also works but it's slower?
				//angle = atan2f(toLeaf.y, toLeaf.x) - M_PI_2;
				angle = angle * 57.295779513082320876798154814105f;

				m_debugPositions.push_back(last.v2 + halfV);
				m_leafRotations.push_back(angle);
			}
			else if (!left)
			{
				m_vertices.push_back(last.v3);
			}
			else if (!right)
			{
				m_vertices.push_back(last.v3);
			}
		}
	}
	else if (!left)
	{
		m_vertices.push_back(last.v3);
	}
	else if (!right)
	{
		m_vertices.push_back(last.v3);
	}

	return true;
}

void CProcedrualBinaryTree::Trunk(float2 start, int& trunk)
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
	//m_vertices.push_back(branchTrunkPos[2]);

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
	Branch(trunkBranch, fTrunkAngle, depth, false);
	//m_vertices.push_back(branchTrunkPos[3]);
	m_vertices.push_back(branchTrunkPos[0]);

	++trunk;
	if (trunk < m_maxTrunks)
		Trunk( trunkEnd, trunk );
}

void CProcedrualBinaryTree::Generate()
{
	srand((int)m_seed + m_maxTrunks);
	int depth = 0;
	int trunk = 0;
	Trunk( float2(0.0f, 0.0f), trunk );
}

#include "stdafx.h"

#include "WinWindow.h"
#include "2DRendererFactory.h"
#include "ScopedLog.h"

using namespace tod;

#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

#define _USE_MATH_DEFINES // for PI!
#include <math.h>

#include <thread>

//#include <png.h>
//#include "zlib.h"

#include "LSystemTree.h"
using namespace lif;
using namespace uti;

struct branchRect
{
public:
	float2 pos[4];

	branchRect()
	{
	}

	branchRect(const float2* _pos)
	{
		pos[0] = _pos[0];
		pos[1] = _pos[1];
		pos[2] = _pos[2];
		pos[3] = _pos[3];
	}
};

vector<rhandle> g_tree;
vector<float2> g_singlePolyTree;
vector<branchRect> g_branches;
vector<float> g_vertices;
int   g_Seed = rand();
rhandle g_hBranchBrush;
rhandle hLeafBrush;

int16 g_x = 0;
int16 g_y = 0;
int16 g_xd = 0;
int16 g_yd = 0;
bool g_rButton;
bool g_mButton;
bool g_lButton;
int16 g_wheelDelta;

tchar g_valueBuffer[255];

bool doSlider( I2DRenderer* pRenderer, const tchar* text, SRect rect, float* pValue, float valueMin, float valueMax)
{
	_stprintf_s<255>(g_valueBuffer, _T("%f"), (*pValue));
	if (g_rButton && g_x > rect.x && g_x < rect.x+rect.w && g_y > rect.y && g_y < rect.y+rect.h )
	{
		SColour col;
		CreateColourFromRGB(col, 0xCC0000FF);
		pRenderer->DrawRectangle(pRenderer->CreateBrush(col), rect);
		float percentage = (*pValue - valueMin) / (valueMax - valueMin);
		float posOffset = rect.w * percentage;
		float markerWidth = 3.0f;
		SRect markerRect = SRect(g_x - markerWidth / 2.0f, rect.y, markerWidth, rect.h);
		float posInSlider = g_x - rect.x - markerWidth / 2.0f;
		percentage = posInSlider / rect.w;
		(*pValue) = percentage * (valueMax - valueMin) + valueMin;
		CreateColourFromRGB(col, 0xFFFFFFFF);
		pRenderer->DrawRectangle(pRenderer->CreateBrush(col), markerRect);
	}
	else if (g_wheelDelta != 0 && g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h)
	{
		(*pValue) += g_wheelDelta * 0.01f * ((valueMax - valueMin) + valueMin);
		g_wheelDelta = 0; // nasty...
		SColour col;
		CreateColourFromRGB(col, 0xCC0000FF);
		pRenderer->DrawRectangle(pRenderer->CreateBrush(col), rect);
		float percentage = (*pValue - valueMin) / (valueMax - valueMin);
		float posOffset = rect.w * percentage;
		float markerWidth = 3.0f;
		SRect markerRect = SRect(rect.x + posOffset - markerWidth / 2.0f, rect.y, markerWidth, rect.h);
		CreateColourFromRGB(col, 0xFFFFFFFF);
		pRenderer->DrawRectangle(pRenderer->CreateBrush(col), markerRect);
	}
	else
	{
		SColour col;
		CreateColourFromRGB(col, 0xCC0000FF);
		pRenderer->DrawRectangle(pRenderer->CreateBrush(col), rect);
		float percentage = (*pValue - valueMin) / (valueMax - valueMin);
		float posOffset = rect.w * percentage;
		float markerWidth = 3.0f;
		SRect markerRect = SRect(rect.x + posOffset - markerWidth / 2.0f, rect.y, markerWidth, rect.h);
		CreateColourFromRGB(col, 0xFFFFFFFF);
		pRenderer->DrawRectangle(pRenderer->CreateBrush(col), markerRect);
	}

	pRenderer->DrawTextString(text, rect);
	rect.y -= rect.h;
	pRenderer->DrawTextString(g_valueBuffer, rect);

	return false;
}

void OnMouseCallback(int16 x, int16 y, bool rButton, bool mButton, bool lButton, int16 wheelDelta)
{
	g_xd = x - g_x;
	g_yd = y - g_y;
	g_x = x;
	g_y = y;
	g_rButton = rButton;
	g_mButton = mButton;
	g_lButton = lButton;
	g_wheelDelta = wheelDelta;
	Logger.Info(_T("%d, %d"), g_x, g_y);
}

void doTree(CWinWindow& window, I2DRenderer* uiRenderer)
{
	SColour col, leafCol;
	CreateColourFromRGB(col, 0x008800FF);
	//SRect rect(50.0f, 50.0f, 50.0f, 50.0f);
	//rhandle hRect = uiRenderer->CreateRectangleGeometry(rect);
	rhandle hRectBrush = uiRenderer->CreateBrush(col);
	CreateColourFromRGB(col, 0x49311cFF);
	rhandle hRectFillBrush = uiRenderer->CreateBrush(col);
	g_hBranchBrush = hRectFillBrush;
	CreateColourFromRGB(leafCol, 0x00CC0022);
	hLeafBrush = uiRenderer->CreateBrush(leafCol);

	int trunks = 0;

	const int numTrees = 10;
	rhandle* trees = new rhandle[numTrees];
	std::vector<int>* leafIndexes = new std::vector<int>[numTrees];
	std::vector<float2>* treeVertices = new std::vector<float2>[numTrees];
	for (int i = 0; i < numTrees; ++i)
	{
		// TODO: more control
		//		- reach of the branches
		//      - random death/stunting
		//      - overgrowth death/stunting
		//		- noise
		//	    - live editing (C# wrapper?)
		CLSystemTree tree;
		tree.m_maxTrunks = 1;
		tree.m_scale = 6.0f;
		tree.m_maxDepth = 8;
		tree.m_interval = 3.14f / 7.0f;
		tree.m_branchPow = 1.0f;
		tree.m_maxTrunkDev = 3.14f / 8.0f;
		tree.m_seed = 214.0f + (float)i;
		tree.m_maxBranchAngle = 0.0f;
		tree.m_trunkScale = 0.1f;
		tree.m_maxTrunkLen = 10.0f;
		tree.Generate();
		std::vector<float3> treeVerts;
		tree.ConsumeVertexBuffer(treeVerts);
		// uses move to get leaf data
		tree.ConsumeAuxIndexBuffer(_T("leaves"), leafIndexes[i]);
		std::size_t polyVerts = treeVerts.size();
		float2* verts = new float2[polyVerts];
		int32 width = window.Width();
		int32 height = window.Height();
		for (int j = 0; j < polyVerts; ++j)
		{
			verts[j] = float2(treeVerts[j].x + (float)(width / 2), treeVerts[j].y + height);
			treeVertices[i].push_back(verts[j]);
		}
		rhandle hOnePolyTree = uiRenderer->CreateFillGeometry(verts, (uint32)polyVerts);
		delete verts;
		trees[i] = hOnePolyTree;
	}
	rhandle hLeaf = uiRenderer->CreateEllipseGeometry(float2(), float2(2.0f, 2.0f));

	window.RegisterMouseInput(OnMouseCallback);

	float xpostest = -500.0f;
	window.Show();
	while (!window.ShouldQuit())
	{
		window.Update();

		uiRenderer->BeginDraw();

		// TODO: now optimise for overdraw
		for (int i = 0; i < numTrees; ++i)
		{
			float2 treePos = float2(xpostest + ((float)i*100.0f), 0.0f);
			uiRenderer->DrawFillGeometry(trees[i], hRectFillBrush, treePos);

			for (auto leafIter = leafIndexes[i].begin(); leafIter != leafIndexes[i].end(); ++leafIter)
			{
				float2 pos = treeVertices[i][(*leafIter)];
				uiRenderer->DrawFillGeometry(hLeaf, hLeafBrush, float2(pos.x + treePos.x, pos.y + treePos.y));
			}
		}

		doSlider(uiRenderer, _T("x pos test"), SRect(100.0f, 100.0f, 100.0f, 25.0f), &xpostest, -1000.0f, 1000.0f);

		//uiRenderer->DrawFillGeometry(hOnePolyTree, hRectFillBrush);

		//for (auto leafIter = treeLeaves.begin(); leafIter != treeLeaves.end(); ++leafIter)
		//{
		//	float2 pos = verts[(*leafIter)];
		//	uiRenderer->DrawFillGeometry(hLeaf, hLeafBrush, pos);
		//}

		uiRenderer->EndDraw();
	}
}

int lif_main()
{
	CreateScopedLogger();

	uint32 width = 900;
	uint32 height = 600;

	CWinWindow window;
	window.Initialise(width, height, false, _T("Lif"));
	I2DRenderer* uiRenderer = nullptr;
	if (!C2DRendererFactory::Create(&uiRenderer, e2DRenderer_Direct2D))
	{
		Logger.Error(_T("Fatal: Failed to create Direct2D UI renderer"));
		return -1;
	}
	if (!uiRenderer->Initialise(window.GetHandle(), width, height))
	{
		Logger.Error(_T("Fatal: Failed to initialise UI renderer"));
		return -1;
	}

	doTree(window, uiRenderer);

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int result = lif_main();

	return result;
}

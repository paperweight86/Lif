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
bool g_rButtonUp;
bool g_mButton;
bool g_lButton;
int16 g_wheelDelta;

tchar g_valueBuffer[255];

const int numTrees = 1;
rhandle* g_trees = new rhandle[numTrees];
std::vector<int>* g_leafIndexes = new std::vector<int>[numTrees];
std::vector<float2>* g_treeVertices = new std::vector<float2>[numTrees];

float g_branchPow = 1.0f;
float g_scale = 6.0f;
float g_maxBranchAngle = 0.0f;
float g_thickScale = 1.0f;
bool  g_drawLeaves = true;
float g_maxDepth = 8.0f;

rhandle g_hLeaf = nullrhandle;

rhandle g_winrt;
rhandle g_itemrt;

rhandle g_hTextBrush;

bool doSlider( I2DRenderer* pRenderer, const tchar* text, SRect rect, float* pValue, float valueMin, float valueMax)
{
	bool didSlide = false;
	static bool wasRDown = false;
	_stprintf_s<255>(g_valueBuffer, _T("%f"), (*pValue));
	float percentage = 0.0f;
	float markerWidth = 2.0f;
	if (g_rButton && (g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h || wasRDown))
	{
		float posInSlider = g_x - rect.x;
		percentage = std::fmax(std::fmin(posInSlider / rect.w, 1.0f), 0.0f);
		(*pValue) = percentage * (valueMax - valueMin) + valueMin;
		//wasRDown = true;
		didSlide = true;
	}
	else if (g_wheelDelta != 0 && g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h)
	{
		float range = valueMax - valueMin;
		float add = g_wheelDelta * 0.01f * range;
		(*pValue) += add;
		(*pValue) = std::fmax(std::fmin((*pValue), valueMax), valueMin);
		wasRDown = false;
		didSlide = true;
	}
	else
		wasRDown = false;

	SColour col;
	CreateColourFromRGB(col, 0x6073B2FF);
	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), rect);
	percentage = (*pValue - valueMin) / (valueMax - valueMin);
	float posOffset = rect.w * percentage;
	SRect markerRect = SRect(rect.x + posOffset - markerWidth / 2.0f, rect.y, markerWidth, rect.h);
	CreateColourFromRGB(col, 0x000000FF);
	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), markerRect);

	pRenderer->DrawTextString(text, rect, g_hTextBrush);
	rect.y -= rect.h;
	pRenderer->DrawTextString(g_valueBuffer, rect, g_hTextBrush);

	return didSlide;
}

bool doCheckBox(I2DRenderer* pRenderer, const tchar* text, SRect rect, bool* pValue)
{
	if (g_rButtonUp && g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h)
	{
		(*pValue) = !(*pValue);
	}

	SColour col;
	CreateColourFromRGB(col, 0x6073B2FF);
	SRect boxRect = rect;
	boxRect.w = boxRect.w * 0.2f;
	boxRect.x = rect.w / 2.0f + boxRect.w / 4.0f;
	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), boxRect);
	if ((*pValue))
	{ 
		SRect markerRect = SRect(boxRect.x + boxRect.w*0.1f, boxRect.y + boxRect.w*0.1f, 
								 boxRect.w - boxRect.w*0.2f, boxRect.h - boxRect.h*0.2f);
		CreateColourFromRGB(col, 0x000000FF);
		pRenderer->DrawRectangle(pRenderer->CreateBrush(col), markerRect);
	}

	rect.y -= rect.h;
	pRenderer->DrawTextString(text, rect, g_hTextBrush);

	return (*pValue);
}

void OnMouseCallback(int16 x, int16 y, bool rButton, bool mButton, bool lButton, int16 wheelDelta)
{
	g_xd = x - g_x;
	g_yd = y - g_y;
	g_x = x;
	g_y = y;
	if (g_rButton && !rButton)
		g_rButtonUp = true;
	g_rButton = rButton;
	g_mButton = mButton;
	g_lButton = lButton;
	g_wheelDelta = wheelDelta;
	//Logger.Info(_T("%d, %d"), g_x, g_y);
}

void buildTrees(CWinWindow window, I2DRenderer* p2dRenderer)
{
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
		tree.m_scale = g_scale;
		tree.m_maxDepth = (uint8)g_maxDepth;
		tree.m_interval = 3.14f / 7.0f;
		tree.m_branchPow = g_branchPow;
		tree.m_maxTrunkDev = 3.14f / 8.0f;
		tree.m_seed = 214.0f + (float)i;
		tree.m_maxBranchAngle = g_maxBranchAngle;
		tree.m_trunkScale = g_scale/6.0f*0.1f*g_thickScale;
		tree.m_maxTrunkLen = 10.0f*g_scale/6.0f;
		tree.Generate();
		std::vector<float3> treeVerts;
		tree.ConsumeVertexBuffer(treeVerts);
		// uses move to get leaf data
		tree.ConsumeAuxIndexBuffer(_T("leaves"), g_leafIndexes[i]);
		std::size_t polyVerts = treeVerts.size();
		g_treeVertices[i].resize(polyVerts);
		float2* verts = new float2[polyVerts];
		int32 width = window.Width();
		int32 height = window.Height();
		for (int j = 0; j < polyVerts; ++j)
		{
			verts[j] = float2(treeVerts[j].x + (float)(width / 2), treeVerts[j].y + height);
			g_treeVertices[i][j] = verts[j];
		}
		// TODO: update don't recreate?
		if (g_trees[i] != nullrhandle)
			p2dRenderer->DestroyResource(g_trees[i]);
		rhandle hOnePolyTree = p2dRenderer->CreateFillGeometry(verts, (uint32)polyVerts);
		delete verts;
		g_trees[i] = hOnePolyTree;
	}
	// TODO: update scale don't recreate
	p2dRenderer->DestroyResource(g_hLeaf);
	g_hLeaf = p2dRenderer->CreateEllipseGeometry(float2(), float2(2.0f*g_scale/6.0f, 2.0f*g_scale/6.0f));
}


void doTree(CWinWindow& window, I2DRenderer* uiRenderer)
{
	uiRenderer->SetRenderTarget(g_itemrt);

	SColour col, leafCol;
	CreateColourFromRGB(col, 0x008800FF);
	//SRect rect(50.0f, 50.0f, 50.0f, 50.0f);
	//rhandle hRect = uiRenderer->CreateRectangleGeometry(rect);
	rhandle hRectBrush = uiRenderer->CreateBrush(col);
	CreateColourFromRGB(col, 0x49311cFF);
	rhandle hRectFillBrush = uiRenderer->CreateBrush(col);
	g_hBranchBrush = hRectFillBrush;
	CreateColourFromRGB(leafCol, 0x006600AA);
	hLeafBrush = uiRenderer->CreateBrush(leafCol);

	SColour text;
	CreateColourFromRGB(text, 0x000000FF);
	g_hTextBrush = uiRenderer->CreateBrush(text);

	int trunks = 0;
	buildTrees(window, uiRenderer);

	window.RegisterMouseInput(OnMouseCallback);

	float xpostest = 0.0f;
	window.Show();
	while (!window.ShouldQuit())
	{
		window.Update();

		uiRenderer->BeginDraw();

		// TODO: now optimise for overdraw
		for (int i = 0; i < numTrees; ++i)
		{
			float2 treePos = float2(xpostest + ((float)i*100.0f), 0.0f);
			uiRenderer->DrawFillGeometry(g_trees[i], hRectFillBrush, treePos);

			if (g_drawLeaves)
			{
				for (auto leafIter = g_leafIndexes[i].begin(); leafIter != g_leafIndexes[i].end(); ++leafIter)
				{
					float2 pos = g_treeVertices[i][(*leafIter)];
					uiRenderer->DrawFillGeometry(g_hLeaf, hLeafBrush, float2(pos.x + treePos.x, pos.y + treePos.y));
				}
			}
		}

		bool paramChanged = false;
		if (doSlider(uiRenderer, _T("branch pow"), SRect(20.0f, 20.0f, 100.0f, 20.0f), &g_branchPow, 0.1f, 30.0f))
			paramChanged = true;
		if (doSlider(uiRenderer, _T("scale"), SRect(20.0f, 65.0f, 100.0f, 20.0f), &g_scale, 0.1f, 100.0f))
			paramChanged = true;
		if (doSlider(uiRenderer, _T("max angle"), SRect(20.0f, 110.0f, 100.0f, 20.0f), &g_maxBranchAngle, -M_PI*2.0f, M_PI*2.0f))
			paramChanged = true;
		if (doSlider(uiRenderer, _T("thick scale"), SRect(20.0f, 155.0f, 100.0f, 20.0f), &g_thickScale, 0.1f, 10.0f))
			paramChanged = true;
		doCheckBox(uiRenderer, _T("draw leaves"), SRect(20.0f, 200.0f, 100.0f, 20.0f), &g_drawLeaves);
		if (doSlider(uiRenderer, _T("max depth"), SRect(20.0f, 245.0f, 100.0f, 20.0f), &g_maxDepth, 1.0f, 20.0f))
			paramChanged = true;
		if (paramChanged)
		{
			buildTrees(window, uiRenderer);
		}

		//uiRenderer->DrawFillGeometry(hOnePolyTree, hRectFillBrush);

		//for (auto leafIter = treeLeaves.begin(); leafIter != treeLeaves.end(); ++leafIter)
		//{
		//	float2 pos = verts[(*leafIter)];
		//	uiRenderer->DrawFillGeometry(hLeaf, hLeafBrush, pos);
		//}

		uiRenderer->EndDraw();

		uiRenderer->SetRenderTarget(g_winrt);
		rhandle rtImg = uiRenderer->CreateImageFromRenderTarget(g_itemrt);
		uiRenderer->BeginDraw();
		uiRenderer->DrawBitmap(rtImg, float2(), float2(1.0, 1.0));
		uiRenderer->EndDraw();

		uiRenderer->SetRenderTarget(g_itemrt);

		g_rButtonUp = false; // kinda nasty...
		g_wheelDelta = 0;
	}

	// Free trees etc.
	for (int i = 0; i < numTrees; ++i)
	{
		uiRenderer->DestroyResource(g_trees[i]);
		g_trees[i] = nullrhandle;
	}
	uiRenderer->DestroyResource(g_hLeaf);
	uiRenderer->DestroyResource(g_hBranchBrush);
	uiRenderer->DestroyResource(hLeafBrush);
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

	g_winrt = uiRenderer->CreateRenderTarget(/*present:*/true);//RenderTargetType_Window);
	g_itemrt = uiRenderer->CreateRenderTarget(/*present:*/false);//RenderTargetType_Texture);

	/*
	guiRenderer->SetRenderTarget(winrt);
	// create resources...
	guiRenderer->SetRenderTarget(itemrt);
	// create resources...
	// loop
	// generate item
	// render item
	guiRenderer->SetRenderTarget(winrt);
	// render ui
	guiRenderer->SetRenderTarget(itemrt);
	*/

	doTree(window, uiRenderer);

	delete uiRenderer;

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	for (int i = 0; i < numTrees; ++i)
		g_trees[i] = nullrhandle;
	int result = lif_main();

#ifndef NDEBUG
	system("PAUSE");
#endif

	return result;
}


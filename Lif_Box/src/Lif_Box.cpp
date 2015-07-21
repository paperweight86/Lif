#include "stdafx.h"

#include "WinWindow.h"
#include "2DRendererFactory.h"
#include "ScopedLog.h"

using namespace tod;

#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <ctime>

using namespace std;

#define _USE_MATH_DEFINES // for PI!
#include <math.h>

#include <thread>

//#include <png.h>
//#include "zlib.h"

#include "LSystemTree.h"

#include "PerlinNoise.h"

#include "InputKeyboardKeys.h"

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

i16 g_x = 0;
i16 g_y = 0;
i16 g_xd = 0;
i16 g_yd = 0;
bool g_rButton;
bool g_rButtonUp;
bool g_mButton;
bool g_lButton;
i16 g_wheelDelta;

tchar g_valueBuffer[255];

const int numTrees = 1;
rhandle* g_trees = new rhandle[numTrees];
std::vector<int>* g_leafIndexes = new std::vector<int>[numTrees];
std::vector<float2>* g_treeVertices = new std::vector<float2>[numTrees];

r32 g_branchPow = 1.0f;
r32 g_scale = 6.0f;
r32 g_maxBranchAngle = 0.0f;
r32 g_thickScale = 1.0f;
bool  g_drawLeaves = true;
r32 g_maxDepth = 8.0f;

rhandle g_hLeaf = nullrhandle;

rhandle g_winrt = nullrhandle;
rhandle g_itemrt = nullrhandle;

rhandle g_hTextBrush = nullrhandle;
rhandle g_hWhiteTextBrush = nullrhandle;

rhandle g_hTerrain = nullrhandle;
float2* g_pTerrainVerts;
rhandle g_hTerrainBrushFar = nullrhandle;
rhandle g_hTerrainBrushMid = nullrhandle;
rhandle g_hTerrainBrushNear = nullrhandle;

r32 g_nearYOffset;
r32 g_farYOffset;
r32 g_midYOffset;

r32 g_persistance;
r32 g_octaves;

r32 g_nearHeightScale;
r32 g_midHeightScale;
r32 g_farHeightScale;

r32 g_nearHeightOffset;
r32 g_midHeightOffset;
r32 g_farHeightOffset;

rhandle g_nearTerrain = nullrhandle;
rhandle g_midTerrain  = nullrhandle;
rhandle g_farTerrain  = nullrhandle;

r32 g_speedFar;
r32 g_speedMid;
r32 g_speedNear;

// Focusable UI uses this to identify itself
u32 g_nextUiId;
u32 g_focusedUiId;
u32 g_focusedTextPos = uint32_max;
#define CREATE_UI_ID ++g_nextUiId
tchar g_focusValueBuffer[255];

bool g_keysDown[0xFF];
bool g_keysLast[0xFF];

bool doButton(I2DRenderer* pRenderer, const tchar* text, SRect rect)
{
	bool isDown = g_rButtonUp && (g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h);
	SColour col;
	// Border
	CreateColourFromRGB(col, 0x6073B2FF);
	SRect borderRect = rect;
	borderRect.x -= 1.0f;
	borderRect.y -= 1.0f;
	borderRect.w += 2.0f;
	borderRect.h += 2.0f;
	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), borderRect);
	// Background
	rhandle textBrush = g_hTextBrush;
	if (g_rButton && (g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h))
	{
		CreateColourFromRGB(col, 0x000000FF);
		textBrush = g_hWhiteTextBrush;
	}
	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), rect);

	pRenderer->DrawTextString(text, rect, textBrush);

	return isDown;
}

bool doToggleButton(I2DRenderer* pRenderer, const tchar* text, SRect rect, bool* toggle )
{
	bool isDown = g_rButtonUp && (g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h);
	SColour col;
	// Border
	CreateColourFromRGB(col, 0x6073B2FF);
	SRect borderRect = rect;
	borderRect.x -= 1.0f;
	borderRect.y -= 1.0f;
	borderRect.w += 2.0f;
	borderRect.h += 2.0f;
	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), borderRect);
	// Background
	rhandle textBrush = g_hTextBrush;
	if (isDown)
	{
		(*toggle) = !(*toggle);
	}

	if (*toggle)
	{
		CreateColourFromRGB(col, 0x000000FF);
		textBrush = g_hWhiteTextBrush;
	}

	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), rect);

	pRenderer->DrawTextString(text, rect, textBrush);

	return isDown;
}

// TODO: This is pretty broken right now we need to fix delete without keeping copying back from the main buffer
bool doTextBox(I2DRenderer* pRenderer, const tchar* text, SRect rect, float* pValue, float valueMin, float valueMax, const tchar* format = _T("%f"), bool bRound = false)
{
	u32 id = CREATE_UI_ID;

	_stprintf_s<255>(g_valueBuffer, format, (*pValue));

	bool mouseOver = g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h;
	if (g_rButtonUp && mouseOver && g_focusedUiId != id)
	{
		g_focusedUiId = id;
		g_focusedTextPos = uint32_max;
		_stprintf_s<255>(g_focusValueBuffer, _T("%s"), g_valueBuffer);
		auto valueLen = _tcslen(g_focusValueBuffer);
		if (g_focusedTextPos >= valueLen)
			g_focusedTextPos = (u32)max<size_t>(valueLen, 0);
		tstring str = g_focusValueBuffer;
		tstring first = str.substr(0, g_focusedTextPos);
		tstring last = _T("");
		if (valueLen != g_focusedTextPos)
			last = str.substr(g_focusedTextPos, valueLen - g_focusedTextPos);
		_stprintf_s<255>(g_focusValueBuffer, _T("%s|%s"), first.c_str(), last.c_str());
	}
	else if (g_focusedUiId == id && g_rButtonUp && !mouseOver)
	{
		g_focusedUiId = 0;
		tstring str = g_focusValueBuffer;
		str = str.replace(g_focusedTextPos, 1, _T(""));
		_stprintf_s<255>(g_valueBuffer, _T("%s"), str.c_str());
		(*pValue) = max<float>(min<float>(TSTR_TO_FLOAT(g_valueBuffer), valueMax), valueMin);
		g_focusedTextPos = uint32_max;
	}
	else if (g_focusedUiId == id)
	{
		if (!g_keysDown[UTI_KEYBOARD_RIGHT] && g_keysLast[UTI_KEYBOARD_RIGHT])
		{
			if (g_focusedTextPos != _tcslen(g_focusValueBuffer) - 1)
			{
				g_focusedTextPos++;
				auto valueLen = _tcslen(g_focusValueBuffer);
				g_focusedTextPos = (u32)min<size_t>(max<size_t>(g_focusedTextPos, 0), valueLen);
				tstring str = g_focusValueBuffer;
				str = str.replace(g_focusedTextPos - 1, 1, _T(""));
				tstring first = str.substr(0, g_focusedTextPos);
				tstring last = _T("");
				last = str.substr(g_focusedTextPos, valueLen - g_focusedTextPos);
				_stprintf_s<255>(g_focusValueBuffer, _T("%s|%s"), first.c_str(), last.c_str());
			}
		}
		else if (!g_keysDown[UTI_KEYBOARD_LEFT] && g_keysLast[UTI_KEYBOARD_LEFT])
		{
			if (g_focusedTextPos != 0)
			{
				g_focusedTextPos--;
				auto valueLen = _tcslen(g_focusValueBuffer);
				g_focusedTextPos = (u32)min<size_t>(max<size_t>(g_focusedTextPos, 0), valueLen);
				tstring str = g_focusValueBuffer;
				str = str.replace(g_focusedTextPos + 1, 1, _T(""));
				tstring first = str.substr(0, g_focusedTextPos);
				tstring last = _T("");
				last = str.substr(g_focusedTextPos, valueLen - g_focusedTextPos);
				_stprintf_s<255>(g_focusValueBuffer, _T("%s|%s"), first.c_str(), last.c_str());
			}
		}
		else if (!g_keysDown[UTI_KEYBOARD_BACK] && g_keysLast[UTI_KEYBOARD_BACK])
		{
			if (g_focusedTextPos > 0)
			{
				tstring str = g_focusValueBuffer;
				str = str.replace(g_focusedTextPos, 1, _T(""));
				str = str.replace(g_focusedTextPos - 1, 1, _T(""));
				
				auto valueLen = str.length();
				g_focusedTextPos--;
				g_focusedTextPos = (u32)min<size_t>(max<size_t>(g_focusedTextPos, 0), valueLen);
				tstring first = str.substr(0, g_focusedTextPos);
				tstring last = _T("");
				if (valueLen != g_focusedTextPos)
					last = str.substr(g_focusedTextPos, valueLen - g_focusedTextPos);
				_stprintf_s<255>(g_focusValueBuffer, _T("%s|%s"), first.c_str(), last.c_str());
			}
		}
		else
		{
			tstring str = g_focusValueBuffer;
			bool num = false;
			char keyCode = 0;
			if (str.substr(0, 2) != _T("|-") || g_focusedTextPos != 0)
			{
				for (keyCode = '0'; keyCode <= '9'; ++keyCode)
				{
					if (!g_keysDown[keyCode] && g_keysLast[keyCode])
					{
						num = true;
						break;
					}
				}
			}
			if (num)
			{
				tstring str = g_focusValueBuffer;
				str = str.replace(g_focusedTextPos, 1, _T(""));

				auto valueLen = str.length() + 1;
				g_focusedTextPos++;
				g_focusedTextPos = (u32)min<size_t>(max<size_t>(g_focusedTextPos, 0), valueLen);
				tstring first = str.substr(0, g_focusedTextPos-1);
				first.push_back((wchar_t)keyCode);
				tstring last;
				if (valueLen != g_focusedTextPos-1)
					last = str.substr(g_focusedTextPos-1, valueLen - g_focusedTextPos);
				_stprintf_s<255>(g_focusValueBuffer, _T("%s|%s"), first.c_str(), last.c_str());
			}
			else if (!g_keysDown[UTI_KEYBOARD_OEM_PERIOD] && g_keysLast[UTI_KEYBOARD_OEM_PERIOD])
			{
				if (str.find(_T(".")) == -1)
				{
					str = str.replace(g_focusedTextPos, 1, _T(""));

					auto valueLen = str.length() + 1;
					g_focusedTextPos++;
					g_focusedTextPos = (u32)min<size_t>(max<size_t>(g_focusedTextPos, 0), valueLen);
					tstring first = str.substr(0, g_focusedTextPos - 1);
					first.append(_T("."));
					tstring last;
					if (valueLen != g_focusedTextPos - 1)
						last = str.substr(g_focusedTextPos - 1, valueLen - g_focusedTextPos);
					_stprintf_s<255>(g_focusValueBuffer, _T("%s|%s"), first.c_str(), last.c_str());
				}
			}
			else if (!g_keysDown[UTI_KEYBOARD_OEM_MINUS] && g_keysLast[UTI_KEYBOARD_OEM_MINUS])
			{
				if (str.substr(0,2) != _T("|-") && g_focusedTextPos == 0)
				{
					str = str.replace(g_focusedTextPos, 1, _T(""));

					auto valueLen = str.length() + 1;
					g_focusedTextPos++;
					g_focusedTextPos = (u32)min<size_t>(max<size_t>(g_focusedTextPos, 0), valueLen);
					tstring first = str.substr(0, g_focusedTextPos - 1);
					first.insert(0,_T("-"));
					tstring last;
					if (valueLen != g_focusedTextPos - 1)
						last = str.substr(g_focusedTextPos - 1, valueLen - g_focusedTextPos);
					_stprintf_s<255>(g_focusValueBuffer, _T("%s|%s"), first.c_str(), last.c_str());
				}
			}
			else if (!g_keysDown[UTI_KEYBOARD_HOME] && g_keysLast[UTI_KEYBOARD_HOME])
			{
			}
			else if (!g_keysDown[UTI_KEYBOARD_END] && g_keysLast[UTI_KEYBOARD_END])
			{
			}
			else if (!g_keysDown[UTI_KEYBOARD_RETURN] && g_keysLast[UTI_KEYBOARD_RETURN])
			{
				g_focusedUiId = 0;
				tstring str = g_focusValueBuffer;
				str = str.replace(g_focusedTextPos, 1, _T(""));
				_stprintf_s<255>(g_valueBuffer, _T("%s"), str.c_str());
				(*pValue) = max<float>(min<float>(TSTR_TO_FLOAT(g_valueBuffer), valueMax), valueMin);
				g_focusedTextPos = uint32_max;
			}
			else if (!g_keysDown[UTI_KEYBOARD_ESCAPE] && g_keysLast[UTI_KEYBOARD_ESCAPE])
			{
				g_focusedUiId = 0;
				g_focusedTextPos = uint32_max;
			}
		}
	}

	SColour col;
	if(g_focusedUiId != id)
		CreateColourFromRGB(col, 0x6073B2FF);
	else
		CreateColourFromRGB(col, 0xFFFFFFFF);
	pRenderer->DrawRectangle(pRenderer->CreateBrush(col), rect);

	if (g_focusedUiId == id)
	{
		pRenderer->DrawTextString(g_focusValueBuffer, rect, g_hTextBrush);
	}
	else
		pRenderer->DrawTextString(g_valueBuffer, rect, g_hTextBrush);

	return false;
}

bool doSlider( I2DRenderer* pRenderer, const tchar* text, SRect rect, float* pValue, float valueMin, float valueMax, bool bRound = false)
{
	bool didSlide = false;
	if (bRound)
		_stprintf_s<255>(g_valueBuffer, _T("%d"), (int)(*pValue));
	else
		_stprintf_s<255>(g_valueBuffer, _T("%f"), (*pValue));
	float percentage = 0.0f;
	float markerWidth = 2.0f;
	if (g_rButton && (g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h))
	{
		float posInSlider = g_x - rect.x;
		percentage = std::fmax(std::fmin(posInSlider / rect.w, 1.0f), 0.0f);
		(*pValue) = percentage * (valueMax - valueMin) + valueMin;
		didSlide = true;
	}
	else if (g_wheelDelta != 0 && g_x > rect.x && g_x < rect.x + rect.w && g_y > rect.y && g_y < rect.y + rect.h)
	{
		float range = valueMax - valueMin;
		float add = g_wheelDelta * 0.01f * range;
		(*pValue) += add;
		(*pValue) = std::fmax(std::fmin((*pValue), valueMax), valueMin);
		didSlide = true;
	}

	if (bRound && didSlide)
		(*pValue) = (float)((int)((*pValue) + 0.5f));

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
	//pRenderer->DrawTextString(g_valueBuffer, rect, g_hTextBrush);
	doTextBox(pRenderer, g_valueBuffer, rect, pValue, valueMin, valueMax, _T("%f"), bRound);

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

void OnKeyboardCallback(u32 keyCode, bool isDown, bool isSyskey)
{
	g_keysDown[keyCode] = isDown;
}

void buildTerrain(rhandle& hTerrain, int y, float xOffset, float yOffset, int width, int height, int xResolution, I2DRenderer* pRenderer)
{
	float xPos = 0.0f;
	int x = 0;
	g_pTerrainVerts[x] = float2(xPos, 1000.0f);
	x++;
	for (; x < xResolution+1; ++x)
	{
		xPos = (float)x*(width / xResolution);
		float x_ = (float)((((float)(int)xPos + 1) - (((int)xPos + 1) / width * width)) / ((float)width)) * 15.0f;
		float y_ = (float)((((float)y + 1) / width) / ((float)height)) * 15.0f;
		float yPos = (float)height - yOffset - (Perlin::Get2DNoise(x_ + xOffset, (float)y, g_persistance, g_octaves)) * (float)height;
		//float yPos = (float)height - (Perlin::Get2DNoise(xPos, 100.0f, 0.5f, 8)) * (float)height;

		g_pTerrainVerts[x] = float2(xPos, yPos);
	}
	g_pTerrainVerts[x] = float2(xPos, 1000.0f);
	if (hTerrain != nullrhandle)
		pRenderer->UpdateGeometry(hTerrain, g_pTerrainVerts, (uint32)xResolution + 2);
	else
		hTerrain = pRenderer->CreateFillGeometry(g_pTerrainVerts, (uint32)xResolution + 2);
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
	g_pTerrainVerts = new float2[window.Width() + 2];
	//buildTerrain(0, 0.0f, 100.0f, window.Width(), window.Height()/2, uiRenderer);

	SColour terrainBase;

	CreateColourFromRGB(terrainBase, 0x162E1CFF);
	g_hTerrainBrushNear = uiRenderer->CreateBrush(terrainBase);

	CreateColourFromRGB(terrainBase, 0x3C7D4DFF);
	g_hTerrainBrushMid = uiRenderer->CreateBrush(terrainBase);

	CreateColourFromRGB(terrainBase, 0x56B26EFF);
	g_hTerrainBrushFar = uiRenderer->CreateBrush(terrainBase);

	uiRenderer->SetRenderTarget(g_winrt);
	SColour text, whiteText;
	CreateColourFromRGB(text, 0x000000FF);
	g_hTextBrush = uiRenderer->CreateBrush(text);
	CreateColourFromRGB(whiteText, 0xFFFFFFFF);
	g_hWhiteTextBrush = uiRenderer->CreateBrush(whiteText);

	window.RegisterMouseInput(OnMouseCallback);
	window.RegisterKeyboardInput(OnKeyboardCallback);

	SColour white;
	CreateColourFromRGB(white, 0xFFFFFFFF);

	SColour clearColour;
	CreateColourFromRGB(clearColour, 0x00000000);// dark sky blue 0x175691FF
	
	uiRenderer->SetRenderTarget(g_itemrt);

	int trunks = 0;
	buildTrees(window, uiRenderer);

	float xpostest = 0.0f;
	float ypostest = 0.0f;
	window.Show();
	float xOffsetFar = 0;
	float xOffsetMid= 0;
	float xOffsetNear = 0;

	bool bDoTree = false;
	bool bDoTerrain = false;

	g_nearYOffset = 1000.0f;
	g_midYOffset = 0.0f;
	g_farYOffset = -1000.0f;

	g_persistance = 0.5f;
	g_octaves = 4;

	g_farHeightScale = 0.37f;
	g_midHeightScale = 0.29f;
	g_nearHeightScale = 0.33f;

	g_nearHeightOffset = -240.0f;
	g_midHeightOffset = -140.0f;
	g_farHeightOffset = -120.0f;

	g_speedFar  = 0.001f;
	g_speedMid  = 0.002f;
	g_speedNear = 0.003f;

	chrono::system_clock::time_point start = std::chrono::high_resolution_clock::now();

	while (!window.ShouldQuit())
	{
		chrono::system_clock::time_point end = std::chrono::high_resolution_clock::now();

		auto dt = end - start;
		
		std::chrono::milliseconds dtms = chrono::duration_cast<std::chrono::milliseconds>(dt);

		xOffsetFar  += g_speedFar  * ((float)dtms.count()/1000.0f);
		xOffsetMid  += g_speedMid  * ((float)dtms.count()/1000.0f);
		xOffsetNear += g_speedNear * ((float)dtms.count()/1000.0f);

		//ypostest += 0.1f;
		window.Update();

		uiRenderer->SetClearColor(clearColour);
		uiRenderer->BeginDraw();

		if (bDoTerrain)
		{
			buildTerrain(g_farTerrain, g_farYOffset + (int)ypostest, xOffsetFar, g_farHeightOffset, window.Width(), window.Height() * g_farHeightScale, window.Width() / 2, uiRenderer);
			uiRenderer->DrawFillGeometry(g_farTerrain, g_hTerrainBrushFar);
			buildTerrain(g_midTerrain, g_midYOffset + (int)ypostest, xOffsetMid, g_midHeightOffset, window.Width(), window.Height() * g_midHeightScale, window.Width() / 2, uiRenderer);
			uiRenderer->DrawFillGeometry(g_midTerrain, g_hTerrainBrushMid);
			buildTerrain(g_nearTerrain, g_nearYOffset + (int)ypostest, xOffsetNear, g_nearHeightOffset, window.Width(), window.Height() * g_nearHeightScale, window.Width() / 2, uiRenderer);
			uiRenderer->DrawFillGeometry(g_nearTerrain, g_hTerrainBrushNear);
		}

		if (bDoTree)
		{
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
		}

		//for (int i = 0; i < 20; ++i)
		//	uiRenderer->DrawFillGeometry(g_hLeaf, hLeafBrush, g_pTerrainVerts[i]);

		//uiRenderer->DrawFillGeometry(hOnePolyTree, hRectFillBrush);

		//for (auto leafIter = treeLeaves.begin(); leafIter != treeLeaves.end(); ++leafIter)
		//{
		//	float2 pos = verts[(*leafIter)];
		//	uiRenderer->DrawFillGeometry(hLeaf, hLeafBrush, pos);
		//}

		uiRenderer->EndDraw();

		uiRenderer->SetRenderTarget(g_winrt);
		rhandle rtImg = uiRenderer->CreateImageFromRenderTarget(g_itemrt);
		uiRenderer->SetClearColor(white);
		uiRenderer->BeginDraw();
		uiRenderer->DrawBitmap(rtImg, float2(), float2(1.0, 1.0));
		uiRenderer->DestroyResource(rtImg);

		if (doToggleButton(uiRenderer, _T("Tree"), SRect(0.0f, 0.0f, 100.0f, 20.0f), &bDoTree))
			if (bDoTerrain)
				bDoTerrain = false;
		if (doToggleButton(uiRenderer, _T("Terrain"), SRect(103.0f, 0.0f, 100.0f, 20.0f), &bDoTerrain))
			if (bDoTree)
				bDoTree = false;

		bool paramChanged = false;
		if (bDoTree)
		{
			if (doSlider(uiRenderer, _T("branch pow"), SRect(20.0f, 40.0f, 100.0f, 20.0f), &g_branchPow, 0.1f, 30.0f))
				paramChanged = true;
			if (doSlider(uiRenderer, _T("scale"), SRect(20.0f, 85.0f, 100.0f, 20.0f), &g_scale, 0.1f, 100.0f))
				paramChanged = true;
			if (doSlider(uiRenderer, _T("max angle"), SRect(20.0f, 130.0f, 100.0f, 20.0f), &g_maxBranchAngle, -M_PI*2.0f, M_PI*2.0f))
				paramChanged = true;
			if (doSlider(uiRenderer, _T("thick scale"), SRect(20.0f, 175.0f, 100.0f, 20.0f), &g_thickScale, 0.1f, 10.0f))
				paramChanged = true;
			doCheckBox(uiRenderer, _T("draw leaves"), SRect(20.0f, 220.0f, 100.0f, 20.0f), &g_drawLeaves);
			if (doSlider(uiRenderer, _T("max depth"), SRect(20.0f, 265.0f, 100.0f, 20.0f), &g_maxDepth, 1.0f, 20.0f))
				paramChanged = true;
			if (paramChanged)
			{
				buildTrees(window, uiRenderer);
			}
			bool savePng = doButton(uiRenderer, _T("save"), SRect(20.0f, 310.0f, 100, 20.0f));
			if (savePng)
			{
				uiRenderer->SavePngImage(g_itemrt);
				Logger.Info(_T("Saved image to C:\\Output.png"));
			}
		}
		else if (bDoTerrain)
		{
			if (doSlider(uiRenderer, _T("Near Seed"), SRect(20.0f, 40.0f, 100.0f, 20.0f), &g_nearYOffset, -1000.0f, 1000.0f, true))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Mid Seed"), SRect(20.0f, 90.0f, 100.0f, 20.0f), &g_midYOffset, -1000.0f, 1000.0f, true))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Far Seed"), SRect(20.0f, 135.0f, 100.0f, 20.0f), &g_farYOffset, -1000.0f, 1000.0f, true))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Persistance"), SRect(20.0f, 180.0f, 100.0f, 20.0f), &g_persistance, 0.0f, 1.0f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Octaves"), SRect(20.0f, 225.0f, 100.0f, 20.0f), &g_octaves, 1.0f, 20.0f, true))
				paramChanged = true;
			
			if (doSlider(uiRenderer, _T("Far Scale"), SRect(20.0f, 270.0f, 100.0f, 20.0f), &g_farHeightScale , 0.0f, 1.0f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Mid Scale"), SRect(20.0f, 315.0f, 100.0f, 20.0f), &g_midHeightScale, 0.0f, 1.0f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Near Scale"), SRect(20.0f, 360.0f, 100.0f, 20.0f), &g_nearHeightScale, 0.0f, 1.0f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Far Offset"), SRect(20.0f, 405.0f, 100.0f, 20.0f), &g_farHeightOffset, -1000.0f, 1000.0f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Mid Offset"), SRect(20.0f, 450.0f, 100.0f, 20.0f), &g_midHeightOffset, -1000.0f, 1000.0f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Near Offset"), SRect(20.0f, 495.0f, 100.0f, 20.0f), &g_nearHeightOffset, -1000.0f, 1000.0f))
				paramChanged = true;

			// Right Side
			if (doSlider(uiRenderer, _T("Far Speed"), SRect(window.Width()-220.0f, 40.0f, 200.0f, 20.0f), &g_speedFar, -0.1f, 0.1f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Mid Speed"), SRect(window.Width()-220.0f, 90.0f, 200.0f, 20.0f), &g_speedMid, -0.1f, 0.1f))
				paramChanged = true;

			if (doSlider(uiRenderer, _T("Near Speed"), SRect(window.Width()-220.0f, 135.0f, 200.0f, 20.0f), &g_speedNear, -0.1f, 0.1f))
				paramChanged = true;

			bool savePng = doButton(uiRenderer, _T("save"), SRect(20.0f, 540.0f, 100, 20.0f));
			if (savePng)
			{
				uiRenderer->SavePngImage(g_itemrt);
				Logger.Info(_T("Saved image to C:\\Output.png"));
			}
		}
		uiRenderer->EndDraw();

		uiRenderer->SetRenderTarget(g_itemrt);

		g_rButtonUp = false; // kinda nasty...
		g_wheelDelta = 0;

		g_nextUiId = 0;
		memcpy(g_keysLast, g_keysDown, sizeof(g_keysDown)*sizeof(bool));
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

	memset(g_keysDown, 0, sizeof(g_keysDown)*sizeof(bool));
	memset(g_keysLast, 0, sizeof(g_keysLast)*sizeof(bool));
	memset(g_valueBuffer, 0, 255 * sizeof(char));
	memset(g_focusValueBuffer, 0, 255 * sizeof(char));

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

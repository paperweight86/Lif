// Lif.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "WinWindow.h"
#include "2DRendererFactory.h"

using namespace tod;

#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

#define _USE_MATH_DEFINES // for PI!
#include <math.h>

#include <thread>

#include "Header.h"
#include "Mesh.h"
#include "CompressedData.h"

//#include <png.h>
#include "zlib.h"

#include "LSystemTree.h"

#include "HCAErosion.h"

#include "PerlinNoise.h"

using namespace lif;

int lif_main();
void generateTree( I2DRenderer* pRenderer, const float2& start, int& trunks );
void doTree(CWinWindow& window, I2DRenderer* uiRenderer);

struct branchRect
{
public:
	float2 pos[4];

	branchRect( )
	{
	}

	branchRect( const float2* _pos )
	{
		pos[0] = _pos[0];
		pos[1] = _pos[1];
		pos[2] = _pos[2];
		pos[3] = _pos[3];
	}
};

//struct SBranch
//{
//	float2 start;
//	float2 end;
//	float startThickness;
//	float endThickness;
//	branchRect rect;
//};

vector<rhandle> g_tree;
vector<float2> g_singlePolyTree;
vector<branchRect> g_branches;
vector<float> g_vertices;
int   g_Seed = rand();

int _tmain(int argc, _TCHAR* argv[])
{
	//TODO: make return value a static in the logger?!
	int32 iReturn = lif_main( );

	system("PAUSE");

	return iReturn;
}

rhandle g_hBranchBrush;

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

	delete uiRenderer;

	return 0;
}

void DoErosion(CWinWindow& window, I2DRenderer* uiRenderer)
{
	CHCAErosion ca(256, 256);
	ca.CreateCells();
	vector<float> heights;
	//Perlin::Get2DNoiseArray(&heights[0], 256*256, 256, 256, 0.25f, 1);

	float heightmax = -float_max;
	float heightmin =  float_max;

	//retrieve random x and y offsets for the noise
	float Randx = (float)rand();
	float Randy = (float)rand();

	uint16 t_width, t_height;
	t_width  = 256;
	t_height = 256;

	//generate the 2D perlin noise
	for (unsigned long i = 0; i < t_width * t_height; i++)
	{
		float iF = (float)i;
		float x_ = (float)(((iF + 1) - ((i + 1) / t_width * t_width)) / ((float)t_width) + Randx) * 24.0f;
		float y_ = (float)(((iF + 1) / t_width) / ((float)t_height) + Randy) * 24.0f;
		heights.push_back(((Perlin::Get2DNoise(x_ + Randx, y_, 0.5f, 8))));

		//check for min and max values
		if (heights[heights.size() - 1] > heightmax)
			heightmax = heights[heights.size() - 1];

		if (heights[heights.size() - 1] < heightmin)
			heightmin = heights[heights.size() - 1];
	}
	//sample the heights upto the display boundries
	for (unsigned long i = 0; i < t_width * t_height; i++)
	{
		heights[i] = (heights[i] - heightmin) / (heightmax - heightmin);
	}
	window.Show();
	rhandle hImg = nullrhandle;
	ca.InitialiseCells(heights);
	for (int i = 0; i < 1000; ++i)
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(250));
		ca.Step(0.0f);
	}

	while (!window.ShouldQuit())
	{
		window.Update();

		std::vector<uint8> vImg(256 * 256 * 4);
		auto pixelIter = vImg.begin();
		auto cells = ca.GetCells();
		float wMax = float_min;
		float wMin = float_max;
		std::for_each(cells.begin(), cells.end(), [&wMax, &wMin](CHexCACell* cell)
		{
			CHCAErosionCell* erCell = static_cast<CHCAErosionCell*>(cell);
			wMax = erCell->m_water > wMax ? erCell->m_water : wMax;
			wMin = erCell->m_water < wMin ? erCell->m_water : wMin;
		});
		for (auto cellIter = cells.begin(); cellIter != cells.end(); ++cellIter)
		{
			CHCAErosionCell* erCell = static_cast<CHCAErosionCell*>(*cellIter);
			(*pixelIter) = 0; // r
			++pixelIter;
			float water = std::min(std::max(erCell->m_water - wMin, 0.0f) / wMax - wMin, 1.0f);
			// TODO: normalise terrain height
			//(*pixelIter) = std::min<uint8>((uint8)(erCell->m_terrain*255.0f), 255); // g
			++pixelIter;
			//if (erCell->m_water >= 0.01f)
			//(*pixelIter) = std::min<uint8>((uint8)(erCell->m_water*100.0f*255.0f), 255);; // b
			(*pixelIter) = water*255.0f;
			++pixelIter;
			(*pixelIter) = 255; // a
			++pixelIter;
		}
		std::vector<uint8> vImg2(256 * 256 * 4);
		pixelIter = vImg2.begin();
		for (auto cellIter = cells.begin(); cellIter != cells.end(); ++cellIter)
		{
			CHCAErosionCell* erCell = static_cast<CHCAErosionCell*>(*cellIter);
			(*pixelIter) = 0; // r
			++pixelIter;
			//float water = std::min(std::max(erCell->m_water - wMin, 0.0f) / wMax - wMin, 1.0f);
			// TODO: normalise terrain height
			(*pixelIter) = std::min<uint8>((uint8)(erCell->m_terrain*255.0f), 255); // g
			++pixelIter;
			//if (erCell->m_water >= 0.01f)
			//(*pixelIter) = std::min<uint8>((uint8)(erCell->m_water*100.0f*255.0f), 255);; // b
			//(*pixelIter) = water*255.0f;
			++pixelIter;
			(*pixelIter) = 255; // a
			++pixelIter;
		}
		std::vector<uint8> vImg3(256 * 256 * 4);
		pixelIter = vImg3.begin();
		for (auto cellIter = cells.begin(); cellIter != cells.end(); ++cellIter)
		{
			CHCAErosionCell* erCell = static_cast<CHCAErosionCell*>(*cellIter);
			(*pixelIter) = std::min<uint8>((uint8)(erCell->m_sediment*1000.0f*255.0f), 255); // r
			++pixelIter;
			//float water = std::min(std::max(erCell->m_water - wMin, 0.0f) / wMax - wMin, 1.0f);
			// TODO: normalise terrain height
			(*pixelIter) = std::min<uint8>((uint8)(erCell->m_sediment*1000.0f*255.0f), 255); // g
			++pixelIter;
			//if (erCell->m_water >= 0.01f)
			//(*pixelIter) = std::min<uint8>((uint8)(erCell->m_water*100.0f*255.0f), 255);; // b
			//(*pixelIter) = water*255.0f;
			++pixelIter;
			(*pixelIter) = 255; // a
			++pixelIter;
		}
		std::vector<uint8> vImgDiff(256 * 256 * 4);
		auto heightIter = heights.cbegin();
		auto cellIter2 = cells.begin();
		pixelIter = vImgDiff.begin();
		for (; heightIter != heights.cend(); ++heightIter)
		{
			CHCAErosionCell* erCell = static_cast<CHCAErosionCell*>(*cellIter2);

			float heightDiff = (*heightIter) - erCell->m_terrain;

			uint8 r, g, b, a;
			r = g = b = 0;
			a = 255;

			if (heightDiff > float_epsilon)
			{
				b = std::min<uint8>((uint8)(heightDiff*255.0f), 255);
			}
			else if (heightDiff < -float_epsilon)
			{
				r = std::min<uint8>((uint8)(fabsf(heightDiff)*255.0f), 255);
			}
			else
			{
				//a = 0;
			}

			g = std::min<uint8>((uint8)(erCell->m_terrain*255.0f*0.1f), 255);

			(*pixelIter) = r; // r
			++pixelIter;
			//float water = std::min(std::max(erCell->m_water - wMin, 0.0f) / wMax - wMin, 1.0f);
			// TODO: normalise terrain height
			(*pixelIter) = g; // g
			++pixelIter;
			//if (erCell->m_water >= 0.01f)
			(*pixelIter) = b; // b
			//(*pixelIter) = water*255.0f;
			++pixelIter;
			(*pixelIter) = a; // a
			++pixelIter;

			++cellIter2;
		}
		uiRenderer->BeginDraw();
		// TODO: stop creating bitmaps and just update them
		hImg = uiRenderer->CreateImage(256, 256, &vImg[0]);
		uiRenderer->DrawBitmap(hImg, float2(0.0f, 0.0f), float2(1.0f, 1.0f));
		hImg = uiRenderer->CreateImage(256, 256, &vImg2[0]);
		uiRenderer->DrawBitmap(hImg, float2(256.0f, 0.0f), float2(1.0f, 1.0f));
		hImg = uiRenderer->CreateImage(256, 256, &vImg3[0]);
		uiRenderer->DrawBitmap(hImg, float2(256.0f, 256.0f), float2(1.0f, 1.0f));
		hImg = uiRenderer->CreateImage(256, 256, &vImgDiff[0]);
		uiRenderer->DrawBitmap(hImg, float2(0.0f, 256.0f), float2(1.0f, 1.0f));
		uiRenderer->EndDraw();
	}
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
	CreateColourFromRGB(leafCol, 0x00FF0022);
	rhandle hLeafBrush = uiRenderer->CreateBrush(leafCol);

	int trunks = 0;

	// TODO: more control
	//		- reach of the branches
	//      - random death/stunting
	//      - overgrowth death/stunting
	//		- noise
	//	    - live editing
	CLSystemTree tree;
	tree.m_maxTrunks = 1;
	//tree.m_scale /= 1.5f;
	tree.m_maxDepth = 10;
	tree.m_interval = 3.14f / 10.0f;
	tree.m_maxTrunkDev = 3.14f / 8.0f;
	tree.m_seed = 214;
	tree.m_maxBranchAngle = -3.14f;
	tree.Generate();
	auto treeVerts = tree.GetVerts();
	auto treeLeaves = tree.GetLeaves();
	std::size_t polyVerts = treeVerts.size();
	float2* verts = new float2[polyVerts];
	int32 width = window.Width();
	int32 height = window.Height();
	for (int i = 0; i < polyVerts; ++i)
	{
		verts[i] = float2(treeVerts[i].x + (float)(width / 2), treeVerts[i].y + height);
	}
	rhandle hOnePolyTree = uiRenderer->CreateFillGeometry(verts, polyVerts);
	rhandle hLeaf = uiRenderer->CreateEllipseGeometry(float2(), float2(10.0f, 10.0f));

	window.Show();
	while (!window.ShouldQuit())
	{
		window.Update();

		uiRenderer->BeginDraw();

		// TODO: now optimise for overdraw
		uiRenderer->DrawFillGeometry(hOnePolyTree, hRectFillBrush);

		//for (auto leafIter = treeLeaves.begin(); leafIter != treeLeaves.end(); ++leafIter)
		//{
		//	float2 pos = verts[(*leafIter)];
		//	uiRenderer->DrawFillGeometry(hLeaf, hLeafBrush, pos);
		//}

		uiRenderer->EndDraw();
	}
}

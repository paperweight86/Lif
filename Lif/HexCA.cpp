#include "stdafx.h"

#include "HexCA.h"
#include "HexCACell.h"

using namespace lif;

/*
	The first column is "offset" up then the second down and so on 
	   __	 __	   __  
	  /0 \__/4 \__/8 \  
	  \__/2 \__/6 \__/	A 4 x 2 hex grid stored 0,1,2,3,4,5,6,7,8,9 in an array (column major)
      /1 \__/5 \__/9 \  
	  \__/3 \__/7 \__/  
	     \__/  \__/    
		              
	Indices for the adjacency go clockwise from top to upper left
	
	  _0		0 - up	
	5/  \1		1 - right up
	4\__/2		2 - right down
	   3		3 - down
				4 - left down
				5 - left up
*/
#define IS_OFFSET_UP(i)		 (i != 0 && i % 2 != 0)

#define IS_FIRST_COLUMN(i)   (i == 0 || i < m_height)
#define IS_LAST_COLUMN(i)    (i >= m_cells.size() - m_height)
#define IS_ENDOF_COLUMN(i)   ((i + 1) % m_height == 0)
#define IS_STARTOF_COLUMN(i) (i == 0 || i % m_height == 0)

#define HAS_UP(i)						  !IS_STARTOF_COLUMN(i)							
#define HAS_RIGHTUP(i)   IS_OFFSET_UP(i)? !IS_LAST_COLUMN(i)    && !IS_STARTOF_COLUMN(i): !IS_LAST_COLUMN(i)
#define HAS_RIGHTDOWN(i) IS_OFFSET_UP(i)? !IS_LAST_COLUMN(i)							: !IS_LAST_COLUMN(i)  && !IS_ENDOF_COLUMN(i)
#define HAS_DOWN(i)						  !IS_ENDOF_COLUMN(i)							
#define HAS_LEFTDOWN(i)	 IS_OFFSET_UP(i)? !IS_FIRST_COLUMN(i)							: !IS_ENDOF_COLUMN(i)   && !IS_FIRST_COLUMN(i)
#define HAS_LEFTUP(i)	 IS_OFFSET_UP(i)? !IS_STARTOF_COLUMN(i) && !IS_FIRST_COLUMN(i)	: !IS_FIRST_COLUMN(i)

#define GET_UP(i)						  m_cells[i-1		  ]						   ;
#define GET_RIGHTUP(i)   IS_OFFSET_UP(i)? m_cells[i+m_height-1]	: m_cells[i+m_height  ];
#define GET_RIGHTDOWN(i) IS_OFFSET_UP(i)? m_cells[i+m_height  ]	: m_cells[i+m_height+1];
#define GET_DOWN(i)						  m_cells[i+1		  ]						   ;
#define GET_LEFTDOWN(i)  IS_OFFSET_UP(i)? m_cells[i-m_height  ]	: m_cells[i-m_height+1];
#define GET_LEFTUP(i)	 IS_OFFSET_UP(i)? m_cells[i-m_height-1]	: m_cells[i-m_height  ];

CHexCA::CHexCA( uint64 width, uint64 height ) : m_width(width), m_height(height)
{
	m_cells.resize(width * height, NULL);
	std::array<CHexCACell*,6> defAdj = { 0, 0, 0, 0, 0, 0 };
	m_adjacency.resize(width * height, defAdj);
}

void CHexCA::CreateCells( )
{
	for( size_t i = 0; i < m_width*m_height; ++i )
	{
		assert( m_cells[i] == NULL );
		m_cells[i] = CreateCell();
		std::array<CHexCACell*,6>& adj = m_adjacency[i];
		adj[eAdjIndex_Up]  = NULL;
		adj[eAdjIndex_RU]  = NULL;
		adj[eAdjIndex_RD]  = NULL;
		adj[eAdjIndex_Dn]  = NULL;
		adj[eAdjIndex_LD]  = NULL;
		adj[eAdjIndex_LU]  = NULL;
	}
	for( size_t i = 0; i < m_width*m_height; ++i )
	{
		std::array<CHexCACell*,eAdjIndex_COUNT>& adj = m_adjacency[i];
		// Up
		if( HAS_UP(i) )
		{
			adj[eAdjIndex_Up] = GET_UP(i);
		}
		// Right Up
		if( HAS_RIGHTUP(i) )
		{
			adj[eAdjIndex_RU] = GET_RIGHTUP(i);
		}
		// Right Down
		if( HAS_RIGHTDOWN(i) )
		{
			adj[eAdjIndex_RD] = GET_RIGHTDOWN(i);
		}
		// Down
		if( HAS_DOWN(i) )
		{
			adj[eAdjIndex_Dn] = GET_DOWN(i);
		}
		// Left Down
		if( HAS_LEFTDOWN(i) )
		{
			adj[eAdjIndex_LD] = GET_LEFTDOWN(i);
		}
		// Left Up
		if( HAS_LEFTUP(i) )
		{
			adj[eAdjIndex_LU] = GET_LEFTUP(i);
		}
	}
}

CHexCA::~CHexCA( )
{
}

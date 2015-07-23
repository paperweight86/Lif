#pragma once

#include <vector>
#include <array>
#include <tuple>

namespace lif
{
	enum EHexCellAdjIndex : uint8
	{
		eAdjIndex_Up,
		eAdjIndex_RU,
		eAdjIndex_RD,
		eAdjIndex_Dn,
		eAdjIndex_LD,
		eAdjIndex_LU,
		eAdjIndex_COUNT
	};
	class CHexCACell;
	class CHexCA
	{
	protected:
		uint64 m_width, 
			   m_height;
		// Cells are stored column major
		std::vector<CHexCACell*>			   m_cells;
		std::vector<std::array<CHexCACell*,6>> m_adjacency;
	private:
		virtual CHexCACell* CreateCell( ) = 0;
	public:
		CHexCA( uint64 width, uint64 height );
		virtual ~CHexCA( );
		virtual void CreateCells();
		virtual bool InitialiseCells( const std::vector<float>& data ) = 0;
		virtual bool InitialiseCells( const std::vector<CHexCACell*>& data ) = 0;
		virtual void Step( float dt ) = 0;
		const std::vector<CHexCACell*>& GetCells() { return m_cells; }
	};
}

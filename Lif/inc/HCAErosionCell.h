#pragma once
#include "HexCACell.h"

namespace lif
{
	class CHCAErosionCell : public CHexCACell
	{
	public:
		// The height of the terrain in this cell
		float m_terrain;
		// The amount of disolved sediment in this cell
		float m_sediment;
		// The amount of water in this cell
		float m_water;
	public:
		CHCAErosionCell( );
		virtual ~CHCAErosionCell( );
	};
}

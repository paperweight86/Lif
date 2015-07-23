#pragma once
#include "HexCA.h"
#include "HCAErosionCell.h"

namespace lif
{
	class CHCAErosion : public CHexCA
	{
	private:
		// The amount of rainfall per step
		float m_rain;
		// The amount of water evaporated per step
		float m_evap;
		// The solubility of the terrain
		float m_solu;
		// The maximum sediment which can be transported (TODO: per what?!)
		float m_trans;
	private:
		virtual CHexCACell* CreateCell( );
	public:
		CHCAErosion( uint64 width, uint64 height );
		virtual ~CHCAErosion( );
		virtual bool InitialiseCells( const std::vector<float>& data);
		virtual bool InitialiseCells( const std::vector<CHexCACell*>& data);
		virtual void Step( float dt );
		
	};
}

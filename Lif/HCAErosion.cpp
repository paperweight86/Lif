#include "stdafx.h"
#include "HCAErosion.h"
#include "HCAErosionCell.h"

#include <algorithm>

using namespace lif;

CHCAErosion::CHCAErosion(uint64 width, uint64 height) : CHexCA(width, height), 
														m_rain(0.01f),  m_evap(0.5f), 
														m_solu(0.0025f), m_trans(0.01f)
{
}

CHCAErosion::~CHCAErosion( )
{
}

CHexCACell* CHCAErosion::CreateCell( )
{
	return new CHCAErosionCell( );
}

void CHCAErosion::Step( float dt )
{
	size_t i = 0;

	// It rains and sediment is disolved into the water (erosion)
	for (auto iter = m_cells.begin(); iter != m_cells.end(); ++iter)
	{
		CHCAErosionCell* cell = static_cast<CHCAErosionCell*>((*iter));
		cell->m_water += m_rain;
		float transSediment = m_solu * cell->m_terrain; // TODO: water should be included here too - limiting factor of transport capability
		cell->m_terrain -= transSediment;
		cell->m_sediment += transSediment;
	}

	// Sediment and water a distributed to neighbouring cells (flow)
	for( auto iter = m_cells.begin(); iter != m_cells.end(); ++iter )
	{
		CHCAErosionCell* cell = static_cast<CHCAErosionCell*>((*iter));
		std::array<CHexCACell*,eAdjIndex_COUNT>& adj = m_adjacency[i++];
		// Calculate neighbour metrics prior to distribution
		int numOverHeight = 0;
		float totalDelta = 0.0f;
		float totalHeight = 0.0f;
		float cellTotalHeight = cell->m_terrain + cell->m_water;
		std::for_each(  adj.cbegin(), adj.cend(),
						[&numOverHeight, &totalDelta, &totalHeight, cellTotalHeight](const CHexCACell* neighbour)
						{
							if (neighbour == NULL)
								return;
							const CHCAErosionCell* neiCell = static_cast<const CHCAErosionCell*>(neighbour);
							float neiTotalHeight  = neiCell->m_terrain + neiCell->m_water;
							if( neiTotalHeight < cellTotalHeight )
							{
								totalHeight += neiTotalHeight;
								float hdelta = cellTotalHeight - neiTotalHeight;
								totalDelta += hdelta;
								numOverHeight++;
							}
							// TODO: the diff in height can the total heights should be cached
						}
					 );
		float avgHeight = totalHeight/numOverHeight;
		// Calculate the distribution for each cell
		// TODO: This should happen from the highest points downwards & outwards accumulating velocity and loosing it to decreasing
		//       gradient increasing sediment pickup
		if( numOverHeight > 0 )
		{
			std::for_each(  adj.begin(), adj.end(), 
							[cell, avgHeight, totalDelta](CHexCACell* neighbour)
							{
								if (neighbour == NULL)
									return;
								CHCAErosionCell* neiCell = static_cast<CHCAErosionCell*>(neighbour);
								float neiTotalHeight  = neiCell->m_terrain + neiCell->m_water;
								float cellTotalHeight = cell->m_terrain + cell->m_water;
								if( neiTotalHeight < cellTotalHeight )
								{
									float a = std::min( cellTotalHeight - avgHeight, cell->m_water );
									float b = (cellTotalHeight - neiTotalHeight) / totalDelta;
									float waterDist = a * b;
									float sedimDist = cell->m_sediment * waterDist / cell->m_water;
									// Remove from this cell
									cell->m_water	 -= waterDist;
									cell->m_sediment -= sedimDist;
									// Add to neighbour cell
									neiCell->m_water	+= waterDist;
									neiCell->m_sediment += sedimDist;
									//_tprintf( _T("") );
								}
							}
						 );
		}
	}

	// Water is evaporated and sediment is deposited
	for (auto iter = m_cells.begin(); iter != m_cells.end(); ++iter)
	{
		CHCAErosionCell* cell = static_cast<CHCAErosionCell*>((*iter));
		// TODO: Something about this seems incorrect?
		cell->m_water *= 1.0f - m_evap;
		// Deposit the amount of sediment over the maximum the remaining water can carry
		float maxDepos = cell->m_water * m_trans;
		float deposit = std::max<float>(0.0f, cell->m_sediment - maxDepos);
		cell->m_sediment -= deposit;
		cell->m_terrain += deposit;
	}
}

bool CHCAErosion::InitialiseCells(const std::vector<float>& data)
{
	assert(data.size() >= m_cells.size());
	auto dataIter = data.cbegin();
	for (auto iter = m_cells.begin(); iter != m_cells.end(); ++iter)
	{
		CHCAErosionCell* cell = static_cast<CHCAErosionCell*>((*iter));
		(*cell).m_terrain = (*dataIter);
		++dataIter;
	}
	return true;
}

bool CHCAErosion::InitialiseCells(const std::vector<CHexCACell*>& data)
{
	assert(data.size() >= m_cells.size());
	auto dataIter = data.cbegin();
	for (auto iter = m_cells.begin(); iter != m_cells.end(); ++iter)
	{
		CHCAErosionCell* cell = static_cast<CHCAErosionCell*>((*iter));
		CHCAErosionCell* srcCell = static_cast<CHCAErosionCell*>((*iter));
		(*cell).m_sediment = (*srcCell).m_sediment;
		(*cell).m_terrain = (*srcCell).m_terrain;
		(*cell).m_water = (*srcCell).m_water;
		++dataIter;
	}
	return true;
}



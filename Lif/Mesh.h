#pragma once

#include "VertexFormat.h"

namespace lif
{
	struct SMesh
	{
		float*			  vertData;
		uint32  numPrimatives;
		uint8   numVertFormat;
		VertexFormat*	  vertFormat;
	};
}
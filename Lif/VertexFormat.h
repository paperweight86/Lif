#pragma once

namespace lif
{
	typedef uint16 VertexFormat;

	enum EVertDataType : uint8
	{
		eVertexFormat_Unknown,

		eVertexFormat_Postition,
		
		eVertexFormat_Normal,

		eVertexFormat_Texture
	};

	VertexFormat makeVertFormat( EVertDataType type, uint8 len )
	{
		return (VertexFormat)type | ((VertexFormat)len << 8);
	}

	void readVertFormat( const VertexFormat&  fmt, EVertDataType& type, uint8& len )
	{
		type = (EVertDataType)fmt;
		len = (uint8)(fmt >> 8);
	}
}
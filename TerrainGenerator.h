#pragma once

#include "CustomTerrain.h" 
#include "CustomTerrain2.h"

namespace Terrain
{
	class TerrainGenerator
	{
	public:
		TerrainGenerator()
		{

		}

		static CustomTerrain2* Generate(CustomTerrain2::Algorithm algorithmType, octet::vec3 size, octet::ivec3 dimensions)
		{
			return new CustomTerrain2(size, dimensions, algorithmType);
		}
	};
}


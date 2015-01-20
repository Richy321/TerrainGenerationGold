#pragma once

#include "CustomTerrain.h" 

namespace Terrain
{
	class TerrainGenerator
	{
	public:
		TerrainGenerator()
		{

		}

		static CustomTerrain* Generate(CustomTerrain::Algorithm algorithmType, octet::vec3 size, octet::ivec3 dimensions)
		{
			return new CustomTerrain(size, dimensions, algorithmType);
		}
	};
}


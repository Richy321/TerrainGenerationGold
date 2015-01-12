#pragma once
#include "../../octet.h"
#include "TerrainGenerator.h"

namespace Terrain
{
	class CustomTerrain : public octet::mesh
	{
	public:
		enum Algorithm
		{
			DiamondSquare
		};

		CustomTerrain(octet::vec3_in size, octet::ivec3 dimensions) : mesh(), dimensions(dimensions)
		{
			set_default_attributes();
			set_aabb(octet::aabb(octet::vec3(0, 0, 0), size));
			update();
		}

		void update()
		{
			octet::dynarray<mesh::vertex> vertices;
			octet::dynarray<uint32_t> indices;
			vertices.reserve((dimensions.x() + 1) * (dimensions.z() + 1));

			octet::vec3 dimensionF = (octet::vec3)dimensions; //convert to float vector
			octet::aabb boundingBox = get_aabb();
			octet::vec3 bb_delta = boundingBox.get_half_extent() / dimensionF * 2.0f;
			bb_delta.y() = 0;

			octet::vec3 uv_min = octet::vec3(0);
			octet::vec3 uv_delta = octet::vec3(30.0f / dimensionF.x(), 30.0f / dimensionF.z(), 0);

			for (int x = 0; x <= dimensions.x(); ++x)
			{
				for (int z = 0; z <= dimensions.z(); ++z)
				{
					octet::vec3 xz = octet::vec3((float)x, 0, (float)z) * bb_delta;

					octet::vec3 normal(0,1,0);
					octet::vec3 uvw(0);
					mesh::vertex vert(xz, normal, uvw);

					vertices.push_back(vert);
				}
			}

			indices.reserve(dimensions.x() * dimensions.z() * 6);
			int stride = dimensions.x() + 1;
			for (int x = 0; x < dimensions.x(); ++x)
			{
				for (int z = 0; z < dimensions.z(); ++z)
				{
					// 01 11
					// 00 10
					indices.push_back((x + 0) + (z + 0)*stride);
					indices.push_back((x + 0) + (z + 1)*stride);
					indices.push_back((x + 1) + (z + 0)*stride);
					indices.push_back((x + 1) + (z + 0)*stride);
					indices.push_back((x + 0) + (z + 1)*stride);
					indices.push_back((x + 1) + (z + 1)*stride);
				}
			}

			set_vertices(vertices);
			set_indices(indices);
		}

	private:
		octet::ivec3 dimensions;
		Algorithm curAlgorithm = Algorithm::DiamondSquare;
	};
}

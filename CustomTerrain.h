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
			MidpointDisplacement,
			DiamondSquare
		};

		struct vertex 
		{
			octet::vec3 pos;
			octet::vec3 normal;
			octet::vec2 uv;

			vertex() { }

			vertex(octet::vec3_in pos, octet::vec3_in normal, octet::vec3_in uvw) {
				this->pos = pos;
				this->normal = normal;
				this->uv = uvw.xy();
			}
		};

		octet::random rand;
		octet::ivec3 dimensions;
		octet::vec3 size;

		float scale = 100.0f;

		Algorithm algorithmType = Algorithm::MidpointDisplacement;

		CustomTerrain(octet::vec3 size, octet::ivec3 dimensions, Algorithm algorithmType) : mesh(), dimensions(dimensions)
		{
			this->algorithmType = algorithmType;
			this->dimensions = dimensions;
			this->size = size;

			rand = octet::random();
			set_default_attributes();
			set_aabb(octet::aabb(octet::vec3(0, 0, 0), size));
			update();
		}

		void update()
		{
			octet::dynarray<vertex> vertices;

			octet::dynarray<uint32_t> indices;

			vertices.reserve((dimensions.x() + 1) * (dimensions.z() + 1));

			octet::vec3 dimensionF = (octet::vec3)dimensions; //convert to float vector
			octet::aabb boundingBox = get_aabb();
			octet::vec3 bb_delta = boundingBox.get_half_extent() / dimensionF * 2.0f;
			bb_delta.y() = 0;

			octet::vec3 uv_min = octet::vec3(0);
			octet::vec3 uv_delta = octet::vec3(30.0f / dimensionF.x(), 30.0f / dimensionF.z(), 0);

			//initialise grid to 0 height;
			for (int x = 0; x <= dimensions.x(); ++x)
			{
				for (int z = 0; z <= dimensions.z(); ++z)
				{
					octet::vec3 xz = octet::vec3((float)x, 0, (float)z) * bb_delta;

					octet::vec3 normal(0.0f, 1.0f, 0.0f);
					octet::vec3 uvw(0);
					vertex vert(xz, normal, uvw);

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

			switch (algorithmType)
			{

				case Algorithm::MidpointDisplacement:
					//MidpointDisplacementAlgorithm(vertices);
					break;
				case Algorithm::DiamondSquare:
					//DiamondSquareAlgorithm(vertices);
					break;
			}

			set_vertices(vertices);
			set_indices(indices);
		}

	private:
		Algorithm curAlgorithm = Algorithm::DiamondSquare;


		void MidpointDisplacementAlgorithm(octet::dynarray<vertex>& vertices)
		{
			octet::vec2 from(0, 0);
			octet::vec2 to(dimensions.x(), dimensions.z());

			midpoint(vertices, from, to, scale);
		}

		void midpoint(octet::dynarray<vertex>& vertices, octet::vec2 from, octet::vec2 to, float scale)
		{
			//find the midpoint
			octet::vec2 mp = (from + to) * 0.5f;

			float displacementAmount = rand.get(-1.0f, 1.0f);
			displacementAmount *= scale;

			//do displacement
			int index = mp.x() * size.y() + mp.y();
			vertices[index].pos = octet::vec3(vertices[index].pos.x(), displacementAmount, vertices[index].pos.z());

			printf("new pos y: %f", vertices[index].pos.y());

			//stop clause
			octet::vec2 diff = to - from;
			float length = diff.length();
			if (length <= 2)
				return;

			scale *= 0.5f;

			//split into 4
			octet::vec2 middlePoint = octet::vec2((to.x() - from.x()) * 0.5f, (to.y() - from.y()) * 0.5f);

			//bottom left quarter
			from = octet::vec2(from.x(), from.y());
			to = middlePoint;
			midpoint(vertices, from, to, scale);

			//top left quarter
			from = octet::vec2(from.x(), (to.y() - from.y()) * 0.5f);
			to = octet::vec2((to.x() - from.x()) * 0.5f, to.y());
			midpoint(vertices, from, to, scale);

			//top right quarter
			from = middlePoint;
			to = octet::vec2(to.x(), to.y());
			midpoint(vertices, from, to, scale);

			//bottom right quarter
			from = octet::vec2((to.x() - from.x()) * 0.5f, from.y());
			to = octet::vec2(to.x(), (to.y() - from.y()) * 0.5f);
			midpoint(vertices, from, to, scale);
		}
		
		void DiamondSquareAlgorithm(octet::dynarray<vertex>& vertices)
		{
			float rndCornerHeight = rand.get(40.0f, 50.0f);

			//assign hieght value to each of the four corners
			vertices[0].pos = octet::vec3(vertices[0].pos.x(), rndCornerHeight, vertices[0].pos.z());
		}
	};
}

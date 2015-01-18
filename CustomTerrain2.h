#pragma once
#include "../../octet.h"
#include "TerrainGenerator.h"

class CustomTerrain2 : public octet::mesh
{
	
private:

	octet::random rand;
	octet::ivec3 dimensions;
	octet::vec3 size;

public:
	enum Algorithm
	{
		MidpointDisplacement,
		DiamondSquare
	};

	struct CustomVertex
	{
		octet::vec3 pos;
		octet::vec3 normal;
		octet::vec2 uv;

		CustomVertex() { }

		CustomVertex(octet::vec3_in pos, octet::vec3_in normal, octet::vec3_in uvw) {
			this->pos = pos;
			this->normal = normal;
			this->uv = uvw.xy();
		}
	};

	Algorithm algorithmType = Algorithm::MidpointDisplacement;

	float scale = 1.0f;
	float roughnessFrom = -20.0f;
	float roughnessTo = 20.0f;

	CustomTerrain2(octet::vec3 size, octet::ivec3 dimensions, Algorithm algorithmType)
	{
		this->algorithmType = algorithmType;
		this->dimensions = dimensions;
		this->size = size;

		set_default_attributes();
		__int64 theTime = time(NULL);
		printf("Time:%i", theTime);
		rand = octet::random(theTime);
		
		set_default_attributes();
		set_aabb(octet::aabb(octet::vec3(0, 0, 0), size));
		
		update();
	}

	~CustomTerrain2()
	{
	}


	void update() override
	{
		octet::dynarray<CustomVertex> vertices;
		octet::dynarray<uint32_t> indices;

		vertices.reserve((dimensions.x() + 1) * (dimensions.z() + 1));

		octet::vec3 dimf = (octet::vec3)(dimensions);
		octet::aabb bb = get_aabb();
		octet::vec3 bb_min = bb.get_min();
		octet::vec3 bb_delta = bb.get_half_extent() / dimf * 2.0f;
		bb_delta.y() = 0;
		octet::vec3 uv_min = octet::vec3(0);
		octet::vec3 uv_delta = octet::vec3(30.0f / dimf.x(), 30.0f / dimf.z(), 0);
		for (int x = 0; x <= dimensions.x(); ++x) 
		{
			for (int z = 0; z <= dimensions.z(); ++z) 
			{
				octet::vec3 xz = octet::vec3((float)x, 0, (float)z) * bb_delta;
				CustomVertex vertex;

				vertex.pos = xz;
				vertex.normal = octet::vec3(0.0f, 1.0f, 0.0f);
				vertex.uv = octet::vec2(x * uv_delta.x(), z * uv_delta.y());

				vertices.push_back(vertex);
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
				MidpointDisplacementAlgorithm(vertices);
				break;
			case Algorithm::DiamondSquare:
				//DiamondSquareAlgorithm(vertices);
				break;
		}

		set_vertices(vertices);
		set_indices(indices);
		this->set_mode(GL_LINES);
	}

	void MidpointDisplacementAlgorithm(octet::dynarray<CustomVertex>& vertices)
	{
		octet::vec2 from(0, 0);
		octet::vec2 to(dimensions.x(), dimensions.z());

		vertices[0].pos.y() = rand.get(roughnessFrom, roughnessTo);
		vertices[dimensions.x()].pos.y() = rand.get(roughnessFrom, roughnessTo);
		vertices[vertices.size() - 1 - dimensions.z()].pos.y() = rand.get(roughnessFrom, roughnessTo);
		vertices[vertices.size()-1].pos.y() = rand.get(roughnessFrom, roughnessTo);

		midpoint(vertices, from, to, scale);
	}

	int GetVertexIndex(const octet::vec2 posCoord)
	{
		return posCoord.x() * dimensions.z() + posCoord.y();
	}
	
	void midpoint(octet::dynarray<CustomVertex>& vertices, octet::vec2 from, octet::vec2 to, float scale)
	{
		//stop clause
		octet::vec2 diff = to - from;
		float length = diff.length();
		if (length <= 2)
			return;

		//Divide into quarters
		octet::vec2 topLeft = octet::vec2(from.x(), to.y());
		octet::vec2 topRight = octet::vec2(to.x(), to.y());
		octet::vec2 bottomLeft = octet::vec2(from.x(), from.y());
		octet::vec2 bottomRight = octet::vec2(to.x(), from.y());

		octet::vec2 left = (topLeft - bottomLeft) * 0.5;
		octet::vec2 top = (topRight - topLeft) * 0.5f;
		octet::vec2 right = (topRight - bottomRight) *0.5f;
		octet::vec2 bottom = (bottomRight - bottomLeft) * 0.5f;

		//middle point
		octet::vec2 mp = (from + to) * 0.5f;

		//set mean values of corners of parent triangle

		float meanLeft = (vertices[GetVertexIndex(topLeft)].pos.y() + vertices[GetVertexIndex(bottomLeft)].pos.y()) * 0.5f;
		vertices[GetVertexIndex(left)].pos.y() = meanLeft;

		float meanBottom = (vertices[GetVertexIndex(bottomLeft)].pos.y() + vertices[GetVertexIndex(bottomRight)].pos.y()) *0.5f;
		vertices[GetVertexIndex(bottom)].pos.y() = meanBottom;

		float meanTop = (vertices[GetVertexIndex(topLeft)].pos.y() + vertices[GetVertexIndex(topRight)].pos.y()) *0.5f;
		vertices[GetVertexIndex(top)].pos.y() = meanTop;

		float meanRight = (vertices[GetVertexIndex(topRight)].pos.y() + vertices[GetVertexIndex(bottomRight)].pos.y()) *0.5f;
		vertices[GetVertexIndex(right)].pos.y() = meanRight;

		float displacementAmount = rand.get(roughnessFrom, roughnessTo);
		displacementAmount *= scale;

		//displace center point
		displacementAmount += (vertices[GetVertexIndex(topLeft)].pos.y() + vertices[GetVertexIndex(bottomLeft)].pos.y() + vertices[GetVertexIndex(topRight)].pos.y() + vertices[GetVertexIndex(bottomRight)].pos.y()) * 0.25f;
		vertices[GetVertexIndex(mp)].pos.y() = displacementAmount;

		scale *= 0.5f;

		//split into 4

		//bottom left quarter
		midpoint(vertices, bottomLeft, mp, scale);

		//top left quarter
		midpoint(vertices, left, top, scale);

		//top right quarter
		midpoint(vertices, mp, topRight, scale);

		//bottom right quarter
		midpoint(vertices, bottom, right, scale);
	}
	


	void DiamondSquareAlgorithm(octet::dynarray<vertex>& vertices)
	{
		/*
		float rndCornerHeight = rand.get(40.0f, 50.0f);

		//assign hieght value to each of the four corners
		vertices[0].pos = octet::vec3(vertices[0].pos.x(), rndCornerHeight, vertices[0].pos.z());
		*/
	}
};


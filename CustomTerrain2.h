#pragma once
#include "../../octet.h"
#include "TerrainGenerator.h"

class CustomTerrain2 : public octet::mesh
{
	
private:

	octet::random rand;
	octet::ivec3 dimensions;
	octet::vec3 size;

	std::vector<std::vector<float>> map;

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

	float scale = 10.0f;
	float scaleModifier = 0.7f;

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
		
		map.resize(dimensions.x() +1, std::vector<float>(dimensions.z() +1, 0.0f));

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
				MidpointDisplacementAlgorithm2(vertices);
				break;
			case Algorithm::DiamondSquare:
				//DiamondSquareAlgorithm(vertices);
				break;
		}

		//Set points into the vertex structure
		for (int x = 0; x <= dimensions.x(); ++x)
		{
			for (int z = 0; z <= dimensions.z(); ++z)
			{
				vertices[x * (dimensions.z()+1) + z].pos.y() = map[x][z];
			}
		}

		set_vertices(vertices);
		set_indices(indices);
		this->set_mode(GL_LINES);
	}

	void MidpointDisplacementAlgorithm(octet::dynarray<CustomVertex>& vertices)
	{
		octet::vec2 from(0, 0);
		octet::vec2 to(dimensions.x(), dimensions.z());

		

		map[0][0] = rand.get(0.0f, scale);
		map[dimensions.x()][0] = rand.get(0.0f, scale);
		map[0][dimensions.y()] = rand.get(0.0f, scale);
		map[dimensions.x()][dimensions.y()] = rand.get(0.0f, scale);

		//vertices[0].pos.y() = rand.get(0.0f, scale);
		//vertices[dimensions.x()].pos.y() = rand.get(0.0f, scale);
		//vertices[vertices.size() - 1 - dimensions.z()].pos.y() = rand.get(0.0f, scale);
		//vertices[vertices.size()-1].pos.y() = rand.get(0.0f, scale);

		midpoint(from, to, scale);
	}


	void MidpointDisplacementAlgorithm2(octet::dynarray<CustomVertex>& vertices)
	{
		const unsigned hgrid = 33,//x dimension of the grid
			vgrid = hgrid;//y dimension of the grid

		int	i, j, k,//iterators
			x, y,//location variables
			sgrid = ((hgrid < vgrid) ? hgrid : vgrid) - 1,//whichever is smaller (minus 1)
			offset = sgrid / 2;//offset is the width of the square or diamond we are working with


		//set the four corners
		for (i = 0; i<vgrid; i += sgrid){
			for (j = 0; j<hgrid; j += sgrid){
				map[j][i] = 10.0f + rand.get(0.0f, 2.0f);
			}
		}


		float range = 50.0f,//range for random numbers
			rangeModifier = 0.7f,//the range is multipiled by this number each pass, making the map smoother
			total,//variable for storing a mean value
			temp;//stores the new value for a slot so more calculations can be done

		bool oddy, oddx;//these are used to tell if we're working with a side or center

		float min = 10000.0f;
		float max = 0.0f;//for averaging

		//get started
		//2.1 while the size of the squares is larger than 1...
		while (offset > 0)
		{
			oddy = false;

			for (y = 0; y < vgrid; y += offset, oddy = !oddy)
			{
				oddx = false;

				for (x = 0; x < hgrid; x += offset, oddx = !oddx)
				{
					if (oddx || oddy)
					{
						//if this is a center...
						if (oddx && oddy)
						{
							//this point gets the average of its four corners plus a small 'error'
							temp = (map[x - offset][y - offset] + map[x + offset][y - offset] + map[x - offset][y + offset] + map[x + offset][y + offset]) / 4 + rand.get(0.0f, range);
						}

						//if this is a side...
						else
						{
							//horizontal side
							if (oddx)
							{
								temp = (map[x - offset][y] + map[x + offset][y]) / 2 + rand.get(0.0f, range);
							}

							//vertical side
							else
							{
								temp = (map[x][y - offset] + map[x][y + offset]) / 2 + rand.get(0.0f, range);
							}
						}

						//set the value
						map[x][y] = temp;

						//now that we have a value, check min and max
						if (temp > max)
							max = temp;
						if (temp < min)
							min = temp;
					}
				}
			}

			//adjust the range and offset
			range *= rangeModifier;
			offset /= 2;
		}
	}

	int GetVertexIndex(const octet::vec2 posCoord)
	{
		return posCoord.x() * dimensions.z() + posCoord.y();
	}
	
	void midpoint(octet::vec2 from, octet::vec2 to, float scale)
	{
		/*	


		//stop clause
		octet::vec2 diff = to - from;

		if (diff.x() <= 1 || diff.y() <= 1)
			return;

		//Divide into quarters
		octet::vec2 topLeft = octet::vec2(from.x(), to.y());
		octet::vec2 topRight = octet::vec2(to.x(), to.y());
		octet::vec2 bottomLeft = octet::vec2(from.x(), from.y());
		octet::vec2 bottomRight = octet::vec2(to.x(), from.y());

		octet::vec2 left = (topLeft - bottomLeft) * 0.5f;
		octet::vec2 top = (topRight - topLeft) * 0.5f;
		octet::vec2 right = (topRight - bottomRight) *0.5f;
		octet::vec2 bottom = (bottomRight - bottomLeft) * 0.5f;

		//middle point
		octet::vec2 mp = (from + to) * 0.5f;

		//set mean values of corners of parent triangle

		float meanLeft = (vertices[GetVertexIndex(topLeft)].pos.y() + vertices[GetVertexIndex(bottomLeft)].pos.y()) * 0.5f;
		vertices[GetVertexIndex(left)].pos.y() = meanLeft + rand.get(0.0f, scale);

		float meanBottom = (vertices[GetVertexIndex(bottomLeft)].pos.y() + vertices[GetVertexIndex(bottomRight)].pos.y()) *0.5f;
		vertices[GetVertexIndex(bottom)].pos.y() = meanBottom + rand.get(0.0f, scale);

		float meanTop = (vertices[GetVertexIndex(topLeft)].pos.y() + vertices[GetVertexIndex(topRight)].pos.y()) *0.5f;
		vertices[GetVertexIndex(top)].pos.y() = meanTop + rand.get(0.0f, scale);

		float meanRight = (vertices[GetVertexIndex(topRight)].pos.y() + vertices[GetVertexIndex(bottomRight)].pos.y()) *0.5f;
		vertices[GetVertexIndex(right)].pos.y() = meanRight + rand.get(0.0f, scale);

		

		//displace center point
		float displacementAmount = (vertices[GetVertexIndex(topLeft)].pos.y() + vertices[GetVertexIndex(bottomLeft)].pos.y() + vertices[GetVertexIndex(topRight)].pos.y() + vertices[GetVertexIndex(bottomRight)].pos.y()) * 0.25f + rand.get(0.0f, scale);
		vertices[GetVertexIndex(mp)].pos.y() = displacementAmount;

		scale *= scaleModifier;

		//split into 4

		//bottom left quarter
		midpoint(vertices, bottomLeft, mp, scale);

		//top left quarter
		midpoint(vertices, left, top, scale);

		//top right quarter
		midpoint(vertices, mp, topRight, scale);

		//bottom right quarter
		midpoint(vertices, bottom, right, scale);
		*/
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


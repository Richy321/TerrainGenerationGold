#pragma once
#include "../../octet.h"
#include "PerlinNoiseGenerator.h"

#include <ctime>
#include <unordered_map>




namespace Terrain
{
	class CustomTerrain : public octet::mesh
	{
		typedef void (CustomTerrain::*pMemberFunc_t)(std::vector<std::vector<float>> &map);

	public:
		enum Algorithm
		{
			MidpointDisplacement,
			DiamondSquare,
			PerlinNoise,
			FractionalBrownianMotion,
			MultiFractal,
		};

	private:

		octet::random rand;
		octet::ivec3 dimensions;
		octet::vec3 size;

		std::vector<std::vector<float>> heightMap;
		std::unordered_map<Algorithm, pMemberFunc_t> algorithmToFunction;
		
		octet::material *customMaterial;



	public:

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

		Algorithm algorithmType = Algorithm::FractionalBrownianMotion;

		float scale = 10.0f;
		float scaleModifier = 0.7f;


		octet::material* GetMaterial() { return customMaterial; }

		void InitialiseAlgorithmDispatchMap()
		{
			algorithmToFunction[Algorithm::MidpointDisplacement] = &CustomTerrain::MidpointDisplacementAlgorithm;
			algorithmToFunction[Algorithm::DiamondSquare] = &CustomTerrain::DiamondSquareAlgorithm;
			algorithmToFunction[Algorithm::PerlinNoise] = &CustomTerrain::PerlinNoiseAlgorithm;
			algorithmToFunction[Algorithm::FractionalBrownianMotion] = &CustomTerrain::FractionalBrownianMotionAlgorithm;
			algorithmToFunction[Algorithm::MultiFractal] = &CustomTerrain::MultiFractalAlgorithm;
		}

		CustomTerrain(octet::vec3 size, octet::ivec3 dimensions, Algorithm algorithmType)
		{
			InitialiseAlgorithmDispatchMap();

			this->algorithmType = algorithmType;
			this->dimensions = dimensions;
			this->size = size;

			set_default_attributes();
			__int64 theTime = time(NULL);
			printf("Time:%i", theTime);
			rand = octet::random((unsigned)theTime);

			set_default_attributes();
			set_aabb(octet::aabb(octet::vec3(0, 0, 0), size));

			heightMap.resize(dimensions.x() + 1, std::vector<float>(dimensions.z() + 1, 0.0f));


			//octet::image *img = new octet::image("assets/grass.jpg");

			octet::param_shader* shader = new octet::param_shader("shaders/default.vs", "src/examples/terrain-generation/shaders/MultiLayerTerrain.fs");

			customMaterial = new octet::material(octet::vec4(0, 1, 0, 1), shader);

			//customMaterial = new octet::material(img);

			update();
		}


		~CustomTerrain()
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
			
			float fTextureU = float(dimensions.z())*0.01f;
			float fTextureV = float(dimensions.x())*0.01f;

			for (int x = 0; x <= dimensions.x(); ++x)
			{
				for (int z = 0; z <= dimensions.z(); ++z)
				{
					float fScaleC = float(z) / float(dimensions.z());
					float fScaleR = float(x) / float(dimensions.x());

					octet::vec3 xz = octet::vec3((float)x, 0, (float)z) * bb_delta;
					CustomVertex vertex;
					vertex.pos = xz;
					vertex.normal = octet::vec3(0.0f, 1.0f, 0.0f);
					vertex.uv = octet::vec2(fTextureU*fScaleC, fTextureV*fScaleR);
					//vertex.uv = octet::vec2(x * uv_delta.x(), z * uv_delta.y());
					//vertex.uv = (uv_min + octet::vec3((float)vertex.pos.x(), (float)vertex.pos.z(), 0) * uv_delta).xy();
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

			//dispatch to correct algorithm
			pMemberFunc_t algFunc = algorithmToFunction[algorithmType];
			(this->*algFunc)(heightMap);

			//Set points into the vertex structure
			for (int x = 0; x <= dimensions.x(); ++x)
			{
				for (int z = 0; z <= dimensions.z(); ++z)
				{
					int index = x * (dimensions.z() + 1) + z;
					vertices[index].pos.y() = heightMap[x][z];
				}
			}

			//calc cheap norms based on nearest neighbouring heights
			//http://www.flipcode.com/archives/Calculating_Vertex_Normals_for_Height_Maps.shtml

			for (int z = 0; z <= dimensions.z(); ++z)
			{
				for (int x = 0; x <= dimensions.x(); ++x)
				{
					float nX = heightMap[x < dimensions.x() ? x + 1 : x][z] - heightMap[x > 0 ? x - 1 : x][z];
					if (x == 0 || x == dimensions.x())
						nX *= 2;

					float nZ = heightMap[x][z < dimensions.z() ? z + 1 : z] - heightMap[x][z > 0 ? z - 1 : z];
					if (z == 0 || z == dimensions.z())
						nZ *= 2;

					int index = x * (dimensions.z() + 1) + z;
					vertices[index].normal.x() = nX;
					vertices[index].normal.y() = 2;
					vertices[index].normal.z() = nZ;
					vertices[index].normal.normalize();
				}
			}

			set_vertices(vertices);
			set_indices(indices);
			//this->set_mode(GL_LINES);
		}

		void MidpointDisplacementAlgorithmRecursive(std::vector<std::vector<float>> &map)
		{
			octet::vec2 from(0, 0);
			octet::vec2 to(dimensions.x(), dimensions.z());

			map[0][0] = rand.get(0.0f, scale);
			map[dimensions.x()][0] = rand.get(0.0f, scale);
			map[0][dimensions.z()] = rand.get(0.0f, scale);
			map[dimensions.x()][dimensions.z()] = rand.get(0.0f, scale);

			midpointRecurse(from, to, scale, map);
		}

		void midpointRecurse(octet::vec2 from, octet::vec2 to, float scale, std::vector<std::vector<float>> &map)
		{
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
			float meanLeft = (map[topLeft.x()][topLeft.y()] + map[bottomLeft.x()][bottomLeft.y()]) * 0.5f;
			map[left.x()][left.y()] = meanLeft + rand.get(0.0f, scale);

			float meanBottom = (map[bottomLeft.x()][bottomLeft.y()] + map[bottomRight.x()][bottomRight.y()]) *0.5f;
			map[bottom.x()][bottom.y()] = meanBottom + rand.get(0.0f, scale);

			float meanTop = (map[topLeft.x()][topLeft.y()] + map[topRight.x()][topRight.y()]) *0.5f;
			map[top.x()][top.y()] = meanTop + rand.get(0.0f, scale);

			float meanRight = (map[topRight.x()][topRight.y()] + map[bottomRight.x()][bottomRight.y()]) *0.5f;
			map[right.x()][right.y()] = meanRight + rand.get(0.0f, scale);

			//displace center point
			float displacementAmount = ((map[topLeft.x()][topLeft.y()] + map[bottomLeft.x()][bottomLeft.y()] + map[topRight.x()][topRight.y()] + map[bottomRight.x()][bottomRight.y()]) * 0.25f) + rand.get(0.0f, scale);
			map[mp.x()][mp.y()] = displacementAmount;

			scale *= scaleModifier;

			//split into 4

			//bottom left quarter
			midpointRecurse(bottomLeft, mp, scale, map);

			//top left quarter
			midpointRecurse(left, top, scale, map);

			//top right quarter
			midpointRecurse(mp, topRight, scale, map);

			//bottom right quarter
			midpointRecurse(bottom, right, scale, map);
		}

		int GetVertexIndex(const octet::vec2 posCoord)
		{
			return (int)(posCoord.x() * dimensions.z() + posCoord.y());
		}

		void MidpointDisplacementAlgorithm(std::vector<std::vector<float>> &map)
		{
			const unsigned hgrid = dimensions.x() + 1,//x dimension of the grid
				vgrid = hgrid;//y dimension of the grid

			int	i, j, //iterators
				x, y,//location variables
				sgrid = ((hgrid < vgrid) ? hgrid : vgrid) - 1,//whichever is smaller (minus 1)
				offset = sgrid / 2;//offset is the width of the square or diamond we are working with


			//set the four corners
			for (i = 0; i < vgrid; i += sgrid){
				for (j = 0; j < hgrid; j += sgrid){
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

		void DiamondSquareAlgorithm(std::vector<std::vector<float>> &map)
		{
		}


		void PerlinNoiseAlgorithm(std::vector<std::vector<float>> &map)
		{
			PerlinNoiseGenerator noise;
			float frequency = 1.0f / (float)(dimensions.x() + 1);

			for (int y = 0; y < dimensions.x() + 1; y++)
			{
				for (int x = 0; x < dimensions.x() + 1; x++)
				{
					float noiseValue = noise.GenerateNoise((float)x * frequency, (float)y * frequency);
					map[x][y] = noiseValue * 100;
				}
			}
		}


		void FractionalBrownianMotionAlgorithm(std::vector<std::vector<float>> &map)
		{
			float gain = 0.65f;
			float lacunarity = 2.0f;
			unsigned octaves = 4;
			PerlinNoiseGenerator noise;
			
			for (int y = 0; y < dimensions.y() + 1; y++)
			{
				for (int x = 0; x < dimensions.x() + 1; x++)
				{
					//for each pixel, get the value
					float total = 0.0f;
					float frequency = 1.0f / (float)(dimensions.x() + 1);
					float amplitude = gain;

					for (int i = 0; i < octaves; ++i)
					{
						total += noise.GenerateNoise((float)x * frequency, (float)y * frequency) * amplitude;
						frequency *= lacunarity;
						amplitude *= gain;
					}

					//now that we have the value, put it in
					map[x][y] = total * 100;
				}
			}
		}

		void MultiFractalAlgorithm(std::vector<std::vector<float>> &map)
		{


		}
	};
}

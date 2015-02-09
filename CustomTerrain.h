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
			MultiFractal
		};

	private:

		octet::random rand;
		PerlinNoiseGenerator noise;

		octet::ivec3 dimensions;
		octet::vec3 size;

		std::vector<std::vector<float>> heightMap;
		std::unordered_map<Algorithm, pMemberFunc_t> algorithmToFunction;
		
		octet::material *customMaterial;

		octet::ref<octet::param_uniform> heightRange;


		octet::dynarray<vertex> vertices;
		octet::dynarray<uint32_t> indices;

		octet::dynarray<octet::ref<octet::image>> terrainLayers;

	public:

		Algorithm algorithmType;
		float heightScale = 50.0f;
		bool usePerlinRandom = false;

		octet::material* GetMaterial() { return customMaterial; }

		void InitialiseAlgorithmDispatchMap()
		{
			algorithmToFunction[Algorithm::MidpointDisplacement] = &CustomTerrain::MidpointDisplacementAlgorithm;
			algorithmToFunction[Algorithm::DiamondSquare] = &CustomTerrain::DiamondSquareAlgorithm;
			algorithmToFunction[Algorithm::PerlinNoise] = &CustomTerrain::PerlinNoiseAlgorithm;
			algorithmToFunction[Algorithm::FractionalBrownianMotion] = &CustomTerrain::FractionalBrownianMotionAlgorithm;
			algorithmToFunction[Algorithm::MultiFractal] = &CustomTerrain::MultiFractalAlgorithm;
		}

		void InitialiseImageLayers()
		{
			octet::image *img0 = new octet::image("src/examples/terrain-generation/textures/water2.jpg");
			customMaterial->add_sampler(0, octet::app_utils::get_atom("layer0"), img0, new octet::sampler());

			octet::image *img1 = new octet::image("src/examples/terrain-generation/textures/sand2.jpg");
			customMaterial->add_sampler(1, octet::app_utils::get_atom("layer1"), img1, new octet::sampler());
			
			octet::image *img2 = new octet::image("src/examples/terrain-generation/textures/grass5.jpg");
			customMaterial->add_sampler(2, octet::app_utils::get_atom("layer2"), img2, new octet::sampler());

			octet::image *img3 = new octet::image("src/examples/terrain-generation/textures/rock.jpg");
			customMaterial->add_sampler(3, octet::app_utils::get_atom("layer3"), img3, new octet::sampler());

			octet::image *img4 = new octet::image("src/examples/terrain-generation/textures/snow.jpg");
			customMaterial->add_sampler(4, octet::app_utils::get_atom("layer4"), img4, new octet::sampler());
		}

		CustomTerrain(octet::vec3 size, octet::ivec3 dimensions, Algorithm algorithmType)
		{
			InitialiseAlgorithmDispatchMap();

			this->algorithmType = algorithmType;
			this->dimensions = dimensions;
			this->size = size;

			__int64 theTime = time(NULL);
			printf("Time:%i", theTime);
			rand = octet::random((unsigned)theTime);

			set_default_attributes();
			set_aabb(octet::aabb(octet::vec3(0, 0, 0), size));

			heightMap.resize(dimensions.x() + 1, std::vector<float>(dimensions.z() + 1, 0.0f));

			octet::param_shader* shader = new octet::param_shader("shaders/default.vs", "src/examples/terrain-generation/shaders/MultiLayerTerrain.fs");
			customMaterial = new octet::material(octet::vec4(0, 1, 0, 1), shader);
			InitialiseImageLayers();

			octet::atom_t atom_heightRange = octet::app_utils::get_atom("heightRange");
			heightRange = customMaterial->add_uniform(nullptr, atom_heightRange, GL_FLOAT_VEC2, 1, octet::param::stage_fragment);

			generate();
		}

		~CustomTerrain()
		{

		}

		void generate()
		{
			buildPlane();

			//dispatch to correct algorithm
			pMemberFunc_t algFunc = algorithmToFunction[algorithmType];
			(this->*algFunc)(heightMap);

			float min = 999999.0f;
			float max = -999999.0f;

			//Set points into the vertex structure
			for (int x = 0; x <= dimensions.x(); ++x)
			{
				for (int z = 0; z <= dimensions.z(); ++z)
				{
					int index = x * (dimensions.z() + 1) + z;

					octet::vec3 pos(vertices[index].pos);
					pos.y() = heightMap[x][z] * heightScale;

					vertices[index].pos = octet::vec3p(pos.x(), pos.y(), pos.z());

					min = pos.y() < min ? pos.y() : min;
					max = pos.y() > max ? pos.y() : max;
				}
			}

			for (int z = 0; z <= dimensions.z(); ++z)
			{
				for (int x = 0; x <= dimensions.x(); ++x)
				{
					/*
					//calc cheap norms based on nearest neighbouring heights
					//http://www.flipcode.com/archives/Calculating_Vertex_Normals_for_Height_Maps.shtml
					float nX = heightMap[x < dimensions.x() ? x + 1 : x][z] - heightMap[x > 0 ? x - 1 : x][z];
					if (x == 0 || x == dimensions.x())
						nX *= 2;

					float nZ = heightMap[x][z < dimensions.z() ? z + 1 : z] - heightMap[x][z > 0 ? z - 1 : z];
					if (z == 0 || z == dimensions.z())
						nZ *= 2;
					*/
			
					int centre = x * (dimensions.z()+1) + z;
					int east = (x <= (dimensions.x()+1) ? x + 1 : x) * (dimensions.z()+1) + z;
					int south = x * (dimensions.z()+1) + ((z <= dimensions.z()+1) ? z + 1 : z);
					int west = (x > 0 ? x - 1 : x) * (dimensions.z()+1) + z;
					int north = x * (dimensions.z()+1) + (z > 0 ? z - 1 : z);
					//int centre = (x & dimensions.x()) + (z & (dimensions.z())) * dimensions.x();
					//int east = ((x+1) & (dimensions.x())) + (z & (dimensions.z())) * dimensions.x();
					//int south = (x & (dimensions.x())) + ((z+1) & (dimensions.z())) * dimensions.x();
					//int west = ((x-1) & (dimensions.x())) + (z & (dimensions.z())) * dimensions.x();
					//int north = (x & (dimensions.x())) + ((z-1) & (dimensions.z())) * dimensions.x();
					
					//Averaged Normals
					octet::vec3 centrePos = vertices[centre].pos;
					octet::vec3 eastPos = vertices[east].pos;
					octet::vec3 southPos = vertices[south].pos;
					octet::vec3 westPos = vertices[west].pos;
					octet::vec3 northPos = vertices[north].pos;

					octet::vec3 v1 = northPos - centrePos;
					octet::vec3 v2 = eastPos - centrePos;
					octet::vec3 ne = cross(v1, v2);

					v1 = eastPos - centrePos;
					v2 = southPos - centrePos;
					octet::vec3 se = cross(v1, v2);

					v1 = southPos - centrePos;
					v2 = westPos - centrePos;
					octet::vec3 sw = cross(v1, v2);

					v1 = northPos - centrePos;
					v2 = westPos - centrePos;
					octet::vec3 nw = cross(v1, v2);


					//octet::vec3 norm(0.0f, 1.0f, 0.0f);
					octet::vec3 norm = ne + nw + se + sw;
					//octet::vec3 norm = ne;
					//norm = norm.normalize();
					vertices[centre].normal = octet::vec3p(norm);
				}
			}

			//pass min and max to shader for height colouring
			octet::vec2 heights(min, max);
			customMaterial->set_uniform(heightRange, &heights, sizeof(heights));

			set_vertices(vertices);
			set_indices(indices);

			//dump(octet::log(""));

			//this->set_mode(GL_LINES);
		}

		void buildPlane()
		{
			vertices.reserve((dimensions.x() + 1) * (dimensions.z() + 1));

			octet::vec3 dimf = (octet::vec3)(dimensions);
			octet::aabb bb = get_aabb();
			octet::vec3 bb_min = bb.get_min();
			octet::vec3 bb_delta = bb.get_half_extent() / dimf * 2.0f;
			bb_delta.y() = 0;
			octet::vec3 uv_min = octet::vec3(0);
			octet::vec3 uv_delta = octet::vec3(30.0f / dimf.x(), 30.0f / dimf.z(), 0);

			float tiling = 0.1f;

			float fTextureUStep = 1.0f / (dimensions.x() * tiling);
			float fTextureVStep = 1.0f / (dimensions.z() * tiling);

			for (int x = 0; x <= dimensions.x(); ++x)
			{
				for (int z = 0; z <= dimensions.z(); ++z)
				{
					octet::vec3 xz = octet::vec3((float)x, 0, (float)z) * bb_delta;
					vertex vertex;
					vertex.pos = octet::vec3p(xz);
					vertex.normal = octet::vec3p(0.0f, 1.0f, 0.0f);
					vertex.uv = octet::vec2p(x * fTextureUStep, z * fTextureVStep);
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
		}

		int GetVertexIndex(const octet::vec2 posCoord)
		{
			return (int)(posCoord.x() * dimensions.z() + posCoord.y());
		}

		void MidpointDisplacementAlgorithm(std::vector<std::vector<float>> &map)
		{
			int gridSize = ((dimensions.x() + 1 < dimensions.z() + 1) ? dimensions.x() + 1 : dimensions.z() + 1) - 1;
			int offset = gridSize / 2;//offset is the width of the square we're working on
			float rangeModifier = 0.7f; //modifier on the range to smooth
			float randomScale = 1.0f;
			bool isOddY, isOddX; //these are used to tell if we're working with a side or center

			if (usePerlinRandom)
				noise.RandomisePermutations();

			//set the four corners
			for (int i = 0; i < dimensions.z() + 1; i += gridSize)
			{
				for (int j = 0; j < dimensions.x() + 1; j += gridSize)
				{
					map[j][i] = rand.get(-1.0f, 1.0f);//GetRandom(i,j);
				}
			}

			while (offset > 0)
			{
				isOddY = false;

				for (int y = 0; y < dimensions.z() + 1; y += offset, isOddY = !isOddY)
				{
					isOddX = false;

					for (int x = 0; x < dimensions.x() + 1; x += offset, isOddX = !isOddX)
					{
						if (isOddX || isOddY)
						{
							float height = 0.0f;
							
							// center
							if (isOddX && isOddY)
							{
								//average the four corners plus a small random amount (error)
								height = (map[x - offset][y - offset] + map[x + offset][y - offset] + map[x - offset][y + offset] + map[x + offset][y + offset]) / 4 + GetRandom(x, y) * randomScale;
							}
							else
							{
								//side 
								if (isOddX)
								{
									//average horizontal sides corners plus small random amount (error)
									height = (map[x - offset][y] + map[x + offset][y]) / 2 + GetRandom(x,y) * randomScale;
								}
								else
								{
									//average this vertical side corners plus small random amount (error)
									height = (map[x][y - offset] + map[x][y + offset]) / 2 + GetRandom(x, y) * randomScale;
								}
							}

							//set the value
							map[x][y] = height;
						}
					}
				}

				//adjust the range and offset
				randomScale *= rangeModifier;
				offset /= 2;
			}
		}

		void DiamondSquareAlgorithm(std::vector<std::vector<float>> &vectorMap)
		{
			int gridSize = ((dimensions.x() + 1 < dimensions.z() + 1) ? dimensions.x() + 1 : dimensions.z() + 1) - 1;
			int sampleSize = gridSize;//offset is the width of the square we're working on
			float scale = 1.0f;	

			float* map = new float[dimensions.x() * dimensions.z()];

			if(usePerlinRandom)
				noise.RandomisePermutations();

			//Set four corners
			for (int y = 0; y < dimensions.z() + 1; y += sampleSize)
			{
				for (int x = 0; x < dimensions.x() + 1; x += sampleSize)
				{
					SetSample(x, y, GetRandom(x,y), map);
				}
			}		

			while (sampleSize > 1)
			{
				DiamondSquareCore(sampleSize, scale, map);

				sampleSize /= 2;
				scale /= 2.0f;
			}

			//Convert to vector map to fit in with other algorithms
			for (int x = 0; x < dimensions.x() + 1; x++)
			{
				for (int y = 0; y < dimensions.z() + 1; y++)
				{
					vectorMap[x][y] = Sample(x, y, map);
				}
			}

			//cleanup
			delete map;
		}

		void DiamondSquareCore(int stepSize, float scale, float* map)
		{
			int halfStep = stepSize / 2;

			for (int y = halfStep; y < dimensions.z() + 1 + halfStep; y += stepSize)
			{
				for (int x = halfStep; x < dimensions.x() + 1 + halfStep; x += stepSize)
				{
					SampleSquare(x, y, stepSize, GetRandom(x,y) * scale, map);
				}
			}

			for (int y = 0; y < dimensions.z() + 1; y += stepSize)
			{
				for (int x = 0; x < dimensions.x() + 1; x += stepSize)
				{
					SampleDiamond(x + halfStep, y, stepSize, GetRandom(x,y) * scale, map);
					SampleDiamond(x, y + halfStep, stepSize, GetRandom(x,y) * scale, map);
				}
			}
		}

		float Sample(int x, int y, float* map)
		{
			return map[(x & (dimensions.x() - 1)) + (y & (dimensions.z() - 1)) * dimensions.x()];
		}

		void SetSample(int x, int y, float value, float* map)
		{
			map[(x & (dimensions.x() - 1)) + (y & (dimensions.z() - 1)) * dimensions.x()] = value;
		}

		void SampleSquare(int x, int y, int size, float value, float* map)
		{
			int halfSize = size / 2;

			float topLeft = Sample(x - halfSize, y - halfSize, map);
			float topRight = Sample(x + halfSize, y - halfSize, map);
			float bottomLeft = Sample(x - halfSize, y + halfSize, map);
			float bottomRight = Sample(x + halfSize, y + halfSize, map);

			SetSample(x, y, ((topLeft + topRight + bottomLeft + bottomRight) / 4.0f) + value, map);
		}

		void SampleDiamond(int x, int y, int size, float value, float* map)
		{
			int halfSize = size / 2;

			float left = Sample(x - halfSize, y, map);
			float right = Sample(x + halfSize, y, map);
			float top = Sample(x, y - halfSize, map);
			float bottom = Sample(x, y + halfSize, map);

			SetSample(x, y, ((left + right + top + bottom) / 4.0f) + value, map);
		}

		void PerlinNoiseAlgorithm(std::vector<std::vector<float>> &map)
		{
			noise.RandomisePermutations();
			float frequency = 5.0f / (float)(dimensions.x() + 1);

			for (int y = 0; y < dimensions.x() + 1; y++)
			{
				for (int x = 0; x < dimensions.x() + 1; x++)
				{
					float noiseValue = noise.GenerateNoise((float)x * frequency, (float)y * frequency);
					map[x][y] = noiseValue;
				}
			}
		}

		void FractionalBrownianMotionAlgorithm(std::vector<std::vector<float>> &map)
		{
			float gain = 0.65f;
			float lacunarity = 2.0f;
			unsigned octaves = 16;
			noise.RandomisePermutations();

			for (int y = 0; y < dimensions.z() + 1; y++)
			{
				for (int x = 0; x < dimensions.x() + 1; x++)
				{
					//for each pixel, get the value
					float total = 0.0f;
					float frequency = 1.0f / (float)(dimensions.x() + 1);
					float amplitude = gain;

					for (unsigned i = 0; i < octaves; ++i)
					{
						total += noise.GenerateNoise((float)x * frequency, (float)y * frequency) * amplitude;
						frequency *= lacunarity;
						amplitude *= gain;
					}

					//now that we have the value, put it in
					map[x][y] = total;
				}
			}
		}

		void MultiFractalAlgorithm(std::vector<std::vector<float>> &map)
		{


		}

		float GetRandom(int x, int y)
		{
			float frequency = 7.0f / (float)(dimensions.x() + 1);

			if (usePerlinRandom)
				return noise.GenerateNoise((float)x * frequency, (float)y * frequency);
			else
				return rand.get(-1.0f, 1.0f);
		}
	};
}

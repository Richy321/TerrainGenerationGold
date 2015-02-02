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

	private:

		octet::random rand;
		octet::ivec3 dimensions;
		octet::vec3 size;

		std::vector<std::vector<float>> heightMap;
		std::unordered_map<Algorithm, pMemberFunc_t> algorithmToFunction;
		
		octet::material *customMaterial;

		octet::ref<octet::param_uniform> heightRange;


		octet::dynarray<CustomVertex> vertices;
		octet::dynarray<uint32_t> indices;

	public:

		Algorithm algorithmType;

		float heightScale = 50.0f;

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

			octet::param_shader* shader = new octet::param_shader("shaders/default.vs", "src/examples/terrain-generation/shaders/MultiLayerTerrain.fs");
			customMaterial = new octet::material(octet::vec4(0, 1, 0, 1), shader);
			
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
					vertices[index].pos.y() = heightMap[x][z] * heightScale;

					min = vertices[index].pos.y() < min ? vertices[index].pos.y() : min;
					max = vertices[index].pos.y() > max ? vertices[index].pos.y() : max;
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

			//pass min and max to shader for hieght colouring
			octet::vec2 heights(min, max);
			customMaterial->set_uniform(heightRange, &heights, sizeof(heights));

			set_vertices(vertices);
			set_indices(indices);

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
		}

		int GetVertexIndex(const octet::vec2 posCoord)
		{
			return (int)(posCoord.x() * dimensions.z() + posCoord.y());
		}

		void MidpointDisplacementAlgorithm(std::vector<std::vector<float>> &map)
		{
			int gridSize = ((dimensions.x() + 1 < dimensions.z() + 1) ? dimensions.x() + 1 : dimensions.z() + 1) - 1;
			int offset = gridSize / 2;//offset is the width of the square we're working on
			octet::vec2 randomRange(-1.0f, 1.0f);
			float rangeModifier = 0.7f; //modifier on the range to smooth
			bool isOddY, isOddX; //these are used to tell if we're working with a side or center

			//set the four corners
			for (int i = 0; i < dimensions.z() + 1; i += gridSize)
			{
				for (int j = 0; j < dimensions.x() + 1; j += gridSize)
				{
					map[j][i] = rand.get(randomRange.x(), randomRange.y());
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
								height = (map[x - offset][y - offset] + map[x + offset][y - offset] + map[x - offset][y + offset] + map[x + offset][y + offset]) / 4 + rand.get(randomRange.x(), randomRange.y());
							}
							else
							{
								//side 
								if (isOddX)
								{
									//average horizontal sides corners plus small random amount (error)
									height = (map[x - offset][y] + map[x + offset][y]) / 2 + rand.get(randomRange.x(), randomRange.y());
								}
								else
								{
									//average this vertical side corners plus small random amount (error)
									height = (map[x][y - offset] + map[x][y + offset]) / 2 + rand.get(randomRange.x(), randomRange.y());
								}
							}

							//set the value
							map[x][y] = height;
						}
					}
				}

				//adjust the range and offset
				randomRange *= rangeModifier;
				offset /= 2;
			}
		}

		void DiamondSquareAlgorithm(std::vector<std::vector<float>> &vectorMap)
		{
			int gridSize = ((dimensions.x() + 1 < dimensions.z() + 1) ? dimensions.x() + 1 : dimensions.z() + 1) - 1;
			int sampleSize = gridSize;//offset is the width of the square we're working on
			float scale = 1.0f;
			octet::vec2 randomRange(-1.0f, 1.0f);
		

			float* map = new float[dimensions.x() * dimensions.z()];
			//randomise initial values
			for (int y = 0; y < dimensions.z() + 1; y += sampleSize)
			{
				for (int x = 0; x < dimensions.x() + 1; x += sampleSize)
				{
					SetSample(x, y, rand.get(randomRange.x(), randomRange.y()), map);
				}
			}		

			while (sampleSize > 1)
			{
				DiamondSquareCore(sampleSize, scale, randomRange, map);

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

		void DiamondSquareCore(int stepSize, float scale, octet::vec2 randomRange, float* map)
		{
			int halfStep = stepSize / 2;

			for (int y = halfStep; y < dimensions.z() + 1 + halfStep; y += stepSize)
			{
				for (int x = halfStep; x < dimensions.x() + 1 + halfStep; x += stepSize)
				{
					SampleSquare(x, y, stepSize, rand.get(randomRange.x(), randomRange.y()) * scale, map);
				}
			}

			for (int y = 0; y < dimensions.z() + 1; y += stepSize)
			{
				for (int x = 0; x < dimensions.x() + 1; x += stepSize)
				{
					SampleDiamond(x + halfStep, y, stepSize, rand.get(randomRange.x(), randomRange.y()) * scale, map);
					SampleDiamond(x, y + halfStep, stepSize, rand.get(randomRange.x(), randomRange.y()) * scale, map);
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
			PerlinNoiseGenerator noise;
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
			PerlinNoiseGenerator noise;
			
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
	};
}

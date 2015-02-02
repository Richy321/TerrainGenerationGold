#pragma once
#define PI_DIV_4 0.785398163f

#include <random>

using uint32 = unsigned int;

namespace Terrain
{
	class PerlinNoiseGenerator
	{
	private:
		float gradients[8][2];
		int permutations[256];
		std::mt19937 randomEngine{ std::random_device{}() };

		uint32 GetRandom(uint32 min, uint32 max) { return std::uniform_int_distribution<uint32>{min, max}(randomEngine); }

		static float lerp(float a0, float a1, float t)
		{
			return a0 + t * (a1 - a0);
		}

		static float fade(float t)
		{
			return t * t * t * (t * (t * 6 - 15) + 10);
		}

		static float dotProduct(float grad[], float x, float y)
		{
			return(grad[0] * x + grad[1] * y);
		}

	public:
		
		PerlinNoiseGenerator(std::mt19937::result_type seed) : randomEngine(seed)
		{
			PerlinNoiseGenerator();
		}

		PerlinNoiseGenerator()
		{
			//Create Gradient table
			//8 equally distributed angles around unit circle
			for (int i = 0; i < 8; i++)
			{
				gradients[i][0] = octet::cos(PI_DIV_4 * (float)i);
				gradients[i][1] = octet::sin(PI_DIV_4 * (float)i);
			}

			RandomisePermutations();
		}

		~PerlinNoiseGenerator()
		{

		}

		void RandomisePermutations()
		{
			//randomise numbers table
			for (int i = 0; i < 256; i++)
			{
				permutations[i] = i;
			}

			for (int i = 0; i < 256; i++)
			{
				int j = GetRandom(0, 255);
				int k = permutations[i];
				permutations[i] = permutations[j];
				permutations[j] = k;
			}
		}

		float GenerateNoise(float x, float y)
		{
			//Grid definition - get x,y indices relating to the on the grid
			int x0 = (x > 0.0 ? (int)x : (int)x - 1);
			int x1 = x0 + 1;
			int y0 = (y > 0.0 ? (int)y : (int)y - 1);
			int y1 = y0 + 1;

			//determine interpolation weights
			float fractionalX = x - (float)x0;
			float fractionalY = y - (float)y0;

			//Get gradient indices for 4 surrounding points (& with number will inclusively cap the number in that range)
			int grad11 = permutations[(x0 + permutations[y0 & 255]) & 255] & 7;
			int grad12 = permutations[(x1 + permutations[y0 & 255]) & 255] & 7;
			int grad21 = permutations[(x0 + permutations[y1 & 255]) & 255] & 7;
			int grad22 = permutations[(x1 + permutations[y1 & 255]) & 255] & 7;

			//Get noise from each corner
			float noise11 = dotProduct(gradients[grad11], fractionalX, fractionalY);
			float noise12 = dotProduct(gradients[grad12], fractionalX - 1.0f, fractionalY);
			float noise21 = dotProduct(gradients[grad21], fractionalX, fractionalY - 1.0f);
			float noise22 = dotProduct(gradients[grad22], fractionalX - 1.0f, fractionalY - 1.0f);

			//Get fade/interp values
			fractionalX = fade(fractionalX);
			fractionalY = fade(fractionalY);

			//Interpolate on the x axis
			float interpolatedX1 = lerp(noise11, noise12, fractionalX);
			float interpolatedX2 = lerp(noise21, noise22, fractionalX);

			float interpolatedXY = lerp(interpolatedX1, interpolatedX2, fractionalY);

			return interpolatedXY;
		}
	};
}

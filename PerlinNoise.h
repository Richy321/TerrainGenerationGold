#pragma once
class PerlinNoise
{
public:
	
	static float lerp(float a0, float a1, float w) 
	{
		return (1.0 - w)*a0 + w*a1;
	}

	float noise(float x, float y)
	{
		float rval;

		//Grid definition
		int x0 = (x > 0.0 ? (int)x : (int)x - 1);
		int x1 = x0 + 1;
		int y0 = (y > 0.0 ? (int)y : (int)y - 1);
		int y1 = y0 + 1;

		//Dot product

		//Interpolation




		return rval;
	}
};


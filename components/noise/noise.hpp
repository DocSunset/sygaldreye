#pragma once

namespace noise {

float perlin(float x, float y);
float perlin(float x, float y, float z);
float fbm(float x, float y, int octaves, float lacunarity, float gain);

}

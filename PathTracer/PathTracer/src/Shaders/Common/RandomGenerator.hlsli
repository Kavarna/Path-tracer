#ifndef _RANDOM_GENERATOR_HLSLI_
#define _RANDOM_GENERATOR_HLSLI_

#include "ConstantBuffers.hlsli"

struct RandomGenerator
{
    float2 seed;
    
    float GetRandomNumber()
    {
        seed -= cb0.randomVector.xy;
        return frac(sin(dot(seed, float2(12.9898f, 78.233f))) * 43758.5453);
    }
};

#endif // _RANDOM_GENERATOR_HLSLI_
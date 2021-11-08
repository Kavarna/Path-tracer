#ifndef _HIT_POINT_HLSLI_
#define _HIT_POINT_HLSLI_

#include "Ray.hlsli"

struct HitPoint
{
	float4 Color;
	float3 Position;
	float3 Normal;
    bool isLight;
	
    float4 emissiveColor;

    unsigned int hitMaterial;
};

HitPoint EmptyHitPoint()
{
	HitPoint hp;
	hp.Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	hp.Position = float3(0.0f, 0.0f, 0.0f);
	hp.Normal = float3(0.0f, 0.0f, 0.0f);
    hp.isLight = false;
    hp.emissiveColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	return hp;
}


#endif // _HIT_POINT_HLSLI_

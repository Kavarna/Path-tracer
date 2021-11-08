#ifndef _LINE_HLSLI_
#define _LINE_HLSLI_

#include "Ray.hlsli"
#include "HitPoint.hlsli"
#include "ConstantBuffers.hlsli"

static const float coPlanarThreshold = 100.f;
static const float lengthErrorThreshold = 1e-3;

struct Line
{
	float3 Start;
    float pad;
	float3 End;
    float pad2;
	float4 Color;
	
    // Was inefficient, so it got deleted
	bool Intersect(in Ray r, out float t)
	{
        return false;
    }
    
    void FillHitPoint(in Ray r, in float t, out HitPoint hp)
    {
        float bias = 1e-4;
        hp.Position = r.position + r.direction * (t - 0.1f);
        hp.Normal = float3(0.0f, 1.0f, 0.0f);
        hp.Color = Color;
        
        hp.emissiveColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
        hp.hitMaterial = 0;
    }
	
};


#endif // _LINE_HLSLI_
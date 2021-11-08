#ifndef _SPHERE_HLSLI_
#define _SPHERE_HLSLI_

#include "Ray.hlsli"
#include "HitPoint.hlsli"

struct Sphere {
	float3 Position;
	float Radius;
	float4 Color;
	float2 LightProperties;
	float2 pad;
	
	bool Intersect(in Ray r, out float2 t) {
		
		float3 l = Position - r.position;
		float tca = dot(l, r.direction);
		if (tca < 0) {
			return false;
		}
		float d2 = dot(l, l) - tca * tca;
		if (d2 > Radius * Radius) {
			return false;
		}
		
		float thc = sqrt(Radius * Radius - d2);
		t.x = tca - thc; // First intersesction point
		t.y = tca + thc; // Second intersection point
		
		return true;
	}
	
	void FillHitPoint(in Ray r, in float2 t, out HitPoint hp)
    {
        float bias = 1e-4;
        hp.Position = r.position + r.direction * t.x;
        hp.Normal = normalize(hp.Position - Position);
        hp.Color = Color;
        bool inside = dot(r.direction, hp.Normal) > 0;
        if (inside)
        {
            hp.Normal = -hp.Normal;
        }

        hp.emissiveColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
        hp.hitMaterial = 0;

    }
};


#endif // _SPHERE_HLSLI_
#ifndef _UTILS_HLSLI_
#define _UTILS_HLSLI_

#include "Ray.hlsli"
#include "../../Common/Limits.h"
#include "ConstantBuffers.hlsli"


float distanceSq(float pt1, float pt2 = 0.0f)
{
	float distance = pt1 - pt2;
	return dot(distance, distance);
}

float distanceSq(float2 pt1, float2 pt2 = float2(0.0f, 0.0f))
{
	float2 distance = pt1 - pt2;
	return dot(distance, distance);
}

float distanceSq(float3 pt1, float3 pt2 = float3(0.0f, 0.0f, 0.0f))
{
	float3 distance = pt1 - pt2;
	return dot(distance, distance);
}

float distanceSq(float4 pt1, float4 pt2 = float4(0.0f, 0.0f, 0.0f, 0.0f))
{
	float4 distance = pt1 - pt2;
	return dot(distance, distance);
}


bool IntersectAABB(in float3 minAABB, in float3 maxAABB, in Ray r, out float t)
{
    float3 invRayDir = 1.0f / r.direction;
	
    float3 f = (minAABB - r.position) * invRayDir;
    float3 n = (maxAABB - r.position) * invRayDir;
	
    float3 tmax = max(f, n);
    float3 tmin = min(f, n);
	
    float t0 = max(tmin.x, max(tmin.y, tmin.z));
    float t1 = min(tmax.x, min(tmax.y, tmax.z));
	
	[branch]
    if (t1 >= t0)
    {
        float tTemp = t0 > 0 ? t0 : t1;
        if (tTemp > 0.0f && tTemp < r.length)
        {
            t = tTemp;
            return true;
        }
        else
        {
            return false; // <- for some reason this DOES NOT WORK!
            // return tTemp > 0.0f;
        }
    }
    else
    {
        return false;
    }
}

bool IntersectTriangleSlow(in float3 A, in float3 B, in float3 C, in float3 Normal, in Ray r, out float t)
{
    if (abs(dot(r.direction, Normal)) <= EPSILON)
    {
        // Almost parralel
        return false;
    }
    
    float D = dot(Normal, A);
    
    float tTemp = -((dot(Normal, r.position) + D) / dot(Normal, r.direction));
    if (tTemp < 0.0f)
    {
        return false;
    }
    
    float3 hitPosition = r.position + tTemp * r.direction;
    float3 crossProduct;
    
    float3 AB = B - A;
    float3 AP = A - hitPosition;
    crossProduct = cross(AB, AP);
    if (dot(Normal, crossProduct) < 0.0f)
        return false;
    
    float3 CA = A - C;
    float3 CP = C - hitPosition;
    crossProduct = cross(CA, CP);
    if (dot(Normal, crossProduct) < 0.0f)
        return false;
    
    float3 BC = C - B;
    float3 BP = B - hitPosition;
    crossProduct = cross(BC, BP);
    if (dot(Normal, crossProduct) < 0.0f)
        return false;
    
    
    t = tTemp;
    return true;
}

bool IntersectTriangleFast(in float3 A, in float3 B, in float3 C, in Ray r, out float t, out float u, out float v)
{
    float3 v0v1 = B - A;
    float3 v0v2 = C - A;
    
    float3 P = cross(r.direction, v0v2);
    float det = dot(v0v1, P);
    
    if (abs(det) <= EPSILON)
        return false;
    
    float invDet = 1.0f / det;
    
    u = invDet * dot(r.position - A, P);
    
    v = invDet * dot(r.direction, cross(r.position - A, v0v1));
    
    
    if (u < 0.0f || u > 1.0f || v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    float tTemp =  invDet * dot(cross(r.position - A, v0v1), v0v2);
    if (tTemp > 0.f && tTemp < r.length)
    {
        t = tTemp;
        return true;
    }
    
    return false;
}

float4 GetColorFromTextureByIndex(Texture2D tex, in unsigned int index)
{
    // index = 27696 => col = 11312; row = 1
    return tex.Load(int3(index % (MAX_TEXTURE_COLUMNS), index / (MAX_TEXTURE_COLUMNS), 0));
}


float GTR2(float NH, float a)
{
    float t = 1.0f + (a * a - 1.0f) * NH * NH;
    return (a * a) / (PI * t * t);
}

float4 SchlickFresnel(float VH, float4 f0)
{
    return f0 + (1.0f - f0) * pow(1.0f - VH, 5.0f);
}

float SmithG_GGX(float NV, float alpha)
{
    float a = alpha * alpha;
    float b = NV * NV;
    return 1.0 / (NV + sqrt(a + b - a * b));
}

float SchlickFresnel(float u)
{
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m;
}

float4 LambertBRDF(float4 diffuse)
{
    return diffuse / PI;
}

float SchlickGGX(float NL, float NV, float roughness)
{
    float k = roughness + 1.0;
    k *= k * 0.125;
    float gl = NL / (NL * (1.0 - k) + k);
    float gv = NV / (NV * (1.0 - k) + k);
    
    return gl * gv;
}

float GGX(float NH, float roughness)
{
    float a4 = roughness * roughness;
    a4 *= a4;
    float denom = NH * NH * (a4 - 1.0) + 1.0;
    return a4 / (PI * denom * denom);
}

float4 CookTorraceBRDF(float NL, float NV, float NH, float VH, float4 F, float roughness)
{
    float4 DFG = GGX(NH, roughness) * F * SchlickGGX(NL, NV, roughness);
    float denom = 4.0 * NL * NV + 0.0001;
    return DFG / denom;
}

void CreateLocalCoordinateSystem(in float3 N, out float3 Nt, out float3 Nb)
{
    if (abs(N.x) > abs(N.y))
    {
        Nt = float3(N.z, 0.0f, -N.x);
    }
    else
    {
        Nt = float3(0.0f, -N.z, N.y);
    }
    Nt = normalize(Nt);
    
    Nb = cross(N, Nt);
}

float3 SampleDirectionFromHemisphere(float r1, float r2)
{
    float sinTheta = sqrt(1.0f - r1 * r1);
    float phi = 2 * PI * r2;
    float x = sinTheta * cos(phi);
    float z = sinTheta * sin(phi);
    
    return float3(x, r1, z);
}

float3 GetDirectionFromCoordinateSystemAndDirection(float3 N, float3 Nt, float3 Nb, float3 direction)
{
    return Nt * direction.x + N * direction.y + Nb * direction.z;
}


#endif // _UTILS_HLSLI_

#ifndef _CONSTANT_BUFFERS_HLSLI_
#define _CONSTANT_BUFFERS_HLSLI_

static const float EPSILON = 1e-5f;
#define FLT_MAX 3.40282e+38 
#define PI 3.14159265359f

#include "Sphere.hlsli"
#include "Line.hlsli"
#include "../../Common/Limits.h"

struct Light
{
    float3 Position;
    float Radius;
    float4 Emissive; // Should include intensity
	
    void Illuminate(in float3 p, in float2 jitter, out float3 lightDir, out float4 lightIntensity, out float distance)
    {
        // lightDir = p - Position;
        
        
        // ---------   <- LightSource
        // \   |   /
        //  \  |  /
        //   \ | /
        //    \|/ <- HitPoint
        // Generate a random direction inside the cone between HitPoint and LightSource
        float3 lightDirection = Position - p;
        float3 u = cross(float3(0.0f, 1.0f, 0.0f), lightDirection);
        float3 v = cross(lightDirection, u);
        
        float r1 = jitter.x, r2 = jitter.y;
        float3 newPoint = Position + r1 * Radius * u + r2 * Radius * v;
        
        lightDir = p - newPoint;
        
        distance = length(lightDir);
        lightDir = normalize(lightDir);
		
        lightIntensity = Emissive / (4 * PI * distance * distance);
    }
	
    bool Intersect(in Ray r, out float2 t)
    {
        float3 l = Position - r.position;
        float tca = dot(l, r.direction);
        if (tca < 0)
        {
            return false;
        }
        float d2 = dot(l, l) - tca * tca;
        if (d2 > Radius * Radius)
        {
            return false;
        }
		
        float thc = sqrt(Radius * Radius - d2);
        t.x = tca - thc; // First intersesction point
        t.y = tca + thc; // Second intersection point
		
        return true;
    }
};

struct RayTraceLowResCB
{
	float2 textureDimensions;
	unsigned int hasSkybox;
    unsigned int isCameraMoving;
	
    float3 randomVector;
    float gamma;
	
    unsigned int numPasses;
    unsigned int depth;
};

struct CameraCB
{
	float3 position;
	float focalDist;
	float3 direction;
	float fov;
	float3 right;
	float pad1;
	float3 up;
	float pad2;
};

struct SpheresCB
{
	Sphere spheres[MAX_SPHERES];
	unsigned int numSpheres;
	float3 pad;
};

struct LinesCB
{
	Line lines[MAX_LINES];
	unsigned int numLines;
	float3 pad;
};

struct LightsCB
{
    Light lights[MAX_LIGHTS];
    unsigned int numLights;
    float3 pad;
};

ConstantBuffer<RayTraceLowResCB> cb0 : register(b0);
ConstantBuffer<CameraCB> cb1 : register(b1);

ConstantBuffer<SpheresCB> cb2 : register(b2);
ConstantBuffer<LinesCB> cb3 : register(b3);
ConstantBuffer<LightsCB> cb4 : register(b4);

TextureCube skyboxTexture : register(t0);
Texture2D SceneTree : register(t1);
Texture2D ModelsTrees : register(t2);
Texture2D ScenePrimitives : register(t3);
Texture2D ModelsPrimitives : register(t4);

Texture2D VertexBuffer : register(t5);
Texture2D Materials : register(t6);
Texture2DArray Textures : register(t7);

RWTexture2D<float4> OutputTexture : register(u0);

SamplerState linearClampSampler : register(s0);



#endif // _CONSTANT_BUFFERS_HLSLI_

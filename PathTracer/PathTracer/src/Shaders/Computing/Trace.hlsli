#ifndef _TRACE_HLSLI_
#define _TRACE_HLSLI_

#include "../Common/Ray.hlsli"
#include "../Common/Sphere.hlsli"
#include "../Common/Line.hlsli"
#include "../Common/HitPoint.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/Utils.hlsli"
#include "../Common/BVHTreeNode.hlsli"
#include "../Common/RandomGenerator.hlsli"

bool ClosestHitSphere(in Ray r, out HitPoint hp)
{
    float2 closestHit = FLT_MAX;
    int objectIndex = -1;
	
    for (int i = 0; i < cb2.numSpheres; ++i)
    {
        float2 t;
        if (cb2.spheres[i].Intersect(r, t))
        {
            if (t.x < closestHit.x && t.x <= r.length && t.x > 0.0f && t.y > 0.0f)
            {
                closestHit = t.x;
                objectIndex = i;
            }
        }
    }
	
    if (objectIndex != -1)
    {
        cb2.spheres[objectIndex].FillHitPoint(r, closestHit, hp);
        return true;
    }
    return false;
}

bool ClosestHitLine(in Ray r, out HitPoint hp)
{
    float closestHit = FLT_MAX;
    int objectIndex = -1;
	
    for (int i = 0; i < cb3.numLines; ++i)
    {
        float t;
		if (cb3.lines[i].Intersect(r, t))
        {
            if (t.x < closestHit && t.x < r.length)
            {
                closestHit = t.x;
                objectIndex = i;
            }
        }
    }
	
    if (objectIndex != -1)
    {
        cb3.lines[objectIndex].FillHitPoint(r, closestHit, hp);
        return true;
    }
    return false;
}

bool IntersectLight(in Ray r, out HitPoint hp)
{
    float closestHit = FLT_MAX;
    int lightIndex = -1;
    for (int i = 0; i < cb4.numLights; ++i)
    {
        float2 t;
        if (cb4.lights[i].Intersect(r, t))
        {
            if (t.x < closestHit.x && t.x <= r.length && t.x > 0.0f)
            {
                closestHit = t.x;
                lightIndex = i;
            }
        }
    }
    
    if (lightIndex != -1)
    {
        float maxChannel = max(cb4.lights[lightIndex].Emissive.x, max(cb4.lights[lightIndex].Emissive.y, max(cb4.lights[lightIndex].Emissive.z, cb4.lights[lightIndex].Emissive.w)));
        hp.Color = cb4.lights[lightIndex].Emissive / maxChannel;
        hp.isLight = true;
        return true;
    }

    return false;
}

bool ClosestHit(in Ray r, out HitPoint hp)
{
    HitPoint bestHitPoint = EmptyHitPoint(), currentHitpoint = EmptyHitPoint();
    bool found = false;
    
    if (IntersectScene(r, currentHitpoint))
    {
        if (!found || distance(r.position, currentHitpoint.Position) < distance(r.position, bestHitPoint.Position))
        {
            bestHitPoint = currentHitpoint;
        }
        found = true;
    }
    // else
    {
        if (ClosestHitSphere(r, currentHitpoint))
        {
            if (!found || distance(r.position, currentHitpoint.Position) < distance(r.position, bestHitPoint.Position))
            {
                bestHitPoint = currentHitpoint;
            }
            found = true;
        }
    
        if (ClosestHitLine(r, currentHitpoint))
        {
            if (!found || distance(r.position, currentHitpoint.Position) < distance(r.position, bestHitPoint.Position))
            {
                bestHitPoint = currentHitpoint;
            }
            found = true;
        }
    }
    
    hp = bestHitPoint;
    return found;
}


float4 RayTrace(in Ray r) {
	
	float4 finalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	HitPoint hp = EmptyHitPoint();
	Ray currentRay = r;
	if (ClosestHit(currentRay, hp))
	{
		// float3 sunDir = normalize(float3(-0.69f, 0.1f, -0.71f));
        // float3 sunDir = normalize(float3(-1.0f, 1.0f, 1.0f));
        float3 sunDir = normalize(float3(0.0f, 1.0f, 1.0f));
        // float3 sunDir = normalize(float3(1.0f, 0.3f, 0.0f));
        // float3 sunDir = normalize(float3(0.3f, 1.0f, 0.0f));
		Ray toLightRay;
        toLightRay.position = hp.Position + hp.Normal * 0.0009f;
		toLightRay.direction = sunDir;
        toLightRay.length = MAXIMUM_RAY_LENGTH;
		float lightIntensity = 1.0f;
		HitPoint lightHp = EmptyHitPoint();
        if (ClosestHit(toLightRay, lightHp))
        {
            lightIntensity = 0.1f;
        }
        float sunLight = dot(hp.Normal, sunDir);
        //float sunLight = 1.0f;
        //lightIntensity = 1.0f;
        finalColor += hp.Color * lightIntensity + float4(0.1f, 0.1f, 0.1f, 0.1f);
    }
	else
	{
		if (cb0.hasSkybox)
		{
			finalColor += skyboxTexture.SampleLevel(linearClampSampler, currentRay.direction.xyz, 0);
		}
		else
		{
			finalColor += float4(0.2f, 0.2f, 0.2f, 1.0f);
		}
	}
	return finalColor;
}

bool ClosestHitEx(in Ray r, out HitPoint hp)
{
    HitPoint bestHitPoint = EmptyHitPoint(), currentHitpoint = EmptyHitPoint();
    bool found = false;
    bestHitPoint.isLight = false;
    
    if (IntersectLight(r, currentHitpoint))
    {
        if (!found || distance(r.position, currentHitpoint.Position) < distance(r.position, bestHitPoint.Position))
        {
            bestHitPoint = currentHitpoint;
            bestHitPoint.isLight = true;
        }
        found = true;
    }
    
    if (IntersectScene(r, currentHitpoint))
    {
        if (!found || distance(r.position, currentHitpoint.Position) < distance(r.position, bestHitPoint.Position))
        {
            bestHitPoint = currentHitpoint;
            bestHitPoint.isLight = false;
        }
        found = true;
    }
    // else
    {
        if (ClosestHitSphere(r, currentHitpoint))
        {
            if (!found || distance(r.position, currentHitpoint.Position) < distance(r.position, bestHitPoint.Position))
            {
                bestHitPoint = currentHitpoint;
                bestHitPoint.isLight = false;
            }
            found = true;
        }
    
        if (ClosestHitLine(r, currentHitpoint))
        {
            if (!found || distance(r.position, currentHitpoint.Position) < distance(r.position, bestHitPoint.Position))
            {
                bestHitPoint = currentHitpoint;
                bestHitPoint.isLight = false;
            }
            found = true;
        }
    }
    
    hp = bestHitPoint;
    return found;
}

float4 GetDirectLight(float3 position, float3 normal, RandomGenerator rg)
{
    float4 directLighting = 0.0f;
    
    for (unsigned int i = 0; i < cb4.numLights; ++i)
    {
        float3 lightDir;
        float4 lightIntensity;
        float distance;
        float2 jitter = float2(2.0f * rg.GetRandomNumber() - 1, 2.0f * rg.GetRandomNumber() - 1);
        cb4.lights[i].Illuminate(position, jitter, lightDir, lightIntensity, distance);

        Ray toLightRay;
        toLightRay.position = position + normal * EPSILON;
        toLightRay.direction = -lightDir;
        toLightRay.length = MAXIMUM_RAY_LENGTH;

        HitPoint hp = EmptyHitPoint();
        bool inShadow = ClosestHit(toLightRay, hp); // TODO: Replace this with an intersection function that stops after first intersection
        
        directLighting += (1.0f - inShadow) * max(0.0f, dot(normal, -lightDir)) * lightIntensity;
    }
    
    return directLighting;
}

float4 PathTrace(in Ray r, in RandomGenerator rg)
{
    float4 radiance = 0.0f, throughput = 1.0f;
    HitPoint hp = EmptyHitPoint();
    Ray currentRay = r;
    for (unsigned int i = 0; i < cb0.depth; ++i)
    {
        
        float throughputValue = max(max(throughput.x, max(throughput.y, throughput.z)), 0.001f);
        if (rg.GetRandomNumber() > throughputValue)
        {
            break;
        }
        else
        {
            throughput *= 1.0f / throughputValue;
        }
        
        if (ClosestHitEx(currentRay, hp))
        {
            if (hp.isLight)
            {
                radiance += hp.Color * throughput;
                break;
            }
            else
            {
                
                Material m = GetMaterialByIndex(hp.hitMaterial);
                
                float3 newDirection = m.GetMaterialSample(r, hp, rg);
                
                float4 directLight = GetDirectLight(hp.Position, hp.Normal, rg);
                float pdf = m.GetMaterialPDF(r, hp, newDirection);
                float4 materialEval = m.GetMaterialEval(r, hp, newDirection);
                
                // float4 L = materialEval * pdf * directLight;
                // float4 L = float4(newDirection, 1.0f);
                float4 L = directLight * hp.Color;
                radiance += L * throughput;
                
                
                if (pdf > 0.0f)
                {
                    throughput *= m.GetMaterialEval(r, hp, newDirection) * max(0.0f, dot(hp.Normal, newDirection));
                }
                else
                {
                    break;
                }

                currentRay.position = hp.Position + newDirection * EPSILON;
                currentRay.direction = newDirection;
                currentRay.length = MAXIMUM_RAY_LENGTH;
                
                
            }
        }   
        else
        {
            if (cb0.hasSkybox)
            {
                radiance += skyboxTexture.SampleLevel(linearClampSampler, currentRay.direction.xyz, 0) * throughput;
            }
            else
            {
                radiance += float4(0.0f, 0.0f, 0.0f, 1.0f) * throughput;
            }
            break;
        }
    }
    return radiance;
}

#endif // _TRACE_HLSLI_

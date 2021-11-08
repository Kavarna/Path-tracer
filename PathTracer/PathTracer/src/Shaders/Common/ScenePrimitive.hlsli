
#ifndef _SCENE_PRIMITIVE_HLSLI_
#define _SCENE_PRIMITIVE_HLSLI_

#include "Utils.hlsli"

struct ScenePrimitive
{
    float3 minAABB;
    unsigned int modelOffset;
    float3 maxAABB;
    float pad1;
};

ScenePrimitive EmptyScenePrimitive()
{
    ScenePrimitive sp;
    
    sp.minAABB = float3(0.0f, 0.0f, 0.0f);
    sp.maxAABB = float3(0.0f, 0.0f, 0.0f);
    sp.modelOffset = 0;
    sp.pad1 = 0.f;
    
    return sp;
}

ScenePrimitive GetScenePrimitive(in int index)
{
    ScenePrimitive sp;
    
    int offset = index * sizeof(ScenePrimitive) / sizeof(float4);
    // float4 firstRead = ScenePrimitives.Load(int3(offset, 0, 0));
    float4 firstRead = GetColorFromTextureByIndex(ScenePrimitives, offset);
    // float4 secondRead = ScenePrimitives.Load(int3(offset + 1, 0, 0));
    float4 secondRead = GetColorFromTextureByIndex(ScenePrimitives, offset + 1);
    
    sp.minAABB = firstRead.xyz;
    sp.modelOffset = asuint(firstRead.w);
    sp.maxAABB = secondRead.xyz;
    
    return sp;
}

#endif

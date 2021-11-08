#ifndef _MODEL_PRIMITIVE_HLSLI_
#define _MODEL_PRIMITIVE_HLSLI_

#include "Utils.hlsli"

struct ModelPrimitive
{
    unsigned int index0;
    unsigned int index1;
    unsigned int index2;
    float pad0;
};

ModelPrimitive EmptyModelPrimitive()
{
    ModelPrimitive mp;
    
    mp.index0 = 0;
    mp.index1 = 0;
    mp.index2 = 0;
    mp.pad0 = 0.f;
    
    return mp;
}

ModelPrimitive GetModelPrimitive(in int index)
{
    ModelPrimitive mp = EmptyModelPrimitive();
    
    int offset = index * sizeof(ModelPrimitive) / sizeof(float4);
    // float4 firstRead = ModelsPrimitives.Load(int3(offset, 0, 0));
    float4 firstRead = GetColorFromTextureByIndex(ModelsPrimitives, offset);
    
    mp.index0 = asuint(firstRead.x);
    mp.index1 = asuint(firstRead.y);
    mp.index2 = asuint(firstRead.z);
    
    return mp;
}

#endif
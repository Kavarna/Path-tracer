#ifndef _VERTEX_HLSLI_
#define _VERTEX_HLSLI_

#include "ConstantBuffers.hlsli"
#include "Utils.hlsli"

struct Vertex
{
    float3 position;
    float pad;
    float3 normal;
    unsigned int materialIndex;
    float4 texCoords;
};

Vertex EmptyVertex()
{
    Vertex v;
    v.position = float3(0.f, 0.f, 0.f);
    v.pad = 0.f;
    v.normal = float3(0.f, 0.f, 0.f);
    v.materialIndex = 0;
    v.texCoords = float4(0.f, 0.f, 0.f, 0.f);
    return v;
}

Vertex GetVertex(in int index)
{
    Vertex v = EmptyVertex();
    
    int offset = index * sizeof(Vertex) / sizeof(float4);
    float4 firstRead = GetColorFromTextureByIndex(VertexBuffer, offset);
    float4 secondRead = GetColorFromTextureByIndex(VertexBuffer, offset + 1);
    float4 thirdRead = GetColorFromTextureByIndex(VertexBuffer, offset + 2);
    
    v.position = firstRead.xyz;
    v.normal = secondRead.xyz;
    v.materialIndex = asuint(secondRead.w);
    v.texCoords = thirdRead.xyzw;
    
    return v;
}

#endif
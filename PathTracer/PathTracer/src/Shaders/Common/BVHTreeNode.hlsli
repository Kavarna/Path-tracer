#ifndef _BVH_TREE_NODE_HLSLI_
#define _BVH_TREE_NODE_HLSLI_

#include "Utils.hlsli"
#include "ConstantBuffers.hlsli"
#include "ScenePrimitive.hlsli"
#include "SceneHitPoint.hlsli"
#include "ModelPrimitive.hlsli"
#include "Vertex.hlsli"
#include "HitPoint.hlsli"
#include "Material.hlsli"

struct BVHTreeNode
{
    float3 minAABB;
    unsigned int primitiveOffset;
	
    float3 maxAABB;
    unsigned int secondChildOffset;
	
    unsigned int numberOfPrimitives;
    unsigned int axis;
    unsigned int nodeType;
    unsigned int additionalInfo;
};

BVHTreeNode EmptyBVHTreeNode()
{
    BVHTreeNode tn;
    tn.minAABB = float3(0.0f, 0.0f, 0.0f);
    tn.primitiveOffset = 0;
    
    tn.maxAABB = float3(0.0f, 0.0f, 0.0f);
    tn.secondChildOffset = 0;
    
    tn.numberOfPrimitives = 0;
    tn.axis = 0;
    tn.nodeType = 0;
    tn.additionalInfo = 0;
    
    return tn;
}

BVHTreeNode GetNodeFromTexture(in Texture2D tex, int index)
{
    BVHTreeNode node;
    
    // I don't really know a better method than this
    int offset = index * sizeof(BVHTreeNode) / sizeof(float4);
    //float4 firstRead = tex.Load(int3(offset, 0, 0));
    float4 firstRead = GetColorFromTextureByIndex(tex, offset);
    //float4 secondRead = tex.Load(int3(offset + 1, 0, 0));
    float4 secondRead = GetColorFromTextureByIndex(tex, offset + 1);
    //float4 thirdRead = tex.Load(int3(offset + 2, 0, 0));
    float4 thirdRead = GetColorFromTextureByIndex(tex, offset + 2);

    node.minAABB = firstRead.xyz;
    node.primitiveOffset = asuint(firstRead.w);
    
    node.maxAABB = secondRead.xyz;
    node.secondChildOffset = asuint(secondRead.w);
    
    node.numberOfPrimitives = asuint(thirdRead.x);
    node.axis = asuint(thirdRead.y);
    node.nodeType = asuint(thirdRead.z);
    node.additionalInfo = asuint(thirdRead.w);
    
    // Maybe add a fail-case?
    return node;
}

bool IntersectSceneNode(in int currentOffset, in Ray r, out SceneHitPoint shp)
{
    int dirIsNeg[3] = { r.direction.x < 0, r.direction.y < 0, r.direction.z < 0 };
    bool hit = false;
    int stackIndex = 0;
    int stack[64];
    
    while (true)
    {
        BVHTreeNode currentNode = GetNodeFromTexture(SceneTree, currentOffset);
        float tIntersectNode;
        
        [branch]
        if (IntersectAABB(currentNode.minAABB, currentNode.maxAABB, r, tIntersectNode))
        {
            [branch]
            if (currentNode.numberOfPrimitives > 0)
            {
                for (int i = 0; i < currentNode.numberOfPrimitives; ++i)
                {
                    ScenePrimitive sp = GetScenePrimitive(currentNode.primitiveOffset + i);
                    if (IntersectAABB(sp.minAABB, sp.maxAABB, r, r.length))
                    {
                        shp.sceneNodeIndex = currentOffset;
                        shp.t = r.length;
                        shp.modelOffset = sp.modelOffset;
                        hit = true;
                    }
                }
                if (stackIndex == 0)
                    break;
                currentOffset = stack[--stackIndex];
            }
            else
            {
                [flatten]
                if (dirIsNeg[currentNode.axis])
                {
                    stack[stackIndex++] = currentOffset + 1;
                    currentOffset = currentNode.secondChildOffset;
                }
                else
                {
                    stack[stackIndex++] = currentNode.secondChildOffset;
                    currentOffset = currentOffset + 1;
                }
            }
        }
        else
        {
            if (stackIndex == 0)
                break;
            currentOffset = stack[--stackIndex];
        }
    }
    
    return hit;
}

bool IntersectModelNode(in int currentOffset, inout Ray r, out HitPoint hp)
{
    int dirIsNeg[3] = { r.direction.x < 0, r.direction.y < 0, r.direction.z < 0 };
    bool hit = false;
    int stackIndex = 0;
    int stack[64];
    
    while (true)
    {
        BVHTreeNode currentNode = GetNodeFromTexture(ModelsTrees, currentOffset);
        float tIntersectNode;
        
        [branch]
        if (IntersectAABB(currentNode.minAABB, currentNode.maxAABB, r, tIntersectNode))
        {
            [branch]
            if (currentNode.numberOfPrimitives > 0)
            {
                for (int i = 0; i < currentNode.numberOfPrimitives; ++i)
                {
                    ModelPrimitive mp = GetModelPrimitive(currentNode.primitiveOffset + i);
                    Vertex primitiveFace[3] =
                    {
                        GetVertex(mp.index0),
                        GetVertex(mp.index1),
                        GetVertex(mp.index2)
                    };
                    float t, u, v;
                    if (IntersectTriangleFast(primitiveFace[0].position, primitiveFace[1].position, primitiveFace[2].position, r, t, u, v))
                    {
                        r.length = t;
                        hp.Position = r.position + r.direction * r.length;
                        // hp.Position = u * primitiveFace[0].position + v * primitiveFace[1].position + (1 - u - v) * primitiveFace[2].position;
                        hp.Normal = u * primitiveFace[1].normal + v * primitiveFace[2].normal + (1 - u - v) * primitiveFace[0].normal;
                        hp.Normal = normalize(hp.Normal);
                        
                        unsigned int materialIndex = primitiveFace[0].materialIndex; // Should be the same for all vertices in triangle
                        Material m = GetMaterialByIndex(materialIndex);
                        if (m.textureIndex == -1)
                        {
                            hp.Color = m.diffuseColor;
                        }
                        else
                        {
                            float2 texCoords = u * primitiveFace[1].texCoords.xy + v * primitiveFace[2].texCoords.xy + (1 - u - v) * primitiveFace[0].texCoords.xy;
                            hp.Color = Textures.SampleLevel(linearClampSampler, float3(texCoords, m.textureIndex), 0.f);
                            
                            // hp.Color = float4(m.textureIndex, m.textureIndex, 1.0f, 1.0f);
                            // hp.Color = float4(m.textureIndex, m.textureIndex, 1.0f, 1.0f);
                        }
                        hp.emissiveColor = m.emissiveColor;
                        hp.hitMaterial = materialIndex;
                        
                        
                        // hp.Position += hp.Normal * 0.00008f;
                        hit = true;
                    }
                }
                if (stackIndex == 0)
                    break;
                currentOffset = stack[--stackIndex];
            }
            else
            {
                [flatten]
                if (dirIsNeg[currentNode.axis])
                {
                    stack[stackIndex++] = currentOffset + 1;
                    currentOffset = currentNode.secondChildOffset;
                }
                else
                {
                    stack[stackIndex++] = currentNode.secondChildOffset;
                    currentOffset = currentOffset + 1;
                }
            }
        }
        else
        {
            if (stackIndex == 0)
                break;
            currentOffset = stack[--stackIndex];
        }
    }
    
    return hit;
}


bool IntersectScene(in Ray r, out HitPoint hp)
{
    int dirIsNeg[3] = { r.direction.x < 0, r.direction.y < 0, r.direction.z < 0 };
    bool hit = false;
    int stackIndex = 0;
    int stack[64];
    int currentOffset = 0;
    
    Ray originalRay;
    originalRay.direction = r.direction;
    originalRay.position = r.position;
    originalRay.length = r.length;
    
    while (true)
    {
        BVHTreeNode currentNode = GetNodeFromTexture(SceneTree, currentOffset);
        float tIntersectNode;
        
        [branch]
        if (IntersectAABB(currentNode.minAABB, currentNode.maxAABB, r, tIntersectNode))
        {
            [branch]
            if (currentNode.numberOfPrimitives > 0)
            {
                for (int i = 0; i < currentNode.numberOfPrimitives; ++i)
                {
                    ScenePrimitive sp = GetScenePrimitive(currentNode.primitiveOffset + i);
                    float t;
                    if (IntersectAABB(sp.minAABB, sp.maxAABB, r, t) && IntersectModelNode(sp.modelOffset, originalRay, hp))
                    {
                        r.length *= t;
                        hit = true;
                    }
                }
                if (stackIndex == 0)
                    break;
                currentOffset = stack[--stackIndex];
            }
            else
            {
                [flatten]
                if (dirIsNeg[currentNode.axis])
                {
                    stack[stackIndex++] = currentOffset + 1;
                    currentOffset = currentNode.secondChildOffset;
                }
                else
                {
                    stack[stackIndex++] = currentNode.secondChildOffset;
                    currentOffset = currentOffset + 1;
                }
            }
        }
        else
        {
            if (stackIndex == 0)
                break;
            currentOffset = stack[--stackIndex];
        }
    }
    
    return hit;
}

bool IntersectAny(in Ray r, out HitPoint hp)
{
    int dirIsNeg[3] = { r.direction.x < 0, r.direction.y < 0, r.direction.z < 0 };
    bool hit = false;
    int stackIndex = 0;
    int stack[64];
    int currentOffset = 0;
    
    Ray originalRay;
    originalRay.direction = r.direction;
    originalRay.position = r.position;
    originalRay.length = r.length;
    
    while (true)
    {
        BVHTreeNode currentNode = GetNodeFromTexture(SceneTree, currentOffset);
        float tIntersectNode;
        
        [branch]
        if (IntersectAABB(currentNode.minAABB, currentNode.maxAABB, r, tIntersectNode))
        {
            [branch]
            if (currentNode.numberOfPrimitives > 0)
            {
                for (int i = 0; i < currentNode.numberOfPrimitives; ++i)
                {
                    ScenePrimitive sp = GetScenePrimitive(currentNode.primitiveOffset + i);
                    float t;
                    if (IntersectAABB(sp.minAABB, sp.maxAABB, r, t) && IntersectModelNode(sp.modelOffset, originalRay, hp))
                    {
                        r.length *= t;
                        return true;
                    }
                }
                if (stackIndex == 0)
                    break;
                currentOffset = stack[--stackIndex];
            }
            else
            {
                [flatten]
                if (dirIsNeg[currentNode.axis])
                {
                    stack[stackIndex++] = currentOffset + 1;
                    currentOffset = currentNode.secondChildOffset;
                }
                else
                {
                    stack[stackIndex++] = currentNode.secondChildOffset;
                    currentOffset = currentOffset + 1;
                }
            }
        }
        else
        {
            if (stackIndex == 0)
                break;
            currentOffset = stack[--stackIndex];
        }
    }
    
    return hit;
}

#endif

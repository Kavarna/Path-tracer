

#include "../Common/Ray.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/BVHTreeNode.hlsli"
#include "Trace.hlsli"

struct ComputeShaderInput
{
    uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
    uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

#define RayTraceLowRes_RootSignature \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 3), "\
    "DescriptorTable(UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1), CBV(b1, numDescriptors = 3), SRV(t1, numDescriptors = 7))," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

[RootSignature(RayTraceLowRes_RootSignature)]
[numthreads(32, 32, 1)]
void main(ComputeShaderInput IN)
{
	float2 coords = IN.DispatchThreadID.xy / cb0.textureDimensions;
	coords = 2.0f * coords - 1.0f;
	
	float2 d = coords;
	float scale = tan(radians(cb1.fov) * 0.5f);
	d.y *= cb0.textureDimensions.y / cb0.textureDimensions.x * scale;
	d.x *= scale;
	float3 direction = normalize(d.x * cb1.right + d.y * cb1.up + cb1.direction);
	float3 focalPoint = cb1.focalDist * direction;
	
	Ray currentRay;
	currentRay.position = cb1.position;
	currentRay.direction = normalize(focalPoint);
    currentRay.length = MAXIMUM_RAY_LENGTH;
	 
	OutputTexture[IN.DispatchThreadID.xy] = RayTrace(currentRay);
    
    //Material newMaterial = GetMaterialByIndex(1);
    //if (newMaterial.textureIndex == -1)
    //{
    //    OutputTexture[IN.DispatchThreadID.xy] = newMaterial.diffuseColor;
    //}
    //else
    //{
    //    OutputTexture[IN.DispatchThreadID.xy] = Textures.Load(int4(IN.DispatchThreadID.xy, newMaterial.textureIndex, 0));
    //}
    // OutputTexture[IN.DispatchThreadID.xy] = Textures.Load(int4(IN.DispatchThreadID.xy, 0, 0));
    // float2 uvCoords = IN.DispatchThreadID.xy / cb0.textureDimensions;
    // OutputTexture[IN.DispatchThreadID.xy] = Textures.SampleLevel(linearClampSampler, float3(uvCoords, 0.0f), 0.f);
	
    //BVHTreeNode node = GetNodeFromTexture(ModelsTrees, 5461);
    //OutputTexture[IN.DispatchThreadID.xy] = float4(node.maxAABB, 1.0f);
    
    //Vertex v = GetVertex(5461);
    //v.normal.x *= -1;
    //OutputTexture[IN.DispatchThreadID.xy] = float4(v.normal, 1.0f);
    
    // BVHTreeNode node = GetNodeFromTexture(SceneTree, 1);
    //OutputTexture[IN.DispatchThreadID.xy] = float4(asfloat(node.numberOfPrimitives), asfloat(node.axis), asfloat(node.nodeType), asfloat(node.additionalInfo));
    
    //SceneHitPoint shp = EmptySceneHitPoint();
    //if (IntersectSceneNode(0, currentRay, shp))
    //{
    //    HitPoint triangleHp = EmptyHitPoint();
    //    if (IntersectModelNode(shp.modelOffset, currentRay, triangleHp))
    //    {
    //        OutputTexture[IN.DispatchThreadID.xy] = triangleHp.Color;
    //    }
    //    else
    //    {
    //        OutputTexture[IN.DispatchThreadID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    //    }
    //    // OutputTexture[IN.DispatchThreadID.xy] = float4(1.0f, 1.0f, 0.0f, 1.0f);
    //}
    //else
    //{
    //    OutputTexture[IN.DispatchThreadID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    //}
    
    //HitPoint triangleHp = EmptyHitPoint();
    //if (IntersectScene(currentRay, triangleHp))
    //{
    //    OutputTexture[IN.DispatchThreadID.xy] = triangleHp.Color;
    //}

}

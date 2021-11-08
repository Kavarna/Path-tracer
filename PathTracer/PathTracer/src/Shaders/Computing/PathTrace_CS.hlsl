
#include "../Common/Ray.hlsli"
#include "../Common/ConstantBuffers.hlsli"
#include "../Common/BVHTreeNode.hlsli"
#include "../Common/RandomGenerator.hlsli"
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
    "DescriptorTable(UAV(u0, numDescriptors = 1), SRV(t0, numDescriptors = 1), CBV(b1, numDescriptors = 3), SRV(t1, numDescriptors = 7), CBV(b4, numDescriptors = 4))," \
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
	
    RandomGenerator rg;
    rg.seed = coords;
    
    float r1, r2;
    r1 = 2.0f * rg.GetRandomNumber();
    r2 = 2.0f * rg.GetRandomNumber();
    
    float2 jitter;
    jitter.x = r1 < 1.0f ? sqrt(r1) - 1.0f : 1.0f - sqrt(2.0f - r1);
    jitter.y = r2 < 1.0f ? sqrt(r2) - 1.0f : 1.0f - sqrt(2.0f - r2);
    
    float2 d = coords;
    
    jitter /= (cb0.textureDimensions * 0.5f);
    d += jitter;
    
    float scale = tan(radians(cb1.fov) * 0.5f);
    d.y *= cb0.textureDimensions.y / cb0.textureDimensions.x * scale;
    d.x *= scale;
    float3 direction = normalize(d.x * cb1.right + d.y * cb1.up + cb1.direction);
    float3 focalPoint = cb1.focalDist * direction;
	
    Ray currentRay;
    currentRay.position = cb1.position;
    currentRay.direction = normalize(focalPoint);
    currentRay.length = MAXIMUM_RAY_LENGTH;
    
    float4 lastColor = OutputTexture[IN.DispatchThreadID.xy];
    if (cb0.isCameraMoving)
    {
        lastColor = float4(0.0f, 0.0f, 0.0f, 0.0f); 
    }
	 
    float4 finalColor = lastColor + PathTrace(currentRay, rg);    
    
    OutputTexture[IN.DispatchThreadID.xy] = finalColor;

}

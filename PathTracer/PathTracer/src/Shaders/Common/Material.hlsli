#ifndef _MATERIAL_HLSLI
#define _MATERIAL_HLSLI

#include "ConstantBuffers.hlsli"
#include "Utils.hlsli"
#include "Ray.hlsli"
#include "HitPoint.hlsli"
#include "RandomGenerator.hlsli"

#define MATERIAL_TYPE_DIFFUSE 0 
#define MATERIAL_TYPE_SPECULAR 1

struct Material
{
    float4 diffuseColor;
    float4 emissiveColor;
    
    float metallic;
    float roughness;
    float ior;
    float unused;
    
    int textureIndex;
    int materialType;
    float2 unused2;
    
    float DiffuseGetMaterialPDF(in Ray r, in HitPoint hp, float3 dirBSDF)
    {

        float3 N = hp.Normal;
        float3 V = -r.direction;
        float3 L = dirBSDF;
        
        float specularAlpha = max(0.001f, roughness);
        
        float diffuseRatio = 0.5f * (1.0f - metallic);
        float specularRatio = 1.0f - diffuseRatio;
        
        float3 halfDirection = normalize(V + L);
        
        float cosTheta = abs(dot(halfDirection, N));
        float pdfGTR2 = GTR2(cosTheta, specularAlpha) * cosTheta;
        
        float specularPDF = pdfGTR2 / (4.0f * abs(dot(L, halfDirection)));
        float diffusePDF = abs(dot(L, N)) * (1.0f / PI);
        
        return specularPDF * specularRatio + diffusePDF * diffuseRatio;
    }
    
    float3 DiffuseGetMaterialSample(in Ray r, in HitPoint hp, in RandomGenerator rg)
    {
        float probability = rg.GetRandomNumber();
        float diffuseRatio = 0.5f * (1.0f - metallic);
        
        float3 tangent, binormal;
        CreateLocalCoordinateSystem(hp.Normal, tangent, binormal);
    
        float r1 = rg.GetRandomNumber();
        float r2 = rg.GetRandomNumber();
        
        if (probability < diffuseRatio)
        {
            float3 hemisphereDirection = SampleDirectionFromHemisphere(r1, r2);
            float3 sample = GetDirectionFromCoordinateSystemAndDirection(hp.Normal, tangent, binormal, hemisphereDirection);
            return normalize(sample);
        }
        else
        {
            float specularAlpha = max(0.001f, roughness);
            
            float phi = r1 * 2.0f * PI;
            float sinPhi = sin(phi), cosPhi = cos(phi);
            
            float cosTheta = sqrt((1.0f - r2) / (1.0f + (specularAlpha * specularAlpha - 1.0f) * r2));
            float sinTheta = clamp(sqrt(1.0f - (cosTheta * cosTheta)), 0.0f, 1.0f);
            
            
            float3 newDirection = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            float3 sample = GetDirectionFromCoordinateSystemAndDirection(hp.Normal, tangent, binormal, newDirection);
            return normalize(reflect(-r.direction, sample));
        }

    }

    float4 DiffuseGetMaterialEval(in Ray r, in HitPoint hp, float3 dirBSDF)
    {
        float3 N = hp.Normal;
        float3 V = -r.direction;
        float3 L = dirBSDF;
        
        float NL = dot(N, L);
        float NV = dot(N, V);
        
        if (NL <= 0.0f || NV <= 0.0f)
        {
            return float4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        
        float3 halfDir = normalize(L + V);
        float NH = dot(N, halfDir);
        float LH = dot(L, halfDir);
        
        float specular = 0.5f;
        float4 specularColor = lerp(float4(1.0f, 1.0f, 1.0f, 1.0f) * specular, hp.Color, metallic);
        float specularAlpha = max(0.001f, roughness);
        
        float Ds = GTR2(NH, specularAlpha);
        float FH = SchlickFresnel(LH);
        float4 Fs = lerp(specularColor, float4(1.0f, 1.0f, 1.0f, 1.0f), FH);
        float roughnessCoefficient = roughness * 0.5f + 0.5f; // [-1, 1] -> [0, 1]
        roughnessCoefficient = roughness * roughness;
        
        float Gs = SmithG_GGX(NL, roughnessCoefficient) * SmithG_GGX(NV, roughnessCoefficient);
        
        return (hp.Color / PI) * (1.0f - metallic) + Fs * Gs * Ds;

    }
    
    float3 SpecularGetMaterialSample(in Ray r, in HitPoint hp, in RandomGenerator rg)
    {
        float n1 = 1.0f; // air IoR
        float n2 = ior;
        
        float R0 = (n1 - n2) / (n1 + n2);
        R0 = R0 * R0;
        
        float theta = dot(-r.direction, hp.Normal);
        
        
        bool isInside = dot(-r.direction, hp.Normal) < 0.0f;
        float refractionIndex = isInside ? (n2 / n1) : (n1 / n2);
        
        float r1 = rg.GetRandomNumber();
        if (r1 < metallic)
        {
            return normalize(reflect(r.direction, hp.Normal));
        }
        else
        {
            return normalize(refract(r.direction, hp.Normal, refractionIndex));
        }

    }
    
    float GetMaterialPDF(in Ray r, in HitPoint hp, float3 dirBSDF)
    {
        if (materialType == 0)
        {
            return DiffuseGetMaterialPDF(r, hp, dirBSDF);
        }
        else
        {
            return 1.0f;
        }
    }
    
    float3 GetMaterialSample(in Ray r, in HitPoint hp, in RandomGenerator rg)
    {
        if (materialType == 0)
        {
            return DiffuseGetMaterialSample(r, hp, rg);
        }
        else
        {
            return SpecularGetMaterialSample(r, hp, rg);
        }
    }

    float4 GetMaterialEval(in Ray r, in HitPoint hp, float3 dirBSDF)
    {
        if (materialType == 0)
        {
            return DiffuseGetMaterialEval(r, hp, dirBSDF);
        }
        else
        {
            return float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
};

Material EmptyMaterial()
{
    Material m;
    
    m.diffuseColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    m.emissiveColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    m.metallic = 0.f;
    m.roughness = 0.f;
    m.ior = 0.f;
    
    m.textureIndex = 0;
    m.materialType = 0;
    
    return m;
}

Material GetMaterialByIndex(int index)
{
    int offset = index * sizeof(Material) / sizeof(float4);
    
    float4 firstRead = GetColorFromTextureByIndex(Materials, offset);
    float4 secondRead = GetColorFromTextureByIndex(Materials, offset + 1);
    float4 thirdRead = GetColorFromTextureByIndex(Materials, offset + 2);
    float4 fourhtRead = GetColorFromTextureByIndex(Materials, offset + 3);
    
    Material m;
    
    m.diffuseColor = firstRead;
    m.emissiveColor = secondRead;
    
    m.metallic = thirdRead.x;
    m.roughness = thirdRead.y;
    m.ior = thirdRead.z;
    
    
    m.textureIndex = asint(fourhtRead.x);
    m.materialType = asint(fourhtRead.y);
    return m;
}



#endif
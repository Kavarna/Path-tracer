
struct RenderingCB
{
    unsigned int applyGamma;
    float invGamma;
    unsigned int numPasses;
};

ConstantBuffer<RenderingCB> cb1 : register(b1);

struct PSIn {
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

float4 main(PSIn input) : SV_TARGET {
    float4 color;
    if (cb1.applyGamma)
    {
        // return float4(1.0f, 1.0f, 0.0f, 1.0f);
        color = t1.Sample(s1, input.texCoord.xy);
        color = pow(color / cb1.numPasses, 1.0f / cb1.invGamma);
        
    }
    else
    {
        color = t1.Sample(s1, input.texCoord.xy);
    }
    
    // Vignetting
    float dist = length(2.0f * input.texCoord.xy - 1.0f);
    float vig = smoothstep(1.5f, 0.5f, dist);
    color *= vig;
    
    return color;
}

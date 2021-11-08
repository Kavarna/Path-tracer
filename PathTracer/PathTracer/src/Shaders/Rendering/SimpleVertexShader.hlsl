
struct VSOut {
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};


VSOut main( float3 pos : POSITION, float2 texCoord : TEXCOORD ) {
    VSOut output = (VSOut) 0;
    
    output.position = float4(pos, 1.0f);
    output.texCoord = texCoord;
    
    return output;
}

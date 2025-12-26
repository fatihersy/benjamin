cbuffer CameraBuffer : register(b0) {
    row_major float4x4 modelMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

Texture2D diffuseTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD) {
    PSInput result;
    result.position = mul(float4(position, 1.0f), mul(modelMatrix, mul(viewMatrix, projectionMatrix)));
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return diffuseTexture.Sample(linearSampler, input.uv);
}
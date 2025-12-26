cbuffer CameraBuffer : register(b0) {
    row_major float4x4 modelMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float3 position : POSITION, float4 color : COLOR) {
    PSInput result;
    result.position = mul(float4(position, 1.0f), mul(modelMatrix, mul(viewMatrix, projectionMatrix)));
    result.color = color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.color;
}
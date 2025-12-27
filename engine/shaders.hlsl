cbuffer CameraBuffer : register(b0) {
    row_major float4x4 modelMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

cbuffer LightBuffer : register(b1) {
    float3 lightDirection;
    float lightIntensity;
    float3 lightColor;
    float ambientIntensity;
};

Texture2D diffuseTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD) {
    PSInput result;
    float4 worldPos = mul(float4(position, 1.0f), modelMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    result.position = mul(viewPos, projectionMatrix);
    result.normal = mul(normal, (float3x3)modelMatrix);
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-lightDirection);
    
    float diffuse = max(dot(normal, lightDir), 0.0f) * lightIntensity;
    float3 ambient = ambientIntensity * lightColor;
    float3 diffuseLight = diffuse * lightColor;

    float4 texColor = diffuseTexture.Sample(linearSampler, input.uv);
    return float4(texColor.rgb * (ambient + diffuseLight), texColor.a);
}

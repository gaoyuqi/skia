STRINGIFY(
    layout(builtin=15) float4 sk_FragCoord;
    half4 sample(fragmentProcessor fp);
    half4 sample(fragmentProcessor fp, float2 coords);
    half4 sample(fragmentProcessor fp, float3x3 transform);
)

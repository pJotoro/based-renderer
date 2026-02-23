module shader;

[shader("vertex")]
float4 vs(float3 in_pos : POSITION, uint vertex_id : SV_VertexID) : SV_Position
{
    float2 vertices[6] = {
        float2(0.5, 0.5),
        float2(0.5, -0.5),
        float2(-0.5, 0.5),
        float2(0.5, -0.5),
        float2(-0.5, -0.5),
        float2(-0.5, 0.5)
    };

    return float4(vertices[vertex_id], 0.0, 1.0);
}

[shader("pixel")]
float4 ps(float4 in_pos : SV_POSITION) : SV_TARGET
{
    return in_pos;
}
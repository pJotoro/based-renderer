module shader;

[shader("vertex")]
float4 vs(uint vertex_id : SV_VertexID) : SV_Position
{
    float2 vertices[3] = {
        float2(-0.5, 0.5),
        float2(0.5, 0.5),
        float2(0.0, -0.5)
    };

    return float4(vertices[vertex_id], 0.0, 1.0);
}

[shader("pixel")]
float4 ps() : SV_TARGET
{
    return float4(0.1, 0.2, 0.3, 1.0);
}
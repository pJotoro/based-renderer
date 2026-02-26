module shader;

[shader("vertex")]
float4 vs(uint vertex_id : SV_VertexID) : SV_Position
{
    float2 vertices[3] = {
        float2(-0.5f, -0.5f),
        float2(0.5f, -0.5f),
        float2(0.0f,  0.5f)
    };

    return float4(vertices[vertex_id], 0.0, 1.0);
}

[shader("pixel")]
float4 ps(float4 in_pos : SV_POSITION) : SV_TARGET
{
    return float4(0.1, 0.2, 0.3, 1.0);
}
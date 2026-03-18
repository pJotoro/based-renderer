module cube;

struct Uniforms
{
    matrix<float,4,4> model;
    matrix<float,4,4> view;
    matrix<float,4,4> proj;
};
ConstantBuffer<Uniforms> u;

[shader("vertex")]
float4 vs(uint vertex_id : SV_VertexID) : SV_Position
{
    float3 vertices[36] = {
        float3(-0.5, -0.5, -0.5),
        float3(0.5, -0.5, -0.5),
        float3(0.5, 0.5, -0.5),
        float3(0.5, 0.5, -0.5),
        float3(-0.5, 0.5, -0.5),
        float3(-0.5, -0.5, -0.5), 

        float3(-0.5, -0.5, 0.5),
        float3( 0.5, -0.5, 0.5),
        float3( 0.5, 0.5, 0.5),
        float3( 0.5, 0.5, 0.5),
        float3(-0.5, 0.5, 0.5),
        float3(-0.5, -0.5, 0.5),  

        float3(-0.5, 0.5, 0.5),
        float3(-0.5, 0.5, -0.5),
        float3(-0.5, -0.5, -0.5),
        float3(-0.5, -0.5, -0.5),
        float3(-0.5, -0.5, 0.5),
        float3(-0.5, 0.5, 0.5),   

        float3( 0.5, 0.5, 0.5),
        float3( 0.5, 0.5, -0.5),
        float3( 0.5, -0.5, -0.5),
        float3( 0.5, -0.5, -0.5),
        float3( 0.5, -0.5, 0.5),
        float3( 0.5, 0.5, 0.5),   

        float3(-0.5, -0.5, -0.5),
        float3( 0.5, -0.5, -0.5),
        float3( 0.5, -0.5, 0.5),
        float3( 0.5, -0.5, 0.5),
        float3(-0.5, -0.5, 0.5),
        float3(-0.5, -0.5, -0.5), 

        float3(-0.5, 0.5, -0.5),
        float3( 0.5, 0.5, -0.5),
        float3( 0.5, 0.5, 0.5),
        float3( 0.5, 0.5, 0.5),
        float3(-0.5, 0.5, 0.5),
        float3(-0.5, 0.5, -0.5),
    };

    return float4(vertices[vertex_id], 1.0);
}

[shader("pixel")]
float4 ps() : SV_TARGET
{
    return float4(0.1, 0.2, 0.3, 1.0);
}
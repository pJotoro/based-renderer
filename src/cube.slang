module cube;

struct Uniforms
{
    matrix<float,4,4> model;
    matrix<float,4,4> view;
    matrix<float,4,4> proj;
};
ConstantBuffer<Uniforms> u;

struct Vertex
{
    float4 pos : SV_Position;
    nointerpolation float3 color;
};

[shader("vertex")]
Vertex vs(uint vertex_id : SV_VertexID)
{
    Vertex res;

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

    float3 colors[6] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0),
        float3(1.0, 1.0, 0.0),
        float3(0.0, 1.0, 1.0),
        float3(1.0, 0.0, 1.0),
    };

    float3 normals[6] = {
        float3(0.0, 0.0, -1.0),
        float3(0.0, 0.0, 1.0),
        float3(-1.0, 0.0, 0.0),
        float3(1.0, 0.0, 0.0),
        float3(0.0, -1.0, 0.0),
        float3(0.0, 1.0, 0.0),
    };

    matrix<float,4,4> transform = mul(u.proj, mul(u.view, u.model));
    res.pos = mul(transform, float4(vertices[vertex_id], 1.0));
    res.color = colors[vertex_id/6];
    return res;
}

[shader("pixel")]
float4 ps(Vertex vertex) : SV_TARGET
{
    return float4(vertex.color, 1.0);
}
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
    nointerpolation float3 normal;
    nointerpolation float3 color;
    nointerpolation float4 light_dir;
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

    float3 light_dir = float3(1.0, 0.0, 0.0);
    matrix<float,4,4> transform = mul(u.proj, mul(u.view, u.model));
    res.light_dir = mul(transform, float4(light_dir, 0.0));
    res.pos = mul(transform, float4(vertices[vertex_id], 1.0)));
    res.normal = normals[vertex_id/6];
    res.color = colors[vertex_id/6];
    return res;
}

[shader("pixel")]
float4 ps(Vertex vertex) : SV_TARGET
{
    float3 light_dir = vertex.light_dir.xyz;
    float diff = max(dot(vertex.normal, light_dir), 0.0);
    float3 ambient = float3(0.1, 0.1, 0.1);
    float3 diffuse = diff*vertex.color;
    float3 result = (ambient + diffuse) * vertex.color;
    return float4(result, 1.0);
}

/*
I think I have identified the problems that I am having:

1. I don't know what the light direction should be in world space.
2. I don't know which coordinate system the light direction should be in when I use it.
3. I don't know if the light direction should always be the same, or if I have to change it depending on the vertex index.
4. I don't know how to linearly interpolate the light direction, or if that's even necessary.

That's quite a lot of unknowns! No wonder I'm so stuck on how to implement diffuse reflection.

That said, this isn't necessarily a bad thing. The above shader code was more or less copied from Learn OpenGL and then translated into Slang from GLSL. If it really did just work out of the box somehow for what I'm doing, then I wouldn't be learning anything, and would most likely get even more stuck later on. But of course, it didn't work out of the box because I am trying to make a light point at a rotating cube, which is not what Learn OpenGL teaches.
*/
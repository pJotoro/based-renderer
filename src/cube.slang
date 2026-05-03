module cube;

// ChatGPT was used to write this function.
matrix<float,4,4> adjugate(matrix<float,4,4> m)
{
    float4 a = m[0];
    float4 b = m[1];
    float4 c = m[2];
    float4 d = m[3];

    float s0 = a.x * b.y - a.y * b.x;
    float s1 = a.x * b.z - a.z * b.x;
    float s2 = a.x * b.w - a.w * b.x;
    float s3 = a.y * b.z - a.z * b.y;
    float s4 = a.y * b.w - a.w * b.y;
    float s5 = a.z * b.w - a.w * b.z;

    float c0 = c.x * d.y - c.y * d.x;
    float c1 = c.x * d.z - c.z * d.x;
    float c2 = c.x * d.w - c.w * d.x;
    float c3 = c.y * d.z - c.z * d.y;
    float c4 = c.y * d.w - c.w * d.y;
    float c5 = c.z * d.w - c.w * d.z;

    matrix<float,4,4> adj;

    adj[0][0] =  b.y * c5 - b.z * c4 + b.w * c3;
    adj[0][1] = -a.y * c5 + a.z * c4 - a.w * c3;
    adj[0][2] =  d.y * s5 - d.z * s4 + d.w * s3;
    adj[0][3] = -c.y * s5 + c.z * s4 - c.w * s3;

    adj[1][0] = -b.x * c5 + b.z * c2 - b.w * c1;
    adj[1][1] =  a.x * c5 - a.z * c2 + a.w * c1;
    adj[1][2] = -d.x * s5 + d.z * s2 - d.w * s1;
    adj[1][3] =  c.x * s5 - c.z * s2 + c.w * s1;

    adj[2][0] =  b.x * c4 - b.y * c2 + b.w * c0;
    adj[2][1] = -a.x * c4 + a.y * c2 - a.w * c0;
    adj[2][2] =  d.x * s4 - d.y * s2 + d.w * s0;
    adj[2][3] = -c.x * s4 + c.y * s2 - c.w * s0;

    adj[3][0] = -b.x * c3 + b.y * c1 - b.z * c0;
    adj[3][1] =  a.x * c3 - a.y * c1 + a.z * c0;
    adj[3][2] = -d.x * s3 + d.y * s1 - d.z * s0;
    adj[3][3] =  c.x * s3 - c.y * s1 + c.z * s0;

    return adj;
}

struct Uniforms
{
    matrix<float,4,4> model;
    matrix<float,4,4> view;
    matrix<float,4,4> proj;
}
ConstantBuffer<Uniforms> u;

struct Vertex
{
    float4 pos : SV_Position;
    nointerpolation float3 color;
    nointerpolation float3 normal;
    float2 texcoord;
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

    float2 texcoords[6] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(1.0, 1.0),
        float2(1.0, 1.0),
        float2(0.0, 1.0),
        float2(0.0, 0.0),
    };

    float4 pos = float4(vertices[vertex_id], 1.0);
    res.pos = mul(u.proj, mul(u.view, mul(u.model, pos)));
    res.color = colors[vertex_id/6];
    res.normal = mul(float4(normals[vertex_id/6], 0.0), adjugate(u.model)).xyz;
    res.texcoord = texcoords[vertex_id%6];
    return res;
}

[format("rgba8"), vk::binding(0, 1)]
Sampler2D<float> texture_sampler;

[shader("pixel")]
float4 ps(Vertex vertex) : SV_TARGET
{
    // FGED Eq 7.26
    //float3 view_dir = float3(0.0, 0.0, -1.0);
    float3 light_dir = float3(0.0, 0.0, -1.0);
    //float3 halfway_dir = normalize(view_dir + light_dir);
    float diffuse = saturate(dot(vertex.normal, light_dir));
    //float alpha = 10.0;
    //float specular = pow(saturate(dot(vertex.normal, halfway_dir)), alpha);
    return diffuse*texture_sampler.Sample(vertex.texcoord);
}
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
    nointerpolation float3 normal;
};

matrix<float,4,4> cofactor(matrix<float,4,4> m)
{
    matrix<float,4,4> res;

    {
        matrix<float,3,3> c;
        c[0] = m[1].yzw;
        c[1] = m[2].yzw;
        c[2] = m[3].yzw;
        res[0][0] = determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].yzw;
        c[1] = m[2].yzw;
        c[2] = m[3].yzw;
        res[1][0] = -determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].yzw;
        c[1] = m[1].yzw;
        c[2] = m[3].yzw;
        res[2][0] = determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].yzw;
        c[1] = m[1].yzw;
        c[2] = m[2].yzw;
        res[3][0] = -determinant(c);
    }

    {
        matrix<float,3,3> c;
        c[0] = m[1].xzw;
        c[1] = m[2].xzw;
        c[2] = m[3].xzw;
        res[0][1] = -determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xzw;
        c[1] = m[2].xzw;
        c[2] = m[3].xzw;
        res[1][1] = determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xzw;
        c[1] = m[1].xzw;
        c[2] = m[3].xzw;
        res[2][1] = -determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xzw;
        c[1] = m[1].xzw;
        c[2] = m[2].xzw;
        res[3][1] = determinant(c);
    }

    {
        matrix<float,3,3> c;
        c[0] = m[1].xyw;
        c[1] = m[2].xyw;
        c[2] = m[3].xyw;
        res[0][2] = determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xyw;
        c[1] = m[2].xyw;
        c[2] = m[3].xyw;
        res[1][2] = -determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xyw;
        c[1] = m[1].xyw;
        c[2] = m[3].xyw;
        res[2][2] = determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xyw;
        c[1] = m[1].xyw;
        c[2] = m[2].xyw;
        res[3][2] = -determinant(c);
    }

    {
        matrix<float,3,3> c;
        c[0] = m[1].xyz;
        c[1] = m[2].xyz;
        c[2] = m[3].xyz;
        res[0][3] = -determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xyz;
        c[1] = m[2].xyz;
        c[2] = m[3].xyz;
        res[1][3] = determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xyz;
        c[1] = m[1].xyz;
        c[2] = m[3].xyz;
        res[2][3] = -determinant(c);
    }
    {
        matrix<float,3,3> c;
        c[0] = m[0].xyz;
        c[1] = m[1].xyz;
        c[2] = m[2].xyz;
        res[3][3] = determinant(c);
    }

    return res;
}

matrix<float,4,4> adjugate(matrix<float,4,4> m)
{
    return transpose(cofactor(m));
}

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

    float4 pos = float4(vertices[vertex_id], 1.0);
    res.pos = mul(u.proj, mul(u.view, mul(u.model, pos)));
    res.color = colors[vertex_id/6];
    res.normal = mul(float4(normals[vertex_id/6], 0.0), adjugate(u.model)).xyz; 
    return res;
}

[shader("pixel")]
float4 ps(Vertex vertex) : SV_TARGET
{
    // FGED Eq 7.26
    float3 view_dir = float3(0.0, 0.0, -1.0);
    float3 light_dir = float3(0.0, -0.6, -0.8);
    float3 halfway_dir = normalize(view_dir + light_dir);
    float4 diffuse = float4(vertex.color*saturate(dot(vertex.normal, light_dir)), 1.0);
    float alpha = 10.0;
    float4 specular = float4(vertex.color*pow(saturate(dot(vertex.normal, halfway_dir)), alpha), 1.0);
    return diffuse + specular;
}
module box;

struct Uniforms
{
    matrix<float,4,4> model;
    matrix<float,4,4> view;
    matrix<float,4,4> proj;
}
ConstantBuffer<Uniforms> u;

struct Vertex
{
	float4 pos : POSITION;
	float4 normal : NORMAL;
}

struct Pixel
{
	float4 pos : SV_POSITION;
	nointerpolation float4 normal;
}

[shader("vertex")]
Pixel vs(Vertex vertex)
{
	Pixel res;

	res.pos = mul(u.proj, mul(u.view, mul(u.model, vertex.pos)));
	res.normal = vertex.normal;

	return res;
}

[shader("pixel")]
float4 ps(Pixel vertex) : SV_TARGET
{
	// FGED Eq 7.26
    //float3 view_dir = float3(0.0, 0.0, -1.0);
    float4 light_dir = float4(0.0, 0.0, -1.0, 0.0);
    //float3 halfway_dir = normalize(view_dir + light_dir);
    float diffuse = saturate(dot(vertex.normal, light_dir));
    //float alpha = 10.0;
    //float specular = pow(saturate(dot(vertex.normal, halfway_dir)), alpha);
    return diffuse;
}
#define VERTICES 3

cbuffer CBUFFER : register(b0)
{
	float4x4 world;
	float4x4 view;
	float4x4 project;
};

struct GS_IN
{
	float4 VPos : SV_POSITION;
	float2 Tex : TEXCOORD;
};

struct GS_OUT
{
	float4 VPos : SV_POSITION; //Vertex Position
	float4 pointPos : POSITION; //Point on surface position
	float3 Normal : NORMAL; //surface Normal
	float2 Tex : TEXCOORD; //UV coordinates
};


[maxvertexcount(6)]
void GS_main(triangle GS_IN input[3], inout TriangleStream<GS_OUT> PS_Stream)
{
	GS_OUT output			= (GS_OUT)0;
	float4x4 transformation = mul(world, mul(view, project)); 
	float3 normal			= normalize(cross(input[2].VPos.xyz - input[0].VPos.xyz, input[1].VPos.xyz - input[0].VPos.xyz));
	
	for (int i = 0; i < VERTICES; i++)
	{
		output.VPos		= mul(input[i].VPos, transformation); //Project vertices.
		output.pointPos = mul(input[i].VPos, world); //Multiply with world matrix, not whole transformation
		output.Normal	= mul(normal, transformation); //Project normal.
		output.Tex		= input[i].Tex; //Copy texture coordinates.
		PS_Stream.Append(output);
	}
	PS_Stream.RestartStrip(); //Reset
	//Create a duplicate triangle
	for (int i = 0; i<VERTICES; i++)
	{
		output.VPos		= mul(input[i].VPos - float4(normal, 0.0f), transformation); //Move the vertices of the duplicated triangle by the length of the original triangle's normalized normal.
		output.pointPos = mul(input[i].VPos - float4(normal, 0.0f), world);
		output.Normal	= mul(normal, transformation);
		output.Tex		= input[i].Tex;
		PS_Stream.Append(output);
	}
	PS_Stream.RestartStrip(); //Reset
	for (int i = 0; i < VERTICES; i++)
	{
		output.VPos = mul(input[i].VPos + float4(normal, 0.0f), transformation); //Move the vertices of the duplicated triangle by the length of the original triangle's normalized normal.
		output.pointPos = mul(input[i].VPos + float4(normal, 0.0f), world);
		output.Normal = mul(normal, transformation);
		output.Tex = input[i].Tex;
		PS_Stream.Append(output);
	}
	//Main executes once per triangle, meaning that we duplicate both triangles we insert
}
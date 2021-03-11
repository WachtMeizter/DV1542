struct VS_IN
{
	float3 VPos : POSITION;
	float2 Tex : TEXCOORD;
};

struct VS_OUT
{
	float4 VPos : SV_POSITION;
	float2 Tex : TEXCOORD;
};

VS_OUT VS_main(VS_IN input)
{
	VS_OUT output = (VS_OUT)0;
	output.VPos = float4(input.VPos, 1.0f);
	output.Tex = input.Tex;

	return output;
}
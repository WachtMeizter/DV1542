#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define HEIGHT 480.0f
#define WIDTH 640.0f

struct float2 
{
	float x;
	float y;
};

struct float3 
{
	float x;
	float y;
	float z;
};


struct float4 
{
	float x;
	float y;
	float z;
	float w;
};

struct Matrix3x3
{
	float3 row1;
	float3 row2;
	float3 row3;
};

struct Matrix4x4
{
	float4 row1;
	float4 row2;
	float4 row3;
};


__declspec(align(16)) //Align with 16 bytes
struct Vertex 
{
	float3 xyz;
	float3 rgb;
	float2 uv;
};
#endif // !CONSTANTS_H_

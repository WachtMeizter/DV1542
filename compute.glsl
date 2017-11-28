
#version 450 core
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_image_load_store : enable

// output of compute shader is a TEXTURE
layout (rgba8, binding = 0) uniform image2D outTexture;

// receive as inputs, rays from the centre of the camera to the frustum corners
layout( location = 0 ) uniform vec4 ray00; // top left in world space
layout( location = 1 ) uniform vec4 ray01; // top right in world space
layout( location = 2 ) uniform vec4 ray10; // bottom left in world space
layout( location = 3 ) uniform vec4 ray11; // bottom right in world space

// receive the camera position
layout( location = 4 ) uniform vec3 camPos; // camera position

// receive how many primitives of each type
// geomCount.x = number of spheres
// geomCount.y = number of triangles
// geomCount.z = number of planes
// geomCount.w = number of obbs
layout( location = 5 ) uniform ivec4 geomCount; // sphere, triangle, planes, obbs

// how many lights to process (how many elements in the array "light")
layout( location = 6 ) uniform int lightCount;

// receive an ARRAY of light_type, 
struct light_type {
	 vec4 position;
	 vec4 colour;
};
layout(std430, binding = 2) buffer light
{
	light_type lights_data[];
};

struct sphere_type {
	vec4 centre_radius;
	vec4 colour;
};
// 8 floats per sphere: cx cy cz rad r g b a
layout(std430, binding = 3) buffer sphere
{
	sphere_type spheres_data[];
};

struct plane_type {
	vec4 normal_d;
	vec4 colour;
};
// 8 floats per plane: nx ny nz d r g b a
layout(std430, binding = 4) buffer plane
{
	plane_type planes_data[];
};

struct triangle_type {
	vec4 vtx0, vtx1, vtx2;
	vec4 colour;
};
layout(std430, binding = 5) buffer triangle
{
	triangle_type triangles_data[];
};

// 20 floats per obb: centre4, u3, hu, v3, hv, w3, hw, rgba4
struct obb_type {
	vec4 centre, u_hu, v_hv, w_hw, colour;
};
layout(std430, binding = 6) buffer obb
{
	obb_type obb_data[];
};

#define SPHERE_TYPE 0
#define LIGHT_TYPE 1
#define PLANE_TYPE 2
#define TRIANGLE_TYPE 3
#define OBB_TYPE 4

// primitives tests forward declarations
float sphereTest(vec3 rayDir, vec3 rayOrigin, vec4 centre_radius);
float planeTest(vec3 rayDir, vec3 rayOrigin, vec4 plane_info);
float triangleTest(vec3 rayDir, vec3 rayOrigin, triangle_type tri);
float obbTest(vec3 rayDir, vec3 rayOrigin, obb_type obb);

// entire scene test forward declaration
void sceneTest(in vec4 ray_dir, inout float lastT, inout int objIndex, inout int objType);

// shade a point of a surface (for any primitive) forward declaration
vec4 shade(in vec3 pointOnSurface, in vec3 normal, in vec3 colour);

vec4 castRay(ivec2 pos)
{
	// normalise values from [ 0,width; 0,height ] to [ 0,1; 0,1]
	vec2 interp = vec2(pos) / imageSize(outTexture); 

	// compute ray as interpolation of input rays (corners)
	vec4 ray_dir = normalize(
		mix(
			mix(ray00,ray01,interp.y), // left corners together
			mix(ray10,ray11,interp.y), // right corners together
			interp.x // join new interpolated rays into a final ray
		)
	);

	// lastT is the last intersection found (will keep the closest value)
	float lastT=-1;
	// will remember which object index was hit if any.
	int objIndex = -1;
	// will remember which object type was hit if any.
	int objType = -1;

	// do ray versus scene test
	sceneTest(ray_dir, lastT, objIndex, objType); 

	// set pixel to BACKGROUND colour
	vec4 finalPixelOut = vec4(0.3,0.4,0.3,1.0);
	
	if (objIndex >= 0) {
		// did we hit something?
		// IMPLEMENT HERE.
		// FIND POINT IN SPACE WHERE THE INTERSECTION HAPPENED.
		vec3 pointOnSurface;

		if (objType==SPHERE_TYPE)
			{
			finalPixelOut = spheres_data[objIndex].colour;
				// IMPLEMENT HERE
				// COMPUTE FINAL PIXEL COLOUR USING SHADE() FUNCTION
			}
		else if (objType==LIGHT_TYPE)
		{
			finalPixelOut = lights_data[objIndex].colour;
		}
		else if (objType == PLANE_TYPE)
		{
		finalPixelOut = planes_data[objIndex].colour;
			// IMPLEMENT HERE
			// COMPUTE FINAL PIXEL COLOUR USING SHADE() FUNCTION
		}
		else if (objType == TRIANGLE_TYPE) 
		{
			// IMPLEMENT HERE
			// COMPUTE FINAL PIXEL COLOUR USING SHADE() FUNCTION
		}
		else if (objType == OBB_TYPE)
		{
			// IMPLEMENT HERE
			// COMPUTE FINAL PIXEL COLOUR USING SHADE() FUNCTION
		}
	}
	return finalPixelOut;
}

layout (local_size_x = 20, local_size_y = 20) in;
void main()
{
	// pixel coordinate, x in [0,width-1], y in [0,height-1]
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	vec4 finalPixel = castRay(pos);
	imageStore(outTexture, pos, finalPixel);
	return;
};

void sceneTest(in vec4 ray_dir, inout float lastT, inout int objIndex, inout int objType)
	// test all spheres (in geomCount.x)
	{
	for (int i=0;i< geomCount.x;i++)
	{
		sphere_type sphere = spheres_data[i];
		float sphere_test = sphereTest(ray_dir.xyz, camPos.xyz, sphere.centre_radius);
		if((sphere_test != -1))
		{
			if(lastT > sphere_test || (lastT == -1))
			{
				lastT = sphere_test;
				objIndex = i;
				objType = SPHERE_TYPE;
			}
		}
		// TEST SPHERE, UPDATE lastT, objIndex and objType if necessary.
	}
	// lights (we render lights as spheres)
	for (int i=0; i<lightCount; i++)
	{
		light_type light = lights_data[i];
		float sphere_test = sphereTest(ray_dir.xyz, camPos.xyz, light.position);
		if((sphere_test != -1))
		{
			if(lastT > sphere_test || (lastT == -1))
			{
				lastT = sphere_test;
				objIndex = i;
				objType = LIGHT_TYPE;
			}
		}
		// IMPLEMENT HERE.
		// TEST SPHERE (LIGHT), UPDATE lastT, objIndex and objType if necessary.
	}
	// planes
	for (int i = 0; i<geomCount.y; i++)
	{
		plane_type plane = planes_data[i];
		float plane_test = planeTest(ray_dir.xyz, camPos, plane.normal_d);
		if((plane_test != -1))
		{
			if(lastT > plane_test || lastT == -1)
			{
				lastT = plane_test;
				objIndex = i;
				objType = PLANE_TYPE;
			}
		}
		// TEST PLANE, UPDATE lastT, objIndex and objType if necessary.
	}
	// triangles
	for (int i = 0; i<geomCount.z; i++)
	{
		triangle_type triangle = triangles_data[i];
		// IMPLEMENT HERE.
		// TEST TRIANGLE, UPDATE lastT, objIndex and objType if necessary.
	}
	// OBBs
	for (int i = 0; i < geomCount.w; i++)
	{
		obb_type obb = obb_data[i];
		// IMPLEMENT HERE.
		// TEST OBB, UPDATE lastT, objIndex and objType if necessary.
	}
}

float sphereTest(in vec3 rayDir, in vec3 rayOrigin, in vec4 centre_radius)
{
	//Starting Values
	float rtn_val = -1;
	float t_1 = -1;
	float t_2 = -1;
	float sqrt_t = -1;
	
	//Calculate b and cs
	float b  = dot(rayDir, (rayOrigin - centre_radius.xyz));
	float c = dot((rayOrigin - centre_radius.xyz), (rayOrigin - centre_radius.xyz)) - (centre_radius.w * centre_radius.w);
	sqrt_t = (b*b)-c;
	//Check if 
	
	if(sqrt_t > 0.00000000001 )
	{
		t_1 = -b + sqrt(sqrt_t);
		t_2 = -b - sqrt(sqrt_t);
		rtn_val = min(t_1, t_2);
		if(rtn_val < 0.00000000001)
		{
		rtn_val = -1;
		}
	}
	return rtn_val;
}

float planeTest(vec3 rayDir, vec3 rayOrigin, vec4 plane_info)
{
	float t = -1;
	
	if(dot(rayDir,plane_info.xyz) != 0)
	{
		t = (-plane_info.w - dot(plane_info.xyz, rayOrigin))/dot(rayDir,plane_info.xyz);
		if(t < 0.00000000001)
		{
		t = -1;
		}
	}
	return t;
}

float triangleTest(vec3 rayDir, vec3 rayOrigin, triangle_type tri)
{
	float val = -1;
	
	return val;
}

float obbTest(vec3 rayDir, vec3 rayOrigin, obb_type o)
{
	float val = -1;
	
	return val;
};

// everything in World Space
vec4 shade(in vec3 pointOnSurface, in vec3 normal, in vec3 colour)
{
	// ambient
	vec4 final_colour = vec4(0.3,0.3,0.3,1.0) * vec4(colour, 1);

	// diffuse, no attenuation.
	vec3 diffuseFactor = pointOnSurface * normal;
	for (int i = 0; i < lightCount; i++)
	{
		vec4 light_pos = lights_data[i].position;
		vec4 light_colour = lights_data[i].colour;
		//vec3 temp_colour = colour*light_colour*diffuseFactor*light_colour.w*(1.0/distancePointToLight);
		final_colour += vec4(colour, 1.0);
	}
	
	final_colour = min(final_colour, vec4(1.0, 1.0, 1.0, 1.0));
	
	return final_colour;
}


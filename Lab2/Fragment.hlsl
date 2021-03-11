Texture2D txDiffuse : register(t0); 
SamplerState sampAni; //Use linear interpolation and mip-level sampling. Wrap U and V.

cbuffer LIGHT : register(b0)
{
	float4 lightpos; //position of light
	float4 lightcolor; //color of light
	float lightambient; //ambient factor
	float lightspecular; //specular power
};

struct PS_IN
{
	float4 VPos : SV_POSITION;  //Vertex position
	float4 pointPos: POSITION; //Point on surface
	float3 Normal : NORMAL;    //Surface normal
	float2 Tex : TEXCOORD;     //UV coordinates
};

float4 PS_main(PS_IN input) : SV_Target
{
	float4 finalColor = (float4)0; //Output color
	float4 texColor; //Color of loaded texture
	float4 lightdir; //Direction of light
	float4 viewdir;  //Direction of vector from camera to point on surface
	float3 reflection; //Reflection factor 
	float  intensity;
	float specularintensity;

	float4 specular = float4(0.0f, 0.0f, 0.0f, 1.0f); //Initialize specular
	float4 camerapos = { 0.0f, 0.0f, -2.0f, 1.0f };
	float4 ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	ambient = ambient * lightambient; //Multiply this light with the ambient factor to get ambient color;

	texColor = float4(txDiffuse.Sample(sampAni, input.Tex).xyz, 1.0f); //Sample from the texture created in initTextures;
    texColor = float4((texColor.xyz * lightcolor.xyz), 1.0f); //Multiply colors in texture color with color of light
	finalColor = texColor * lightambient;

	//Diffuse calcs
	lightdir = normalize(input.pointPos - lightpos); //Directional vector of the light toward the object, normalized
	intensity = max(dot(lightdir.xyz, input.Normal), 0);//Calculate intensity of light and clamp to 0 if it's negative
	
	if (intensity > lightambient)//Only apply diffuse and specular if pixel is brighter than the ambient, otherwise keep it the ambient color;
	{
		finalColor = saturate(mul(texColor, intensity));
		//Specular calcs
		reflection = -normalize(2 * intensity * input.Normal - lightdir.xyz); 
		viewdir = normalize(camerapos - input.pointPos); //Directional vector from camera to point on surface
		specularintensity = max(dot(reflection, viewdir.xyz), 0); //Calculate cos of angle between reflection and view
		specular = saturate(pow(specularintensity, lightspecular)); //
	}
	if(finalColor.x > ambient.x || finalColor.y > ambient.y || finalColor.z > ambient.z) //Only applying specular if pixel light is brighter than the ambient light color
		finalColor = saturate(finalColor + specular); //Adds no specular if the viewing angle theta<PI/2 RAD [90°]
	return finalColor;
};
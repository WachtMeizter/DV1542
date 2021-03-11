//Sebastian Lorensson, 9601296793, 2021-03-10
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <chrono>  //Time

#include "bth_image.h" //image
#include "WindowHelper.h" //Helper class

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

//Constants
#define VERTEXCOUNT 6 //Amount of vertices drawn
#define PI 3.141592f 
#define CAM_POS { 0.0f, 0.0f, -2.0f, 1.0f }
#define MULTISAMPLES 4 //For making sure swapchain and texture desc have the same amount of multisamplings

using namespace DirectX; //Don't want to use DirectX:: whenever we use DirectXMath

//===============================================================================
// Global pointers
//===============================================================================
ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gDeviceContext = nullptr;
IDXGISwapChain* gSwapChain = nullptr;
ID3D11RenderTargetView* gBackbufferRTV = nullptr;
//Vshader========================================================================
ID3D11Buffer* gVertexBuffer = nullptr;
ID3D11InputLayout* gVertexLayout = nullptr;
ID3D11VertexShader* gVertexShader = nullptr;
//Fshader========================================================================
ID3D11PixelShader* gPixelShader = nullptr;
ID3D11Buffer* gPixelCBuffer = nullptr;
//Gshader========================================================================
ID3D11GeometryShader* gGeometryShader = nullptr;
ID3D11Buffer* gGeoCBuffer = nullptr;
//Textures=======================================================================
ID3D11Texture2D* gTexture = nullptr;
ID3D11ShaderResourceView* gTextureView = nullptr;
ID3D11SamplerState* gSamplerState = nullptr;
//Z-buffer=======================================================================
ID3D11Texture2D* gDepthStencilBuffer = nullptr;
ID3D11DepthStencilView* gDepthStencilView = nullptr;
//===============================================================================
// Structs & global variables
//===============================================================================
//WVP matrices
__declspec(align(16))
struct Matrices {
	XMFLOAT4X4	world;
	XMFLOAT4X4  view;
	XMFLOAT4X4	project;
};
//Light
__declspec(align(16)) //To make sure byte width is always a multiple of 16, (for buffer creation)
struct Light 
{
	XMFLOAT4 pos;
	XMFLOAT4 color;
	float ambient;
	float specular;
};
//Define a triangle with uv-coords
struct TriangleVertex {
	float x, y, z;
	float u, v;
};

Matrices wvp; //For WVP matrix storage
Light light; //For the diffuse light
float rot = 0.0f; //Rotation variable
auto s_time = std::chrono::high_resolution_clock::now();
//===============================================================================
//Declaring all functions to order them easier
//===============================================================================
void SetViewport(); //Set up viewport
HRESULT CreateSwapChainAndDirect3DContext(HWND wndHandle);//Setting up Direct3D
void CreateQuadData(); //Creating quads from static data
void CreateShaders();  //Creating Vertex, Fragment and Pixel shaders
void CreateTexture();  //Creating texture from data
void CreateConstantBufferData(); //Filling WVP matrices with necessary data
void CreateConstantBuffers(); //Creating CBuffer to pass CPU information to GeoShader
void Render(); //Render scene
//Helper functions
void ReleaseCOMObjects();
void Rotate();
//===============================================================================
//Function definitions below this point
//===============================================================================
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) //Main
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //Memory leaks

	MSG msg = { 0 }; //Needed for Windows operations

	HWND wndHandle = InitWindow(hInstance); //0. Create window


	if (wndHandle) //Only continues if window is created
	{
		CreateSwapChainAndDirect3DContext(wndHandle); //1. Create and connect Swap chain, Device and Device Context

		CreateTexture(); //2. Initialise texture, sampler and depth stencil buffer.

		SetViewport(); //3. Set up context viewport

		CreateShaders(); //4. Create vertex, fragment and geometry shaders

		CreateQuadData(); //5. Define triangle vertices, create vertex buffer and input layout from these

		CreateConstantBufferData(); //6. Create World, View  and Projection Matrices, as well as Light for fragment shader

		CreateConstantBuffers(); //7. Create constant buffer.

		ShowWindow(wndHandle, nShowCmd); //Display window

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Render(); //8. Render to window
				gSwapChain->Present(0, 0); //9. Switch front- and back-buffers
			}
		}
		//When program is done rendering
		ReleaseCOMObjects(); //Release all global COM objects
		DestroyWindow(wndHandle); //Destroy main window
	}

	return (int)msg.wParam;
}

//==========================================================
//DirectX beyond this point
//==========================================================
//Set up DirectX
HRESULT CreateSwapChainAndDirect3DContext(HWND wndHandle)
{
	// Create a desc to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC swapcdesc;
	// Making sure the struct is empty before we use it (don't want any random junk in there).
	ZeroMemory(&swapcdesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	// Fill the swap chain description struct
	swapcdesc.BufferCount = 1; // one back buffer
	swapcdesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // Use 32-bit color, [0,1]
	swapcdesc.BufferDesc.Height = (UINT)HEIGHT;	// Resolution height for backbuffer
	swapcdesc.BufferDesc.Width = (UINT)WIDTH;  // Resolution width for backbuffer
	swapcdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // Use swap chain as output render target
	swapcdesc.OutputWindow = wndHandle; // The window rendered to
	swapcdesc.SampleDesc.Count = MULTISAMPLES; // How many multisamples will be used per pixel
	swapcdesc.Windowed = TRUE;									// Windowed mode

	// Create a device, device context and swap chain using the information in the scd struct
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		NULL, //Default adapter
		D3D_DRIVER_TYPE_HARDWARE, //Driver for the purposes of rendering and hardware acceleration
		NULL, //no need for software rasterizer
		NULL,//D3D11_CREATE_DEVICE_DEBUG, //Don't need any specific flags
		NULL, //Don't need to specify feature levels
		NULL, //Not using feature levels, don't need nr of elements
		D3D11_SDK_VERSION, //Using 
		&swapcdesc, //reference to swapchain description
		&gSwapChain, //reference to swapchain we want to create
		&gDevice, //Reference device we want to create
		NULL, //Don't need to specify feature levels
		&gDeviceContext); //immediate context

	if (SUCCEEDED(hr)) //If device and swapchain was created, then: 
	{
		// Get the address of the back buffer
		ID3D11Texture2D* pBackBuffer = nullptr; //pointer to the back buffer
		gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer); //Get the back buffer from swapchain

		// Use the back buffer address to create the render target
		HRESULT hr = gDevice->CreateRenderTargetView(pBackBuffer, NULL, &gBackbufferRTV);
		//Check to see if creating the Render target view failed
		if (FAILED(hr))
			exit(-1);  
		pBackBuffer->Release();//Don't need this object any more so we release it

		gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, gDepthStencilView); //Set render target as back buffer and bind our depth stencil view to it. 
	}
	return hr;
}

void CreateTexture() //Create texture and depth stencil
{
	//Texture Description
	D3D11_TEXTURE2D_DESC texDesc;

	ZeroMemory(&texDesc, sizeof(texDesc)); //Clear memory before usage
		texDesc.Width = BTH_IMAGE_WIDTH; //How wide is the image (in texels [texture pixels])
		texDesc.Height = BTH_IMAGE_HEIGHT; //How tall is the image (texels)
		texDesc.MipLevels = 1; //Max number of mipmaps, using 1 since we're multisampling
		texDesc.ArraySize = 1; //Number of textures
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //32 bit color 
		texDesc.SampleDesc.Count = 1; //Number of multisamples per pixel
		texDesc.SampleDesc.Quality = 0; //Not concerned with quality
		texDesc.Usage = D3D11_USAGE_DEFAULT; //Will read from and write to GPU, no CPU required
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; //We want to bind this texture buffer to the fragment shader stage 
		texDesc.MiscFlags = 0; //Don't need any misc flags
		texDesc.CPUAccessFlags = 0; //Don't need to access CPU

	//Create the texture from raw data (float*), BTH_IMAGE_DATA[] in bth_image.h
	D3D11_SUBRESOURCE_DATA texData; //For data storage
	ZeroMemory(&texData, sizeof(texData)); //Clear texData
		texData.pSysMem = (void*)BTH_IMAGE_DATA;//Point to the image's data
		texData.SysMemPitch = BTH_IMAGE_WIDTH * sizeof(char) * 4; //Every line of this texture is 4 chars long,
		texData.SysMemSlicePitch = 0; //No distance between slices

	HRESULT hr = gDevice->CreateTexture2D(&texDesc, &texData, &gTexture);
	if (FAILED(hr)) //Exit program if creation failed
		exit(-1);

	//Set up resource view description
	D3D11_SHADER_RESOURCE_VIEW_DESC resViewDesc;
	ZeroMemory(&resViewDesc, sizeof(resViewDesc));
		resViewDesc.Format = texDesc.Format; //32 bit color
		resViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; //We're getting a 2D texture
		resViewDesc.Texture2D.MipLevels = texDesc.MipLevels; //number of mipmaps
		resViewDesc.Texture2D.MostDetailedMip = 0; //Most detailed level of mipmap to use
	//Create the resource view
	hr = gDevice->CreateShaderResourceView(gTexture, &resViewDesc, &gTextureView);
	if (FAILED(hr))
		exit(-1);
	gTexture->Release(); //Don't need this object anymore, so it's released
	//Sampler description
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; //Use point sampling for minification and magnification; use linear interpolation for mip-level sampling.
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; //Wrap if textured object is larger than texture, i.e. moves outside [0,1]
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; // -||-
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; // -||-
		samplerDesc.MinLOD = 0; //Lower limit on Level of Detail
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; //No upper limit on LOD for mipmap
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; //Don't compare stored data to already stored data
	//Create Sampler
	hr = gDevice->CreateSamplerState(&samplerDesc, &gSamplerState);
	if (FAILED(hr))
		exit(-1);
	//==========================================================
	//Enable depth stencil
	//==========================================================
	D3D11_TEXTURE2D_DESC gDepthBufferDesc;
	ZeroMemory(&gDepthBufferDesc, sizeof(gDepthBufferDesc));
		gDepthBufferDesc.Width = (UINT)WIDTH; //width of viewport, not image.
		gDepthBufferDesc.Height = (UINT)HEIGHT; //height of vp
		gDepthBufferDesc.MipLevels = 1; //
		gDepthBufferDesc.ArraySize = 1; //Number of depth buffers
		gDepthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //24 bits for depth, 8 for stencil
		gDepthBufferDesc.SampleDesc.Count = MULTISAMPLES; //Same as in Direct3DContext, otherwise creates problems
		gDepthBufferDesc.SampleDesc.Quality = 0; //Don't need high quality
		gDepthBufferDesc.Usage = D3D11_USAGE_DEFAULT; //Read-write by GPU, not touched by CPU.
		gDepthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; //Binding a depth stencil to the shader stage
		gDepthBufferDesc.CPUAccessFlags = 0; //Don't need CPU access
		gDepthBufferDesc.MiscFlags = 0; //Don't need anything from here

	hr = gDevice->CreateTexture2D(&gDepthBufferDesc, nullptr, &gDepthStencilBuffer);
	if (FAILED(hr))
		exit(-1);


	//Create a view of the depth stencil buffer.
	D3D11_DEPTH_STENCIL_VIEW_DESC gDepthStencilViewDesc;
	ZeroMemory(&gDepthStencilViewDesc, sizeof(gDepthStencilViewDesc));
		gDepthStencilViewDesc.Format = gDepthBufferDesc.Format; //Same format as depth buffer
		gDepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS; //The resource will be accessed as 2D texture with multisampling
	
		hr = gDevice->CreateDepthStencilView(gDepthStencilBuffer, &gDepthStencilViewDesc, &gDepthStencilView);
	if (FAILED(hr))
		exit(-1);
	gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, gDepthStencilView);// Set render target to back buffer
	gDepthStencilBuffer->Release();
	
	//Depth stencil desc
	D3D11_DEPTH_STENCIL_DESC gDepthStencilDesc;
		//Depth test parameters
		gDepthStencilDesc.DepthEnable = true;  //enable depth testing
		gDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; //what portion of the stencil can be modified by depth data
		gDepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; //If the comparison with existing data is less than what exists, then pass.
		// Stencil test parameters
		gDepthStencilDesc.StencilEnable = true;
		gDepthStencilDesc.StencilReadMask = 0xFF;
		gDepthStencilDesc.StencilWriteMask = 0xFF;
		// Stencil operations if pixel is front-facing
		gDepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		gDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		gDepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		gDepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		// Stencil operations if pixel is back-facing
		gDepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		gDepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		gDepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		gDepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	ID3D11DepthStencilState* gDepthStencilState;
	hr = gDevice->CreateDepthStencilState(&gDepthStencilDesc, &gDepthStencilState);
	if (FAILED(hr))
		exit(-1);
	gDeviceContext->OMSetDepthStencilState(gDepthStencilState, 0); //Bind to output-merger

	gDepthStencilState->Release(); //Don't need, release
};

void SetViewport()//Setting up DirectX viewport for context
{
	D3D11_VIEWPORT vp;
	vp.Width = WIDTH;
	vp.Height = HEIGHT;
	vp.MinDepth = 0.0f; // 0<=z
	vp.MaxDepth = 1.0f; // z<=1
	vp.TopLeftX = 0; // -----------> x
	vp.TopLeftY = 0; // v y
	gDeviceContext->RSSetViewports(1, &vp);
}

void CreateShaders()
{
	//Vertex shader
	ID3DBlob* pVS = nullptr; //make a blob to contain data for our shader
	D3DCompileFromFile(
		L"Vertex.hlsl", // Name of file
		nullptr,		// optional macros
		nullptr,		// optional include files
		"VS_main",		// entry point in .hlsl file
		"vs_5_0",		// shader model (target) (using version 5.0)
		0,				// shader compile options			
		0,				// effect compile options
		&pVS,			// double pointer to ID3DBlob		
		nullptr			// pointer for Error Blob messages.
	);

	HRESULT hr = gDevice->CreateVertexShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr/*optional class linkage*/, &gVertexShader);
	if (FAILED(hr))
		exit(-1);

	//create input layout (verified using vertex shader)
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = gDevice->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), pVS->GetBufferPointer(), pVS->GetBufferSize(), &gVertexLayout); //create input-layout for the assembler stage
	if (FAILED(hr))
		exit(-1);
	// Since we do not need this COM object anymore, we release it.
	pVS->Release();

	//create pixel shader
	ID3DBlob* pPS = nullptr;
	D3DCompileFromFile(
		L"Fragment.hlsl", // file name
		nullptr,		// optional macros
		nullptr,		// optional include files
		"PS_main",		// entry point
		"ps_5_0",		// shader model (target)
		0,				// shader compile options
		0,				// effect compile options
		&pPS,			// double pointer to ID3DBlob		
		nullptr			// pointer for Error Blob messages.
	);

	hr = gDevice->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &gPixelShader);
	if (FAILED(hr))
		exit(-1);
	pPS->Release();

	ID3DBlob* pGS = nullptr;
	D3DCompileFromFile(
		L"GeometryShader.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"GS_main",		// entry point
		"gs_5_0",		// shader model (target)
		0,				// shader compile options
		0,				// effect compile options
		&pGS,
		nullptr
	);

	hr = gDevice->CreateGeometryShader(pGS->GetBufferPointer(), pGS->GetBufferSize(), nullptr, &gGeometryShader);
	if (FAILED(hr))
		exit(-1);
	pGS->Release();
}

void CreateQuadData() //Create vertex buffer of a quad
{
	//Define quad
	TriangleVertex triangleVertices[VERTEXCOUNT] =
	{
		//Triangle 1
		-0.5f, 0.5f, 0.0f,	//v0 pos
		0.0f, 0.0f,			//v0 tex coordinates (uv)
		0.5f, 0.5f, 0.0f,	//v1
		1.0f, 0.0f,			//v1 tex
		0.5f, -0.5f, 0.0f,  //v2
		1.0f, 1.0f,			//v2 tex

		//Triangle 2
		-0.5f, 0.5f, 0.0f,	//v0 pos
		0.0f, 0.0f,			//v0 tex
		0.5f, -0.5f, 0.0f,	//v1 pos  
		1.0f, 1.0f,			//v1 tex
		-0.5f, -0.5f, 0.0f, //v2 pos
		0.0f, 1.0f,			//v2 tex

	};

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(triangleVertices);

	D3D11_SUBRESOURCE_DATA data = {}; //stores data
	data.pSysMem = triangleVertices; //Points to data
	HRESULT hr = gDevice->CreateBuffer(&bufferDesc, &data, &gVertexBuffer); //Create buffer and store it in data
	if (FAILED(hr))
		exit(-1);
}

void CreateConstantBufferData()
{
	//world
	XMStoreFloat4x4(&wvp.world, XMMatrixTranspose(XMMatrixRotationY(0))); //Define world as a rotation matrix of 0 rads.
	//view
	XMStoreFloat4x4(&wvp.view,
		(
			XMMatrixTranspose(XMMatrixLookAtLH( //Left-handed,i.e. x is right and z is in
				CAM_POS,	//Eye Position
				{ 0.0f, 0.0f, 0.0f, 1.0f },	//Look at position
				{ 0.0f, 1.0f, 0.0f, 1.0f }  //Up
			)))
	);
	//proj
	XMStoreFloat4x4(&wvp.project,
		(
			XMMatrixTranspose(
				XMMatrixPerspectiveFovLH(
					PI * 0.45f,		//FOV 80
					(WIDTH / HEIGHT),  //Aspect Ratio
					0.1f,			//NearZ
					20.0f			//FarZ
				))
			)
	);

	//Light for fragment shader
	XMStoreFloat4(&light.pos, { 0.0f, 1.0f, -3.0f, 1.0f });
	XMStoreFloat4(&light.color, { 1.0f, 0.84f, 0.0f, 1.0f }); //Gold
	XMStoreFloat(&light.ambient, { 0.2f });
	XMStoreFloat(&light.specular, { 10.0f }); //How "Shiny" the material will be; higher number means less shiny, 0.0f is maximum shine
}

void CreateConstantBuffers() //Constant buffer to supply matrices to the GeoShader
{
	//Create a description for the constant buffer
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; //Dynamic because on top of GPU operations, the CPU will write to it (matrices)
	bufferDesc.ByteWidth = sizeof(Matrices);//Size of the buffer will be 3 4x4float matrices
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; //Identity of the buffer
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //Allowed to write, not read
	bufferDesc.MiscFlags = 0; //No additional flags, don't need them
	bufferDesc.StructureByteStride = 0; //Not using structured buffers

	//Checking if the creation failed for any reason
	HRESULT hr = 0;
	hr = gDevice->CreateBuffer(&bufferDesc, nullptr, &gGeoCBuffer);
	if (FAILED(hr)) //If creation failed, exit program;
		exit(-1);

	bufferDesc.ByteWidth = sizeof(Light); //Change Byte width for this one, keep the rest of the settings the same
	hr = gDevice->CreateBuffer(&bufferDesc, nullptr, &gPixelCBuffer);
	if (FAILED(hr)) //If creation failed, exit program;
		exit(-1);

}

void Render()
{
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	gDeviceContext->ClearRenderTargetView(gBackbufferRTV, clearColor); //Clear backbuffer
	gDeviceContext->ClearDepthStencilView(gDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); //Clear Depth stencil view

	UINT32 vertexSize = sizeof(TriangleVertex); //Will be the size of 5 floats, x y z u v
	UINT32 offset = 0;
	
	//Setting Shaders for this render pass
	//VertexShader
	gDeviceContext->VSSetShader(gVertexShader, nullptr, 0); //set vshader, no class instances
	gDeviceContext->IASetVertexBuffers(0, 1, &gVertexBuffer, &vertexSize, &offset);  //Set buffer to the one created in "Create Quad Data"
	gDeviceContext->IASetInputLayout(gVertexLayout); //Set input layout to the layout of our vertex shader
	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //could use strips but this is easier for me
	//GeoShader
	gDeviceContext->GSSetShader(gGeometryShader, nullptr, 0);
	//Pixelshader + texture
	gDeviceContext->PSSetShader(gPixelShader, nullptr, 0);
	gDeviceContext->PSSetShaderResources(0, 1, &gTextureView); //Set texture view
	gDeviceContext->PSSetSamplers(0, 1, &gSamplerState); //Set texture sampler

	//Mapping constant buffer for geo
	D3D11_MAPPED_SUBRESOURCE dataPtr; //stores data
	gDeviceContext->Map(gGeoCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr); //Gets a pointer to the data contained in a subresource, and denies the GPU access to that subresource.
	Rotate(); //Rotate based on delta time
	//Feed updated rotation variable into matrices
	XMStoreFloat4x4(&wvp.world, XMMatrixTranspose(XMMatrixRotationY(rot)));
	memcpy(dataPtr.pData, &wvp, sizeof(wvp)); //Store data into 
	//Unmap CBuffer
	gDeviceContext->Unmap(gGeoCBuffer, 0);
	//Feed the constant buffer to Geometry Shader (omnomnom)
	gDeviceContext->GSSetConstantBuffers(0, 1, &gGeoCBuffer);

	D3D11_MAPPED_SUBRESOURCE dataPtr2; //stores data
	gDeviceContext->Map(gPixelCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr2);
	memcpy(dataPtr2.pData, &light, sizeof(light));
	gDeviceContext->Unmap(gPixelCBuffer, 0);
	gDeviceContext->PSSetConstantBuffers(0, 1, &gPixelCBuffer);

	// Draw geometry
	gDeviceContext->Draw(VERTEXCOUNT, 0); //Submits vertices to the rendering pipeline
}

//===============================================================================
//Helper functions
//===============================================================================
void ReleaseCOMObjects() //Release all the objects we no longer need.
{
	//Release shaders
	gVertexBuffer->Release();
	gVertexLayout->Release();
	gVertexShader->Release();
	gGeometryShader->Release();
	gPixelShader->Release();
	//Release texture
	gTextureView->Release();
	gDepthStencilBuffer->Release();
	gDepthStencilView->Release();
	gSamplerState->Release();
	//Release CBuffers
	gGeoCBuffer->Release();
	gPixelCBuffer->Release();
	//Release context and swapchain interface
	gBackbufferRTV->Release();
	gSwapChain->Release();
	gDevice->Release();
	gDeviceContext->Release();
}

void Rotate()
{
	auto r_time = std::chrono::high_resolution_clock::now();
	float deltaT = 0.0f;
	std::chrono::duration<float, std::milli> dur = r_time - s_time;
	deltaT = dur.count();
	if (deltaT >= ((1.0f / 60.0f) * 1000))
	{
		rot += (1.0f * deltaT) / 1000.0f;
		//rot /= 1.0f;
		s_time = r_time;
	}
}
#include "D3DHandler.h"

#include <iostream>
#include <fstream>

bool D3DHandler::CreateInterfaces(HWND window) //Creating context and swapchain
{
	UINT flags = 0;
	if (_DEBUG)
		flags = D3D11_CREATE_DEVICE_DEBUG; //If debugging, use this flag
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC)); //Make sure it's empty

	swapChainDesc.BufferDesc.Width = (UINT)WIDTH;
	swapChainDesc.BufferDesc.Height = (UINT)HEIGHT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = window;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, featureLevels, 1, D3D11_SDK_VERSION, &swapChainDesc, & this->swapChain, & this->device, NULL, &this->immediateContext);
	
	return (!FAILED(hr));
}
bool D3DHandler::CreateRTV()
{
	ID3D11Texture2D * backBuffer = nullptr;

	if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer))))//https://en.cppreference.com/w/cpp/language/reinterpret_cast
	{
		std::cerr << "Failed to get back buffer!" << std::endl;
	}

	HRESULT hr = device->CreateRenderTargetView(backBuffer, NULL, &this->rtv);
	backBuffer->Release();
	return !(FAILED(hr));
}
bool D3DHandler::CreateDepthStencil() 
{
	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = (UINT)WIDTH;
	textureDesc.Height = (UINT)HEIGHT;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	if (FAILED(device->CreateTexture2D(&textureDesc, nullptr, &dsTexture)))
	{
		std::cerr << "Failed to create depth stencil texture!" << std::endl;
		return false;
	}

	HRESULT hr = device->CreateDepthStencilView(dsTexture, 0, &dsView);
	return !(FAILED(hr));
}

void D3DHandler::SetViewport()
{
	this->viewport.TopLeftX = 0;
	this->viewport.TopLeftY = 0;
	this->viewport.Width = WIDTH;
	this->viewport.Height = HEIGHT;
	this->viewport.MinDepth = 0;
	this->viewport.MaxDepth = 1;
}



D3DHandler::D3DHandler()
{
	this->device = nullptr;
	this->immediateContext = nullptr;
	this->swapChain = nullptr;
	this->rtv = nullptr;
	this->dsTexture = nullptr;
	this->dsView = nullptr;
	D3D11_VIEWPORT viewport = {};
}

D3DHandler::~D3DHandler()
{
	//Release COM objects if they haven't already been released
	this->ReleaseCOMObjects();
}
D3DHandler::D3DHandler(const D3DHandler & origObj)
{
}

bool D3DHandler::SetupDirect3D(HWND &wndHandle)
{
	if (!this->CreateInterfaces(wndHandle)) 
	{
		std::cerr << "Error creating interfaces!" << std::endl;
		return false;
	}

	if (!this->CreateRTV())
	{
		std::cerr << "Error creating rtv!" << std::endl;
		return false;
	}

	if (!this->CreateDepthStencil())
	{
		std::cerr << "Error creating depth stencil view!" << std::endl;
		return false;
	}

	SetViewport();

	if (!this->SetupPipeline())
	{
		std::cerr << "Error setting up the pipeline!" << std::endl;
		return false;
	}

	return true;
}

bool D3DHandler::SetupPipeline()
{
	return (ph.SetupPipeline(this->device));
}

void D3DHandler::Render()
{
}

void D3DHandler::PresentBackBuffer()
{
	this->swapChain->Present(0, 0);
}

void D3DHandler::ReleaseCOMObjects()
{
	//Release COM objects if they haven't already been released
	if (!(this->device == nullptr))
		this->device->Release();
	if (!(this->immediateContext == nullptr))
		this->immediateContext->Release();
	if (!(this->swapChain == nullptr))
		this->swapChain->Release();
	if (!(this->rtv == nullptr))
		this->rtv->Release();
	if (!(this->dsTexture == nullptr))
		this->dsTexture->Release();
	if (!(this->dsView == nullptr))
		this->dsView->Release();
}

ID3D11Device * D3DHandler::GetDevice() const
{
	return this->device;
}

ID3D11DeviceContext * D3DHandler::GetImmediateContext() const
{
	return immediateContext;
}

IDXGISwapChain * D3DHandler::GetSwapChain() const
{
	return this->swapChain;
}

ID3D11RenderTargetView * D3DHandler::GetRTV() const
{
	return this->rtv;
}

ID3D11Texture2D * D3DHandler::GetDepthStencilTexture() const
{
	return this->dsTexture;
}

ID3D11DepthStencilView * D3DHandler::GetDepthStencilView() const
{
	return this->dsView;
}

D3D11_VIEWPORT D3DHandler::GetViewPort() const
{
	return this->viewport;
}


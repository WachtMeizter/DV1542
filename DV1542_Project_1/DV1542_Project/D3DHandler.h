#ifndef D3DHANDLER_H_
#define D3DHANDLER_H_

#include <d3d11.h>
//#include <d3dcompiler.h>

#include "Constants.h"
#include "PipelineHandler.h"

class D3DHandler 
{	
private:
	//For pipelined related stuff
	PipelineHandler ph;
	//Interface
	ID3D11Device* device;
	ID3D11DeviceContext* immediateContext;
	IDXGISwapChain* swapChain;
	ID3D11RenderTargetView* rtv;
	ID3D11Texture2D* dsTexture;
	ID3D11DepthStencilView* dsView;
	D3D11_VIEWPORT viewport;
	//Helper functions
	bool CreateInterfaces(HWND wHandle);
	bool CreateRTV();
	bool CreateDepthStencil();
	void SetViewport();
public:
	D3DHandler();
	~D3DHandler();
	D3DHandler(const D3DHandler &origObj);
	bool SetupDirect3D(HWND &wndHandle);
	bool SetupPipeline();
	void Render();
	void PresentBackBuffer();
	void ReleaseCOMObjects();
	ID3D11Device* GetDevice()const;
	ID3D11DeviceContext* GetImmediateContext()const;
	IDXGISwapChain* GetSwapChain()const;
	ID3D11RenderTargetView* GetRTV()const;
	ID3D11Texture2D* GetDepthStencilTexture()const;
	ID3D11DepthStencilView* GetDepthStencilView()const;
	D3D11_VIEWPORT GetViewPort()const;

};
#endif
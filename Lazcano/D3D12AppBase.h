#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <wrl.h>

class D3D12AppBase
{
public:

	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	D3D12AppBase();
	virtual ~D3D12AppBase();

	void Initialize(HWND _hwnd);
	void Terminate();

	virtual void Render();

	virtual void Prepare() {};
	virtual void CleanUp() {};
	virtual void MakeCommand(ComPtr<ID3D12CommandList>& _command) {};

	const UINT FrameBufferCount = 2;  // バッファリング数

protected:



};
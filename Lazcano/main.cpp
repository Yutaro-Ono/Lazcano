#include <Windows.h>
#include <tchar.h>
#include <vector>
// DirectX�w�b�_�[/dxgi���C�u���� �C���N���[�h&�����N
#include <d3d12.h>
#include <dxgi1_6.h>   // �듮�삷��ꍇ��1_4�ɂ��Ă݂�
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;

// �ϐ��Q
const unsigned int WINDOW_WIDTH = 1600;
const unsigned int WINDOW_HEIGHT = 900;
// Direct3D��{�I�u�W�F�N�g
ID3D12Device* dev = nullptr;
IDXGIFactory6* dxgiFactory = nullptr;
ID3D12CommandAllocator* cmdAllocator = nullptr;
ID3D12GraphicsCommandList* cmdList = nullptr;
ID3D12CommandQueue* cmdQueue = nullptr;
IDXGISwapChain4* swapChain = nullptr;

// @brief �R���\�[����ʂɃt�H�[�}�b�g�t���������`��
// @param format �t�H�[�}�b�g (%d %f)
// @param �ϒ�����
// @remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFormatString(const char* _format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, _format);
	vprintf(_format, valist);
	va_end(valist);
#endif
}

// �E�B���h�E�����p �E�B���h�E�v���V�[�W��
LRESULT WindowProcedure(HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam)
{
	// �E�B���h�E�j�����ɌĂяo��
	if (_msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OS�ɑ΂��ăA�v���I���̖���
		return 0;
	}

	return DefWindowProc(_hwnd, _msg, _wparam, _lparam);
}

#ifdef _DEBUG

int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif

	// �A�v���P�[�V�����̃C���X�^���X�n���h���擾
	HINSTANCE hInst = GetModuleHandle(nullptr);

	// �E�B���h�E�N���X�̐���&�o�^
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;           // �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("Lazcano");                    // �A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr);             // �n���h���̎擾
	RegisterClassEx(&w);                                // �A�v���P�[�V�����N���X(�E�B���h�E�N���X�̎w���OS�ɓ`�B)

	RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };   // �E�B���h�E�T�C�Y�w��
	// �֐�����E�B���h�E�̃T�C�Y�␳
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow
	(
		w.lpszClassName,        // �N���X���w��
		_T("Lazcano"),          // �^�C�g���o�[�\������
		WS_OVERLAPPEDWINDOW,    // �^�C�g���o�[�Ƃ̋��E��������E�B���h�E���w��
		CW_USEDEFAULT,          // �E�B���h�E�\����x���W��OS�ɔC����
		CW_USEDEFAULT,          // �E�B���h�E�\����y���W��OS�ɔC����
		wrc.right - wrc.left,   // �E�B���h�E��
		wrc.bottom - wrc.top,   // �E�B���h�E����
		nullptr,                // �e�E�B���h�E�̃n���h��(�e�̂��ߎw��Ȃ�)
		nullptr,                // ���j���[�n���h��
		w.hInstance,            // �Ăяo���A�v���P�[�V�����n���h��
		nullptr                 // �ǉ��p�����[�^
	);

	// DirectX �e�������֐��̐�������m�F�p
	HRESULT result = S_OK;

#ifdef _DEBUG

	// Direct3D�̃f�o�b�O���C���[ON (�o�͂ɏ��\��)
	ID3D12Debug* debugLayer = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	if (result == S_OK)
	{
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

#endif

	//-------------------------------------------------//
	// Direct3D�֘A ������
	//------------------------------------------------//
	// dxgi �f�o�b�O�ƃ����[�X�ŕ�����
#ifdef _DEBUG
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory));
	if (result != S_OK)
	{
		result = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
	}
#else
	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
#endif

	// ����GPU����PC�΍�
	// �A�_�v�^�R���e�i�ɏ����i�[���A"NVIDIA"���[�h������A�_�v�^��D��g�p
	IDXGIAdapter* tmpAdapter = nullptr;
	std::vector<IDXGIAdapter*> adapters;
	for (int i = 0; dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.emplace_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		// �A�_�v�^�����I�u�W�F�N�g�擾
		DXGI_ADAPTER_DESC adapterDesc = {};
		adpt->GetDesc(&adapterDesc);
		// "NVIDIA"���[�h�����閼�O���g�p
		std::wstring strDesc = adapterDesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}
	if (tmpAdapter == nullptr)
	{
		// NVIDIA GPU���擾�ł��Ȃ������ꍇ�͍ŏ��̃A�_�v�^��I��
		tmpAdapter = adapters[0];
	}
	// DirectX12 �t�B�[�`���[���x�� : �h���C�o�ɑΉ����Ă��Ȃ�PC�ł͈�i����Lv�𗎂Ƃ��Ă���
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	D3D_FEATURE_LEVEL featureLevel;

	// Direct3D�f�o�C�X������
	for (auto fLv : featureLevels)
	{
		// �Ԃ�l�̕�"HRESULT"�͐�������"S_OK"�ƂȂ�A���s����"S_OK"�ȊO�ƂȂ�
		if (D3D12CreateDevice(tmpAdapter, fLv, IID_PPV_ARGS(&dev)) == S_OK)
		{
			featureLevel = fLv;
			break;
		}
	}
	
	// �R�}���h�A���P�[�^/�R�}���h���X�g�̍쐬
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));

	// �R�}���h�L���[�̐��� (�R�}���h�A���P�[�^�ɒ~�ς����R�}���h���X�g�����s����ɂ̓L���[���K�v)
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;            // �^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;                                     // �g�pGPU�A�_�v�^��1�̏ꍇ��0
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;   // �v���C�I���e�B(�D��x)�w��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;            // �R�}���h���X�g��TYPE�ƍ��킹��
	result = dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));

	// �X���b�v�`�F�C���̐��� (�_�u���o�b�t�@�����O�ɕK�v)
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = WINDOW_WIDTH;                             // ��ʉ𑜓x(��)
	swapChainDesc.Height = WINDOW_HEIGHT;                           // ��ʉ𑜓x(�c)
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;              // �s�N�Z���t�H�[�}�b�g
	swapChainDesc.Stereo = false;                                   // �X�e���I�\���t���O(3D�f�B�X�v���C�̃X�e���I���[�h)
	swapChainDesc.SampleDesc.Count = 1;                             // �}���`�T���v���̎w��(Count = 1�ł悢)
	swapChainDesc.SampleDesc.Quality = 0;                           // �}���`�T���v���̎w��(Quality = 1�ł悢)
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;             // DXGI_USAGE_BACK_BUFFER�ł悢
	swapChainDesc.BufferCount = 2;                                  // �_�u���o�b�t�@��2
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;                   // �o�b�N�o�b�t�@�̐L�k���\
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;       // �t���b�v��ɑ��j��
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;          // �A���t�@���[�h�̎w��Ȃ�
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // �E�B���h�E�ƃt���X�N���[���̐؂�ւ����\
	result = dxgiFactory->CreateSwapChainForHwnd
	(
		cmdQueue,
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapChain
	);

	// �f�B�X�N���v�^�q�[�v�̐���
	// �f�B�X�N���v�^�Ƃ�GPU���\�[�X�̗p�r��g�p�@���������f�[�^
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;     // �����_�[�^�[�Q�b�g�r���[(RTV)
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;                        // �\��2�Ԃ�
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;   // �w��Ȃ�
	ID3D12DescriptorHeap* rtvHeaps = nullptr;           // �����_�[�^�[�Q�b�g�r���[�p�q�[�v
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	// �f�B�X�N���v�^�ƃX���b�v�`�F�C���̃o�b�N�o�b�t�@�R�t��(�e�\��2��)
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = swapChain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (unsigned int i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
		// �����_�[�^�[�Q�b�g�r���[�ƃo�b�N�o�b�t�@�̕R�t��
		dev->CreateRenderTargetView(backBuffers[i], nullptr, descriptorHandle);
		// ���̃f�B�X�N���v�^�Ɉړ�
		descriptorHandle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �t�F���X�̐���
	// �t�F���X�Ƃ�GPU���̏���(�R�}���h���X�g�̖���)�������������ǂ�����m�点��d�g��
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;
	result = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	if (result != S_OK)
	{
		return -1;
	}

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	// ���b�Z�[�W���[�v
	MSG msg = {};
	// �A�v���P�[�V�����I������message��WM_QUIT�ɂȂ�
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//-------------------------------------------//
		// DirectX�`�揈��
		//------------------------------------------//
		// �X���b�v�`�F�C�����猻�݂̃o�b�N�o�b�t�@(�����_�[�^�[�Q�b�g�r���[)�̃C���f�b�N�X�擾
		auto bbIndex = swapChain->GetCurrentBackBufferIndex();

		// ���\�[�X�o���A��`
		// �o���A�Ƃ�GPU�Ƀ��\�[�X�̏�ԑJ�ڂ̏󋵂�`�B�������
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;                      // �^�C�v:�J��
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Transition.pResource = backBuffers[bbIndex];                        // ���\�[�X(�o�b�N�o�b�t�@)
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;              // ���O��PRESENT���
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;         // ���ꂩ�烌���_�[�^�[�Q�b�g���
		cmdList->ResourceBarrier(1, &barrierDesc);                                      // �o���A���s

		// �����_�[�^�[�Q�b�g�̎w��
		auto rtvHandle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// �����_�[�^�[�Q�b�g�̃N���A
		float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };   // ���F
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);   // ���݂̃����_�[�^�[�Q�b�g���w��F�ŃN���A

		// �o���A�̑J�ڏ�Ԃ�PRESENT��
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		cmdList->ResourceBarrier(1, &barrierDesc);

		// �R�}���h���X�g(�`�施��)���s�O�ɕK���R�}���h���X�g�̃N���[�Y���s��
		cmdList->Close();

		// �R�}���h���X�g�̎��s
		ID3D12CommandList* cmdLists[] = { cmdList };
		cmdQueue->ExecuteCommandLists(1, cmdLists);
		// �R�}���h���X�g�̎��s��GPU���ŏI������܂őҋ@
		cmdQueue->Signal(fence, ++fenceVal);
		while (fence->GetCompletedValue() != fenceVal)
		{
			// �C�x���g�n���h���̎擾
			auto eventHandle = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, eventHandle);
			// �C�x���g����������܂őҋ@
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
		
		// �R�}���h���X�g���s��̓L���[�ƃ��X�g�����Z�b�g����
		cmdAllocator->Reset();
		cmdList->Reset(cmdAllocator, nullptr);


		// ��ʂ̃X���b�v
		swapChain->Present(1, 0);   // ��1�����͑ҋ@�t���[����(�ҋ@���鐂�������̐�):0�ɂ���Ɛ���������҂����������A


	}

	// �N���X�̓o�^����
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}

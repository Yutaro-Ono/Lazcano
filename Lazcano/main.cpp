#include <Windows.h>
#include <tchar.h>
#include <vector>
// DirectX�w�b�_�[/dxgi���C�u���� �C���N���[�h&�����N
#include <d3d12.h>
#include <dxgi1_6.h>       // �듮�삷��ꍇ��1_4�ɂ��Ă݂�
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
// �V�F�[�_�[�R���p�C���ɕK�v
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#include <iostream>
#endif
#include <DirectXMath.h>   // ���w���C�u����
#include <vector>

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

	// ���_�z��
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;   // ���_���W(xyz)
		DirectX::XMFLOAT2 uv;    // UV���W
	};
	
	// ���v���ɂȂ�悤�ɒ��_���`
	Vertex vertices[] =
	{
		{{ -0.4f, -0.7f, 0.0f }, {0.0f, 1.0f}},  // ���� index:0
		{{ -0.4f,  0.7f, 0.0f }, {0.0f, 0.0f}},  // ���� index:1
		{{  0.4f, -0.7f, 0.0f }, {1.0f, 1.0f}},  // �E�� index:2
		{{  0.4f,  0.7f, 0.0f }, {1.0f, 0.0f}},  // �E�� index:3
	};
	// ���_�o�b�t�@�����p�̃f�[�^�Q�𐶐�
	// �q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;                       // CPU�ւ̃A�N�Z�X�����Ȃ� ���̕��x��
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;   // HEAP_TYPE��CUSTOM�ȊO�Ȃ�UNKNOWN�ł悢
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;    // HEAP_TYPE��CUSTOM�ȊO�Ȃ�UNKNOWN�ł悢
	// ���\�[�X�ݒ�\����
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;     // �o�b�t�@�Ƃ��Ďg�p���邽��BUFFER�ł悢
	resourceDesc.Width = sizeof(vertices);                        // ���_�o�b�t�@�ł��邽�ߏ��͕��ɋl�߂�
	resourceDesc.Height = 1;                                      // �摜�f�[�^�ł͂Ȃ�����1�ł悢
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;                    // �摜�ł͂Ȃ�����UNKNOWN�ł悢
	resourceDesc.SampleDesc.Count = 1;                            // 0���ƃf�[�^�����݂��Ȃ����ƂɂȂ邽��1
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;         // �e�N�X�`�����C�A�E�g�����AROW_MAJOR�Ƃ��Ă���

	// ���_�o�b�t�@�̐���
	ID3D12Resource* vertBuffer = nullptr;
	result = dev->CreateCommittedResource
	(
		&heapProp,                           // �q�[�v�ݒ�\���̃A�h���X
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,                       // ���\�[�X�ݒ�\���̃A�h���X
		D3D12_RESOURCE_STATE_GENERIC_READ,   // GPU������͓ǂݎ���p
		nullptr,                             // ���g�p
		IID_PPV_ARGS(&vertBuffer)
	);

	// ���_�o�b�t�@�ɒ��_���R�s�[
	Vertex* vertMap = nullptr;
	result = vertBuffer->Map(0, nullptr, (void**)&vertMap);         // �o�b�t�@�̉��z�A�h���X�擾
	std::copy(std::begin(vertices), std::end(vertices), vertMap);   // �}�b�v�ɒ��_���R�s�[
	vertBuffer->Unmap(0, nullptr);                                  // ���R�s�[���I�����̂Ń}�b�v����

	// ���_�o�b�t�@�r���[�̐���
	// ���_�o�b�t�@�r���[�Ƃ́A���_�o�b�t�@�S�̂̃o�C�g���A1���_������̃o�C�g����m�点��f�[�^
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertBuffer->GetGPUVirtualAddress();   // �o�b�t�@�̉��z�A�h���X�擾
	vertexBufferView.SizeInBytes = sizeof(vertices);                        // �S�o�C�g��
	vertexBufferView.StrideInBytes = sizeof(vertices[0]);                   // 1���_�ӂ�̃o�C�g��

	// �C���f�b�N�X�̒�`
	unsigned short indices[] = { 0,1,2,   2,1,3 };
	ID3D12Resource* indexBuffer = nullptr;
	resourceDesc.Width = sizeof(indices);
	result = dev->CreateCommittedResource
	(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)
	);
	// �C���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIndex = nullptr;
	indexBuffer->Map(0, nullptr, (void**)&mappedIndex);
	std::copy(std::begin(indices), std::end(indices), mappedIndex);
	indexBuffer->Unmap(0, nullptr);
	//�C���f�b�N�X�o�b�t�@�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = sizeof(indices);

	// �V�F�[�_�[�I�u�W�F�N�g�̐���
	// ID3DBlob(BinaryLargeObject)�Ƃ͔ėp�I�ȃf�[�^�^:�f�[�^�̃|�C���^�ƃT�C�Y������
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	// ���_�V�F�[�_�[
	result = D3DCompileFromFile
	(
		L"BasicVertexShader.hlsl",                          // �t�@�C����(���C�h������ L"")
		nullptr,                                            // �V�F�[�_�[�}�N���I�u�W�F�N�g
		D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // �C���N���[�h�I�u�W�F�N�g
		"BasicVS",                                          // �G���g���|�C���g(�Ăяo���V�F�[�_�[��)
		"vs_5_0",                                           // �ǂ̃V�F�[�_�[�����蓖�Ă邩(vs,ps,etc...)
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    // �V�F�[�_�[�R���p�C���I�v�V����
		0,                                                  // �G�t�F�N�g�R���p�C���I�v�V����
		&vsBlob,                                            // �󂯎��|�C���^�A�h���X
		&errorBlob                                          // �G���[�p�|�C���^�A�h���X
	);
	if (result != S_OK)
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			OutputDebugStringA("vs_hlsl::FileNotFound\n");
		}
		else
		{
			std::string errorStr;
			errorStr.resize(errorBlob->GetBufferSize());
			std::copy_n(static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize(), errorStr.begin());
			errorStr += "\n";
			OutputDebugStringA(errorStr.c_str());
		}
	}
	// �s�N�Z���V�F�[�_�[
	result = D3DCompileFromFile
	(
		L"BasicPixelShader.hlsl",                           // �t�@�C����(���C�h������ L"")
		nullptr,                                            // �V�F�[�_�[�}�N���I�u�W�F�N�g
		D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // �C���N���[�h�I�u�W�F�N�g
		"BasicPS",                                          // �G���g���|�C���g(�Ăяo���V�F�[�_�[��)
		"ps_5_0",                                           // �ǂ̃V�F�[�_�[�����蓖�Ă邩(vs,ps,etc...)
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    // �V�F�[�_�[�R���p�C���I�v�V����
		0,                                                  // �G�t�F�N�g�R���p�C���I�v�V����
		&psBlob,                                            // �󂯎��|�C���^�A�h���X
		&errorBlob                                          // �G���[�p�|�C���^�A�h���X
	);
	if (result != S_OK)
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			OutputDebugStringA("ps_hlsl::FileNotFound\n");
		}
		else
		{
			std::string errorStr;
			errorStr.resize(errorBlob->GetBufferSize());
			std::copy_n(static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize(), errorStr.begin());
			errorStr += "\n";
			OutputDebugStringA(errorStr.c_str());
		}
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		// ���W���
		{
		    "POSITION",                                   // �Z�}���e�B�N�X��
		    0,                                            
		    DXGI_FORMAT_R32G32B32_FLOAT,                  // �t�H�[�}�b�g
		    0,
		    D3D12_APPEND_ALIGNED_ELEMENT,
		    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		    0
		},

		// UV���
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
        },
	};

	// �e�N�X�`���f�[�^(��)
	struct TexRGBA
	{
		unsigned char R, G, B, A;
	};
	std::vector<TexRGBA> textureData(256 * 256);   // 256x256
	for (auto& rgba : textureData)
	{
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 255;
	}
	// �e�N�X�`���o�b�t�@�̐��� (WriteToSubresource�œ]��)
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;                          // ����ݒ�̂���CUSTOM
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;   // ���C�g�o�b�N����
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;            // CPU���璼�ړ]���̂���L0
	texHeapProp.CreationNodeMask = 0;                                   // �P��A�_�v�^�̂���
	texHeapProp.VisibleNodeMask = 0;
	// �e�N�X�`���p���\�[�X�ݒ�
	D3D12_RESOURCE_DESC texResourceDesk = {};
	texResourceDesk.Format = DXGI_FORMAT_R8G8B8A8_UNORM;   // RGBA�t�H�[�}�b�g
	texResourceDesk.Width = 256;
	texResourceDesk.Height = 256;
	texResourceDesk.DepthOrArraySize = 1;                             // 2D�Ŕz��ł��Ȃ�����1
	texResourceDesk.SampleDesc.Count = 1;                             // �ʏ�e�N�X�`���̂���AA�������Ȃ�
	texResourceDesk.SampleDesc.Quality = 0;                           // �Œ�N�I���e�B
	texResourceDesk.MipLevels = 1;                                    // �~�b�v�}�b�v�͎g�p���Ȃ�
	texResourceDesk.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;   // 2D�e�N�X�`���p
	texResourceDesk.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;            // ���C�A�E�g�͌��߂Ȃ�
	texResourceDesk.Flags = D3D12_RESOURCE_FLAG_NONE;                 // �t���O�Ȃ�
	// �e�N�X�`���p���\�[�X����


	// �O���t�B�b�N�X�p�C�v���C���̐���
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeline = {};
	graphicsPipeline.pRootSignature = nullptr;
	graphicsPipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	graphicsPipeline.VS.BytecodeLength = vsBlob->GetBufferSize();
	graphicsPipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	graphicsPipeline.PS.BytecodeLength = psBlob->GetBufferSize();
	graphicsPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// �u�����f�B���O�͍��񖢎g�p
	graphicsPipeline.BlendState.AlphaToCoverageEnable = false;
	graphicsPipeline.BlendState.IndependentBlendEnable = false;
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	// �_�����Z�͖��g�p
	renderTargetBlendDesc.LogicOpEnable = false;
	graphicsPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;
	// ���X�^���C�U�ݒ�
	graphicsPipeline.RasterizerState.MultisampleEnable = false;          // �}���`�T���v������
	graphicsPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;    // �J�����O����
	graphicsPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;   // �h��Ԃ�
	graphicsPipeline.RasterizerState.DepthClipEnable = true;             // �[�x�ɂ��N���b�v�͗L��
	graphicsPipeline.RasterizerState.FrontCounterClockwise = false;
	graphicsPipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	graphicsPipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	graphicsPipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	graphicsPipeline.RasterizerState.AntialiasedLineEnable = false;
	graphicsPipeline.RasterizerState.ForcedSampleCount = 0;
	graphicsPipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	graphicsPipeline.DepthStencilState.DepthEnable = false;
	graphicsPipeline.DepthStencilState.StencilEnable = false;
	graphicsPipeline.InputLayout.pInputElementDescs = inputLayout;                     //���C�A�E�g�擪�A�h���X
	graphicsPipeline.InputLayout.NumElements = _countof(inputLayout);                  //���C�A�E�g�z��
	graphicsPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;    //�X�g���b�v���̃J�b�g�Ȃ�
	graphicsPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;   //�O�p�`�ō\��
	graphicsPipeline.NumRenderTargets = 1;                                             //�����_�[�^�[�Q�b�g�͌��݈��
	graphicsPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                       //0�`1�ɐ��K�����ꂽRGBA
	graphicsPipeline.SampleDesc.Count = 1;                                             //�T���v�����O��1�s�N�Z���ɂ�1
	graphicsPipeline.SampleDesc.Quality = 0;                                           //�N�I���e�B�͍Œ�


	// ���[�g�V�O�l�`���̐���
	// �f�B�X�N���v�^�f�[�^�e�[�u�����܂Ƃ߂����́B���_���ȊO�̃f�[�^(�e�N�X�`���A�萔��)���V�F�[�_�[�ɑ��M����
	ID3D12RootSignature* rootSignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ID3DBlob* rootSignatureBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob);
	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	rootSignatureBlob->Release();
	// �O���t�B�b�N�p�C�v���C���X�e�[�g�̐���
	// �V�F�[�_�[���A�O���t�B�b�N�X�p�C�v���C���Ɋւ��ݒ���ЂƂ܂Ƃ߂ɂ�������
	graphicsPipeline.pRootSignature = rootSignature;
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	result = dev->CreateGraphicsPipelineState(&graphicsPipeline, IID_PPV_ARGS(&graphicsPipelineState));

	// �r���[�|�[�g�̐���
	// �E�B���h�E�ɑ΂��ĕ`�挋�ʂ��ǂ��\�����邩
	D3D12_VIEWPORT viewPort = {};
	viewPort.Width = WINDOW_WIDTH;     // �o�͐�̕�
	viewPort.Height = WINDOW_HEIGHT;   // �o�͐�̍���
	viewPort.TopLeftX = 0;             // �o�͐�̍�����WX
	viewPort.TopLeftY = 0;             // �o�͐�̍�����WY
	viewPort.MaxDepth = 1.0f;          // �[�x�ő�l
	viewPort.MinDepth = 0.0f;          // �[�x�ŏ��l

	// �V�U�[���N�g
	// �r���[�|�[�g�ɏo�͂��ꂽ�摜�̕\���̈��ݒ�
	D3D12_RECT scissorRect = {};
	scissorRect.top = 0;                                    // �؂蔲������W
	scissorRect.left = 0;                                   // �؂蔲�������W
	scissorRect.right = scissorRect.left + WINDOW_WIDTH;    // �؂蔲���E���W
	scissorRect.bottom = scissorRect.top + WINDOW_HEIGHT;   // �؂蔲�������W

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

		// �p�C�v���C���X�e�[�g�̃Z�b�g
		cmdList->SetPipelineState(graphicsPipelineState);

		// �����_�[�^�[�Q�b�g�̎w��
		auto rtvHandle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// �����_�[�^�[�Q�b�g�̃N���A
		float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };   // ���F
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);   // ���݂̃����_�[�^�[�Q�b�g���w��F�ŃN���A

		// triangle�`��
		cmdList->RSSetViewports(1, &viewPort);
		cmdList->RSSetScissorRects(1, &scissorRect);
		cmdList->SetGraphicsRootSignature(rootSignature);

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		cmdList->IASetIndexBuffer(&indexBufferView);

		cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

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

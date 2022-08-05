#include <Windows.h>
#include <tchar.h>
#include <vector>
// DirectXヘッダー/dxgiライブラリ インクルード&リンク
#include <d3d12.h>
#include <dxgi1_6.h>       // 誤動作する場合は1_4にしてみる
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
// シェーダーコンパイルに必要
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#include <iostream>
#endif
#include <DirectXMath.h>   // 数学ライブラリ
#include <vector>

using namespace std;

// 変数群
const unsigned int WINDOW_WIDTH = 1600;
const unsigned int WINDOW_HEIGHT = 900;
// Direct3D基本オブジェクト
ID3D12Device* dev = nullptr;
IDXGIFactory6* dxgiFactory = nullptr;
ID3D12CommandAllocator* cmdAllocator = nullptr;
ID3D12GraphicsCommandList* cmdList = nullptr;
ID3D12CommandQueue* cmdQueue = nullptr;
IDXGISwapChain4* swapChain = nullptr;

// @brief コンソール画面にフォーマット付き文字列を描画
// @param format フォーマット (%d %f)
// @param 可変長引数
// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* _format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, _format);
	vprintf(_format, valist);
	va_end(valist);
#endif
}

// ウィンドウ生成用 ウィンドウプロシージャ
LRESULT WindowProcedure(HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam)
{
	// ウィンドウ破棄時に呼び出し
	if (_msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OSに対してアプリ終了の命令
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

	// アプリケーションのインスタンスハンドル取得
	HINSTANCE hInst = GetModuleHandle(nullptr);

	// ウィンドウクラスの生成&登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;           // コールバック関数の指定
	w.lpszClassName = _T("Lazcano");                    // アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr);             // ハンドルの取得
	RegisterClassEx(&w);                                // アプリケーションクラス(ウィンドウクラスの指定をOSに伝達)

	RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };   // ウィンドウサイズ指定
	// 関数からウィンドウのサイズ補正
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow
	(
		w.lpszClassName,        // クラス名指定
		_T("Lazcano"),          // タイトルバー表示文字
		WS_OVERLAPPEDWINDOW,    // タイトルバーとの境界線があるウィンドウを指定
		CW_USEDEFAULT,          // ウィンドウ表示のx座標をOSに任せる
		CW_USEDEFAULT,          // ウィンドウ表示のy座標をOSに任せる
		wrc.right - wrc.left,   // ウィンドウ幅
		wrc.bottom - wrc.top,   // ウィンドウ高さ
		nullptr,                // 親ウィンドウのハンドル(親のため指定なし)
		nullptr,                // メニューハンドル
		w.hInstance,            // 呼び出しアプリケーションハンドル
		nullptr                 // 追加パラメータ
	);

	// DirectX 各初期化関数の成功判定確認用
	HRESULT result = S_OK;

#ifdef _DEBUG

	// Direct3DのデバッグレイヤーON (出力に情報表示)
	ID3D12Debug* debugLayer = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	if (result == S_OK)
	{
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}

#endif

	//-------------------------------------------------//
	// Direct3D関連 初期化
	//------------------------------------------------//
	// dxgi デバッグとリリースで分ける
#ifdef _DEBUG
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory));
	if (result != S_OK)
	{
		result = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
	}
#else
	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
#endif

	// 複数GPU搭載PC対策
	// アダプタコンテナに情報を格納し、"NVIDIA"ワードが入るアダプタを優先使用
	IDXGIAdapter* tmpAdapter = nullptr;
	std::vector<IDXGIAdapter*> adapters;
	for (int i = 0; dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.emplace_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		// アダプタ説明オブジェクト取得
		DXGI_ADAPTER_DESC adapterDesc = {};
		adpt->GetDesc(&adapterDesc);
		// "NVIDIA"ワードが入る名前を使用
		std::wstring strDesc = adapterDesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}
	if (tmpAdapter == nullptr)
	{
		// NVIDIA GPUが取得できなかった場合は最初のアダプタを選択
		tmpAdapter = adapters[0];
	}
	// DirectX12 フィーチャーレベル : ドライバに対応していないPCでは一段ずつLvを落としていく
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	D3D_FEATURE_LEVEL featureLevel;

	// Direct3Dデバイス初期化
	for (auto fLv : featureLevels)
	{
		// 返り値の方"HRESULT"は成功時に"S_OK"となり、失敗時は"S_OK"以外となる
		if (D3D12CreateDevice(tmpAdapter, fLv, IID_PPV_ARGS(&dev)) == S_OK)
		{
			featureLevel = fLv;
			break;
		}
	}
	
	// コマンドアロケータ/コマンドリストの作成
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));

	// コマンドキューの生成 (コマンドアロケータに蓄積したコマンドリストを実行するにはキューが必要)
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;            // タイムアウトなし
	cmdQueueDesc.NodeMask = 0;                                     // 使用GPUアダプタが1つの場合は0
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;   // プライオリティ(優先度)指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;            // コマンドリストのTYPEと合わせる
	result = dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));

	// スワップチェインの生成 (ダブルバッファリングに必要)
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = WINDOW_WIDTH;                             // 画面解像度(幅)
	swapChainDesc.Height = WINDOW_HEIGHT;                           // 画面解像度(縦)
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;              // ピクセルフォーマット
	swapChainDesc.Stereo = false;                                   // ステレオ表示フラグ(3Dディスプレイのステレオモード)
	swapChainDesc.SampleDesc.Count = 1;                             // マルチサンプルの指定(Count = 1でよい)
	swapChainDesc.SampleDesc.Quality = 0;                           // マルチサンプルの指定(Quality = 1でよい)
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;             // DXGI_USAGE_BACK_BUFFERでよい
	swapChainDesc.BufferCount = 2;                                  // ダブルバッファは2
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;                   // バックバッファの伸縮が可能
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;       // フリップ後に即破棄
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;          // アルファモードの指定なし
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // ウィンドウとフルスクリーンの切り替えが可能
	result = dxgiFactory->CreateSwapChainForHwnd
	(
		cmdQueue,
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapChain
	);

	// ディスクリプタヒープの生成
	// ディスクリプタとはGPUリソースの用途や使用法を説明するデータ
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;     // レンダーターゲットビュー(RTV)
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;                        // 表裏2つぶん
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;   // 指定なし
	ID3D12DescriptorHeap* rtvHeaps = nullptr;           // レンダーターゲットビュー用ヒープ
	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	// ディスクリプタとスワップチェインのバックバッファ紐付け(各表裏2つ)
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = swapChain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (unsigned int i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
		// レンダーターゲットビューとバックバッファの紐付け
		dev->CreateRenderTargetView(backBuffers[i], nullptr, descriptorHandle);
		// 次のディスクリプタに移動
		descriptorHandle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// フェンスの生成
	// フェンスとはGPU側の処理(コマンドリストの命令)が完了したかどうかを知らせる仕組み
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;
	result = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	if (result != S_OK)
	{
		return -1;
	}

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	// 頂点配列
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;   // 頂点座標(xyz)
		DirectX::XMFLOAT2 uv;    // UV座標
	};
	
	// 時計回りになるように頂点を定義
	Vertex vertices[] =
	{
		{{ -0.4f, -0.7f, 0.0f }, {0.0f, 1.0f}},  // 左下 index:0
		{{ -0.4f,  0.7f, 0.0f }, {0.0f, 0.0f}},  // 左上 index:1
		{{  0.4f, -0.7f, 0.0f }, {1.0f, 1.0f}},  // 右上 index:2
		{{  0.4f,  0.7f, 0.0f }, {1.0f, 0.0f}},  // 右下 index:3
	};
	// 頂点バッファ生成用のデータ群を生成
	// ヒーププロパティ
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;                       // CPUへのアクセス制限なし その分遅い
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;   // HEAP_TYPEがCUSTOM以外ならUNKNOWNでよい
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;    // HEAP_TYPEがCUSTOM以外ならUNKNOWNでよい
	// リソース設定構造体
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;     // バッファとして使用するためBUFFERでよい
	resourceDesc.Width = sizeof(vertices);                        // 頂点バッファであるため情報は幅に詰める
	resourceDesc.Height = 1;                                      // 画像データではないため1でよい
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;                    // 画像ではないためUNKNOWNでよい
	resourceDesc.SampleDesc.Count = 1;                            // 0だとデータが存在しないことになるため1
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;         // テクスチャレイアウトだが、ROW_MAJORとしておく

	// 頂点バッファの生成
	ID3D12Resource* vertBuffer = nullptr;
	result = dev->CreateCommittedResource
	(
		&heapProp,                           // ヒープ設定構造体アドレス
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,                       // リソース設定構造体アドレス
		D3D12_RESOURCE_STATE_GENERIC_READ,   // GPU側からは読み取り専用
		nullptr,                             // 未使用
		IID_PPV_ARGS(&vertBuffer)
	);

	// 頂点バッファに頂点情報コピー
	Vertex* vertMap = nullptr;
	result = vertBuffer->Map(0, nullptr, (void**)&vertMap);         // バッファの仮想アドレス取得
	std::copy(std::begin(vertices), std::end(vertices), vertMap);   // マップに頂点情報コピー
	vertBuffer->Unmap(0, nullptr);                                  // 情報コピーし終えたのでマップ解除

	// 頂点バッファビューの生成
	// 頂点バッファビューとは、頂点バッファ全体のバイト数、1頂点当たりのバイト数を知らせるデータ
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = vertBuffer->GetGPUVirtualAddress();   // バッファの仮想アドレス取得
	vertexBufferView.SizeInBytes = sizeof(vertices);                        // 全バイト数
	vertexBufferView.StrideInBytes = sizeof(vertices[0]);                   // 1頂点辺りのバイト数

	// インデックスの定義
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
	// インデックスデータをコピー
	unsigned short* mappedIndex = nullptr;
	indexBuffer->Map(0, nullptr, (void**)&mappedIndex);
	std::copy(std::begin(indices), std::end(indices), mappedIndex);
	indexBuffer->Unmap(0, nullptr);
	//インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = sizeof(indices);

	// シェーダーオブジェクトの生成
	// ID3DBlob(BinaryLargeObject)とは汎用的なデータ型:データのポインタとサイズを持つ
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	// 頂点シェーダー
	result = D3DCompileFromFile
	(
		L"BasicVertexShader.hlsl",                          // ファイル名(ワイド文字列 L"")
		nullptr,                                            // シェーダーマクロオブジェクト
		D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // インクルードオブジェクト
		"BasicVS",                                          // エントリポイント(呼び出すシェーダー名)
		"vs_5_0",                                           // どのシェーダーを割り当てるか(vs,ps,etc...)
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    // シェーダーコンパイルオプション
		0,                                                  // エフェクトコンパイルオプション
		&vsBlob,                                            // 受け取るポインタアドレス
		&errorBlob                                          // エラー用ポインタアドレス
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
	// ピクセルシェーダー
	result = D3DCompileFromFile
	(
		L"BasicPixelShader.hlsl",                           // ファイル名(ワイド文字列 L"")
		nullptr,                                            // シェーダーマクロオブジェクト
		D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // インクルードオブジェクト
		"BasicPS",                                          // エントリポイント(呼び出すシェーダー名)
		"ps_5_0",                                           // どのシェーダーを割り当てるか(vs,ps,etc...)
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,    // シェーダーコンパイルオプション
		0,                                                  // エフェクトコンパイルオプション
		&psBlob,                                            // 受け取るポインタアドレス
		&errorBlob                                          // エラー用ポインタアドレス
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
		// 座標情報
		{
		    "POSITION",                                   // セマンティクス名
		    0,                                            
		    DXGI_FORMAT_R32G32B32_FLOAT,                  // フォーマット
		    0,
		    D3D12_APPEND_ALIGNED_ELEMENT,
		    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		    0
		},

		// UV情報
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

	// テクスチャデータ(仮)
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
	// テクスチャバッファの生成 (WriteToSubresourceで転送)
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;                          // 特殊設定のためCUSTOM
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;   // ライトバック方式
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;            // CPUから直接転送のためL0
	texHeapProp.CreationNodeMask = 0;                                   // 単一アダプタのため
	texHeapProp.VisibleNodeMask = 0;
	// テクスチャ用リソース設定
	D3D12_RESOURCE_DESC texResourceDesk = {};
	texResourceDesk.Format = DXGI_FORMAT_R8G8B8A8_UNORM;   // RGBAフォーマット
	texResourceDesk.Width = 256;
	texResourceDesk.Height = 256;
	texResourceDesk.DepthOrArraySize = 1;                             // 2Dで配列でもないため1
	texResourceDesk.SampleDesc.Count = 1;                             // 通常テクスチャのためAAをかけない
	texResourceDesk.SampleDesc.Quality = 0;                           // 最低クオリティ
	texResourceDesk.MipLevels = 1;                                    // ミップマップは使用しない
	texResourceDesk.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;   // 2Dテクスチャ用
	texResourceDesk.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;            // レイアウトは決めない
	texResourceDesk.Flags = D3D12_RESOURCE_FLAG_NONE;                 // フラグなし
	// テクスチャ用リソース生成


	// グラフィックスパイプラインの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeline = {};
	graphicsPipeline.pRootSignature = nullptr;
	graphicsPipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	graphicsPipeline.VS.BytecodeLength = vsBlob->GetBufferSize();
	graphicsPipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	graphicsPipeline.PS.BytecodeLength = psBlob->GetBufferSize();
	graphicsPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// ブレンディングは今回未使用
	graphicsPipeline.BlendState.AlphaToCoverageEnable = false;
	graphicsPipeline.BlendState.IndependentBlendEnable = false;
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	// 論理演算は未使用
	renderTargetBlendDesc.LogicOpEnable = false;
	graphicsPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;
	// ラスタライザ設定
	graphicsPipeline.RasterizerState.MultisampleEnable = false;          // マルチサンプル無効
	graphicsPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;    // カリング無効
	graphicsPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;   // 塗りつぶす
	graphicsPipeline.RasterizerState.DepthClipEnable = true;             // 深度によるクリップは有効
	graphicsPipeline.RasterizerState.FrontCounterClockwise = false;
	graphicsPipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	graphicsPipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	graphicsPipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	graphicsPipeline.RasterizerState.AntialiasedLineEnable = false;
	graphicsPipeline.RasterizerState.ForcedSampleCount = 0;
	graphicsPipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	graphicsPipeline.DepthStencilState.DepthEnable = false;
	graphicsPipeline.DepthStencilState.StencilEnable = false;
	graphicsPipeline.InputLayout.pInputElementDescs = inputLayout;                     //レイアウト先頭アドレス
	graphicsPipeline.InputLayout.NumElements = _countof(inputLayout);                  //レイアウト配列数
	graphicsPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;    //ストリップ時のカットなし
	graphicsPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;   //三角形で構成
	graphicsPipeline.NumRenderTargets = 1;                                             //レンダーターゲットは現在一つ
	graphicsPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                       //0～1に正規化されたRGBA
	graphicsPipeline.SampleDesc.Count = 1;                                             //サンプリングは1ピクセルにつき1
	graphicsPipeline.SampleDesc.Quality = 0;                                           //クオリティは最低


	// ルートシグネチャの生成
	// ディスクリプタデータテーブルをまとめたもの。頂点情報以外のデータ(テクスチャ、定数等)をシェーダーに送信する
	ID3D12RootSignature* rootSignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ID3DBlob* rootSignatureBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob);
	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	rootSignatureBlob->Release();
	// グラフィックパイプラインステートの生成
	// シェーダー等、グラフィックスパイプラインに関わる設定をひとまとめにしたもの
	graphicsPipeline.pRootSignature = rootSignature;
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	result = dev->CreateGraphicsPipelineState(&graphicsPipeline, IID_PPV_ARGS(&graphicsPipelineState));

	// ビューポートの生成
	// ウィンドウに対して描画結果をどう表示するか
	D3D12_VIEWPORT viewPort = {};
	viewPort.Width = WINDOW_WIDTH;     // 出力先の幅
	viewPort.Height = WINDOW_HEIGHT;   // 出力先の高さ
	viewPort.TopLeftX = 0;             // 出力先の左上座標X
	viewPort.TopLeftY = 0;             // 出力先の左上座標Y
	viewPort.MaxDepth = 1.0f;          // 深度最大値
	viewPort.MinDepth = 0.0f;          // 深度最小値

	// シザーレクト
	// ビューポートに出力された画像の表示領域を設定
	D3D12_RECT scissorRect = {};
	scissorRect.top = 0;                                    // 切り抜き上座標
	scissorRect.left = 0;                                   // 切り抜き左座標
	scissorRect.right = scissorRect.left + WINDOW_WIDTH;    // 切り抜き右座標
	scissorRect.bottom = scissorRect.top + WINDOW_HEIGHT;   // 切り抜き下座標

	// メッセージループ
	MSG msg = {};
	// アプリケーション終了時はmessageがWM_QUITになる
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//-------------------------------------------//
		// DirectX描画処理
		//------------------------------------------//
		// スワップチェインから現在のバックバッファ(レンダーターゲットビュー)のインデックス取得
		auto bbIndex = swapChain->GetCurrentBackBufferIndex();

		// リソースバリア定義
		// バリアとはGPUにリソースの状態遷移の状況を伝達するもの
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;                      // タイプ:遷移
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Transition.pResource = backBuffers[bbIndex];                        // リソース(バックバッファ)
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;              // 直前はPRESENT状態
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;         // これからレンダーターゲット状態
		cmdList->ResourceBarrier(1, &barrierDesc);                                      // バリア実行

		// パイプラインステートのセット
		cmdList->SetPipelineState(graphicsPipelineState);

		// レンダーターゲットの指定
		auto rtvHandle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// レンダーターゲットのクリア
		float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };   // 黄色
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);   // 現在のレンダーターゲットを指定色でクリア

		// triangle描画
		cmdList->RSSetViewports(1, &viewPort);
		cmdList->RSSetScissorRects(1, &scissorRect);
		cmdList->SetGraphicsRootSignature(rootSignature);

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		cmdList->IASetIndexBuffer(&indexBufferView);

		cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// バリアの遷移状態をPRESENTに
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		cmdList->ResourceBarrier(1, &barrierDesc);

		// コマンドリスト(描画命令)実行前に必ずコマンドリストのクローズを行う
		cmdList->Close();

		// コマンドリストの実行
		ID3D12CommandList* cmdLists[] = { cmdList };
		cmdQueue->ExecuteCommandLists(1, cmdLists);
		// コマンドリストの実行がGPU側で終了するまで待機
		cmdQueue->Signal(fence, ++fenceVal);
		while (fence->GetCompletedValue() != fenceVal)
		{
			// イベントハンドルの取得
			auto eventHandle = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, eventHandle);
			// イベントが発生するまで待機
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
		
		// コマンドリスト実行後はキューとリストをリセットする
		cmdAllocator->Reset();
		cmdList->Reset(cmdAllocator, nullptr);

		// 画面のスワップ
		swapChain->Present(1, 0);   // 第1引数は待機フレーム数(待機する垂直同期の数):0にすると垂直同期を待たず即時復帰

	}

	// クラスの登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}

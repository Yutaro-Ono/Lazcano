#include <Windows.h>
#include <tchar.h>
#include <vector>
// DirectXヘッダー/dxgiライブラリ インクルード&リンク
#include <d3d12.h>
#include <dxgi1_6.h>   // 誤動作する場合は1_4にしてみる
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#include <iostream>
#endif

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

		// レンダーターゲットの指定
		auto rtvHandle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// レンダーターゲットのクリア
		float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };   // 黄色
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);   // 現在のレンダーターゲットを指定色でクリア

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

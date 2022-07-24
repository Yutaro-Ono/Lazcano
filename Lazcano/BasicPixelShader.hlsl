// ピクセルシェーダー
// 頂点シェーダーから渡された座標情報を変換し、レンダーターゲットへ色情報として渡す
float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
	// SV_POSITIONはスクリーン座標
	float screenW = 1600.0f;
    float screenH = 900.0f;

	return float4(pos.x / screenW, pos.y / screenH, 0.5f, 1.0f);
	//return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

//struct Input
//{
//	float4 pos:POSITION;
//	float4 svpos:SV_POSITION;
//};
//
//// ピクセルシェーダー
//// 頂点シェーダーから渡された座標情報を変換し、レンダーターゲットへ色情報として渡す
//float4 BasicPS(Input input) : SV_TARGET
//{
//	return float4((float2(0.0f, 1.0f) + input.pos.xy) * 0.5f, 1.0f, 1.0f);
//}
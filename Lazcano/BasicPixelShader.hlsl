// ピクセルシェーダー
// 頂点シェーダーから渡された座標情報を変換し、レンダーターゲットへ色情報として渡す
float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
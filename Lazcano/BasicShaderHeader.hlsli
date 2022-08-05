// 頂点シェーダーからピクセルシェーダーのやり取りに使用
struct Output
{
	float4 svpos : SV_POSITION;   // 頂点座標
	float2 uv    : TEXCOORD;      // UV座標
};
#include "BasicShaderHeader.hlsli"

// 頂点シェーダー
//float4 BasicVS( float4 pos : POSITION, float2 uv : TEXCOORD ) : SV_POSITION
//{
//	return pos;
//}

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	// 出力用構造体に保管し、PSに送信
	Output output;
	output.svpos = pos;
	output.uv = uv;
	return output;
}
#include "BasicShaderHeader.hlsli"

// ���_�V�F�[�_�[
//float4 BasicVS( float4 pos : POSITION, float2 uv : TEXCOORD ) : SV_POSITION
//{
//	return pos;
//}

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	// �o�͗p�\���̂ɕۊǂ��APS�ɑ��M
	Output output;
	output.svpos = pos;
	output.uv = uv;
	return output;
}
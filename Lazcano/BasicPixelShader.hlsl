// �s�N�Z���V�F�[�_�[
// ���_�V�F�[�_�[����n���ꂽ���W����ϊ����A�����_�[�^�[�Q�b�g�֐F���Ƃ��ēn��
float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
	// SV_POSITION�̓X�N���[�����W
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
//// �s�N�Z���V�F�[�_�[
//// ���_�V�F�[�_�[����n���ꂽ���W����ϊ����A�����_�[�^�[�Q�b�g�֐F���Ƃ��ēn��
//float4 BasicPS(Input input) : SV_TARGET
//{
//	return float4((float2(0.0f, 1.0f) + input.pos.xy) * 0.5f, 1.0f, 1.0f);
//}
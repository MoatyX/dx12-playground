cbuffer perObj: register(b0) {
	float4 offset;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PSInput main(float4 iPos : POSITION, float4 iColor : COLOR)
{
	float4x4 x = {	1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
	};
	PSInput result;
	result.position = iPos;
	result.color = iColor;
	result.position += offset;
	return result;
}
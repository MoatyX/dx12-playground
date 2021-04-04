cbuffer perObj: register(b0) {
	float4x4 gWorldProjView;
};

void main(float4 iPos : POSITION, float4 iColor : COLOR, out float4 oPos : SV_POSITION, out float4 oColor : COLOR)
{
	float4x4 id = float4x4(	1, 0, 0, 0,
							0, 1, 0, 0,
							0, 0, 1, 0,
							0, 0, 0, 1);
	oPos = mul(iPos, gWorldProjView);
	oColor = iColor;
	if(gWorldProjView[0][0] == 0)
	{
		oPos = iPos;
	}
}
void main(float4 iPos : POSITION, float4 iColor : COLOR, out float4 oPos : SV_POSITION, out float4 oColor : COLOR)
{
	oPos = iPos;
	oColor = iColor;
}
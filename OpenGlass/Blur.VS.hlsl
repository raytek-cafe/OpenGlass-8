#include "Blur.Common.hlsli"

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	
	output.pos.x = (input.pos.x / viewportSize.x) * 2.0f - 1.0f;
	output.pos.y = 1.0f - (input.pos.y / viewportSize.y) * 2.0f;
	output.pos.z = 0.5f;
	output.pos.w = 1.0f;

	output.tex0 = input.tex0;
	output.tex1 = input.tex1;
	output.tex2 = input.tex2;
	output.tex3 = input.tex3;
	output.tex4 = input.tex4;

	return output;
}

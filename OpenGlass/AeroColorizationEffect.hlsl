#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_SIMPLE
#include <d2d1effecthelpers.hlsli>
#include "Common.hlsli"

cbuffer constants : register(b0)
{
	minfloat4 afterglow : packoffset(c0);
	minfloat4 blurBalance : packoffset(c1);
	minfloat4 color : packoffset(c2);
};

static const minfloat3 grayFactor = minfloat3(0.2126, 0.7152, 0.0722);

D2D_PS_ENTRY(AeroColorizationEffect)
{
	minfloat4 input = minfloat4(D2DGetInput(0));
	input.w = dot(input.xyz, grayFactor);
	input.xyz = input.www * afterglow.xyz + blurBalance.xxx * input.xyz + color.xyz;
	input.w = 1;

	return input;
}
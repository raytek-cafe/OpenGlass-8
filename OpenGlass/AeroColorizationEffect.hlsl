#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_SIMPLE
#include <d2d1effecthelpers.hlsli>
#include "Common.hlsli"

cbuffer constants : register(b0)
{
	minfloat4 afterglow;
	minfloat4 blurBalance;
	minfloat4 color;
};

static const minfloat3 grayFactor = minfloat3(0.2126, 0.7152, 0.0722);

D2D_PS_ENTRY(AeroColorizationEffect)
{
	minfloat3 result = minfloat3(D2DGetInput(0).xyz);
	minfloat luminance = dot(result, grayFactor);
	result = minfloat3(blurBalance.xxx) * result;
	result = luminance * minfloat3(afterglow.xyz) + result;
	result = minfloat3(color.xyz) + result;

	return minfloat4(result, 1.0f);
}

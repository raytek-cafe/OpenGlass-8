R"(cbuffer BlurConstants : register(b0)
{
	float2 viewportSize;
	float2 texelSize;
	float4 afterglow;
	float4 blurBalance;
	float4 color;
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

struct VS_INPUT
{
	float4 pos : POSITION;
	float2 tex0 : TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 tex0 : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT) 0;
	
	output.pos.x = (input.pos.x / viewportSize.x) * 2.0f - 1.0f;
	output.pos.y = 1.0f - (input.pos.y / viewportSize.y) * 2.0f;
	output.pos.z = 0.5f;
	output.pos.w = 1.0f;

	output.tex0 = input.tex0;
	
	return output;
}

static const int taps = 8;
static const min10float weights[taps] = { 0.02706f, 0.08890f, 0.18942f, 0.26192f, 0.23510f, 0.13697f, 0.05178f, 0.00885f };
static const min10float offsets[taps] = { -3.1635f, -2.1888f, -1.2155f, -0.2431f, 0.7292f, 1.7020f, 2.6759f, 3.5000f };
static const min10float3 grayFactor = min10float3(0.2126f, 0.7152f, 0.0722f);

min10float4 PS_BlurH(PS_INPUT input) : SV_Target
{
	min10float4 finalColor = 0;
	
	[unroll]
	for (int i = 0; i < taps; i++)
	{
		finalColor += (min10float4)texture0.Sample(sampler0, input.tex0 + float2(offsets[i] * texelSize.x, 0)) * weights[i];
	}

	finalColor.w = 1;

	return finalColor;
}

min10float4 PS_BlurV(PS_INPUT input) : SV_Target
{
	min10float4 finalColor = 0;
	
	[unroll]
	for (int i = 0; i < taps; i++)
	{
		finalColor += (min10float4)texture0.Sample(sampler0, input.tex0 + float2(0, offsets[i] * texelSize.y)) * weights[i];
	}

	finalColor.w = dot(finalColor.xyz, grayFactor);
	finalColor.xyz = (min10float3)mad(finalColor.w, afterglow.xyz, mad(blurBalance.x, finalColor.xyz, color.xyz));
	finalColor.w = 1;

	return finalColor;
}

min10float4 PS_Passthrough(PS_INPUT input) : SV_Target
{
	min10float4 finalColor = (min10float4)texture0.Sample(sampler0, input.tex0);
	
	return finalColor;
})"

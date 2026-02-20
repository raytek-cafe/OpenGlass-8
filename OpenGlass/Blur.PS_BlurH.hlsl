#include "Blur.Common.hlsli"

minfloat4 main(PS_INPUT input) : SV_Target0
{
	minfloat3 result = 0;
	
	result += minfloat3(conv[0].x * texture0.Sample(sampler0, input.tex1.xy).xyz);
	result += minfloat3(conv[1].x * texture0.Sample(sampler0, input.tex1.zw).xyz);
	result += minfloat3(conv[2].x * texture0.Sample(sampler0, input.tex2.xy).xyz);
	result += minfloat3(conv[3].x * texture0.Sample(sampler0, input.tex2.zw).xyz);
	result += minfloat3(conv[4].x * texture0.Sample(sampler0, input.tex3.xy).xyz);
	result += minfloat3(conv[5].x * texture0.Sample(sampler0, input.tex3.zw).xyz);
	result += minfloat3(conv[6].x * texture0.Sample(sampler0, input.tex4.xy).xyz);
	result += minfloat3(conv[7].x * texture0.Sample(sampler0, input.tex4.zw).xyz);

	return minfloat4(result, 1.0f);
}

#if !defined(COMMON_HLSL)
#define COMMON_HLSL

#if defined(MINP_PREFIX)
#   define MINP_CONCAT(a, b) a##b
#   define minp(type) MINP_CONCAT(MINP_PREFIX, type)
#else
#   define minp(type) type
#endif

#define minfloat minp(float)
#define minfloat2 minp(float2)
#define minfloat3 minp(float3)
#define minfloat4 minp(float4)
#define minfloat2x2 minp(float2x2)
#define minfloat3x2 minp(float3x2)
#define minfloat3x3 minp(float3x3)
#define minfloat4x4 minp(float4x4)

minfloat4 Premultiply(minfloat4 color)
{
    color.rgb *= color.a;
    return color;
}

minfloat4 UnPremultiply(minfloat4 color)
{
    color.rgb = (color.a == 0) ? minfloat3(0, 0, 0) : (color.rgb / color.a);
    return color;
}

#endif

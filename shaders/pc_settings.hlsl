// PC-owned presentation overlay. Pixel coordinates use the actual framebuffer.
struct Screen { float2 extent; };
[[vk::push_constant]] ConstantBuffer<Screen> screen : register(b0);
[[vk::binding(0, 0)]] Texture2D<float4> atlas : register(t0);
[[vk::binding(1, 0)]] SamplerState atlasSampler : register(s1);
struct Vertex { float2 position : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
struct Varying { float4 position : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
Varying VSMain(Vertex input) {
 Varying output;
 output.position=float4(input.position.x*2/screen.extent.x-1,1-input.position.y*2/screen.extent.y,0,1);
 output.uv=input.uv;output.color=input.color;
 return output;
}
float4 PSMain(Varying input) : SV_TARGET { return atlas.Sample(atlasSampler,input.uv)*input.color; }

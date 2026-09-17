// ==============================================================================
// ShadowDepth.hlsl - シャドウマップ生成用 深度専用シェーダ
// ==============================================================================

// 定数バッファ: オブジェクトのワールド行列とライトの View-Projection 行列
cbuffer ShadowConstantBuffer : register(b0)
{
    float4x4 world_matrix;            // ローカル -> ワールド変換行列 (64 bytes)
    float4x4 light_view_proj_matrix;  // ワールド -> ライトクリップ空間行列 (64 bytes)
};

// 頂点入力データ（メインパイプラインの Vertex 構造体と互換）
struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
    float2 texcoord : TEXCOORD;
};

// 頂点シェーダ: 頂点を光源から見たクリップ空間へ座標変換
// ※ ピクセルシェーダは使用せず、SV_POSITION の深度値がそのまま深度バッファに書き込まれます
float4 VSMain(VSInput input) : SV_POSITION
{
    float4 world_pos = mul(float4(input.position, 1.0f), world_matrix);
    return mul(world_pos, light_view_proj_matrix);
}

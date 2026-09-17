// ==============================================================================
// SimpleTriangle.hlsl - 3D 変換対応シェーダ
// 定数バッファ (cbuffer) を経由して CPU から MVP 行列を受け取ります
// ==============================================================================

// 定数バッファ (Constant Buffer):
// すべての頂点で共通して使用するデータ（行列や時間など）を格納する
// register(b0): レジスタ番号 0 番の定数バッファ (CBV) に対応
cbuffer TransformBuffer : register(b0)
{
    float4x4 mvp_matrix; // Model-View-Projection 行列 (4x4)
};

// 頂点シェーダの入力データ（C++ 側の Vertex 構造体と一致）
struct VSInput
{
    float3 position : POSITION; // モデルローカル座標 (X, Y, Z)
    float4 color    : COLOR;    // 頂点カラー (R, G, B, A)
};

// 頂点シェーダからピクセルシェーダへの出力データ
struct PSInput
{
    float4 position : SV_POSITION; // 画面クリップ空間座標
    float4 color    : COLOR;
};

// ------------------------------------------------------------------------------
// 頂点シェーダ (Vertex Shader)
// ------------------------------------------------------------------------------
PSInput VSMain(VSInput input)
{
    PSInput output;

    // 3D 空間上のローカル座標を、MVP 行列によって画面クリップ座標系へ変換
    // mul(ベクトル, 行列): ベクトルに行列を掛ける組み込み関数
    output.position = mul(float4(input.position, 1.0f), mvp_matrix);

    // 頂点カラーをそのままピクセルシェーダへ渡す（ラスタライザが自動グラデーション補間）
    output.color = input.color;

    return output;
}

// ------------------------------------------------------------------------------
// ピクセルシェーダ (Pixel Shader)
// ------------------------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}

// ==============================================================================
// Lit.hlsl - ライティング（Lambert 拡散反射 ＋ Blinn-Phong 鏡面反射）シェーダ
// ==============================================================================

// 定数バッファ: オブジェクト・シーン・光源・マテリアルの全パラメータ
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 world_matrix;            // ローカル -> ワールド変換行列 (64 bytes)
    float4x4 view_proj_matrix;       // ワールド -> クリップ空間変換行列 (64 bytes)
    float4   camera_position;        // カメラのワールド座標 (xyz: 座標, w: 未使用) (16 bytes)
    float4   light_direction;        // 平行光源の照射方向 (xyz: 向き, w: 未使用) (16 bytes)
    float4   light_color;            // 光源の色 (rgb) と 光源強度 (a) (16 bytes)
    float4   ambient_color;          // 環境光の色 (rgb: 色, w: 未使用) (16 bytes)
    float4   material_color;         // マテリアルの基本反射色 (rgba) (16 bytes)
    float4   material_params;        // x: specular_power, y: specular_intensity (16 bytes)
    float4x4 light_view_proj_matrix; // 光源視点の View-Projection 行列 (64 bytes)
    float4   shadow_params;          // x: shadow_bias, y: shadow_strength, z: shadow_map_size (16 bytes)
};

// テクスチャおよびサンプラー（Texture2D & Sampler）
Texture2D g_texture : register(t0);                     // アルベドテクスチャ
SamplerState g_sampler : register(s0);                  // バイリニアサンプラー

Texture2D g_shadow_map : register(t1);                  // シャドウマップ深度テクスチャ
SamplerComparisonState g_shadow_sampler : register(s1); // ハードウェア深度比較サンプラー

// 頂点シェーダ入力データ（C++ 側の Graphics::Vertex と完全一致）
struct VSInput
{
    float3 position : POSITION; // モデルローカル座標
    float3 normal   : NORMAL;   // 法線ベクトル
    float4 color    : COLOR;    // 頂点固有カラー
    float2 texcoord : TEXCOORD; // UV 座標
};

// 頂点シェーダからピクセルシェーダへの出力データ
struct PSInput
{
    float4 position       : SV_POSITION; // 画面クリップ空間座標
    float3 world_pos      : POSITION;    // ワールド空間座標（視線ベクトルの算出用）
    float3 world_normal   : NORMAL;      // ワールド空間法線ベクトル
    float4 color          : COLOR;       // 頂点カラー
    float2 texcoord       : TEXCOORD0;   // UV 座標
    float4 light_clip_pos : TEXCOORD1;   // 光源視点のクリップ空間座標
};

// ------------------------------------------------------------------------------
// 頂点シェーダ (Vertex Shader)
// ------------------------------------------------------------------------------
PSInput VSMain(VSInput input)
{
    PSInput output;

    // 1. ローカル座標をワールド座標へ変換
    float4 world_pos = mul(float4(input.position, 1.0f), world_matrix);
    output.world_pos = world_pos.xyz;

    // 2. ワールド座標を画面上のクリップ空間座標へ変換
    output.position = mul(world_pos, view_proj_matrix);

    // 3. 法線ベクトルをワールド空間へ回転変換
    //    ※ スケールや平行移動の影響を排除するため 3x3 行列で乗算し、正規化します
    output.world_normal = normalize(mul(input.normal, (float3x3)world_matrix));

    output.color = input.color;
    output.texcoord = input.texcoord;

    // 4. 光源視点のクリップ空間座標を計算（シャドウマップのサンプリング用）
    output.light_clip_pos = mul(world_pos, light_view_proj_matrix);

    return output;
}

// ------------------------------------------------------------------------------
// ピクセルシェーダ (Pixel Shader)
// ------------------------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    // 頂点補間により歪んだ法線を再正規化
    float3 N = normalize(input.world_normal);

    // サーフェスから光源へ向かうベクトル（照射方向の逆向き）
    float3 L = normalize(-light_direction.xyz);

    // サーフェスからカメラ視点へ向かうベクトル
    float3 V = normalize(camera_position.xyz - input.world_pos);

    // ハーフベクトル（Blinn-Phong 鏡面反射用: L と V の中間ベクトル）
    float3 H = normalize(L + V);

    // --------------------------------------------------------------------------
    // 1. 環境光 (Ambient Light)
    //    直接光が当たらない影の部分も真っ暗にならないよう、全体を均一に照らす光
    // --------------------------------------------------------------------------
    float3 ambient = ambient_color.rgb;

    // --------------------------------------------------------------------------
    // 2. 拡散反射光 (Diffuse / Lambert)
    //    面の傾き（法線 N）と光の方向 L の角度に応じて明るさを計算 (cos θ)
    // --------------------------------------------------------------------------
    float NdotL = max(dot(N, L), 0.0f);
    float3 diffuse = light_color.rgb * light_color.a * NdotL;

    // --------------------------------------------------------------------------
    // 3. 鏡面反射光 (Specular / Blinn-Phong)
    //    視線方向と反射光が一致した時に生じる鋭いツヤ（ハイライト）
    // --------------------------------------------------------------------------
    float NdotH = max(dot(N, H), 0.0f);
    float specular_factor = pow(NdotH, max(material_params.x, 1.0f));
    float3 specular = light_color.rgb * (specular_factor * material_params.y);

    // --------------------------------------------------------------------------
    // 4. シャドウマッピング (Shadow Mapping & 3x3 PCF フィルタ)
    // --------------------------------------------------------------------------
    float shadow_factor = 1.0f; // 1.0 = 光が当たる, 0.0 = 影

    // 光源視点のクリップ空間座標から NDC 座標へ正規化 (-1〜1, 深度 0〜1)
    float3 shadow_coords = input.light_clip_pos.xyz / input.light_clip_pos.w;

    // NDC (-1〜1) を テクスチャ UV (0〜1) へマッピング (DirectX の Y 反転対応)
    float2 shadow_uv;
    shadow_uv.x = shadow_coords.x * 0.5f + 0.5f;
    shadow_uv.y = -shadow_coords.y * 0.5f + 0.5f;
    float current_depth = shadow_coords.z;

    float bias = shadow_params.x;
    float shadow_strength = shadow_params.y;
    float shadow_map_size = max(shadow_params.z, 512.0f);
    float texel_size = 1.0f / shadow_map_size;

    // シャドウマップの視錐台の内側にある場合のみシャドウ判定を行う
    if (shadow_uv.x >= 0.0f && shadow_uv.x <= 1.0f &&
        shadow_uv.y >= 0.0f && shadow_uv.y <= 1.0f &&
        current_depth >= 0.0f && current_depth <= 1.0f)
    {
        // 3x3 PCF (Percentage Closer Filtering) によるソフトシャドウ（半影のぼかし）
        float shadow_sum = 0.0f;
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                float2 offset = float2(x, y) * texel_size;
                // SampleCmpLevelZero は current_depth - bias <= shadow_depth なら 1.0、遮蔽なら 0.0 を返す
                shadow_sum += g_shadow_map.SampleCmpLevelZero(
                    g_shadow_sampler,
                    shadow_uv + offset,
                    current_depth - bias
                );
            }
        }
        float lit_ratio = shadow_sum / 9.0f;
        // 影の濃さ (shadow_strength) をブレンド
        shadow_factor = lerp(1.0f - shadow_strength, 1.0f, lit_ratio);
    }

    // --------------------------------------------------------------------------
    // 5. 最終カラーの合成
    // --------------------------------------------------------------------------
    // テクスチャサンプリング（UV 座標に対応する色を取得）
    float4 tex_color = g_texture.Sample(g_sampler, input.texcoord);

    // オブジェクトの基本色（頂点カラー × マテリアル色 × テクスチャ色）
    float4 base_color = input.color * material_color * tex_color;

    // 直接光（拡散反射光 ＋ 鏡面反射光）にのみ影（shadow_factor）が適用される
    // ※ 環境光 (ambient) は影の中にも回り込むため減衰させないのが物理的に自然
    float3 direct_light = (diffuse + specular) * shadow_factor;
    float3 final_rgb = base_color.rgb * (ambient + direct_light);

    return float4(final_rgb, base_color.a);
}

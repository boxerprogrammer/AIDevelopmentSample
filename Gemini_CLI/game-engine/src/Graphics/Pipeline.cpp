#include "Graphics/Pipeline.hpp"
#include "Graphics/VertexBuffer.hpp"
#include <DirectXMath.h>
#include <iostream>
#include <cstddef>

namespace Graphics
{
    Pipeline::Pipeline(ID3D12Device* device, const Shader& vs, const Shader& ps)
    {
        CreateRootSignature(device);
        CreatePipelineState(device, vs, ps);

        std::cout << "[Graphics] パイプラインステート (PSO) と ルートシグネチャを作成しました。\n";
    }

    void Pipeline::CreateRootSignature(ID3D12Device* device)
    {
        // ルートシグネチャ: シェーダが外から受け取る引数リスト（契約書）
        // Parameter 0: 定数バッファ register(b0) (ルート記述子)
        // Parameter 1: アルベドテクスチャ register(t0) (ディスクリプタテーブル)
        // Parameter 2: シャドウマップテクスチャ register(t1) (ディスクリプタテーブル)
        D3D12_ROOT_PARAMETER root_params[3]{};

        // 1. 定数バッファ (CBV register(b0))
        root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        root_params[0].Descriptor.ShaderRegister = 0;                 // register(b0)
        root_params[0].Descriptor.RegisterSpace = 0;                  // space0
        root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // 頂点・ピクセル両方から参照

        // 2. アルベドテクスチャ (SRV register(t0))
        D3D12_DESCRIPTOR_RANGE albedo_range{};
        albedo_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        albedo_range.NumDescriptors = 1;
        albedo_range.BaseShaderRegister = 0;                             // register(t0)
        albedo_range.RegisterSpace = 0;
        albedo_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_params[1].DescriptorTable.NumDescriptorRanges = 1;
        root_params[1].DescriptorTable.pDescriptorRanges = &albedo_range;
        root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダから参照

        // 3. シャドウマップテクスチャ (SRV register(t1))
        D3D12_DESCRIPTOR_RANGE shadow_range{};
        shadow_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        shadow_range.NumDescriptors = 1;
        shadow_range.BaseShaderRegister = 1;                             // register(t1)
        shadow_range.RegisterSpace = 0;
        shadow_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        root_params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_params[2].DescriptorTable.NumDescriptorRanges = 1;
        root_params[2].DescriptorTable.pDescriptorRanges = &shadow_range;
        root_params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダから参照

        // スタティックサンプラー群の定義
        D3D12_STATIC_SAMPLER_DESC static_samplers[2]{};

        // Sampler 0: アルベド用バイリニアサンプラー (register(s0))
        static_samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;         // バイリニア補間
        static_samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;       // UV > 1.0 でタイリング繰り返し
        static_samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        static_samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        static_samplers[0].MipLODBias = 0.0f;
        static_samplers[0].MaxAnisotropy = 1;
        static_samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        static_samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        static_samplers[0].MinLOD = 0.0f;
        static_samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        static_samplers[0].ShaderRegister = 0;                               // register(s0)
        static_samplers[0].RegisterSpace = 0;
        static_samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Sampler 1: シャドウマップ用ハードウェア比較サンプラー (register(s1))
        // ※ 深度比較と 2x2 バイリニア補間を GPU ハードウェアで一括処理
        static_samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        static_samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        static_samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        static_samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        static_samplers[1].MipLODBias = 0.0f;
        static_samplers[1].MaxAnisotropy = 1;
        static_samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // 深度 <= シャドウマップなら遮蔽なし
        static_samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE; // 範囲外は深度 1.0 (光が当たる)
        static_samplers[1].MinLOD = 0.0f;
        static_samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
        static_samplers[1].ShaderRegister = 1;                               // register(s1)
        static_samplers[1].RegisterSpace = 0;
        static_samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC root_sig_desc{};
        root_sig_desc.NumParameters = _countof(root_params);
        root_sig_desc.pParameters = root_params;
        root_sig_desc.NumStaticSamplers = _countof(static_samplers);
        root_sig_desc.pStaticSamplers = static_samplers;
        root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // シリアライズ（バイナリデータ化）
        ComPtr<ID3DBlob> signature_blob;
        ComPtr<ID3DBlob> error_blob;
        ThrowIfFailed(
            D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob),
            "ルートシグネチャのシリアライズに失敗しました。"
        );

        // ルートシグネチャオブジェクトの生成
        ThrowIfFailed(
            device->CreateRootSignature(
                0,
                signature_blob->GetBufferPointer(),
                signature_blob->GetBufferSize(),
                IID_PPV_ARGS(&root_signature_)
            ),
            "ルートシグネチャの生成に失敗しました。"
        );
    }

    void Pipeline::CreatePipelineState(ID3D12Device* device, const Shader& vs, const Shader& ps)
    {
        // 1. 頂点入力レイアウト（Vertex 構造体のメモリ並び順を GPU に教える）
        // HLSL 側の VSInput (position, normal, color, texcoord) と完全に対応させます
        D3D12_INPUT_ELEMENT_DESC input_elements[] = {
            {
                "POSITION",                                    // 3次元座標
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,                   // float 3つ (12 bytes)
                0,
                static_cast<UINT>(offsetof(Vertex, position)), // オフセット 0
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            },
            {
                "NORMAL",                                      // 法線ベクトル（ライティング用）
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,                   // float 3つ (12 bytes)
                0,
                static_cast<UINT>(offsetof(Vertex, normal)),   // オフセット 12
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            },
            {
                "COLOR",                                       // 頂点固有カラー
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,                // float 4つ (16 bytes)
                0,
                static_cast<UINT>(offsetof(Vertex, color)),    // オフセット 24
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            },
            {
                "TEXCOORD",                                    // テクスチャ UV 座標
                0,
                DXGI_FORMAT_R32G32_FLOAT,                      // float 2つ (8 bytes)
                0,
                static_cast<UINT>(offsetof(Vertex, texcoord)),  // オフセット 40
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            }
        };

        // 2. パイプラインステート (PSO) の設定項目を埋める
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};
        pso_desc.pRootSignature = root_signature_.Get(); // 契約書をセット

        // シェーダバイトコード
        pso_desc.VS = { vs.GetBufferPointer(), vs.GetBufferSize() };
        pso_desc.PS = { ps.GetBufferPointer(), ps.GetBufferSize() };

        // 頂点入力レイアウト
        pso_desc.InputLayout = { input_elements, _countof(input_elements) };

        // ラスタライザステート（ポリゴンの三角形化やカリングの設定）
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 塗りつぶし
        // 3D 空間では、時計回りを「表面」とし、カメラから見て裏側を向いている面を描画省略（背面カリング）
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        pso_desc.RasterizerState.FrontCounterClockwise = FALSE; // 時計回りが表面
        pso_desc.RasterizerState.DepthClipEnable = TRUE;

        // ブレンドステート（不透明・上書き描画）
        D3D12_RENDER_TARGET_BLEND_DESC default_rt_blend{};
        default_rt_blend.BlendEnable = FALSE; // 半透明合成はオフ
        default_rt_blend.LogicOpEnable = FALSE;
        default_rt_blend.SrcBlend = D3D12_BLEND_ONE;
        default_rt_blend.DestBlend = D3D12_BLEND_ZERO;
        default_rt_blend.BlendOp = D3D12_BLEND_OP_ADD;
        default_rt_blend.SrcBlendAlpha = D3D12_BLEND_ONE;
        default_rt_blend.DestBlendAlpha = D3D12_BLEND_ZERO;
        default_rt_blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        default_rt_blend.LogicOp = D3D12_LOGIC_OP_NOOP;
        default_rt_blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RGBA すべて書き込む

        pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
        pso_desc.BlendState.IndependentBlendEnable = FALSE;
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            pso_desc.BlendState.RenderTarget[i] = default_rt_blend;
        }

        // 深度ステンシルステート（Zテストの有効化）
        // 手前にあるピクセルだけを描画し、奥にあるものは自動的に破棄して前後関係を正しく保ちます
        pso_desc.DepthStencilState.DepthEnable = TRUE;
        pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // Z値が現在より小さい（手前）なら上書き
        pso_desc.DepthStencilState.StencilEnable = FALSE;
        pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 深度バッファのフォーマット

        pso_desc.SampleMask = UINT_MAX;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形描画
        pso_desc.NumRenderTargets = 1;                                          // 出力先レンダーターゲットは1枚
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                   // バックバッファのフォーマットと一致
        pso_desc.SampleDesc.Count = 1;                                          // マルチサンプリングなし

        // パイプラインステートの生成
        ThrowIfFailed(
            device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state_)),
            "パイプラインステート (PSO) の生成に失敗しました。"
        );
    }

    void Pipeline::Bind(ID3D12GraphicsCommandList* command_list) const noexcept
    {
        command_list->SetGraphicsRootSignature(root_signature_.Get());
        command_list->SetPipelineState(pipeline_state_.Get());
    }

    void Pipeline::SetConstantBufferView(ID3D12GraphicsCommandList* command_list, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_address) const noexcept
    {
        command_list->SetGraphicsRootConstantBufferView(root_parameter_index, buffer_address);
    }

    void Pipeline::SetDescriptorTable(ID3D12GraphicsCommandList* command_list, UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor) const noexcept
    {
        command_list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
    }
}

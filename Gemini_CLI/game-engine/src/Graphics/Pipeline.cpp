#include "Graphics/Pipeline.hpp"
#include <DirectXMath.h>
#include <iostream>

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
        // register(b0) の定数バッファを受け取るためのルート記述子 (CBV) を1つ定義します
        D3D12_ROOT_PARAMETER root_param{};
        root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 定数バッファビュー
        root_param.Descriptor.ShaderRegister = 0;                 // register(b0)
        root_param.Descriptor.RegisterSpace = 0;                  // space0
        root_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 頂点シェーダから参照

        D3D12_ROOT_SIGNATURE_DESC root_sig_desc{};
        root_sig_desc.NumParameters = 1;                          // ルートパラメータの数: 1
        root_sig_desc.pParameters = &root_param;
        root_sig_desc.NumStaticSamplers = 0;
        root_sig_desc.pStaticSamplers = nullptr;
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
        // HLSL 側の VSInput (float3 position : POSITION, float4 color : COLOR) と完全に対応させます
        D3D12_INPUT_ELEMENT_DESC input_elements[] = {
            {
                "POSITION",                                    // HLSL のセマンティクス名
                0,                                             // インデックス
                DXGI_FORMAT_R32G32B32_FLOAT,                   // float 3つ分 (12 bytes)
                0,                                             // 入力スロット
                0,                                             // 構造体の先頭からのオフセット (バイト)
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    // 頂点ごとのデータ
                0
            },
            {
                "COLOR",                                       // HLSL のセマンティクス名
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,                // float 4つ分 (16 bytes)
                0,
                12,                                            // POSITION (12 bytes) の直後から開始
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
}

#include "Graphics/ShadowPipeline.hpp"
#include "Graphics/VertexBuffer.hpp"
#include <iostream>
#include <cstddef>

namespace Graphics
{
    ShadowPipeline::ShadowPipeline(ID3D12Device* device, const Shader& vs)
    {
        CreateRootSignature(device);
        CreatePipelineState(device, vs);

        std::cout << "[Graphics] ShadowPipeline (深度専用 PSO) を作成しました。\n";
    }

    void ShadowPipeline::CreateRootSignature(ID3D12Device* device)
    {
        // 深度パスでは定数バッファ (register(b0)) だけが必要
        D3D12_ROOT_PARAMETER root_param{};
        root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        root_param.Descriptor.ShaderRegister = 0; // register(b0)
        root_param.Descriptor.RegisterSpace = 0;
        root_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 頂点シェーダのみ参照

        D3D12_ROOT_SIGNATURE_DESC root_sig_desc{};
        root_sig_desc.NumParameters = 1;
        root_sig_desc.pParameters = &root_param;
        root_sig_desc.NumStaticSamplers = 0;
        root_sig_desc.pStaticSamplers = nullptr;
        root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> signature_blob;
        Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
        ThrowIfFailed(
            D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob),
            "ShadowPipeline ルートシグネチャのシリアライズに失敗しました。"
        );

        ThrowIfFailed(
            device->CreateRootSignature(
                0,
                signature_blob->GetBufferPointer(),
                signature_blob->GetBufferSize(),
                IID_PPV_ARGS(&root_signature_)
            ),
            "ShadowPipeline ルートシグネチャの生成に失敗しました。"
        );
    }

    void ShadowPipeline::CreatePipelineState(ID3D12Device* device, const Shader& vs)
    {
        // 頂点入力レイアウト（既存の Vertex 構造体と一致させる）
        D3D12_INPUT_ELEMENT_DESC input_elements[] = {
            {
                "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                static_cast<UINT>(offsetof(Vertex, position)),
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
            },
            {
                "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                static_cast<UINT>(offsetof(Vertex, normal)),
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
            },
            {
                "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                static_cast<UINT>(offsetof(Vertex, color)),
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
            },
            {
                "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                static_cast<UINT>(offsetof(Vertex, texcoord)),
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
            },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};
        pso_desc.pRootSignature = root_signature_.Get();
        pso_desc.VS = { vs.GetBufferPointer(), vs.GetBufferSize() };
        pso_desc.PS = { nullptr, 0 }; // ※ ピクセルシェーダは使用しない（超高速深度書き込み）

        pso_desc.InputLayout = { input_elements, _countof(input_elements) };
        pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // ラスタライザステート
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        pso_desc.RasterizerState.FrontCounterClockwise = FALSE;
        // シャドウアクネ（縞模様の自己遮蔽アーティファクト）を防止するためのハードウェア深度バイアス
        pso_desc.RasterizerState.DepthBias = 100;
        pso_desc.RasterizerState.DepthBiasClamp = 0.0f;
        pso_desc.RasterizerState.SlopeScaledDepthBias = 1.5f;
        pso_desc.RasterizerState.DepthClipEnable = TRUE;

        // ブレンドステート（カラー出力がないためデフォルト設定）
        pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
        pso_desc.BlendState.IndependentBlendEnable = FALSE;

        // 深度ステンシルステート: 深度テストおよび書き込みを有効化
        pso_desc.DepthStencilState.DepthEnable = TRUE;
        pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        pso_desc.DepthStencilState.StencilEnable = FALSE;

        pso_desc.SampleMask = UINT_MAX;
        pso_desc.SampleDesc.Count = 1;
        pso_desc.SampleDesc.Quality = 0;

        // レンダーターゲット設定: カラーターゲットは 0 個（DSV のみ）
        pso_desc.NumRenderTargets = 0;
        pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        ThrowIfFailed(
            device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state_)),
            "ShadowPipeline PSO の生成に失敗しました。"
        );
    }

    void ShadowPipeline::Bind(ID3D12GraphicsCommandList* command_list) const noexcept
    {
        command_list->SetGraphicsRootSignature(root_signature_.Get());
        command_list->SetPipelineState(pipeline_state_.Get());
    }

    void ShadowPipeline::SetConstantBufferView(
        ID3D12GraphicsCommandList* command_list,
        UINT root_parameter_index,
        D3D12_GPU_VIRTUAL_ADDRESS buffer_address) const noexcept
    {
        command_list->SetGraphicsRootConstantBufferView(root_parameter_index, buffer_address);
    }
}

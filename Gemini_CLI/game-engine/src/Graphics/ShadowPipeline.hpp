#pragma once

#include "Graphics/DirectXHelper.hpp"
#include "Graphics/Shader.hpp"

namespace Graphics
{
    /**
     * @brief シャドウマップ書き込み（Depth Pass）専用のパイプラインクラス
     *
     * 【このクラスの責務】
     * 1. 深度書き込み専用の最小ルートシグネチャ（定数バッファ b0 のみ）を作成
     * 2. ピクセルシェーダを持たず、カラー描画を行わない超高速 PSO（RTV なし、DSV のみ）を構築
     * 3. コマンドリストへのパイプラインバインドと定数バッファ設定
     */
    class ShadowPipeline
    {
    public:
        /**
         * @brief コンストラクタ
         * @param device DirectX 12 デバイス
         * @param vs 深度書き込み用頂点シェーダ (ShadowDepth.hlsl)
         */
        ShadowPipeline(ID3D12Device* device, const Shader& vs);

        ~ShadowPipeline() = default;

        ShadowPipeline(const ShadowPipeline&) = delete;
        ShadowPipeline& operator=(const ShadowPipeline&) = delete;
        ShadowPipeline(ShadowPipeline&&) noexcept = default;
        ShadowPipeline& operator=(ShadowPipeline&&) noexcept = default;

        /**
         * @brief シャドウパス描画に先立ち、パイプラインとルートシグネチャをバインド
         */
        void Bind(ID3D12GraphicsCommandList* command_list) const noexcept;

        /**
         * @brief ルートパラメータに定数バッファ (CBV) をセット
         */
        void SetConstantBufferView(
            ID3D12GraphicsCommandList* command_list,
            UINT root_parameter_index,
            D3D12_GPU_VIRTUAL_ADDRESS buffer_address) const noexcept;

        [[nodiscard]] ID3D12RootSignature* GetRootSignature() const noexcept { return root_signature_.Get(); }
        [[nodiscard]] ID3D12PipelineState* GetPipelineState() const noexcept { return pipeline_state_.Get(); }

    private:
        void CreateRootSignature(ID3D12Device* device);
        void CreatePipelineState(ID3D12Device* device, const Shader& vs);

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state_;
    };
}

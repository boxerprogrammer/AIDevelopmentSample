#pragma once

#include "Graphics/DirectXHelper.hpp"
#include "Graphics/Shader.hpp"

namespace Graphics
{
    /**
     * @brief パイプラインステート (PSO) と ルートシグネチャを管理するクラス
     *
     * 【このクラスの責務】
     * 1. ルートシグネチャ（シェーダが外部データを受け取る窓口の契約書）の作成
     * 2. 頂点入力レイアウト（頂点データのメモリ並び）の定義
     * 3. シェーダ、ラスタライザ、ブレンドモード等を1つに焼き固めた PSO の生成
     * 4. コマンドリストへのパイプライン一括バインド
     */
    class Pipeline
    {
    public:
        /**
         * @brief パイプラインステートを構築するコンストラクタ
         * @param device グラフィックスデバイス
         * @param vs 頂点シェーダ
         * @param ps ピクセルシェーダ
         */
        Pipeline(ID3D12Device* device, const Shader& vs, const Shader& ps);

        ~Pipeline() = default;

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;
        Pipeline(Pipeline&&) noexcept = default;
        Pipeline& operator=(Pipeline&&) noexcept = default;

        /**
         * @brief 描画命令に先立ち、ルートシグネチャとパイプラインステートをコマンドリストに適用する
         */
        void Bind(ID3D12GraphicsCommandList* command_list) const noexcept;

        /**
         * @brief ルートパラメータに定数バッファ (CBV) をセットする
         * @param command_list コマンドリスト
         * @param root_parameter_index ルートパラメータのインデックス（今回は 0）
         * @param buffer_address 定数バッファの GPU 仮想アドレス
         */
        void SetConstantBufferView(ID3D12GraphicsCommandList* command_list, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_address) const noexcept;

        [[nodiscard]] ID3D12RootSignature* GetRootSignature() const noexcept { return root_signature_.Get(); }
        [[nodiscard]] ID3D12PipelineState* GetPipelineState() const noexcept { return pipeline_state_.Get(); }

    private:
        void CreateRootSignature(ID3D12Device* device);
        void CreatePipelineState(ID3D12Device* device, const Shader& vs, const Shader& ps);

    private:
        ComPtr<ID3D12RootSignature> root_signature_; // シェーダの引数仕様書
        ComPtr<ID3D12PipelineState> pipeline_state_; // GPU の描画設定を固定化したバイナリ
    };
}

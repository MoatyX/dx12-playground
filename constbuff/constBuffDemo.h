#pragma once

#include "dx_app_base.h"
#include "upload_buffer.h"

using Microsoft::WRL::ComPtr;

class constBuffDemo : public dx_app_base
{
public:
	constBuffDemo(UINT width, UINT height, std::wstring title) : dx_app_base(width, height, title),
		m_rect_(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
		m_vp_(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)) {  }

	void create_root_sig();
	void create_pso();
	void createAssets();
	void create_const_buffer();
	void on_init() override;
	void on_pre_render() override;
	void on_post_render() override;
	void PopulateCommandList();
	void on_render() override;

	void on_update() override;

	struct Vertex
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 color;
	};

	struct __CONST_BUFFER SceneConstantBuffer
	{
		DirectX::XMFLOAT4 offset;
	};

private:
	CD3DX12_RECT m_rect_;
	CD3DX12_VIEWPORT m_vp_;
	ComPtr<ID3D12Resource> m_cbv_;
	ComPtr<ID3D12RootSignature> m_root_sig_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso_;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_constantBuffer;
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap;

	UINT8* m_pCbvDataBegin;
};


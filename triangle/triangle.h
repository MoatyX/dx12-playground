#pragma once
#include "dx_app_base.h"
#include "geometry.h"

/// <summary>
/// simple dx12 app that tries to draw a triangle on the screen
/// </summary>
class triangle : public dx_app_base
{
public:
	triangle(UINT width, UINT height, std::wstring title) : dx_app_base(width, height, title),
			m_rect_(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
			m_vp_(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)){};
	
	void on_init() override;
	void on_pre_render() override;
	void on_post_render() override;
	void on_render() override;

protected:
	//void init_dx() override;

private:
	void set_pipeline_state();
	void create_root_signature();
	void create_vertex_buffer();

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso_;

	geometry m_triangle_;

	CD3DX12_RECT m_rect_;
	CD3DX12_VIEWPORT m_vp_;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
};


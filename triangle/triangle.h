#pragma once

#include "dx_app_base.h"
#include "geometry.h"
#include "upload_buffer.h"
#include "simple_input.h"

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

	void on_update() override;

	void bind_input_system(simple_input* input) { input_ = input; };

private:
	void set_pipeline_state();
	void create_root_signature();
	void create_vertex_buffer();
	void create_const_buffer();

	/// <summary>
	/// example of a constant buffer aimed to be used in the vertex shader per object
	/// </summary>
	struct __CONST_BUFFER per_obj_cb
	{
		//DirectX::XMFLOAT4X4 world_view_proj = math_help::identity4x4();
		DirectX::XMFLOAT4 offset;
	};

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso_;

	geometry m_triangle_;

	CD3DX12_RECT m_rect_;
	CD3DX12_VIEWPORT m_vp_;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbv_heap_;		//constant buffer heap
	std::unique_ptr<upload_buffer<per_obj_cb>> m_triangle_cb_;

	simple_input* input_;
};


#pragma once

#include "dx12.h"

class dx_app_base
{
public:
	dx_app_base(UINT width, UINT height, std::wstring title);
	virtual ~dx_app_base() = default;

	/// <summary>
	/// initialise Dx12 and create a Window
	/// </summary>
	void init_core();

	virtual void on_init() {}
	virtual void on_update() {}
	virtual void on_pre_render();
	virtual void on_render() {}
	virtual void on_post_render();
	virtual void on_destroy() {}

	virtual void on_resize(UINT new_width, UINT new_height);

	/// <summary>
	/// do cmd parsing to init the app differently
	/// </summary>
	/// <param name="argv"></param>
	/// <param name="argc"></param>
	virtual void parse_cmd(WCHAR* argv[], int argc) {}

	UINT get_height() const;
	UINT get_width() const;
	const WCHAR* get_title() const;

	/// <summary>
	/// wait until the all commands in the command queue has finished executing on the GPU
	/// </summary>
	void wait_cmd_queue_finished();

public:
	static const UINT m_swap_chain_buffer_count = 2;

protected:
	virtual void init_dx();

	D3D12_CPU_DESCRIPTOR_HANDLE get_back_buffer_handle() const;
	D3D12_CPU_DESCRIPTOR_HANDLE get_dsv_handle() const;
	UINT get_msaa_sample_count() const;
	UINT get_msaa_quality_count() const;

	void rest_cmd();

protected:
	UINT m_width_;
	UINT m_height_;
	UINT m_client_refresh_rate_ = 60;

	Microsoft::WRL::ComPtr<ID3D12Device> m_gpu_;
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory4_;

	//CPU-GPU sync objects
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence_;
	UINT64 m_fence_value_;
	HANDLE m_fence_event_;

	UINT m_cbv_uav_srv_size_;
	UINT m_rtv_size_;
	UINT m_dsv_size_;

	DXGI_FORMAT m_back_buffer_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_depth_stencil_format_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
	UINT m_msaa_sample_count_ = 4;
	UINT m_msaa_quality_count_ = 0;
	bool m_msaa_ = false;

	UINT m_current_back_buffer_index_;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmd_queue_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmd_allocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmd_list_;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swap_chain_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_heap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsv_heap_;

	//buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depth_stencil_buffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_swap_chain_buffers_[m_swap_chain_buffer_count];

	//viewport and scissors
	D3D12_VIEWPORT m_viewport_;
	RECT m_scissor_;

private:
	void create_rtv_dsv_descriptor_heaps();
	void create_render_target_view();
	void set_view_port_scissor();
	void create_swap_chain();
	void create_dsv_res();

private:
	std::wstring m_title_;
};

#include "dx_app_base.h"

#include <iostream>

#include "macros.h"
#include "win32_app.h"

void dx_app_base::create_rtv_dsv_descriptor_heaps()
{
	//create as many RTVs as buffer count in the swap chain to describe the swap chain's buffer resource
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
	rtv_heap_desc.NumDescriptors = m_swap_chain_buffer_count;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_heap_desc.NodeMask = 0;		//single gpu systems
	THROW_IF_FAILED(m_gpu_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&m_rtv_heap_)))

	//create a heap for depth/stencil descriptor
	D3D12_DESCRIPTOR_HEAP_DESC dsv_desc;
	dsv_desc.NumDescriptors = 1;
	dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsv_desc.NodeMask = 0;
	THROW_IF_FAILED(m_gpu_->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(&m_dsv_heap_)))
}

void dx_app_base::create_render_target_view()
{
	//create frame resource
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handler(m_rtv_heap_->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < m_swap_chain_buffer_count; ++i)
	{
		//get a buffer from the swap chain buffers
		THROW_IF_FAILED(m_swap_chain_->GetBuffer(i, IID_PPV_ARGS(&m_swap_chain_buffers_[i])))

		//create a render target view for it
		m_gpu_->CreateRenderTargetView(m_swap_chain_buffers_[i].Get(), nullptr, rtv_heap_handler);

		//offset the heap
		if (i + 1 < m_swap_chain_buffer_count)
			rtv_heap_handler.Offset(i + 1, m_rtv_size_);
	}
}

void dx_app_base::set_view_port_scissor()
{
	m_viewport_.TopLeftX = 0;
	m_viewport_.TopLeftY = 0;
	m_viewport_.Width = static_cast<float>(m_width_);
	m_viewport_.Height = static_cast<float>(m_height_);
	m_viewport_.MinDepth = 0;
	m_viewport_.MaxDepth = 1;

	m_cmd_list_->RSSetViewports(1, &m_viewport_);

	m_scissor_ = { 0, 0, static_cast<LONG>(m_width_), static_cast<LONG>(m_height_) };
}

void dx_app_base::init_dx()
{
	UINT dxgi_factory_flags = 0;

	/*	first enable the debug layer if in debug mode	*/
#if defined(DEBUG) || defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug>	debug_controller;
	THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
	debug_controller->EnableDebugLayer();
	// Enable additional debug layers.
	dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	//==============================================================================

	//2nd get a GPU device
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adaptor;
	THROW_IF_FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_factory4_)));
	get_hardware_adapter(m_factory4_.Get(), adaptor.GetAddressOf(), true);
	THROW_IF_FAILED(D3D12CreateDevice(adaptor.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_gpu_)))

	//3rd: create a fence and get the descriptor sizes
	THROW_IF_FAILED(m_gpu_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence_)))
	m_fence_event_ = CreateEvent(nullptr, false, false, L"Fence Event");
	if(m_fence_event_ == nullptr) THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()))
	//=================================================================================

	// cache descriptor sizes
	m_cbv_uav_srv_size_ = m_gpu_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_rtv_size_ = m_gpu_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsv_size_ = m_gpu_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//===================================================================================

	// check MSAA support
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms;
	ms.Format = m_back_buffer_format_;
	ms.SampleCount = m_msaa_sample_count_;
	ms.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	THROW_IF_FAILED(m_gpu_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms, sizeof(ms)))
	//for valid input, the MSAA Quality Level must be = 0
	m_msaa_quality_count_ = ms.NumQualityLevels;
	assert(m_msaa_quality_count_ > 0 && "Unexpected MSAA Qualtiy Level.");
	//====================================================================================

	//5th create the commands queue, commands list and commands allocator used to send commands to the gpu
	D3D12_COMMAND_QUEUE_DESC queue_desc;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;		//directly executable by the GPU
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.NodeMask = 0;
	queue_desc.Priority = 0;
	THROW_IF_FAILED(m_gpu_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_cmd_queue_)))
	THROW_IF_FAILED(m_gpu_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmd_allocator_)))

	//we specify 0 in the first argument to say that we only work with single GPU systems
	THROW_IF_FAILED(m_gpu_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocator_.Get(), nullptr, IID_PPV_ARGS(m_cmd_list_.GetAddressOf())))
	//m_cmd_list_->Close();
	//==================================================================================

	create_swap_chain();

	//7th create Render Target and Depth Stencil Descriptor heaps
	create_rtv_dsv_descriptor_heaps();
	create_dsv_res();
	

	//8th create the render target view of the swap chain
	create_render_target_view();

	//9th set viewports and scissors
	set_view_port_scissor();

	m_cmd_list_->Close();
}

UINT dx_app_base::get_msaa_sample_count() const
{
	return m_msaa_ ? m_msaa_sample_count_ : 1;
}

UINT dx_app_base::get_msaa_quality_count() const
{
	return m_msaa_ ? (m_msaa_quality_count_ - 1) : 0;
}

void dx_app_base::rest_cmd()
{
	wait_cmd_queue_finished();

	THROW_IF_FAILED(m_cmd_allocator_->Reset());
	THROW_IF_FAILED(m_cmd_list_->Reset(m_cmd_allocator_.Get(), m_pipeline_state.Get()));
}

void dx_app_base::create_swap_chain()
{
	m_swap_chain_.Reset();		//destroy the old one and create a new

	DXGI_SWAP_CHAIN_DESC1 swd = {0};
	/*describe the buffer*/
	swd.Width = m_width_;
	swd.Height = m_height_;
	swd.Format = m_back_buffer_format_;
	swd.SampleDesc.Count = get_msaa_sample_count();
	swd.SampleDesc.Quality = get_msaa_quality_count();
	swd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//the buffer is a render target
	swd.BufferCount = m_swap_chain_buffer_count;
	swd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	const auto window = win32_app::get_window();
	m_factory4_->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);		//the app does not support fullscreen yet

	//swap chain uses commands queue to perform the buffer flush
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
	THROW_IF_FAILED(m_factory4_->CreateSwapChainForHwnd(m_cmd_queue_.Get(), window, &swd, nullptr, nullptr, &swap_chain))
	THROW_IF_FAILED(swap_chain.As(&m_swap_chain_));

	m_current_back_buffer_index_ = m_swap_chain_->GetCurrentBackBufferIndex();
}

void dx_app_base::create_dsv_res()
{
	/**
	 * depth and stencil buffers are 2D textures that store depth data and help achieve rendering effects respectively.
	 * as always in dx12, we need to create the textures as resource, then bind these resource to the pipeline
	 */

	//first create the depth/stencil texture
	D3D12_RESOURCE_DESC dsv{ D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, m_width_, m_height_, 1, 1, m_depth_stencil_format_,
	{get_msaa_sample_count(), get_msaa_quality_count()}, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL };

	D3D12_CLEAR_VALUE clear{ m_depth_stencil_format_, {0} };
	clear.DepthStencil.Depth = 1.0f;
	clear.DepthStencil.Stencil = 0;

	auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	m_gpu_->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, 
	                                &dsv, D3D12_RESOURCE_STATE_COMMON, &clear, IID_PPV_ARGS(&m_depth_stencil_buffer));
	//==========================================================

	//describe the mip level 0, to use the entire texture
	D3D12_DEPTH_STENCIL_VIEW_DESC dsc;
	dsc.Flags = D3D12_DSV_FLAG_NONE;
	dsc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsc.Format = m_depth_stencil_format_;
	dsc.Texture2D.MipSlice = 0;		//for mip level 0, use the entire resource
	m_gpu_->CreateDepthStencilView(m_depth_stencil_buffer.Get(), &dsc, get_dsv_handle());
	//===========================================================

	//lastly transition the resource from its initial state to be used as a depth buffer
	CD3DX12_RESOURCE_BARRIER trans = CD3DX12_RESOURCE_BARRIER::Transition(m_depth_stencil_buffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_cmd_list_->ResourceBarrier(1, &trans);
	//=====================================================================================
}


dx_app_base::dx_app_base(const UINT width, const UINT height, std::wstring title)
{
	m_width_ = width;
	m_height_ = height;
	m_title_ = title;
}

void dx_app_base::init_core()
{
	init_dx();
	on_init();
}

void dx_app_base::on_pre_render()
{
	//reset the commands list and reuse the allocated memory
	THROW_IF_FAILED(m_cmd_allocator_->Reset());
	THROW_IF_FAILED(m_cmd_list_->Reset(m_cmd_allocator_.Get(), m_pipeline_state.Get()));

	//transition the back buffer as a render target
	auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swap_chain_buffers_[m_current_back_buffer_index_].Get(), 
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_cmd_list_->ResourceBarrier(1, &trans);

	//clear and apply stencil
	auto rtv_cpu_handle = get_back_buffer_handle();
	auto dsv_cpu_handle = get_dsv_handle();
	m_cmd_list_->ClearRenderTargetView(rtv_cpu_handle, DirectX::Colors::LightBlue, 0, nullptr);
	m_cmd_list_->OMSetRenderTargets(1, &rtv_cpu_handle, 
	                                true, &dsv_cpu_handle);
}

void dx_app_base::on_post_render()
{
	//transition the back buffer to being present
	auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swap_chain_buffers_[m_current_back_buffer_index_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_cmd_list_->ResourceBarrier(1, &trans);

	m_cmd_list_->Close();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_cmd_list_.Get() };
	m_cmd_queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	THROW_IF_FAILED(m_swap_chain_->Present(0, 0));
	m_current_back_buffer_index_ = m_swap_chain_->GetCurrentBackBufferIndex();

	wait_cmd_queue_finished();
}

void dx_app_base::on_resize(UINT new_width, UINT new_height)
{
	m_width_ = new_width;
	m_height_ = new_height;
}

UINT dx_app_base::get_height() const
{
	return m_height_;
}

UINT dx_app_base::get_width() const
{
	return m_width_;
}

const WCHAR* dx_app_base::get_title() const
{
	return m_title_.c_str();
}

void dx_app_base::wait_cmd_queue_finished()
{
	//set some value to the fence
	m_fence_value_++;
	THROW_IF_FAILED(m_cmd_queue_->Signal(m_fence_.Get(), m_fence_value_));

	//wait for all commands to finish
	if(m_fence_->GetCompletedValue() < m_fence_value_)
	{
		HANDLE fence_reached_event = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		THROW_IF_FAILED(m_fence_->SetEventOnCompletion(m_fence_value_, fence_reached_event));

		WaitForSingleObject(fence_reached_event, INFINITE);
		CloseHandle(fence_reached_event);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE dx_app_base::get_back_buffer_handle() const
{
	//CD3DX12 constructor to offset "current_back_buffer_" to the RTV of the current back buffer
	const D3D12_CPU_DESCRIPTOR_HANDLE back_buffer_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(),	//heap handle start
		m_current_back_buffer_index_,		//index to offset
		m_rtv_size_);

	return back_buffer_handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE dx_app_base::get_dsv_handle() const
{
	//there is only 1 dsv, so we don't need to do any offset
	return m_dsv_heap_->GetCPUDescriptorHandleForHeapStart();
}

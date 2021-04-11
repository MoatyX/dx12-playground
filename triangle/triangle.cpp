#include "triangle.h"

#include <iostream>

#include "geometry.h"
#include "simple_vertex.h"
#include "win32_app.h"

void triangle::on_init()
{
	LOG_INFO("Triangle App: draw a triangle on the screen");

	rest_cmd();

	create_const_buffer();
	create_root_signature();
	create_vertex_buffer();
	set_pipeline_state();

	m_cmd_list_->Close();
	ID3D12CommandList* ppCommandLists[] = { m_cmd_list_.Get() };
	m_cmd_queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	wait_cmd_queue_finished();

	input_ = simple_input::instance();
}

void triangle::on_pre_render()
{
	//reset the commands list and reuse the allocated memory
	THROW_IF_FAILED(m_cmd_allocator_->Reset());
	THROW_IF_FAILED(m_cmd_list_->Reset(m_cmd_allocator_.Get(), m_pipeline_state.Get()));

	//set state
	m_cmd_list_->SetPipelineState(m_pipeline_state.Get());
	m_cmd_list_->SetGraphicsRootSignature(m_root_signature_.Get());
	m_cmd_list_->RSSetViewports(1, &m_vp_);
	m_cmd_list_->RSSetScissorRects(1, &m_rect_);

	//set desc heap
	ID3D12DescriptorHeap* ppHeaps[] = { m_cbv_heap_.Get() };
	m_cmd_list_->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_cmd_list_->SetGraphicsRootDescriptorTable(0, m_cbv_heap_->GetGPUDescriptorHandleForHeapStart());
	
	//transition the back buffer as a render target
	auto trans = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swap_chain_buffers_[m_current_back_buffer_index_].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_cmd_list_->ResourceBarrier(1, &trans);

	//clear and apply stencil
	auto rtv_cpu_handle = get_back_buffer_handle();
	auto dsv_cpu_handle = get_dsv_handle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(), m_current_back_buffer_index_, m_rtv_size_);
	m_cmd_list_->OMSetRenderTargets(1, &rtv_cpu_handle,
		true, &dsv_cpu_handle);

	m_cmd_list_->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
}

void triangle::on_render()
{
	//render
	m_cmd_list_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	auto vertex_buffer_view = m_triangle_.vertex_buffer_view();
	m_cmd_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	auto index_buffer_view = m_triangle_.vertex_index_view();
	m_cmd_list_->IASetIndexBuffer(&index_buffer_view);
	m_cmd_list_->DrawIndexedInstanced(3, 1, 0, 0, 0);

	//transition the back buffer to being present
	auto trans2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swap_chain_buffers_[m_current_back_buffer_index_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_cmd_list_->ResourceBarrier(1, &trans2);
}

void triangle::on_update()
{
	static bool play_offset = false;
	
	if (input_->is_key_down(VK_F1)) play_offset = !play_offset;

	if (!play_offset) return;
	
	//update the constant buffer
	const float offset = 1.5f;
	const float speed = 1.0 / 144.0;
	static per_obj_cb triangle;
	//triangle.world_view_proj = math_help::identity4x4();
	triangle.offset.x += speed;
	if(triangle.offset.x > offset)
	{
		triangle.offset.x = -offset;
	}
	m_triangle_cb_->copy_data(&triangle);
}

void triangle::on_post_render()
{
	m_cmd_list_->Close();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_cmd_list_.Get() };
	m_cmd_queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	THROW_IF_FAILED(m_swap_chain_->Present(1, 0));
	m_current_back_buffer_index_ = m_swap_chain_->GetCurrentBackBufferIndex();

	wait_cmd_queue_finished();
}

void triangle::set_pipeline_state()
{
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compile_flags = 0;
#endif

	Microsoft::WRL::ComPtr<ID3DBlob> vertex_shader;
	Microsoft::WRL::ComPtr<ID3DBlob> pixel_shader;

	//compile shaders from file
	THROW_IF_FAILED(D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr,
		"main", "vs_5_0", compile_flags, 0, &vertex_shader, nullptr));
	THROW_IF_FAILED(D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr,
		"main", "ps_5_0", compile_flags, 0, &pixel_shader, nullptr));

	//define the vertex input layout
	D3D12_INPUT_ELEMENT_DESC input_elements[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(simple_vertex::position),
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	//create and describe the pipeline state object (PSO)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { input_elements, _countof(input_elements) };
	psoDesc.pRootSignature = m_root_signature_.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;		//TODO: set true
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_back_buffer_format_;
	psoDesc.SampleDesc.Count = get_msaa_sample_count();
	psoDesc.SampleDesc.Quality = get_msaa_quality_count();
	psoDesc.DSVFormat = m_depth_stencil_format_;

	THROW_IF_FAILED(m_gpu_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipeline_state)));
}

void triangle::create_root_signature()
{
	// Create a root signature consisting of a descriptor table with a single CBV.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_gpu_->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
	{
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_DESCRIPTOR_RANGE1 cbv_ranges[1];
	CD3DX12_ROOT_PARAMETER1 root_parameters[1];
	cbv_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	root_parameters[0].InitAsDescriptorTable(1, cbv_ranges, D3D12_SHADER_VISIBILITY_VERTEX);

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	const D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc;
	root_sig_desc.Init_1_1(_countof(root_parameters), root_parameters, 0, nullptr, root_signature_flags);

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;
	THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&root_sig_desc, &signature, &error));
	THROW_IF_FAILED(m_gpu_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_signature_)));
}

void triangle::create_vertex_buffer()
{
	//create the vertex buffer for the triangle
	float ratio = get_width() / get_height();
	auto c0 = DirectX::XMFLOAT4(DirectX::Colors::Red);
	auto c1 = DirectX::XMFLOAT4(DirectX::Colors::Blue);
	auto c2 = DirectX::XMFLOAT4(DirectX::Colors::Green);
	simple_vertex triangle_vertices[] = {
			{ { 0.0f, 0.5, 0.0f, 1 }, c0 },
			{ { 0.5f, -0.5, 0.0f, 1 }, c1 },
			{ { -0.5f, -0.5, 0.0f, 1 }, c2 }
	};

	uint16_t indices[] = { 0, 1, 2 };

	const UINT triangle_size = sizeof(triangle_vertices);
	m_triangle_ = geometry(triangle_size, sizeof(simple_vertex), sizeof(indices));
	m_triangle_.vertex_buffer_gpu = create_default_buffer(m_gpu_.Get(), m_cmd_list_.Get(), triangle_vertices, triangle_size, m_triangle_.vertex_buffer_uploader);
	m_triangle_.index_buffer_gpu = create_default_buffer(m_gpu_.Get(), m_cmd_list_.Get(), indices, sizeof(indices), m_triangle_.index_buffer_uploader);
}


void triangle::create_const_buffer()
{

	/*first create the heap for it*/
	//we are going to draw one object, so make place for 1 cbv descriptor
	D3D12_DESCRIPTOR_HEAP_DESC cbv_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	THROW_IF_FAILED(m_gpu_->CreateDescriptorHeap(&cbv_desc, IID_PPV_ARGS(&m_cbv_heap_)));

	/*now create the constant buffer*/
	m_triangle_cb_ = std::make_unique<upload_buffer<per_obj_cb>>(m_gpu_.Get(), 1);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = { m_triangle_cb_->get_resource()->GetGPUVirtualAddress(), sizeof(per_obj_cb) };
	m_gpu_->CreateConstantBufferView(&cbv, m_cbv_heap_->GetCPUDescriptorHandleForHeapStart());
}

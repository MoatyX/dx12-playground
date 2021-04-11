#include "constBuffDemo.h"


#include "upload_buffer.h"
#include "win32_app.h"


void constBuffDemo::create_root_sig()
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
    THROW_IF_FAILED(m_gpu_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_root_sig_)));
}

void constBuffDemo::create_pso()
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
    THROW_IF_FAILED(D3DCompileFromFile(L"shaders.txt", nullptr, nullptr, "VSMain", "vs_5_0", compile_flags, 0, &vertex_shader, nullptr));
    THROW_IF_FAILED(D3DCompileFromFile(L"shaders.txt", nullptr, nullptr,
        "PSMain", "ps_5_0", compile_flags, 0, &pixel_shader, nullptr));

    //define the vertex input layout
    D3D12_INPUT_ELEMENT_DESC input_elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vertex::position),
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    //create and describe the pipeline state object (PSO)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { input_elements, _countof(input_elements) };
    psoDesc.pRootSignature = m_root_sig_.Get();
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

    THROW_IF_FAILED(m_gpu_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso_)));
}

void constBuffDemo::createAssets()
{

    create_root_sig();
    create_pso();

    // Create the vertex buffer.
    {
        float m_aspectRatio = 800 / 600;;
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f, 1 }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f, 1 }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f, 1 }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        auto x = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto x2 = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        THROW_IF_FAILED(m_gpu_->CreateCommittedResource(
            &x,
            D3D12_HEAP_FLAG_NONE,
            &x2,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        THROW_IF_FAILED(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }
}

static std::unique_ptr<upload_buffer<constBuffDemo::SceneConstantBuffer>> m_constBufferData;
void constBuffDemo::create_const_buffer()
{

    /*first create the heap for it*/
    //we are going to draw one object, so make place for 1 cbv descriptor
    D3D12_DESCRIPTOR_HEAP_DESC cbv_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    THROW_IF_FAILED(m_gpu_->CreateDescriptorHeap(&cbv_desc, IID_PPV_ARGS(&m_cbvHeap)));

    /*now create the constant buffer*/
    m_constBufferData = std::make_unique<upload_buffer<SceneConstantBuffer>>(m_gpu_.Get(), 1);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = { m_constBufferData->get_resource()->GetGPUVirtualAddress(), sizeof(SceneConstantBuffer) };
    m_gpu_->CreateConstantBufferView(&cbv, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void constBuffDemo::on_init()
{
    create_const_buffer();
    createAssets();
}

void constBuffDemo::on_pre_render()
{
}

void constBuffDemo::on_post_render()
{
}

void constBuffDemo::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
// command lists have finished execution on the GPU; apps should use 
// fences to determine GPU execution progress.
    THROW_IF_FAILED(m_cmd_allocator_->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    THROW_IF_FAILED(m_cmd_list_->Reset(m_cmd_allocator_.Get(), m_pso_.Get()));

    // Set necessary state.
    m_cmd_list_->SetGraphicsRootSignature(m_root_sig_.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    m_cmd_list_->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_cmd_list_->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    m_cmd_list_->RSSetViewports(1, &m_vp_);
    m_cmd_list_->RSSetScissorRects(1, &m_scissor_);

    auto cd_3dx12_resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swap_chain_buffers_[m_current_back_buffer_index_].Get(),
                                                                          D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Indicate that the back buffer will be used as a render target.
    m_cmd_list_->ResourceBarrier(1, &cd_3dx12_resource_barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtv_heap_->GetCPUDescriptorHandleForHeapStart(), m_current_back_buffer_index_, m_rtv_size_);
    m_cmd_list_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_cmd_list_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_cmd_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmd_list_->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_cmd_list_->DrawInstanced(3, 1, 0, 0);

    auto resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_swap_chain_buffers_[m_current_back_buffer_index_].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    // Indicate that the back buffer will now be used to present.
    m_cmd_list_->ResourceBarrier(1, &resource_barrier);

    THROW_IF_FAILED(m_cmd_list_->Close());
}

void constBuffDemo::on_render()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_cmd_list_.Get() };
    m_cmd_queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    THROW_IF_FAILED(m_swap_chain_->Present(1, 0));

    m_current_back_buffer_index_ = m_swap_chain_->GetCurrentBackBufferIndex();

    wait_cmd_queue_finished();
}

void constBuffDemo::on_update()
{
    const float translationSpeed = 0.005f;
    const float offsetBounds = 1.25f;

    static SceneConstantBuffer m_constantBufferData;
    m_constantBufferData.offset.x += translationSpeed;
    if (m_constantBufferData.offset.x > offsetBounds)
    {
        m_constantBufferData.offset.x = -offsetBounds;
    }

    m_constBufferData->copy_data(&m_constantBufferData);
	
}

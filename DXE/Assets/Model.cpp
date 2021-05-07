#include "Model.h"

Model::Model() : globalMeshID(0), modelType(ModelType::TEMPLATE_MODEL), IsDynamic(false){}

Model::Model(ModelType _type) : modelType(_type), globalMeshID(0), IsDynamic(false) {
     
}

D3D12_VERTEX_BUFFER_VIEW Model::getVertexBufferView() const {
    return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW Model::getIndexBufferView() const {
    return indexBufferView;
}

void Model::createBuffers(ID3D12Device* device) {
    

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),// this is a default heap
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(VertexBufferByteSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&VertexBufferGPU)
    ));

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(IndexBufferByteSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&IndexBufferGPU)
    ));

    vertexBufferView.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = VertexByteStride;
    vertexBufferView.SizeInBytes = VertexBufferByteSize;
    
    indexBufferView.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
    indexBufferView.Format = IndexFormat;
    indexBufferView.SizeInBytes = IndexBufferByteSize;
}

void Model::uploadBuffers(ID3D12Device* device, CommandObject* cmdObj) {


    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(VertexBufferByteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(VertexBufferUploader.GetAddressOf())

    ));

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(IndexBufferByteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(IndexBufferUploader.GetAddressOf())

    ));

    D3D12_SUBRESOURCE_DATA subResourceDataVertex = {};
    subResourceDataVertex.pData = static_cast<void*>(vertices.data());
    subResourceDataVertex.RowPitch = VertexBufferByteSize;
    subResourceDataVertex.SlicePitch = subResourceDataVertex.RowPitch;

    D3D12_SUBRESOURCE_DATA subResourceDataIndex = {};
    subResourceDataIndex.pData = static_cast<void*>(indices.data());
    subResourceDataIndex.RowPitch = IndexBufferByteSize;
    subResourceDataIndex.SlicePitch = subResourceDataIndex.RowPitch;

    UpdateSubresources<1>(cmdObj->getCommandListPtr(), VertexBufferGPU.Get(), VertexBufferUploader.Get(), 0, 0, 1, &subResourceDataVertex);
    cmdObj->getCommandListPtr()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(VertexBufferGPU.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    UpdateSubresources<1>(cmdObj->getCommandListPtr(), IndexBufferGPU.Get(), IndexBufferUploader.Get(), 0, 0, 1, &subResourceDataIndex);
    cmdObj->getCommandListPtr()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(IndexBufferGPU.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

}

std::unordered_map<std::string, Mesh>& Model::getDrawArgs() {
    return DrawArgs;
}

void Model::setWorldMatrix(const DirectX::XMFLOAT4X4& mat) {
    World = mat;
}
DirectX::XMFLOAT4X4& Model::getWorldMatrix() {
    return World;
}



void Model::InitModel(ID3D12Device* device, CommandObject* cmdObj) {

    switch (modelType)
    {
    case TEMPLATE_MODEL:
        buildGeometry();
        break;
    case ASSIMP_MODEL:
        buildGeometryAssimp();
        break;
    case QUAD_MODEL:
        buildQuadGeometry();
        break;
    case GRID_MODEL:
        buildGridGeometry();
        break;
    case CUBE_MODEL:
        buildCubeGeometry();
        break;
    case SPHERE_MODEL:
        buildSphereGeometry();
        break;
    case CYLINDER_MODEL:
        buildCylinderGeometry();
        break;
    default:
        buildGeometry();
        break;
    }
    createBuffers(device);
    uploadBuffers(device, cmdObj);
}

float Model::getObj2VoxelScale() {
    return Obj2VoxelScale;
}

void Model::setObj2VoxelScale(float _scale) {
    Obj2VoxelScale = _scale;
}

void Model::buildGeometryAssimp(){

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);



    ThrowIfFailed(D3DCreateBlob(vbByteSize, VertexBufferCPU.GetAddressOf()));
    CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, IndexBufferCPU.GetAddressOf()));
    CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    VertexBufferByteSize = vbByteSize;
    VertexByteStride = sizeof(Vertex);
    IndexFormat = DXGI_FORMAT_R32_UINT;
    IndexBufferByteSize = ibByteSize;

}

void Model::appendAssimpMesh(const aiScene* aiscene, aiMesh* aimesh) {
    
    UINT curVertexOffset = vertices.size();
    UINT curIndexOffset = indices.size();
    UINT curIndexCount = 0;

    for (unsigned int i = 0; i < aimesh->mNumVertices; ++i) {
       
        Vertex curVertex;
        curVertex.Pos = DirectX::XMFLOAT3(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z);
        mBounds.updateBounds(curVertex.Pos);
        curVertex.Normal = DirectX::XMFLOAT3(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z);
        if (aimesh->mTextureCoords[0]) {
            curVertex.TexC = DirectX::XMFLOAT2(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y);
        }
        vertices.push_back(curVertex);
    }

    for (unsigned int i = 0; i < aimesh->mNumFaces; ++i) {
        aiFace& aiface = aimesh->mFaces[i];
 
        for (unsigned int j = 0; j < aiface.mNumIndices; ++j) {
            indices.push_back(aiface.mIndices[j]);
            curIndexCount++;
        }
    }

    aiMaterial* aimaterial = aiscene->mMaterials[aimesh->mMaterialIndex];
    auto matname = aimaterial->GetName();

    Mesh curSubMesh;
    curSubMesh.IndexCount = (UINT)curIndexCount;
    curSubMesh.StartIndexLocation = curIndexOffset;
    curSubMesh.BaseVertexLocation = curVertexOffset;
    curSubMesh.materialName = matname.C_Str();
    std::string meshname = "mesh" + std::to_string(globalMeshID);
    DrawArgs[meshname] = curSubMesh;
    globalMeshID++;
}

const d3dUtil::Bound& Model::getBounds() const
{
    return mBounds;
}

void Model::buildQuadGeometry() {
    Geometry mGeo;
    Geometry::MeshData quad = mGeo.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    Mesh quadSubMesh;
    quadSubMesh.IndexCount = (UINT)quad.Indices32.size();
    quadSubMesh.StartIndexLocation = 0;
    quadSubMesh.BaseVertexLocation = 0;
    quadSubMesh.materialName = "mat1";
    DrawArgs["quad"] = quadSubMesh;

    auto totalVertCount = quad.Vertices.size();
    vertices.resize(totalVertCount);

    UINT k = 0;
    for (size_t i = 0; i < quad.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = quad.Vertices[i].Position;
        vertices[k].Normal = quad.Vertices[i].Normal;
        vertices[k].TexC = quad.Vertices[i].TexC;
    }

    indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));
    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, VertexBufferCPU.GetAddressOf()));
    CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, IndexBufferCPU.GetAddressOf()));
    CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    VertexBufferByteSize = vbByteSize;
    VertexByteStride = sizeof(Vertex);
    IndexFormat = DXGI_FORMAT_R32_UINT;
    IndexBufferByteSize = ibByteSize;
}

void Model::buildSphereGeometry() {
    Geometry mGeo;
    Geometry::MeshData sphere = mGeo.CreateSphere(8.f, 20, 20);

    UINT sphereVertexOffset = 0;
    UINT sphereIndexOffset = 0;

    Mesh sphereSubMesh;
    sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubMesh.StartIndexLocation = sphereIndexOffset;
    sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
    sphereSubMesh.materialName = "mat2";
    DrawArgs["sphere"] = sphereSubMesh;

    auto totalVertCount = sphere.Vertices.size();
    vertices.resize(totalVertCount);

    UINT k = 0;
    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
    }

    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, VertexBufferCPU.GetAddressOf()));
    CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, IndexBufferCPU.GetAddressOf()));
    CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    VertexBufferByteSize = vbByteSize;
    VertexByteStride = sizeof(Vertex);
    IndexFormat = DXGI_FORMAT_R32_UINT;
    IndexBufferByteSize = ibByteSize;
}
void Model::buildCubeGeometry() {
    Geometry mGeo;
    Geometry::MeshData sphere = mGeo.CreateBox(2.f, 2.f, 2.f, 3.0);

    UINT sphereVertexOffset = 0;
    UINT sphereIndexOffset = 0;

    Mesh sphereSubMesh;
    sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubMesh.StartIndexLocation = sphereIndexOffset;
    sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
    sphereSubMesh.materialName = "mat2";
    DrawArgs["cube"] = sphereSubMesh;

    auto totalVertCount = sphere.Vertices.size();
    vertices.resize(totalVertCount);

    UINT k = 0;
    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
    }

    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, VertexBufferCPU.GetAddressOf()));
    CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, IndexBufferCPU.GetAddressOf()));
    CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    VertexBufferByteSize = vbByteSize;
    VertexByteStride = sizeof(Vertex);
    IndexFormat = DXGI_FORMAT_R32_UINT;
    IndexBufferByteSize = ibByteSize;
}
void Model::buildGridGeometry() {
    Geometry mGeo;
    Geometry::MeshData sphere = mGeo.CreateGrid(20.f, 20.f, 5.f, 5.0);

    UINT sphereVertexOffset = 0;
    UINT sphereIndexOffset = 0;

    Mesh sphereSubMesh;
    sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubMesh.StartIndexLocation = sphereIndexOffset;
    sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
    sphereSubMesh.materialName = "mat2";
    DrawArgs["grid"] = sphereSubMesh;

    auto totalVertCount = sphere.Vertices.size();
    vertices.resize(totalVertCount);

    UINT k = 0;
    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
    }

    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, VertexBufferCPU.GetAddressOf()));
    CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, IndexBufferCPU.GetAddressOf()));
    CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    VertexBufferByteSize = vbByteSize;
    VertexByteStride = sizeof(Vertex);
    IndexFormat = DXGI_FORMAT_R32_UINT;
    IndexBufferByteSize = ibByteSize;
}
void Model::buildCylinderGeometry() {
    Geometry mGeo;
    Geometry::MeshData sphere = mGeo.CreateCylinder(5.f, 5.f, 5.f, 5.0, 5.0);

    UINT sphereVertexOffset = 0;
    UINT sphereIndexOffset = 0;

    Mesh sphereSubMesh;
    sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubMesh.StartIndexLocation = sphereIndexOffset;
    sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
    sphereSubMesh.materialName = "mat2";
    DrawArgs["grid"] = sphereSubMesh;

    auto totalVertCount = sphere.Vertices.size();
    vertices.resize(totalVertCount);

    UINT k = 0;
    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
    }

    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, VertexBufferCPU.GetAddressOf()));
    CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, IndexBufferCPU.GetAddressOf()));
    CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    VertexBufferByteSize = vbByteSize;
    VertexByteStride = sizeof(Vertex);
    IndexFormat = DXGI_FORMAT_R32_UINT;
    IndexBufferByteSize = ibByteSize;
}

void Model::buildGeometry() {
    Geometry mGeo;
    Geometry::MeshData box = mGeo.CreateBox(2.f, 2.f, 2.f, 3);
    Geometry::MeshData sphere = mGeo.CreateSphere(1.5f, 20, 20);

    UINT boxVertexOffset = 0;
    UINT sphereVertexOffset = (UINT)box.Vertices.size();

    UINT boxIndexOffset = 0;
    UINT sphereIndexOffset = (UINT)box.Indices32.size();

    Mesh boxSubMesh;
    boxSubMesh.IndexCount = (UINT)box.Indices32.size();
    boxSubMesh.StartIndexLocation = boxIndexOffset;
    boxSubMesh.BaseVertexLocation = boxVertexOffset;
    boxSubMesh.materialName = "mat1";
    DrawArgs["box"] = boxSubMesh;

    Mesh sphereSubMesh;
    sphereSubMesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubMesh.StartIndexLocation = sphereIndexOffset;
    sphereSubMesh.BaseVertexLocation = sphereVertexOffset;
    sphereSubMesh.materialName = "mat1";
    DrawArgs["sphere"] = sphereSubMesh;

    auto totalVertCount = box.Vertices.size() + sphere.Vertices.size();
    vertices.resize(totalVertCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Normal = box.Vertices[i].Normal;
        vertices[k].TexC = box.Vertices[i].TexC;
    }
    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
    }


    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);


    ThrowIfFailed(D3DCreateBlob(vbByteSize, VertexBufferCPU.GetAddressOf()));
    CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
    ThrowIfFailed(D3DCreateBlob(ibByteSize, IndexBufferCPU.GetAddressOf()));
    CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    VertexBufferByteSize = vbByteSize;
    VertexByteStride = sizeof(Vertex);
    IndexFormat = DXGI_FORMAT_R32_UINT;
    IndexBufferByteSize = ibByteSize;


}
#ifndef MODEL_H
#define MODEL_H

#include "Utilities/CommandObject.h"
#include "Utilities/Utils.h"

#include "Geometry.h"
#include "Material.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

enum ModelType{ TEMPLATE_MODEL = 0, ASSIMP_MODEL = 1, QUAD_MODEL = 2, GRID_MODEL = 3, CUBE_MODEL = 4, SPHERE_MODEL = 5, CYLINDER_MODEL = 6};

// Defines a subrange of geometry in a MeshGeometry.  This is for when multiple
// geometries are stored in one vertex and index buffer.  It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index 
// buffers so that we can implement the technique described by Figure 6.3.
struct Mesh {

	std::string materialName;

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	d3dUtil::Bound Bounds;
};


class Model {
public:

	Model();
	Model(ModelType _type);

	Model(const Model& rhs) = delete;
	Model& operator=(const Model& rhs) = delete;

	void createBuffers(ID3D12Device* device);
	void uploadBuffers(ID3D12Device* device, CommandObject* cmdObj);

	
	void InitModel(ID3D12Device* device, CommandObject* cmdObj);

	D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const;
	D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const;
	std::unordered_map<std::string, Mesh>& getDrawArgs();

	void setWorldMatrix(const DirectX::XMFLOAT4X4& mat);
	DirectX::XMFLOAT4X4& getWorldMatrix();
	float getObj2VoxelScale();
	void setObj2VoxelScale(float _scale);

	void appendAssimpMesh(const aiScene* aiscene, aiMesh* aimesh);

	const d3dUtil::Bound& getBounds() const;

protected:

	void buildGeometry();
	void buildQuadGeometry();
	void buildGeometryAssimp();

	void buildSphereGeometry();
	void buildCubeGeometry();
	void buildGridGeometry();
	void buildCylinderGeometry();




public:
	std::string Name;	
	
	ModelType modelType;
	
	// A MeshGeometry may store multiple geometries in one vertex/index buffer.
	// Use this container to define the Submesh geometries so we can draw
	// the Submeshes individually.
	std::unordered_map<std::string, Mesh> DrawArgs;

protected:

	

	UINT globalMeshID;

	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices; // specifically for large meshes whoes index size > 65535

	DirectX::XMFLOAT4X4 World = MathUtils::Identity4x4();
	DirectX::XMFLOAT4X4 texTransform = MathUtils::Identity4x4();
	float Obj2VoxelScale = 1.0f;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// System memory copies.  Use Blobs because the vertex/index format can be generic.
	// It is up to the client to cast appropriately.  
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	// buffer info
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	d3dUtil::Bound mBounds;

};

#endif // !MODEL_H


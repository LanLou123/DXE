#include "Scene.h"

Scene::Scene() {
    mObjectInfoLayer.resize((int)RenderLayer::Count);
}


Scene::Scene(ID3D12Device* _device, UINT _mClientWidth, UINT _mClientHeight): md3dDevice(_device), mClientWidth(_mClientWidth), mClientHeight(_mClientHeight) {
    mObjectInfoLayer.resize((int)RenderLayer::Count);
    cpyCommandObject = std::make_shared<CommandObject>(md3dDevice);
}

std::unordered_map<std::string, std::unique_ptr<Material>>& Scene::getMaterialMap() {
	return mMaterials;
}

std::unordered_map<std::string, std::unique_ptr<Texture>>& Scene::getTexturesMap() {
	return mTextures;
}

std::unordered_map<std::string, std::unique_ptr<Model>>& Scene::getModelsMap() {
    return mModels;
}

std::unordered_map<std::string, std::unique_ptr<Camera>>& Scene::getCamerasMap() {
    return mCameras;
}


std::unordered_map<std::string, std::unique_ptr<ObjectInfo>>& Scene::getObjectInfos() {
    return mObjectInfos;
}

const std::vector<std::vector<ObjectInfo*>>& Scene::getObjectInfoLayer() {
    return mObjectInfoLayer;
}

void Scene::populateMeshInfos() {

    UINT curObjectIndex = 0;
    for (auto& curModel : mModels) {

        auto curObjectInfo = std::make_unique<ObjectInfo>();

        for (auto& curMesh : curModel.second->getDrawArgs()) {
            auto curDrawArg = curMesh.second;

            auto curMeshInfo = std::make_unique<MeshInfo>();
            curMeshInfo->Mat = curDrawArg.materialName;
            curMeshInfo->Model = curModel.second.get();
            curMeshInfo->IndexCount = curDrawArg.IndexCount;
            curMeshInfo->StartIndexLocation = curDrawArg.StartIndexLocation;
            curMeshInfo->BaseVertexLocation = curDrawArg.BaseVertexLocation;
            DirectX::XMStoreFloat4x4(&curMeshInfo->World, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) * DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
            curObjectInfo->mMeshInfos.push_back(std::move(curMeshInfo));
        }
        curObjectInfo->Model = curModel.second.get();
        curObjectInfo->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        curObjectInfo->ObjCBIndex = curObjectIndex;
        curObjectInfo->World = curModel.second->getWorldMatrix();
        curObjectInfo->mBound = curModel.second->getBounds();
        curObjectInfo->Obj2VoxelScale = curModel.second->getObj2VoxelScale();
        if (curModel.second->modelType == ModelType::QUAD_MODEL) {
            mObjectInfoLayer[(int)RenderLayer::Debug].push_back(curObjectInfo.get());
            mObjectInfoLayer[(int)RenderLayer::Gbuffer].push_back(curObjectInfo.get());
        }
        else {
            mObjectInfoLayer[(int)RenderLayer::Default].push_back(curObjectInfo.get());
        }
        mObjectInfos[curModel.first] = (std::move(curObjectInfo));
        
        curObjectIndex++;
    }
}

void Scene::buildMaterials() {

	auto mat1 = std::make_unique<Material>();
    mat1->Name = "mat1";
    mat1->MatCBIndex = globalMatCBindex++;
    mat1->DiffuseAlbedo = DirectX::XMFLOAT4(0.2f, 0.6f, 0.6f, 1.0f);
    mat1->FresnelR0 = DirectX::XMFLOAT3(0.01f, 0.01f, 0.01f);
    mat1->Roughness = 0.125f;
    mat1->DiffuseSrvHeapIndex = 0;
    mat1->NormalSrvHeapIndex = 0;
    

    auto mat2 = std::make_unique<Material>();
    mat2->Name = "mat2";
    mat2->MatCBIndex = globalMatCBindex++;
    mat2->DiffuseAlbedo = DirectX::XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
    mat2->FresnelR0 = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);
    mat2->Roughness = 0.0;
    mat2->DiffuseSrvHeapIndex = 1;
    mat2->NormalSrvHeapIndex = 0;

    mMaterials["mat1"] = std::move(mat1);
    mMaterials["mat2"] = std::move(mat2);

}

void Scene::loadTextures() {
    auto mytex1 = std::make_unique<Texture>(L"../Resources/Textures/r1.png", 0);
    mytex1->Name = "tex1";
    mytex1->initializeTextureBuffer(md3dDevice, cpyCommandObject.get());
    globalTextureSRVDescriptorHeapIndex++;

    auto mytex2 = std::make_unique <Texture>(L"../Resources/Textures/fall.jpg", 1);
    mytex2->Name = "tex2";
    mytex2->initializeTextureBuffer(md3dDevice, cpyCommandObject.get());
    globalTextureSRVDescriptorHeapIndex++;

    mTextures[mytex1->Name] = std::move(mytex1);
    mTextures[mytex2->Name] = std::move(mytex2);
}

void Scene::buildCameras() {

    auto normalCam = std::make_unique<Camera>();
    normalCam->SetPosition(0.0f, 92.0f, -15.0f);
    normalCam->SetLens(0.25f * MathUtils::Pi, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
    mCameras["MainCam"] = std::move(normalCam);
}

void Scene::loadModels() {

    auto mymodel1 = std::make_unique<Model>(ModelType::TEMPLATE_MODEL);
    mymodel1->Name = "model1";
    mymodel1->InitModel(md3dDevice, cpyCommandObject.get());
    mymodel1->setObj2VoxelScale(200.0f);
    mModels["model1"] = std::move(mymodel1);

    auto mymodel2 = std::make_unique<Model>(ModelType::SPHERE_MODEL);
    mymodel2->Name = "area";
    mymodel2->InitModel(md3dDevice, cpyCommandObject.get());
    mymodel2->setObj2VoxelScale(200.0f);
    mModels["area"] = std::move(mymodel2);


    auto myquad = std::make_unique<Model>(ModelType::QUAD_MODEL);
    myquad->Name = "quad";
    myquad->InitModel(md3dDevice, cpyCommandObject.get());
    mModels["quad"] = std::move(myquad);
    
}

void Scene::loadAssetFromAssimp(const std::string filepath) {

    auto assimpModel = std::make_unique<Model>(ModelType::ASSIMP_MODEL);

    DirectX::XMFLOAT4X4 worldMat = MathUtils::Identity4x4();// the sponza model is stupidly big, I'm not happy bout dat
    DirectX::XMStoreFloat4x4(&worldMat, DirectX::XMMatrixScaling(0.1f, 0.1f, 0.1f) * DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f));
    assimpModel->setWorldMatrix(worldMat);
    assimpModel->setObj2VoxelScale(200.0f);

    //==============================================
    // assimp init
    //===========================================

    Assimp::Importer importer;
   // importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    const aiScene* aiscene = importer.ReadFile(filepath, 
 
         aiProcess_Triangulate 
        | aiProcess_FlipUVs 
        | aiProcess_CalcTangentSpace
    );

    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
 
        MessageBox(NULL, (L"ERROR::ASSIMP::"+ AnsiToWString(importer.GetErrorString())).c_str(), L"Error!", MB_OK);
        return;
    }

    //=============================================
    // mesh appending (CPU)
    //===========================================

    processNode(aiscene->mRootNode, aiscene, assimpModel.get());

    assimpModel->Name = filepath.substr(filepath.find_last_of('/') + 1);
    assimpModel->InitModel(md3dDevice, cpyCommandObject.get());
    mModels[assimpModel->Name] = std::move(assimpModel);

    //=============================================
    // material construct
    //===========================================

    const std::string directory = filepath.substr(0, filepath.find_last_of('/')) + '/';    // get the directory path
    for (unsigned int i = 0; i < aiscene->mNumMaterials; ++i) {
        aiMaterial* curMaterial = aiscene->mMaterials[i];
        UINT srvDiffID = 0;
        UINT srvNormID = 0;
        for (unsigned int j = 0; j < curMaterial->GetTextureCount(aiTextureType_DIFFUSE); ++j) {

            //===========================================
            // diffuse texture load
            //===========================================

            aiString str;
            curMaterial->GetTexture(aiTextureType_DIFFUSE, j, &str);
            std::string completeTexturePath = directory + str.C_Str();
            auto curtex = std::make_unique<Texture>(AnsiToWString(completeTexturePath), globalTextureSRVDescriptorHeapIndex);
            curtex->Name = completeTexturePath.substr(completeTexturePath.find_last_of('\\') + 1);
            srvDiffID = curtex->textureID;
            if (mTextures.find(curtex->Name) == mTextures.end()) {
                if (!curtex->initializeTextureBuffer(md3dDevice, cpyCommandObject.get())) {
                    continue;
                }
                globalTextureSRVDescriptorHeapIndex++;
                mTextures[curtex->Name] = std::move(curtex);
            }
            else {
                srvDiffID = mTextures[curtex->Name]->textureID;
            }

        }   
        for (unsigned int j = 0; j < curMaterial->GetTextureCount(aiTextureType_NORMALS); ++j) {
            //===========================================
            // normal texture load
            //===========================================

            aiString str;
            curMaterial->GetTexture(aiTextureType_NORMALS, j, &str);
            std::string completeTexturePath = directory + str.C_Str();
            auto curtex = std::make_unique<Texture>(AnsiToWString(completeTexturePath), globalTextureSRVDescriptorHeapIndex);
            curtex->Name = completeTexturePath.substr(completeTexturePath.find_last_of('\\') + 1);
            srvNormID = curtex->textureID;
            if (mTextures.find(curtex->Name) == mTextures.end()) {
                if (!curtex->initializeTextureBuffer(md3dDevice, cpyCommandObject.get())) {
                    continue;
                }
                globalTextureSRVDescriptorHeapIndex++;
                mTextures[curtex->Name] = std::move(curtex);
            }
            else {
                srvNormID = mTextures[curtex->Name]->textureID;
            }
        }
        //=============================================
        // continue material construct
        //===========================================

        auto curmat = std::make_unique<Material>();
        curmat->Name = curMaterial->GetName().C_Str(); 
        curmat->MatCBIndex = globalMatCBindex++; // 2 because 2 default test
        curmat->DiffuseAlbedo = DirectX::XMFLOAT4(0.2f, 0.6f, 0.6f, 1.0f);
        curmat->FresnelR0 = DirectX::XMFLOAT3(0.01f, 0.01f, 0.01f);
        curmat->Roughness = 0.125f;
        curmat->DiffuseSrvHeapIndex = srvDiffID; // use default (srvID = 0) if no texture presented
        curmat->NormalSrvHeapIndex = srvNormID;
        mMaterials[curmat->Name] = std::move(curmat);
    }
}

void Scene::processNode(aiNode* ainode, const aiScene* aiscene, Model* assimpModel) {

    for (unsigned int i = 0; i < ainode->mNumMeshes; ++i) {

        aiMesh* aimesh = aiscene->mMeshes[ainode->mMeshes[i]];

        assimpModel->appendAssimpMesh(aiscene, aimesh);

    }
    for (unsigned int i = 0; i < ainode->mNumChildren; ++i) {
        processNode(ainode->mChildren[i], aiscene, assimpModel);
    }
}

void Scene::initScene() {

   
    
    cpyCommandObject->FlushCommandQueue();
    cpyCommandObject->resetCommandList();

    buildCameras();
    loadModels();
    buildMaterials();
    loadTextures(); 
    //loadAssetFromAssimp("../Resources/Models/sponza/sponza.obj");
    //loadAssetFromAssimp("../Resources/Models/sibenik/sibenik.obj"); 
    //loadAssetFromAssimp("../Resources/Models/island/castle.obj");
    //loadAssetFromAssimp("../Resources/Models/city/city.fbx");
    //loadAssetFromAssimp("../Resources/Models/mr/mr.obj");
    loadAssetFromAssimp("../Resources/Models/ww2/source/ww2.obj");

    cpyCommandObject->endCommandRecording();
    cpyCommandObject->FlushCommandQueue();

    populateMeshInfos();

}

void Scene::resize(const int& _w, const int& _h) {
    mClientWidth = _w;
    mClientHeight = _h;
    mCameras["MainCam"]->SetLens(0.25f * MathUtils::Pi, static_cast<float>(mClientWidth) / mClientHeight, 1.0f, 1000.0f);
}
#ifndef MATERIAL_H
#define MATERIAL_H

#include"Utilities/Utils.h"


class Material {

public:
    std::string Name;
    UINT uniqueID;
    int MatCBIndex = -1; // index into constant buffer for this mat
    int DiffuseSrvHeapIndex = -1; // index  into SRV heap for diffuse texture
    int NormalSrvHeapIndex = -1; // index into SRV heap for normal texture
    int NumFramesDirty = d3dUtil::gNumFrameResources; // make sure it returns to this value after update
    bool IsEmissive = false;

    // material constants
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f ,1.0f ,1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    DirectX::XMFLOAT4X4 MatTransform = MathUtils::Identity4x4();
    float Roughness = 0.25f;
};

#endif // !MATERIAL_H

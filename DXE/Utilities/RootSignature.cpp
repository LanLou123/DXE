#include "RootSignature.h"

const ID3D12RootSignature* RootSignature::getRootSignaturePtr()const {
	return m_rootSignature.Get();
}

ID3D12RootSignature* const* RootSignature::getRootSignaturePPtr()const {
	return m_rootSignature.GetAddressOf();
}
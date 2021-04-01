#ifndef ROOT_SIGNATURE_H
#define ROOT_SIGNATURE_H

#include "Utils.h"

class RootSignature {
public:
	RootSignature();
	RootSignature(const RootSignature& rhs) = delete;
	RootSignature& operator=(const RootSignature& rhs) = delete;
	~RootSignature() = default;

	const ID3D12RootSignature* getRootSignaturePtr()const;
	ID3D12RootSignature* const* getRootSignaturePPtr()const;

	void addHeapRangesParameter(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges);
	void addHeapRangesParameter(std::vector<std::tuple<
		UINT, // base shader register
		UINT, //numdescriptors
		UINT, //register space
		D3D12_DESCRIPTOR_RANGE_TYPE,
		UINT // offsetindescriptorfromtablestart
		>> ranges);

private:
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> m_ranges;
	std::vector<D3D12_ROOT_PARAMETER> m_parameters;
	std::vector<UINT> m_rangeLocations;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
};

#endif // !ROOT_SIGNATURE_H

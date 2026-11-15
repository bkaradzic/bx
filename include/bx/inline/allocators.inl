/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_ALLOCATORS_H_HEADER_GUARD
#	error "Must be included from bx/allocators.h!"
#endif // BX_ALLOCATORS_H_HEADER_GUARD

namespace bx
{
	namespace internal
	{
		template<uint32_t MinBlockSizeT>
		struct SystemArenaStorageT
		{
			using TlsfT = TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, true>;

			SystemArenaStorageT();

			SystemMemoryAllocator                                  m_system;
			ImplementAllocatorBlkInterfaceT<SystemMemoryAllocator> m_systemI;
			TlsfT                                                  m_tlsf;
		};

		template<uint32_t MinBlockSizeT>
		inline SystemArenaStorageT<MinBlockSizeT>::SystemArenaStorageT()
			: m_systemI(m_system)
			, m_tlsf(setAllocatorTracking(&m_systemI, false) )
		{
		}

	} // namespace internal

	template<uint32_t MinBlockSizeT>
	inline SystemArenaAllocatorT<MinBlockSizeT>::SystemArenaAllocatorT(size_t _slabSize)
		: internal::SystemArenaStorageT<MinBlockSizeT>()
		, MoreCoreAllocatorT<typename internal::SystemArenaStorageT<MinBlockSizeT>::TlsfT, SystemMemoryAllocator>(this->m_tlsf, this->m_system, _slabSize)
	{
	}

	template<uint32_t MinBlockSizeT>
	inline SystemArenaAllocatorT<MinBlockSizeT>::SystemArenaAllocatorT(const Config& _config)
		: SystemArenaAllocatorT(_config.slabSize)
	{
	}

} // namespace bx

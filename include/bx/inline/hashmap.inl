/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HASHMAP_H_HEADER_GUARD
#	error "Must be included from bx/hashmap.h!"
#endif // BX_HASHMAP_H_HEADER_GUARD

#include <bx/math.h>

#if BX_COMPILER_MSVC
#	include <intrin.h>
#endif // BX_COMPILER_MSVC

namespace bx
{
	namespace internal
	{
		constexpr uint64_t hashWyRead64(const char* _data)
		{
			if (!isConstantEvaluated() )
			{
				return loadUnaligned<uint64_t>(_data);
			}

			return 0
				| uint64_t(uint8_t(_data[0]) )
				| uint64_t(uint8_t(_data[1]) ) <<  8
				| uint64_t(uint8_t(_data[2]) ) << 16
				| uint64_t(uint8_t(_data[3]) ) << 24
				| uint64_t(uint8_t(_data[4]) ) << 32
				| uint64_t(uint8_t(_data[5]) ) << 40
				| uint64_t(uint8_t(_data[6]) ) << 48
				| uint64_t(uint8_t(_data[7]) ) << 56
				;
		}

		constexpr uint64_t hashWyRead32(const char* _data)
		{
			if (!isConstantEvaluated() )
			{
				return loadUnaligned<uint32_t>(_data);
			}

			return 0
				| uint64_t(uint8_t(_data[0]) )
				| uint64_t(uint8_t(_data[1]) ) <<  8
				| uint64_t(uint8_t(_data[2]) ) << 16
				| uint64_t(uint8_t(_data[3]) ) << 24
				;
		}

		constexpr uint64_t hashWyMul(uint64_t _a, uint64_t _b, uint64_t& _hi)
		{
			if (!isConstantEvaluated() )
			{
#if BX_COMPILER_MSVC && BX_ARCH_64BIT && BX_CPU_X86
				return _umul128(_a, _b, &_hi);
#elif BX_COMPILER_MSVC && BX_ARCH_64BIT
				_hi = __umulh(_a, _b);
				return _a*_b;
#elif (BX_COMPILER_GCC || BX_COMPILER_CLANG) && BX_ARCH_64BIT
				__extension__ using uint128_t = unsigned __int128;
				const uint128_t product = uint128_t(_a)*_b;
				_hi = uint64_t(product>>64);
				return uint64_t(product);
#endif // BX_COMPILER_*
			}

			_hi = mulHi64(_a, _b);
			return _a*_b;
		}

		constexpr uint64_t hashWyMix(uint64_t _a, uint64_t _b)
		{
			uint64_t hi = 0;
			const uint64_t lo = hashWyMul(_a, _b, hi);
			return lo ^ hi;
		}

	} // namespace internal

	constexpr uint32_t hashWy(const char* _data, uint32_t _len)
	{
		constexpr uint64_t kSecret0 = UINT64_C(0x2d358dccaa6c78a5);
		constexpr uint64_t kSecret1 = UINT64_C(0x8bb84b93962eacc9);
		constexpr uint64_t kSecret2 = UINT64_C(0x4b33a62ed433d4a3);
		constexpr uint64_t kSecret3 = UINT64_C(0x4d5a2da51de1aa47);

		const char* data = _data;
		uint64_t    seed = internal::hashWyMix(kSecret0, kSecret1);
		uint64_t    aa   = 0;
		uint64_t    bb   = 0;

		if (_len <= 16)
		{
			if (_len >= 4)
			{
				const uint32_t step = (_len>>3)<<2;
				aa = (internal::hashWyRead32(data           )<<32) | internal::hashWyRead32(data            + step);
				bb = (internal::hashWyRead32(data + _len - 4)<<32) | internal::hashWyRead32(data + _len - 4 - step);
			}
			else if (_len > 0)
			{
				aa = 0
					| uint64_t(uint8_t(data[0        ]) ) << 16
					| uint64_t(uint8_t(data[_len >> 1]) ) <<  8
					| uint64_t(uint8_t(data[_len -  1]) )
					;
			}
		}
		else
		{
			uint32_t ii = _len;

			if (ii > 48)
			{
				uint64_t seed1 = seed;
				uint64_t seed2 = seed;

				while (ii > 48)
				{
					seed  = internal::hashWyMix(internal::hashWyRead64(data)      ^ kSecret1, internal::hashWyRead64(data +  8) ^ seed );
					seed1 = internal::hashWyMix(internal::hashWyRead64(data + 16) ^ kSecret2, internal::hashWyRead64(data + 24) ^ seed1);
					seed2 = internal::hashWyMix(internal::hashWyRead64(data + 32) ^ kSecret3, internal::hashWyRead64(data + 40) ^ seed2);
					data += 48;
					ii   -= 48;
				}

				seed ^= seed1 ^ seed2;
			}

			while (ii > 16)
			{
				seed  = internal::hashWyMix(internal::hashWyRead64(data) ^ kSecret1, internal::hashWyRead64(data + 8) ^ seed);
				data += 16;
				ii   -= 16;
			}

			aa = internal::hashWyRead64(data + ii - 16);
			bb = internal::hashWyRead64(data + ii - 8);
		}

		uint64_t hi = 0;
		const uint64_t lo = internal::hashWyMul(aa ^ kSecret1, bb ^ seed, hi);

		return uint32_t(internal::hashWyMix(lo ^ kSecret0 ^ _len, hi ^ kSecret1) );
	}

	namespace internal
	{
		inline uint32_t hashMix(uint32_t _x)
		{
			const simd32_t x      = simd32_splat(_x);
			const simd32_t c0     = simd32_splat(2246822519u);
			const simd32_t tmp0   = simd32_u32_mul(x, c0);
			const simd32_t tmp1   = simd32_x32_rol(tmp0, 13);
			const simd32_t c1     = simd32_splat(2654435761u);
			const simd32_t result = simd32_u32_mul(tmp1, c1);
			return result.u32;
		}

		inline uint64_t hashMix(uint64_t _x)
		{
			const simd64_t x      = simd64_splat(_x);
			const simd64_t c0     = simd64_splat(uint64_t(14029467366897019727ull) );
			const simd64_t tmp0   = simd64_u64_mul(x, c0);
			const simd64_t tmp1   = simd64_x64_rol(tmp0, 31);
			const simd64_t c1     = simd64_splat(uint64_t(11400714785074694791ull) );
			const simd64_t result = simd64_u64_mul(tmp1, c1);
			return result.u64;
		}

		template<typename Ty>
		inline uint32_t hashKeyPtr(const void* _key)
		{
			return hashKey<Ty>(*reinterpret_cast<const Ty*>(_key) );
		}

		template<typename Ty>
		inline void hashKeyMovePtr(void* _dst, void* _src, bool _swap)
		{
			Ty* dst = reinterpret_cast<Ty*>(_dst);
			Ty* src = reinterpret_cast<Ty*>(_src);

			if (_swap)
			{
				Ty tmp(bx::move(*dst) );
				*dst = bx::move(*src);
				*src = bx::move(tmp);
				return;
			}

			BX_PLACEMENT_NEW(dst, Ty)(bx::move(*src) );
			src->~Ty();
		}

		template<typename StorageT>
		struct IsBlockStorageT
		{
			static constexpr bool value = false;
		};

		template<typename Ty, uint32_t BlockCapacityT>
		struct IsBlockStorageT<BlockStorageT<Ty, BlockCapacityT>>
		{
			static constexpr bool value = true;
		};

	} // namespace internal

	template<typename Ty>
	inline uint32_t hashKey(const Ty& _value)
	{
		static_assert(isTriviallyCopyable<Ty>(), "Ty must be trivially copyable type.");

		if constexpr (4 == sizeof(Ty) )
		{
			return internal::hashMix(bitCast<uint32_t>(_value) );
		}
		else if constexpr (8 == sizeof(Ty) )
		{
			return uint32_t(internal::hashMix(bitCast<uint64_t>(_value) ) >> 32);
		}
		else
		{
			return hash<HashMurmur2A>(addressOf(_value), sizeof(_value) );
		}
	}

	template<>
	inline uint32_t hashKey(const StringView& _sv)
	{
		return hashWy(_sv.getPtr(), uint32_t(_sv.getLength() ) );
	}

	template<>
	inline uint32_t hashKey(const StringLiteral& _literal)
	{
		return hashKey<StringView>(_literal);
	}

	inline constexpr bool HashMapNoSlots::operator==(HashMapNoSlots) const
	{
		return true;
	}

	inline constexpr bool HashMapNoSlots::operator!=(HashMapNoSlots) const
	{
		return false;
	}

	inline simd64_t HashMapGroupRef::load(const uint8_t* _ptr)
	{
		return simd64_ldu(_ptr);
	}

	inline uint64_t HashMapGroupRef::match(simd64_t _group, uint8_t _hash)
	{
		const simd64_t msbs     = simd64_splat(uint8_t(0x80) );
		const simd64_t inv      = simd64_splat(uint8_t(_hash ^ 0x7f) );
		const simd64_t nctrl    = simd64_not(_group);
		const simd64_t fullMsbs = simd64_and(nctrl, msbs);
		const simd64_t fullLsbs = simd64_x64_srl(fullMsbs, 7);
		const simd64_t fullMask = simd64_u64_sub(fullMsbs, fullLsbs);
		const simd64_t fullInv  = simd64_and(fullMask, inv);
		const simd64_t diff     = simd64_xor(_group, fullInv);
		const simd64_t sum      = simd64_u64_add(fullLsbs, diff);
		const simd64_t result   = simd64_and(fullMsbs, sum);

		return result.u64;
	}

	inline uint64_t HashMapGroupRef::matchEmpty(simd64_t _group)
	{
		const simd64_t msbs   = simd64_splat(uint8_t(0x80) );
		const simd64_t nctrl  = simd64_not(_group);
		const simd64_t bit1   = simd64_x64_sll(nctrl, 6);
		const simd64_t tmp0   = simd64_and(bit1, _group);
		const simd64_t result = simd64_and(tmp0, msbs);

		return result.u64;
	}

	inline uint64_t HashMapGroupRef::matchEmptyOrDeleted(simd64_t _group)
	{
		const simd64_t msbs   = simd64_splat(uint8_t(0x80) );
		const simd64_t nctrl  = simd64_not(_group);
		const simd64_t bit0   = simd64_x64_sll(nctrl, 7);
		const simd64_t tmp0   = simd64_and(bit0, _group);
		const simd64_t result = simd64_and(tmp0, msbs);

		return result.u64;
	}

	inline uint32_t HashMapGroupRef::countLeadingEmptyOrDeleted(simd64_t _group)
	{
		const simd64_t lsbs  = simd64_splat(uint8_t(0x01) );
		const simd64_t msbs  = simd64_x64_srl(_group, 7);
		const simd64_t nmsbs = simd64_not(msbs);
		const simd64_t tmp0  = simd64_or(_group, nmsbs);
		const simd64_t mask  = simd64_and(tmp0, lsbs);

		return 0 == mask.u64
			? kWidth
			: countTrailingZeros(mask.u64)/8
			;
	}

	inline bool HashMapGroupRef::anyMatch(simd64_t _group, uint8_t _hash)
	{
		return 0 != match(_group, _hash);
	}

	inline uint32_t HashMapGroupRef::matchFirst(simd64_t _group, uint8_t _hash)
	{
		const uint64_t hits = match(_group, _hash);

		return 0 == hits
			? kWidth
			: lowestIndex(hits)
			;
	}

	inline bool HashMapGroupRef::anyEmpty(simd64_t _group)
	{
		return 0 != matchEmpty(_group);
	}

	inline uint32_t HashMapGroupRef::lowestIndex(uint64_t _mask)
	{
		return countTrailingZeros(_mask)/8;
	}

	inline uint32_t HashMapGroupRef::highestIndex(uint64_t _mask)
	{
		return kWidth - 1 - countLeadingZeros(_mask)/8;
	}

	inline void HashMapGroupRef::convertSpecialToEmptyAndFullToDeleted(uint8_t* _ptr)
	{
		const simd64_t msbs    = simd64_splat(uint8_t(0x80) );
		const simd64_t lsbs    = simd64_splat(uint8_t(0x01) );
		const simd64_t group   = simd64_ldu(_ptr);
		const simd64_t special = simd64_and(group, msbs);
		const simd64_t full    = simd64_not(special);
		const simd64_t carry   = simd64_x64_srl(special, 7);
		const simd64_t sum     = simd64_u64_add(full, carry);
		const simd64_t result  = simd64_andc(sum, lsbs);

		simd64_stu(_ptr, result);
	}

	inline simd128_t HashMapGroupSimd::load(const uint8_t* _ptr)
	{
		return simd_ldu<simd128_t>(_ptr);
	}

	inline uint32_t HashMapGroupSimd::match(simd128_t _group, uint8_t _hash)
	{
		const simd128_t hash  = simd_splat<simd128_t>(uint32_t(_hash) * 0x01010101u);
		const simd128_t equal = simd_i8_cmpeq(_group, hash);

		return uint32_t(simd_x8_signbitsmask(equal) );
	}

	inline uint32_t HashMapGroupSimd::matchEmpty(simd128_t _group)
	{
		const simd128_t bit1   = simd_x32_sll(_group, 6);
		const simd128_t tmp0   = simd_andc(_group, bit1);

		return uint32_t(simd_x8_signbitsmask(tmp0) );
	}

	inline uint32_t HashMapGroupSimd::matchEmptyOrDeleted(simd128_t _group)
	{
		const simd128_t bit0   = simd_x32_sll(_group, 7);
		const simd128_t tmp0   = simd_andc(_group, bit0);

		return uint32_t(simd_x8_signbitsmask(tmp0) );
	}

	inline uint32_t HashMapGroupSimd::countLeadingEmptyOrDeleted(simd128_t _group)
	{
		const uint32_t emptyOrDeleted = matchEmptyOrDeleted(_group);
		const uint32_t fullOrSentinel = ~emptyOrDeleted & 0xffff;

		return 0 == fullOrSentinel
			? kWidth
			: countTrailingZeros(fullOrSentinel)
			;
	}

	inline bool HashMapGroupSimd::anyMatch(simd128_t _group, uint8_t _hash)
	{
		const simd128_t hash  = simd_splat<simd128_t>(uint32_t(_hash) * 0x01010101u);
		const simd128_t equal = simd_i8_cmpeq(_group, hash);

		return simd_x8_signbits_any(equal);
	}

	inline uint32_t HashMapGroupSimd::matchFirst(simd128_t _group, uint8_t _hash)
	{
		const simd128_t hash  = simd_splat<simd128_t>(uint32_t(_hash) * 0x01010101u);
		const simd128_t equal = simd_i8_cmpeq(_group, hash);

		return uint32_t(simd_x8_signbits_first(equal) );
	}

	inline bool HashMapGroupSimd::anyEmpty(simd128_t _group)
	{
		const simd128_t bit1 = simd_x32_sll(_group, 6);
		const simd128_t tmp0 = simd_andc(_group, bit1);

		return simd_x8_signbits_any(tmp0);
	}

	inline uint32_t HashMapGroupSimd::lowestIndex(uint32_t _mask)
	{
		return countTrailingZeros(_mask);
	}

	inline uint32_t HashMapGroupSimd::highestIndex(uint32_t _mask)
	{
		return 31 - countLeadingZeros(_mask);
	}

	inline void HashMapGroupSimd::convertSpecialToEmptyAndFullToDeleted(uint8_t* _ptr)
	{
		const simd128_t msbs    = simd_splat<simd128_t>(0x80808080u);
		const simd128_t lsbs    = simd_splat<simd128_t>(0x01010101u);
		const simd128_t group   = simd_ldu<simd128_t>(_ptr);
		const simd128_t special = simd_and(group, msbs);
		const simd128_t full    = simd_not(special);
		const simd128_t carry   = simd_x32_srl(special, 7);
		const simd128_t sum     = simd_u32_add(full, carry);
		const simd128_t result  = simd_andc(sum, lsbs);

		simd_stu(_ptr, result);
	}

	inline simd256_t HashMapGroupSimd256::load(const uint8_t* _ptr)
	{
		return simd_ldu<simd256_t>(_ptr);
	}

	inline uint32_t HashMapGroupSimd256::match(simd256_t _group, uint8_t _hash)
	{
		const simd256_t hash  = simd_splat<simd256_t>(uint32_t(_hash) * 0x01010101u);
		const simd256_t equal = simd_i8_cmpeq(_group, hash);

		return uint32_t(simd_x8_signbitsmask(equal) );
	}

	inline uint32_t HashMapGroupSimd256::matchEmpty(simd256_t _group)
	{
		const simd256_t bit1   = simd_x32_sll(_group, 6);
		const simd256_t tmp0   = simd_andc(_group, bit1);

		return uint32_t(simd_x8_signbitsmask(tmp0) );
	}

	inline uint32_t HashMapGroupSimd256::matchEmptyOrDeleted(simd256_t _group)
	{
		const simd256_t bit0   = simd_x32_sll(_group, 7);
		const simd256_t tmp0   = simd_andc(_group, bit0);

		return uint32_t(simd_x8_signbitsmask(tmp0) );
	}

	inline uint32_t HashMapGroupSimd256::countLeadingEmptyOrDeleted(simd256_t _group)
	{
		const uint32_t emptyOrDeleted = matchEmptyOrDeleted(_group);
		const uint32_t fullOrSentinel = ~emptyOrDeleted;

		return 0 == fullOrSentinel
			? kWidth
			: countTrailingZeros(fullOrSentinel)
			;
	}

	inline bool HashMapGroupSimd256::anyMatch(simd256_t _group, uint8_t _hash)
	{
		const simd256_t hash  = simd_splat<simd256_t>(uint32_t(_hash) * 0x01010101u);
		const simd256_t equal = simd_i8_cmpeq(_group, hash);

		return simd_x8_signbits_any(equal);
	}

	inline uint32_t HashMapGroupSimd256::matchFirst(simd256_t _group, uint8_t _hash)
	{
		const simd256_t hash  = simd_splat<simd256_t>(uint32_t(_hash) * 0x01010101u);
		const simd256_t equal = simd_i8_cmpeq(_group, hash);

		return uint32_t(simd_x8_signbits_first(equal) );
	}

	inline bool HashMapGroupSimd256::anyEmpty(simd256_t _group)
	{
		const simd256_t bit1 = simd_x32_sll(_group, 6);
		const simd256_t tmp0 = simd_andc(_group, bit1);

		return simd_x8_signbits_any(tmp0);
	}

	inline uint32_t HashMapGroupSimd256::lowestIndex(uint32_t _mask)
	{
		return countTrailingZeros(_mask);
	}

	inline uint32_t HashMapGroupSimd256::highestIndex(uint32_t _mask)
	{
		return 31 - countLeadingZeros(_mask);
	}

	inline void HashMapGroupSimd256::convertSpecialToEmptyAndFullToDeleted(uint8_t* _ptr)
	{
		const simd256_t msbs    = simd_splat<simd256_t>(0x80808080u);
		const simd256_t lsbs    = simd_splat<simd256_t>(0x01010101u);
		const simd256_t group   = simd_ldu<simd256_t>(_ptr);
		const simd256_t special = simd_and(group, msbs);
		const simd256_t full    = simd_not(special);
		const simd256_t carry   = simd_x32_srl(special, 7);
		const simd256_t sum     = simd_u32_add(full, carry);
		const simd256_t result  = simd_andc(sum, lsbs);

		simd_stu(_ptr, result);
	}

	template<typename IndexT, typename GroupT>
	inline HashMapControlViewT<IndexT, GroupT>::HashMapControlViewT()
		: m_control(NULL)
		, m_keys(NULL)
		, m_slots(NULL)
		, m_freeSlots(NULL)
		, m_hashKeyFn(NULL)
		, m_keyMoveFn(NULL)
		, m_capacityMask(0)
		, m_count(0)
		, m_growthLeft(0)
		, m_slotCapacity(0)
		, m_numFreeSlots(0)
		, m_maxFreeSlots(0)
		, m_keySize(0)
		, m_keyAlign(1)
#if BX_CONFIG_DEBUG
		, m_generation(0)
#endif // BX_CONFIG_DEBUG
	{
	}

	template<typename IndexT, typename GroupT>
	inline constexpr uint32_t HashMapControlViewT<IndexT, GroupT>::calcCapacityMask(uint32_t _capacity)
	{
		const uint32_t capacity = _capacity < kMinCapacity ? kMinCapacity : _capacity;
		const uint32_t pow2     = 1u << ceilLog2(capacity);

		return ~0u >> countLeadingZeros(pow2);
	}

	template<typename IndexT, typename GroupT>
	inline constexpr uint32_t HashMapControlViewT<IndexT, GroupT>::calcGrownCapacityMask(uint32_t _capacityMask)
	{
		return calcCapacityMask(nextPow2(_capacityMask) );
	}

	template<typename IndexT, typename GroupT>
	inline constexpr uint32_t HashMapControlViewT<IndexT, GroupT>::calcMaxCount(uint32_t _capacityMask)
	{
		const uint32_t perGroup = _capacityMask/kWidth;
		return _capacityMask - (perGroup < 1 ? 1 : perGroup);
	}

	template<typename IndexT, typename GroupT>
	inline constexpr uint32_t HashMapControlViewT<IndexT, GroupT>::calcSize(uint32_t _keySize, uint32_t _keyAlign, uint32_t _capacityMask, uint32_t _maxFreeSlots)
	{
		const uint32_t maxElements  = _capacityMask + 1;
		const uint32_t controlSize  = _capacityMask + kWidth;
		const uint32_t keysOffset   = (controlSize + _keyAlign - 1) & ~(_keyAlign - 1);
		const uint32_t keysSize     = maxElements*_keySize;

		if constexpr (!kHasSlots)
		{
			BX_UNUSED(_maxFreeSlots);
			return keysOffset + keysSize;
		}
		else
		{
			const uint32_t slotsOffset  = (keysOffset + keysSize + uint32_t(alignof(IndexT) ) - 1) & ~(uint32_t(alignof(IndexT) ) - 1);
			const uint32_t slotsSize    = maxElements*uint32_t(sizeof(IndexT) );
			const uint32_t maxFreeSlots = _maxFreeSlots < maxElements ? maxElements : _maxFreeSlots;

			return slotsOffset + slotsSize + maxFreeSlots*uint32_t(sizeof(IndexT) );
		}
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getCapacity() const
	{
		return 0 == m_capacityMask ? 0 : m_capacityMask + 1;
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getCapacityMask() const
	{
		return m_capacityMask;
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getCount() const
	{
		return m_count;
	}

	template<typename IndexT, typename GroupT>
	inline bool HashMapControlViewT<IndexT, GroupT>::isEmpty() const
	{
		return 0 == m_count;
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getSlotCapacity() const
	{
		return m_slotCapacity;
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getSlotLimit() const
	{
		return 0 == m_capacityMask ? 0 : calcMaxCount(m_capacityMask);
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getNumFreeSlots() const
	{
		return m_numFreeSlots;
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getSizeInBytes() const
	{
		return 0 == m_capacityMask ? 0 : calcSize(m_keySize, m_keyAlign, m_capacityMask, m_maxFreeSlots);
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::getKeySize() const
	{
		return m_keySize;
	}

	template<typename IndexT, typename GroupT>
	inline const uint8_t* HashMapControlViewT<IndexT, GroupT>::getControl() const
	{
		return m_control;
	}

	template<typename IndexT, typename GroupT>
	inline const uint8_t* HashMapControlViewT<IndexT, GroupT>::getKeys() const
	{
		return m_keys;
	}

	template<typename IndexT, typename GroupT>
	inline uint8_t* HashMapControlViewT<IndexT, GroupT>::getKeys()
	{
		return m_keys;
	}

	template<typename IndexT, typename GroupT>
	inline const IndexT* HashMapControlViewT<IndexT, GroupT>::getSlots() const
	{
		return m_slots;
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::findFirstFull(uint32_t _idx) const
	{
		for (;;)
		{
			const typename GroupT::Type group = GroupT::load(m_control + _idx);
			const uint32_t num = GroupT::countLeadingEmptyOrDeleted(group);

			_idx += num;

			if (num != kWidth)
			{
				return _idx;
			}
		}
	}

	template<typename IndexT, typename GroupT>
	inline bool HashMapControlViewT<IndexT, GroupT>::isSentinel(uint32_t _idx) const
	{
		return kSentinel == m_control[_idx];
	}

	template<typename IndexT, typename GroupT>
	inline uint32_t HashMapControlViewT<IndexT, GroupT>::findFirstEmpty(uint32_t _hash) const
	{
		MatchIterator it(m_capacityMask, _hash);

		for (;;)
		{
			const typename GroupT::Type     group = GroupT::load(m_control + it.m_offset);
			const typename GroupT::MaskType hits  = GroupT::matchEmptyOrDeleted(group);

			if (0 != hits)
			{
				return (GroupT::lowestIndex(hits) + it.m_offset) & m_capacityMask;
			}

			it.next();
		}

		BX_ASSUME(false);
	}

	template<typename IndexT, typename GroupT>
	inline bool HashMapControlViewT<IndexT, GroupT>::tryAdd(uint32_t _hash, uint32_t& _idx, IndexT& _slot)
	{
		uint32_t idx = findFirstEmpty(_hash);

		if (0 == m_growthLeft
		&&  m_control[idx] != kDeleted)
		{
			if (m_capacityMask > kWidth
			&&  m_count * 32 <= m_capacityMask * 25)
			{
				dropDeletesWithoutResize();
				idx = findFirstEmpty(_hash);
			}
			else
			{
				return false;
			}
		}

		++m_count;
		m_growthLeft -= m_control[idx] == kEmpty;

		setControl(idx, uint8_t(_hash) & kNotEmptyMask);

		if constexpr (kHasSlots)
		{
			const IndexT slot = allocSlot();
			m_slots[idx] = slot;
			_slot = slot;
		}
		else
		{
			BX_UNUSED(_slot);
		}

		_idx = idx;

		return true;
	}

	template<typename IndexT, typename GroupT>
	inline IndexT HashMapControlViewT<IndexT, GroupT>::removeAt(uint32_t _idx)
	{
		const uint32_t prevIdx = (_idx - kWidth) & m_capacityMask;

		const typename GroupT::Type     groupBefore = GroupT::load(m_control + prevIdx);
		const typename GroupT::MaskType emptyBefore = GroupT::matchEmpty(groupBefore);

		const typename GroupT::Type     groupAfter  = GroupT::load(m_control + _idx);
		const typename GroupT::MaskType emptyAfter  = GroupT::matchEmpty(groupAfter);

		const bool empty = true
			&& 0 != emptyBefore
			&& 0 != emptyAfter
			&& kWidth > (kWidth - 1 - GroupT::highestIndex(emptyBefore) + GroupT::lowestIndex(emptyAfter) )
			;

		setControl(_idx, empty ? kEmpty : kDeleted);

		m_growthLeft += empty;
		--m_count;

		if constexpr (kHasSlots)
		{
			const IndexT slot = m_slots[_idx];
			BX_ASSERT(m_numFreeSlots < m_maxFreeSlots, "Free slot list overflow (%u)!", m_numFreeSlots);
			m_freeSlots[m_numFreeSlots++] = slot;

			return slot;
		}
		else
		{
			return kInvalidSlot;
		}
	}

	template<typename IndexT, typename GroupT>
	inline void HashMapControlViewT<IndexT, GroupT>::clear()
	{
		m_count        = 0;
		m_slotCapacity = 0;
		m_numFreeSlots = 0;

		if (0 == m_capacityMask)
		{
			m_growthLeft = 0;
			return;
		}

		m_growthLeft = calcMaxCount(m_capacityMask);

		memSet(m_control, kEmpty, m_capacityMask + kWidth);
		m_control[m_capacityMask] = kSentinel;
	}

	template<typename IndexT, typename GroupT>
	inline HashMapControlViewT<IndexT, GroupT>::MatchIterator::MatchIterator(uint32_t _capacityMask, uint32_t _hash)
		: m_index(0)
		, m_offset( (_hash >> 7) & _capacityMask)
		, m_capacityMask(_capacityMask)
	{
	}

	template<typename IndexT, typename GroupT>
	inline void HashMapControlViewT<IndexT, GroupT>::MatchIterator::next()
	{
		m_index  += kWidth;
		m_offset += m_index;
		m_offset &= m_capacityMask;
	}

	template<typename IndexT, typename GroupT>
	inline void HashMapControlViewT<IndexT, GroupT>::setControl(uint32_t _idx, uint8_t _control)
	{
		m_control[_idx] = _control;
		m_control[( (_idx - (kWidth-1) ) & m_capacityMask) + ( (kWidth-1) & m_capacityMask)] = _control;
	}

	template<typename IndexT, typename GroupT>
	inline IndexT HashMapControlViewT<IndexT, GroupT>::allocSlot()
	{
		if constexpr (kHasSlots)
		{
			if (0 != m_numFreeSlots)
			{
				return m_freeSlots[--m_numFreeSlots];
			}

			BX_ASSERT(m_slotCapacity < uint32_t(kInvalidSlot), "Out of slots (%u)!", m_slotCapacity);

			return IndexT(m_slotCapacity++);
		}
		else
		{
			return kInvalidSlot;
		}
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::Iterator(const HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>& _control, uint32_t _idx)
		: m_control(_control)
		, m_idx(min(_idx, _control.getCapacityMask() ) )
#if BX_CONFIG_DEBUG
		, m_generation(_control.m_generation)
#endif // BX_CONFIG_DEBUG
	{
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline const typename HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator& HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::operator*() const
	{
		return *this;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::operator!=(const Iterator& _rhs) const
	{
		return m_idx != _rhs.m_idx;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline const typename HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator& HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::operator++()
	{
		next();
		return *this;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::next()
	{
#if BX_CONFIG_DEBUG
		BX_ASSERT(m_generation == m_control.m_generation, "Table rehashed during iteration, elements were added or capacity changed!");
#endif // BX_CONFIG_DEBUG

		if (m_control.isSentinel(m_idx) )
		{
			return false;
		}

		m_idx = m_control.findFirstFull(m_idx + 1);

		return !m_control.isSentinel(m_idx);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline const KeyT& HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::getKey() const
	{
		return m_control.getKey(m_idx);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline IndexT HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::getSlot() const
	{
		return m_control.getSlots()[m_idx];
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline uint32_t HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator::getPosition() const
	{
		return m_idx;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::HashMapControlBaseT()
	{
		this->init(NULL, sizeof(KeyT), alignof(KeyT), 0, 0, internal::hashKeyPtr<KeyT>, kKeyMoveFn);
		debugPtrUpdate();
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::~HashMapControlBaseT()
	{
		destructKeys();
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::destructKeys()
	{
		if constexpr (!isTriviallyDestructible<KeyT>() )
		{
			for (const auto it : *this)
			{
				destructKey(it.getPosition() );
			}
		}
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::destructKey(uint32_t _idx)
	{
		if constexpr (!isTriviallyDestructible<KeyT>() )
		{
			reinterpret_cast<KeyT*>(this->m_keys)[_idx].~KeyT();
		}

		BX_UNUSED(_idx);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::clear()
	{
		destructKeys();
		ViewType::clear();
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::setCapacity(uint32_t _capacity)
	{
		const uint32_t capacity = max<uint32_t>(_capacity, this->m_count, MinCapacityT);
		const uint32_t capacityMask = ViewType::calcCapacityMask(capacity);

		if (0 != this->m_count)
		{
			resize(capacityMask);
			return;
		}

		if constexpr (IsDynamicStorageT<StorageT>::value)
		{
			if (0 == capacity)
			{
				m_storage.setCapacity(0, 0);
				this->init(NULL, sizeof(KeyT), alignof(KeyT), 0, 0, internal::hashKeyPtr<KeyT>, kKeyMoveFn);
				debugPtrUpdate();
				return;
			}
		}

		const uint32_t size = ViewType::calcSize(sizeof(KeyT), alignof(KeyT), capacityMask, 0);

		if constexpr (IsDynamicStorageT<StorageT>::value)
		{
			m_storage.setCapacity(size, 0);
		}

		const uint32_t sizeInBytes = m_storage.getCapacity()*uint32_t(sizeof(*m_storage.getPtr() ) );
		BX_ASSERT(size <= sizeInBytes, "Storage too small for capacity %u (%u bytes, have %u)!", capacity, size, sizeInBytes);
		BX_UNUSED(sizeInBytes);

		this->init(m_storage.getPtr(), sizeof(KeyT), alignof(KeyT), capacityMask, 0, internal::hashKeyPtr<KeyT>, kKeyMoveFn);
		debugPtrUpdate();
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline typename HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::begin() const
	{
		if (0 == this->m_count)
		{
			return end();
		}

		return Iterator(*this, this->findFirstFull(0) );
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline typename HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::Iterator HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::end() const
	{
		return Iterator(*this);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline const KeyT& HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::getKey(uint32_t _idx) const
	{
		return reinterpret_cast<const KeyT*>(this->m_keys)[_idx];
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::contains(const KeyT& _key) const
	{
		return kInvalid != findIndex(_key, hashKey(_key) );
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline IndexT HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::findSlot(const KeyT& _key) const
	{
		const uint32_t idx = findIndex(_key, hashKey(_key) );

		return kInvalid != idx
			? this->m_slots[idx]
			: kInvalidSlot
			;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::tryAdd(const KeyT& _key, IndexT& _slot)
	{
		const uint32_t hash = hashKey(_key);
		const uint32_t idx  = findIndex(_key, hash);

		if (kInvalid != idx)
		{
			if constexpr (ViewType::kHasSlots)
			{
				_slot = this->m_slots[idx];
			}

			return false;
		}

		if constexpr (!IsDynamicStorageT<StorageT>::value)
		{
			if (MinCapacityT <= this->m_count)
			{
				_slot = kInvalidSlot;
				return false;
			}
		}

		uint32_t addIdx = kInvalid;

		if (!ViewType::tryAdd(hash, addIdx, _slot) )
		{
			if (!resize(ViewType::calcGrownCapacityMask(this->m_capacityMask) ) )
			{
				_slot = kInvalidSlot;
				return false;
			}

			const bool added = ViewType::tryAdd(hash, addIdx, _slot);
			BX_ASSERT(added, "Add failed after growing table!");
			BX_UNUSED(added);
		}

		BX_PLACEMENT_NEW(reinterpret_cast<KeyT*>(this->m_keys) + addIdx, KeyT)(_key);

		return true;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::tryAdd(const KeyT& _key)
	{
		IndexT slot = kInvalidSlot;
		return tryAdd(_key, slot);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline IndexT HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::add(const KeyT& _key)
	{
		IndexT slot = kInvalidSlot;
		const bool result = tryAdd(_key, slot);
		BX_ASSERT(result, "Add failed, key already exist or table is full!");
		BX_UNUSED(result);
		return slot;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::tryRemove(const KeyT& _key, IndexT& _slot)
	{
		const uint32_t idx = findIndex(_key, hashKey(_key) );

		if (kInvalid != idx)
		{
			destructKey(idx);
			_slot = this->removeAt(idx);
			return true;
		}

		return false;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::tryRemove(const KeyT& _key)
	{
		IndexT slot = kInvalidSlot;
		return tryRemove(_key, slot);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline IndexT HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::remove(const KeyT& _key)
	{
		IndexT slot = kInvalidSlot;
		const bool result = tryRemove(_key, slot);
		BX_ASSERT(result, "Remove failed, key doesn't exist!");
		BX_UNUSED(result);
		return slot;
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline IndexT HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::remove(const Iterator& _it)
	{
		destructKey(_it.getPosition() );
		return this->removeAt(_it.getPosition() );
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::trim()
	{
		setCapacity(0);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::set(HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>&& _src)
	{
		static_assert(IsDynamicStorageT<StorageT>::value, "Only tables with dynamic storage can be moved.");

		if (this != &_src)
		{
			destructKeys();
			m_storage = bx::move(_src.m_storage);
			static_cast<ViewType&>(*this) = static_cast<const ViewType&>(_src);
			_src.init(NULL, sizeof(KeyT), alignof(KeyT), 0, 0, internal::hashKeyPtr<KeyT>, kKeyMoveFn);
			debugPtrUpdate();
			_src.debugPtrUpdate();
		}
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline uint32_t HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::findIndex(const KeyT& _key, uint32_t _hash) const
	{
		const uint32_t capacityMask = this->m_capacityMask;
		const uint8_t* control      = this->m_control;
		const KeyT*    keys         = reinterpret_cast<const KeyT*>(this->m_keys);
		const uint8_t  hash2        = uint8_t(_hash) & ViewType::kNotEmptyMask;

		typename ViewType::MatchIterator it(capacityMask, _hash);

		for (;;)
		{
			const typename GroupT::Type group = GroupT::load(control + it.m_offset);

			if (GroupT::anyMatch(group, hash2) )
			{
				const uint32_t first = GroupT::matchFirst(group, hash2);
				const uint32_t idx   = (first + it.m_offset) & capacityMask;

				if (_key == keys[idx])
				{
					return idx;
				}

				typename GroupT::MaskType hits = GroupT::match(group, hash2);

				for (hits &= hits - 1; 0 != hits; hits &= hits - 1)
				{
					const uint32_t next = (GroupT::lowestIndex(hits) + it.m_offset) & capacityMask;

					if (_key == keys[next])
					{
						return next;
					}
				}
			}

			if (GroupT::anyEmpty(group) )
			{
				return kInvalid;
			}

			it.next();
		}

		BX_ASSUME(false);
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline bool HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::resize(uint32_t _capacityMask)
	{
		if constexpr (IsDynamicStorageT<StorageT>::value)
		{
			const uint32_t size = ViewType::calcSize(sizeof(KeyT), alignof(KeyT), _capacityMask, this->m_numFreeSlots);

			StorageT storage;
			storage.setAllocatorHandle(m_storage.getAllocatorHandle() );
			storage.setCapacity(size, 0);

			this->rehashInto(storage.getPtr(), _capacityMask);

			m_storage = bx::move(storage);
			debugPtrUpdate();

			return true;
		}
		else
		{
			BX_UNUSED(_capacityMask);
			return false;
		}
	}

	template<typename KeyT, typename StorageT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>::debugPtrUpdate()
	{
#if BX_CONFIG_DEBUG
		m_debugKeys = reinterpret_cast<KeyT*>(this->m_keys);
#endif // BX_CONFIG_DEBUG
	}

	template<typename KeyT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>::HashMapControlT(HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>&& _src)
		: HashMapControlT(_src.getAllocatorHandle(), 0)
	{
		set(bx::move(_src) );
	}

	template<typename KeyT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>::HashMapControlT()
	{
		this->setCapacity(0);
	}

	template<typename KeyT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>::HashMapControlT(AllocatorHandle _allocator)
		: HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>(_allocator, 0)
	{
	}

	template<typename KeyT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>::HashMapControlT(AllocatorHandle _allocator, uint32_t _capacity)
	{
		this->m_storage.setAllocatorHandle(_allocator);
		this->setCapacity(_capacity);
	}

	template<typename KeyT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>& HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>::operator=(HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>&& _src)
	{
		set(bx::move(_src) );
		return *this;
	}

	template<typename KeyT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>::set(HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>&& _src)
	{
		HashMapControlBaseT<KeyT, DynamicStorageT<uint8_t>, IndexT, MinCapacityT, GroupT>::set(bx::move(_src) );
	}

	template<typename KeyT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline AllocatorHandle HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>::getAllocatorHandle() const
	{
		return this->m_storage.getAllocatorHandle();
	}

	template<typename KeyT, uint32_t CapacityT, typename IndexT, typename GroupT>
	inline FixedHashMapControlT<KeyT, CapacityT, IndexT, GroupT>::FixedHashMapControlT()
	{
		this->setCapacity(CapacityT);
	}

	template<typename KeyT, uint32_t CapacityT, typename GroupT>
	inline FixedHashSetT<KeyT, CapacityT, GroupT>::FixedHashSetT()
	{
	}

	template<typename KeyT, typename ControlT>
	template<typename... ArgsT>
	inline HashSetBaseT<KeyT, ControlT>::HashSetBaseT(ArgsT&&... _args)
		: m_control(bx::forward<ArgsT>(_args)...)
	{
	}

	template<typename KeyT, typename ControlT>
	inline HashSetBaseT<KeyT, ControlT>::~HashSetBaseT()
	{
	}

	template<typename KeyT, typename ControlT>
	inline void HashSetBaseT<KeyT, ControlT>::setCapacity(uint32_t _capacity)
	{
		m_control.setCapacity(_capacity);
	}

	template<typename KeyT, typename ControlT>
	inline typename HashSetBaseT<KeyT, ControlT>::Iterator HashSetBaseT<KeyT, ControlT>::begin() const
	{
		return m_control.begin();
	}

	template<typename KeyT, typename ControlT>
	inline typename HashSetBaseT<KeyT, ControlT>::Iterator HashSetBaseT<KeyT, ControlT>::end() const
	{
		return m_control.end();
	}

	template<typename KeyT, typename ControlT>
	inline uint32_t HashSetBaseT<KeyT, ControlT>::getCapacity() const
	{
		return m_control.getCapacity();
	}

	template<typename KeyT, typename ControlT>
	inline uint32_t HashSetBaseT<KeyT, ControlT>::getCount() const
	{
		return m_control.getCount();
	}

	template<typename KeyT, typename ControlT>
	inline bool HashSetBaseT<KeyT, ControlT>::isEmpty() const
	{
		return m_control.isEmpty();
	}

	template<typename KeyT, typename ControlT>
	inline const ControlT& HashSetBaseT<KeyT, ControlT>::getControl() const
	{
		return m_control;
	}

	template<typename KeyT, typename ControlT>
	inline bool HashSetBaseT<KeyT, ControlT>::contains(const KeyT& _key) const
	{
		return m_control.contains(_key);
	}

	template<typename KeyT, typename ControlT>
	inline bool HashSetBaseT<KeyT, ControlT>::tryAdd(const KeyT& _key)
	{
		return m_control.tryAdd(_key);
	}

	template<typename KeyT, typename ControlT>
	inline void HashSetBaseT<KeyT, ControlT>::add(const KeyT& _key)
	{
		const bool result = tryAdd(_key);
		BX_ASSERT(result, "Add failed, key already exist or set is full!");
		BX_UNUSED(result);
	}

	template<typename KeyT, typename ControlT>
	inline bool HashSetBaseT<KeyT, ControlT>::tryRemove(const KeyT& _key)
	{
		return m_control.tryRemove(_key);
	}

	template<typename KeyT, typename ControlT>
	inline void HashSetBaseT<KeyT, ControlT>::remove(const KeyT& _key)
	{
		const bool result = tryRemove(_key);
		BX_ASSERT(result, "Remove failed, key doesn't exist!");
		BX_UNUSED(result);
	}

	template<typename KeyT, typename ControlT>
	inline void HashSetBaseT<KeyT, ControlT>::remove(const Iterator& _it)
	{
		m_control.remove(_it);
	}

	template<typename KeyT, typename ControlT>
	inline void HashSetBaseT<KeyT, ControlT>::clear()
	{
		m_control.clear();
	}

	template<typename KeyT, typename ControlT>
	inline void HashSetBaseT<KeyT, ControlT>::set(HashSetBaseT<KeyT, ControlT>&& _src)
	{
		if (this != &_src)
		{
			m_control.set(bx::move(_src.m_control) );
		}
	}

	template<typename KeyT, typename ControlT>
	inline void HashSetBaseT<KeyT, ControlT>::trim()
	{
		m_control.trim();
	}

	template<typename KeyT, uint32_t MinCapacityT, typename GroupT>
	inline HashSetT<KeyT, MinCapacityT, GroupT>::HashSetT(HashSetT<KeyT, MinCapacityT, GroupT>&& _src)
		: HashSetT(_src.getAllocatorHandle(), 0)
	{
		set(bx::move(_src) );
	}

	template<typename KeyT, uint32_t MinCapacityT, typename GroupT>
	inline HashSetT<KeyT, MinCapacityT, GroupT>::HashSetT()
		: HashSetBaseT<KeyT, HashMapControlT<KeyT, HashMapNoSlots, MinCapacityT, GroupT>>()
	{
	}

	template<typename KeyT, uint32_t MinCapacityT, typename GroupT>
	inline HashSetT<KeyT, MinCapacityT, GroupT>::HashSetT(AllocatorHandle _allocator)
		: HashSetT<KeyT, MinCapacityT, GroupT>(_allocator, 0)
	{
	}

	template<typename KeyT, uint32_t MinCapacityT, typename GroupT>
	inline HashSetT<KeyT, MinCapacityT, GroupT>::HashSetT(AllocatorHandle _allocator, uint32_t _capacity)
		: HashSetBaseT<KeyT, HashMapControlT<KeyT, HashMapNoSlots, MinCapacityT, GroupT>>(_allocator, _capacity)
	{
	}

	template<typename KeyT, uint32_t MinCapacityT, typename GroupT>
	inline HashSetT<KeyT, MinCapacityT, GroupT>& HashSetT<KeyT, MinCapacityT, GroupT>::operator=(HashSetT<KeyT, MinCapacityT, GroupT>&& _src)
	{
		set(bx::move(_src) );
		return *this;
	}

	template<typename KeyT, uint32_t MinCapacityT, typename GroupT>
	inline void HashSetT<KeyT, MinCapacityT, GroupT>::set(HashSetT<KeyT, MinCapacityT, GroupT>&& _src)
	{
		HashSetBaseT<KeyT, HashMapControlT<KeyT, HashMapNoSlots, MinCapacityT, GroupT>>::set(bx::move(_src) );
	}

	template<typename KeyT, uint32_t MinCapacityT, typename GroupT>
	inline AllocatorHandle HashSetT<KeyT, MinCapacityT, GroupT>::getAllocatorHandle() const
	{
		return this->m_control.getAllocatorHandle();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::IteratorT(MapT& _hashMap, const typename ControlT::Iterator& _it)
		: m_hashMap(_hashMap)
		, m_it(_it)
	{
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline const typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::template IteratorT<MapT, Vy>& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::operator*() const
	{
		return *this;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::operator!=(const IteratorT<MapT, Vy>& _rhs) const
	{
		return m_it != _rhs.m_it;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline const typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::template IteratorT<MapT, Vy>& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::operator++()
	{
		++m_it;
		return *this;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::next()
	{
		return m_it.next();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline const KeyT& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::getKey() const
	{
		return m_it.getKey();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline Vy& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::getValue() const
	{
		return *m_hashMap.m_values.getPtr(m_it.getSlot() );
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename MapT, typename Vy>
	inline typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IndexType HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IteratorT<MapT, Vy>::getSlot() const
	{
		return m_it.getSlot();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename... ArgsT>
	inline HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::HashMapBaseT(ArgsT&&... _args)
		: m_control(bx::forward<ArgsT>(_args)...)
	{
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::~HashMapBaseT()
	{
		destructValues();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::setCapacity(uint32_t _capacity)
	{
		m_control.setCapacity(_capacity);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::Iterator HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::begin()
	{
		return Iterator(*this, m_control.begin() );
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::Iterator HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::end()
	{
		return Iterator(*this, m_control.end() );
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::ConstIterator HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::begin() const
	{
		return ConstIterator(*this, m_control.begin() );
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::ConstIterator HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::end() const
	{
		return ConstIterator(*this, m_control.end() );
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline uint32_t HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::getCapacity() const
	{
		return m_control.getCapacity();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline uint32_t HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::getCount() const
	{
		return m_control.getCount();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::isEmpty() const
	{
		return m_control.isEmpty();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline const ControlT& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::getControl() const
	{
		return m_control;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline ValueT& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::getValue(IndexType _slot)
	{
		return *m_values.getPtr(_slot);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline const ValueT& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::getValue(IndexType _slot) const
	{
		return *m_values.getPtr(_slot);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline typename HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::IndexType HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::findSlot(const KeyT& _key) const
	{
		return m_control.findSlot(_key);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline ValueT* HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::find(const KeyT& _key)
	{
		const IndexType slot = m_control.findSlot(_key);

		return kInvalidSlot != slot
			? m_values.getPtr(slot)
			: NULL
			;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline const ValueT* HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::find(const KeyT& _key) const
	{
		const IndexType slot = m_control.findSlot(_key);

		return kInvalidSlot != slot
			? m_values.getPtr(slot)
			: NULL
			;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline ValueT& HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::getOrAdd(const KeyT& _key)
	{
		IndexType slot = kInvalidSlot;

		if (m_control.tryAdd(_key, slot) )
		{
			BX_PLACEMENT_NEW(getValuePtrForSlot(slot), ValueT)();
		}

		BX_ASSERT(kInvalidSlot != slot, "Map is full!");

		return *m_values.getPtr(slot);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::tryAdd(const KeyT& _key, const ValueT& _value, bool _update)
	{
		if (isAliased(addressOf(_value) ) )
		{
			const ValueT value(_value);
			return tryAdd(_key, value, _update);
		}

		IndexType slot = kInvalidSlot;

		if (!m_control.tryAdd(_key, slot) )
		{
			if (_update
			&&  kInvalidSlot != slot)
			{
				*m_values.getPtr(slot) = _value;
				return true;
			}

			return false;
		}

		BX_PLACEMENT_NEW(getValuePtrForSlot(slot), ValueT)(_value);

		return true;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::tryAdd(const KeyT& _key, ValueT&& _value, bool _update)
	{
		if (isAliased(addressOf(_value) ) )
		{
			ValueT value(bx::move(_value) );
			return tryAdd(_key, bx::move(value), _update);
		}

		IndexType slot = kInvalidSlot;

		if (!m_control.tryAdd(_key, slot) )
		{
			if (_update
			&&  kInvalidSlot != slot)
			{
				*m_values.getPtr(slot) = bx::move(_value);
				return true;
			}

			return false;
		}

		BX_PLACEMENT_NEW(getValuePtrForSlot(slot), ValueT)(bx::move(_value) );

		return true;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::add(const KeyT& _key, const ValueT& _value, bool _update)
	{
		const bool result = tryAdd(_key, _value, _update);
		BX_ASSERT(result, "Add failed, key already exist or map is full!");
		BX_UNUSED(result);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::add(const KeyT& _key, ValueT&& _value, bool _update)
	{
		const bool result = tryAdd(_key, bx::move(_value), _update);
		BX_ASSERT(result, "Add failed, key already exist or map is full!");
		BX_UNUSED(result);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename... ArgsT>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::tryAdd(const KeyT& _key, EmplaceTag, ArgsT&&... _args)
	{
		IndexType slot = kInvalidSlot;

		if (!m_control.tryAdd(_key, slot) )
		{
			return false;
		}

		BX_PLACEMENT_NEW(getValuePtrForSlot(slot), ValueT)(bx::forward<ArgsT>(_args)...);

		return true;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	template<typename... ArgsT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::add(const KeyT& _key, EmplaceTag, ArgsT&&... _args)
	{
		const bool result = tryAdd(_key, Emplace, bx::forward<ArgsT>(_args)...);
		BX_ASSERT(result, "Add failed, key already exist or map is full!");
		BX_UNUSED(result);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::tryRemove(const KeyT& _key)
	{
		IndexType slot = kInvalidSlot;

		if (m_control.tryRemove(_key, slot) )
		{
			m_values.getPtr(slot)->~ValueT();
			return true;
		}

		return false;
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::remove(const KeyT& _key)
	{
		const bool result = tryRemove(_key);
		BX_ASSERT(result, "Remove failed, key doesn't exist!");
		BX_UNUSED(result);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::remove(const Iterator& _it)
	{
		const IndexType slot = m_control.remove(_it.m_it);
		m_values.getPtr(slot)->~ValueT();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::clear()
	{
		destructValues();
		m_control.clear();
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::set(HashMapBaseT<KeyT, ValueT, ControlT, StorageT>&& _src)
	{
		if (this != &_src)
		{
			destructValues();
			m_control.set(bx::move(_src.m_control) );
			m_values = bx::move(_src.m_values);
		}
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::trim()
	{
		m_control.trim();
		relocateValues(m_control.getSlotCapacity(), kInvalidSlot);
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline ValueT* HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::getValuePtrForSlot(IndexType _slot)
	{
		if constexpr (internal::IsBlockStorageT<StorageT>::value)
		{
			return m_values.acquirePtr(_slot);
		}
		else
		{
			if (_slot >= m_values.getCapacity() )
			{
				const uint32_t grown = max<uint32_t>(m_values.getCapacity()*2, 16, uint32_t(_slot) + 1);
				relocateValues(max(min(grown, m_control.getSlotLimit() ), uint32_t(_slot) + 1), _slot);
			}

			return m_values.getPtr(_slot);
		}
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline bool HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::isAliased(const ValueT* _ptr) const
	{
		if constexpr (IsDynamicStorageT<StorageT>::value)
		{
			const ValueT* ptr = m_values.getPtr();
			return NULL != ptr
				&& _ptr >= ptr
				&& _ptr <  ptr + m_values.getCapacity()
				;
		}
		else
		{
			BX_UNUSED(_ptr);
			return false;
		}
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::relocateValues(uint32_t _capacity, IndexType _skipSlot)
	{
		if constexpr (internal::IsBlockStorageT<StorageT>::value)
		{
			BX_UNUSED(_skipSlot);
			m_values.setCapacity(_capacity);
		}
		else if constexpr (!IsDynamicStorageT<StorageT>::value)
		{
			BX_UNUSED(_skipSlot);
			BX_ASSERT(_capacity <= m_values.getCapacity(), "Value storage too small for %u slots (have %u)!", _capacity, m_values.getCapacity() );
		}
		else if constexpr (isTriviallyMoveConstructible<ValueT>() || isTriviallyRelocatable<ValueT>())
		{
			BX_UNUSED(_skipSlot);
			m_values.setCapacity(_capacity, 0);
		}
		else
		{
			StorageT values;
			values.setAllocatorHandle(m_values.getAllocatorHandle() );
			values.setCapacity(_capacity, 0);

			for (const auto it : m_control)
			{
				const IndexType slot = it.getSlot();

				if (_skipSlot == slot)
				{
					continue;
				}

				ValueT* src = m_values.getPtr(slot);
				BX_PLACEMENT_NEW(values.getPtr(slot), ValueT)(bx::move(*src) );
				src->~ValueT();
			}

			m_values = bx::move(values);
		}
	}

	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	inline void HashMapBaseT<KeyT, ValueT, ControlT, StorageT>::destructValues()
	{
		if constexpr (!isTriviallyDestructible<ValueT>() )
		{
			for (const auto it : m_control)
			{
				m_values.getPtr(it.getSlot() )->~ValueT();
			}
		}
	}

	template<typename KeyT, typename ValueT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>::HashMapT(HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>&& _src)
		: HashMapT(_src.getAllocatorHandle(), 0)
	{
		set(bx::move(_src) );
	}

	template<typename KeyT, typename ValueT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>::HashMapT()
		: HashMapBaseT<KeyT, ValueT, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>, DynamicStorageT<ValueT>>()
	{
	}

	template<typename KeyT, typename ValueT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>::HashMapT(AllocatorHandle _allocator)
		: HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>(_allocator, 0)
	{
	}

	template<typename KeyT, typename ValueT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>::HashMapT(AllocatorHandle _allocator, uint32_t _capacity)
		: HashMapBaseT<KeyT, ValueT, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>, DynamicStorageT<ValueT>>(_allocator, _capacity)
	{
		this->m_values.setAllocatorHandle(_allocator);
	}

	template<typename KeyT, typename ValueT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>& HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>::operator=(HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>&& _src)
	{
		set(bx::move(_src) );
		return *this;
	}

	template<typename KeyT, typename ValueT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>::set(HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>&& _src)
	{
		HashMapBaseT<KeyT, ValueT, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>, DynamicStorageT<ValueT>>::set(bx::move(_src) );
	}

	template<typename KeyT, typename ValueT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline AllocatorHandle HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>::getAllocatorHandle() const
	{
		return this->m_control.getAllocatorHandle();
	}

	template<typename KeyT, typename ValueT, uint32_t BlockCapacityT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>::BlockHashMapT(BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>&& _src)
		: BlockHashMapT(_src.getAllocatorHandle(), 0)
	{
		set(bx::move(_src) );
	}

	template<typename KeyT, typename ValueT, uint32_t BlockCapacityT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>::BlockHashMapT()
		: HashMapBaseT<KeyT, ValueT, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>, BlockStorageT<ValueT, BlockCapacityT>>()
	{
	}

	template<typename KeyT, typename ValueT, uint32_t BlockCapacityT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>::BlockHashMapT(AllocatorHandle _allocator)
		: BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>(_allocator, 0)
	{
	}

	template<typename KeyT, typename ValueT, uint32_t BlockCapacityT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>::BlockHashMapT(AllocatorHandle _allocator, uint32_t _capacity)
		: HashMapBaseT<KeyT, ValueT, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>, BlockStorageT<ValueT, BlockCapacityT>>(_allocator, _capacity)
	{
		this->m_values.setAllocatorHandle(_allocator);
	}

	template<typename KeyT, typename ValueT, uint32_t BlockCapacityT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>& BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>::operator=(BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>&& _src)
	{
		set(bx::move(_src) );
		return *this;
	}

	template<typename KeyT, typename ValueT, uint32_t BlockCapacityT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline void BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>::set(BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>&& _src)
	{
		HashMapBaseT<KeyT, ValueT, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>, BlockStorageT<ValueT, BlockCapacityT>>::set(bx::move(_src) );
	}

	template<typename KeyT, typename ValueT, uint32_t BlockCapacityT, typename IndexT, uint32_t MinCapacityT, typename GroupT>
	inline AllocatorHandle BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>::getAllocatorHandle() const
	{
		return this->m_control.getAllocatorHandle();
	}

	template<typename KeyT, typename ValueT, uint32_t CapacityT, typename IndexT, typename GroupT>
	inline FixedHashMapT<KeyT, ValueT, CapacityT, IndexT, GroupT>::FixedHashMapT()
	{
#if BX_CONFIG_DEBUG
		m_debugValues = this->m_values.getPtr();
#endif // BX_CONFIG_DEBUG
	}

} // namespace bx

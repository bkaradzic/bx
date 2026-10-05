/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_BITARRAY_H_HEADER_GUARD
#define BX_BITARRAY_H_HEADER_GUARD

#include <bx/bx.h>
#include <bx/bufferstorage.h>
#include <bx/math.h>

namespace bx
{
	/// Base class for immutable bit array views providing read-only access to a contiguous bit set.
	class BitArrayViewBase
	{
	public:
		/// Returns the number of 64-bit words needed for the given number of bits.
		///
		/// @param[in] _numBits Number of bits.
		///
		static constexpr uint32_t getNumWords(uint32_t _numBits) { return alignUp(_numBits, 64)/64; }

		/// Default constructor. Constructs an empty view.
		BitArrayViewBase();

		/// Constructs a bit array view from existing memory.
		///
		/// @param[in] _ptr         Pointer to existing memory.
		/// @param[in] _sizeInBytes Number of bytes, must be multiple of 8-bytes.
		/// @param[in] _count       Number of bits.
		///
		BitArrayViewBase(void* _ptr, uint32_t _sizeInBytes, uint32_t _count = UINT32_MAX);

		/// Returns the value of a single bit.
		///
		/// @param[in] _idx Bit index.
		///
		/// @returns Bit value.
		///
		bool get(uint32_t _idx) const;

		/// Returns bits in the range [_beginBit, _endBit) packed into a uint64_t.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		/// @returns Packed bit values.
		///
		uint64_t get(uint32_t _beginBit, uint32_t _endBit) const;

		/// Returns a const pointer to the underlying memory.
		const void* getPtr() const;

		/// Returns a const pointer past the end of the underlying memory.
		const void* getTerm() const;

		/// Returns the number of bits in the bit set.
		uint32_t getCount() const;

		/// Returns the maximum number of bits the bit set can hold.
		uint32_t getCapacity() const;

		/// Finds a contiguous run of zero bits.
		///
		/// @param[in] _numBits Number of contiguous zero bits to find.
		///
		/// @returns Index of the first bit in the run, or kInvalid if not found.
		///
		uint32_t find(uint32_t _numBits) const;

		/// Finds a contiguous run of zero bits within a range.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _numBits  Number of contiguous zero bits to find.
		///
		/// @returns Index of the first bit in the run, or kInvalid if not found.
		///
		uint32_t find(uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits) const;

		/// Finds the first or last set bit.
		///
		/// @param[in] _element kFirst or kLast.
		///
		/// @returns Index of the found set bit, or kInvalid.
		///
		uint32_t findSet(Element _element) const;

		/// Finds the first set bit in the range [_beginBit, _endBit).
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		/// @returns Index of the found set bit, or kInvalid.
		///
		uint32_t findSet(uint32_t _beginBit, uint32_t _endBit) const;

		/// Finds the first clear bit in the range [_beginBit, _endBit).
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		/// @returns Index of the found clear bit, or kInvalid.
		///
		uint32_t findClear(uint32_t _beginBit, uint32_t _endBit) const;

		/// Returns the total number of set bits.
		uint32_t countBits() const;

		/// Returns the number of set bits in the range [_beginBit, _endBit).
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		/// @returns Number of set bits in the range.
		///
		uint32_t countBits(uint32_t _beginBit, uint32_t _endBit) const;

		/// Returns true if any bit is set.
		bool testAny() const;

		/// Returns true if any bit in the range [_beginBit, _endBit) is set.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		bool testAny(uint32_t _beginBit, uint32_t _endBit) const;

		/// Returns true if all bits are set.
		bool testAll() const;

		/// Returns true if all bits in the range [_beginBit, _endBit) are set.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		bool testAll(uint32_t _beginBit, uint32_t _endBit) const;

		/// Returns true if no bits are set.
		bool testNone() const;

		/// Returns true if no bits in the range [_beginBit, _endBit) are set.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		bool testNone(uint32_t _beginBit, uint32_t _endBit) const;

	protected:
		/// `find` without argument validation. `find` is inline, so that incorrect
		/// usage asserts in caller's code.
		uint32_t findRange(uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits) const;

		uint64_t* m_ptr;
		uint32_t  m_sizeInBytes;
		uint32_t  m_count;
	};

	/// Mutable, non-owning view into an existing bit array.
	class MutableBitArrayView : public BitArrayViewBase
	{
	public:
		/// Default constructor. Constructs an empty mutable view.
		MutableBitArrayView();

		/// Constructs a mutable bit array view from existing memory.
		///
		/// @param[in] _ptr         Pointer to existing memory.
		/// @param[in] _sizeInBytes Number of bytes, must be multiple of 8-bytes.
		/// @param[in] _count       Number of bits.
		///
		MutableBitArrayView(void* _ptr, uint32_t _sizeInBytes, uint32_t _count = UINT32_MAX);

		/// Sets the view to point to the given memory and count.
		///
		/// @param[in] _src   Pointer to existing memory.
		/// @param[in] _count Number of bits.
		///
		void set(void* _src, uint32_t _count);

		/// Sets a single bit to the specified value.
		///
		/// @param[in] _idx   Bit index.
		/// @param[in] _value Bit value.
		///
		void set(uint32_t _idx, bool _value);

		/// Sets all bits in the range [_beginBit, _endBit) to the specified value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Bit value.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, bool _value);

		/// Sets bits in the range [_beginBit, _endBit) from a packed uint64_t value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Packed bit values.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value);

		/// Copies bits within the same bit array.
		///
		/// @param[in] _dstBit  Destination bit index.
		/// @param[in] _srcBit  Source bit index.
		/// @param[in] _numBits Number of bits to copy.
		///
		void blit(uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits);

		/// Clears the bit set.
		///
		/// @param[in] _all If true, clears the entire buffer. If false, clears only the counted bits.
		///
		void clear(bool _all = false);

		/// Returns a mutable pointer to the underlying memory.
		void* getPtr();

		/// Returns a const pointer to the underlying memory.
		const void* getPtr() const;

		/// Returns a mutable pointer past the end of the underlying memory.
		void* getTerm();

		/// Returns a const pointer past the end of the underlying memory.
		const void* getTerm() const;
	};

	/// Immutable, non-owning view into an existing bit array.
	class BitArrayView : public BitArrayViewBase
	{
	public:
	};

	/// Base class for owning bit array containers with contiguous storage.
	///
	/// @tparam StorageT Storage backend type (fixed or dynamic).
	///
	template<typename StorageT>
	class BitArrayBaseT : public MutableBitArrayView
	{
	public:
		/// Default constructor. Constructs an empty bit array.
		BitArrayBaseT();

		/// Constructs a bit array by copying from a bit array view.
		///
		/// @param[in] _src Source bit array view to copy from.
		///
		BitArrayBaseT(const BitArrayViewBase& _src);

		/// Sets the bit array contents from a bit array view.
		///
		/// @param[in] _src Source bit array view to copy from.
		///
		void set(const BitArrayViewBase& _src);

		/// Sets a single bit to the specified value.
		///
		/// @param[in] _idx   Bit index.
		/// @param[in] _value Bit value.
		///
		void set(uint32_t _idx, bool _value);

		/// Sets all bits in the range [_beginBit, _endBit) to the specified value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Bit value.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, bool _value);

		/// Sets bits in the range [_beginBit, _endBit) from a packed uint64_t value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Packed bit values.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value);

		/// Copy assignment operator from another bit array.
		///
		/// @param[in] _rhs Source bit array to copy from.
		///
		template<typename StorageU>
		BitArrayBaseT<StorageT>& operator=(const BitArrayBaseT<StorageU>& _rhs);

		/// Returns the maximum number of bits the bit array can hold.
		uint32_t getCapacity() const;

		/// Sets the storage capacity in bits. May reallocate if needed.
		///
		/// @param[in] _capacityInBits Desired minimum capacity in bits.
		///
		void setCapacity(uint32_t _capacityInBits);

		/// Sets the number of bits. Constructs or destroys bits as needed.
		///
		/// @param[in] _countInBits Desired bit count.
		///
		void setCount(uint32_t _countInBits);

	protected:
		StorageT m_storage;
	};

	/// Fixed-capacity bit array type alias.
	///
	/// @tparam CapacityT Capacity in bits.
	///
	template<uint32_t CapacityT>
	using FixedBitArrayT = BitArrayBaseT<FixedStorageT<uint64_t, BitArrayViewBase::getNumWords(CapacityT)> >;

	/// Dynamic bit array with heap-allocated memory managed by an allocator.
	class BitArray final : public BitArrayBaseT<DynamicStorageT<uint64_t>>
	{
	public:
		/// Default constructor is deleted; an allocator must be provided.
		BitArray() = delete;

		/// Copy constructor.
		///
		/// @param[in] _other Source bit array to copy from.
		///
		BitArray(const BitArray& _other);

		/// Constructs an empty bit array with the specified allocator.
		///
		/// @param[in] _allocator Memory allocator.
		///
		BitArray(AllocatorI* _allocator);

		/// Constructs a bit array by copying from a bit array view.
		///
		/// @param[in] _allocator Memory allocator.
		/// @param[in] _src       Source bit array view to copy from.
		///
		BitArray(AllocatorI* _allocator, const BitArrayViewBase& _src);

		/// Returns the allocator used by this bit array.
		AllocatorI* getAllocator() const;

		/// Copy assignment operator.
		///
		/// @param[in] _other Source bit array to copy from.
		///
		BitArray& operator=(const BitArray& _other);

		/// Move assignment operator. Takes ownership of the source bit array's storage.
		///
		/// @param[in] _other Source bit array to move from.
		///
		BitArray& operator=(BitArray&& _other);

		/// Sets the bit array contents from a bit array view.
		///
		/// @param[in] _src Source bit array view to copy from.
		///
		void set(const BitArrayViewBase& _src);

		/// Sets the bit array contents by moving from another bit array.
		///
		/// @param[in] _other Source bit array to move from.
		///
		void set(BitArray&& _other);

		/// Sets a single bit to the specified value.
		///
		/// @param[in] _idx   Bit index.
		/// @param[in] _value Bit value.
		///
		void set(uint32_t _idx, bool _value);

		/// Sets all bits in the range [_beginBit, _endBit) to the specified value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Bit value.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, bool _value);

		/// Sets bits in the range [_beginBit, _endBit) from a packed uint64_t value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Packed bit values.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value);

	private:
		/// Releases storage and resets the view to empty, so that no member
		/// refers to the freed block.
		///
		void release();
	};

	/// Owning bit array with a mip of full words stored after the words in the same block.
	/// Every mutator keeps the mip in step, so the first clear bit is found without scanning words.
	class MippedBitArrayBase : public BitArrayViewBase
	{
	public:
		/// Returns the number of mip words needed for the given number of words.
		///
		/// @param[in] _numWords Number of 64-bit words.
		///
		static constexpr uint32_t getNumMipWords(uint32_t _numWords) { return alignUp(_numWords, 64)/64; }

		/// Sets a single bit to the specified value.
		///
		/// @param[in] _idx   Bit index.
		/// @param[in] _value Bit value.
		///
		void set(uint32_t _idx, bool _value);

		/// Sets all bits in the range [_beginBit, _endBit) to the specified value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Bit value.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, bool _value);

		/// Sets bits in the range [_beginBit, _endBit) from a packed uint64_t value.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		/// @param[in] _value    Packed bit values.
		///
		void set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value);

		/// Copies bits within the same bit array.
		///
		/// @param[in] _dstBit  Destination bit index.
		/// @param[in] _srcBit  Source bit index.
		/// @param[in] _numBits Number of bits to copy.
		///
		void blit(uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits);

		/// Clears the counted bits.
		void clear();

		/// Finds the first clear bit in the range [_beginBit, _endBit) through the mip.
		///
		/// @param[in] _beginBit First bit index (inclusive).
		/// @param[in] _endBit   Last bit index (exclusive).
		///
		/// @returns Index of the found clear bit, or kInvalid.
		///
		uint32_t findClear(uint32_t _beginBit, uint32_t _endBit) const;

		/// Returns a const pointer to the mip, one bit per word, set when the word is full.
		const uint64_t* getMip() const;

	protected:
		/// Default constructor. Constructs an empty bit array.
		MippedBitArrayBase();

		/// Binds to `_numWords` words followed by their mip, and zeroes the mip.
		///
		/// @param[in] _ptr      Pointer to the words.
		/// @param[in] _numWords Number of 64-bit words.
		///
		void rebind(uint64_t* _ptr, uint32_t _numWords);

		/// Moves the mip after the number of words changed, without rebuilding it.
		///
		/// @param[in] _ptr         Pointer to the words.
		/// @param[in] _oldNumWords Previous number of 64-bit words.
		/// @param[in] _numWords    New number of 64-bit words.
		///
		void moveMip(uint64_t* _ptr, uint32_t _oldNumWords, uint32_t _numWords);

		/// Recomputes the mip bits for words [_beginWord, _endWord).
		///
		/// @param[in] _beginWord First word index (inclusive).
		/// @param[in] _endWord   Last word index (exclusive).
		///
		void updateMip(uint32_t _beginWord, uint32_t _endWord);

		/// Marks the mip bits past the last word as full, so searches stop there.
		void setMipTail();

		uint64_t* m_mip;
	};

	/// Mipped bit array with contiguous storage for the words and their mip.
	///
	/// @tparam StorageT Storage backend type (fixed or dynamic).
	///
	template<typename StorageT>
	class MippedBitArrayT : public MippedBitArrayBase
	{
	public:
		/// Default constructor. Constructs an empty bit array.
		MippedBitArrayT();

		/// Returns the maximum number of bits the bit array can hold.
		uint32_t getCapacity() const;

		/// Sets the storage capacity in bits. May reallocate, and moves the mip if it does.
		///
		/// @param[in] _capacityInBits Desired capacity in bits.
		///
		void setCapacity(uint32_t _capacityInBits);

		/// Sets the number of bits. New bits are cleared.
		///
		/// @param[in] _countInBits Desired bit count.
		///
		void setCount(uint32_t _countInBits);

	protected:
		StorageT m_storage;
		uint32_t m_numWords;
	};

	/// Fixed-capacity mipped bit array type alias.
	///
	/// @tparam CapacityT Capacity in bits.
	///
	template<uint32_t CapacityT>
	using FixedMippedBitArrayT = MippedBitArrayT<FixedStorageT<uint64_t, 0
		+ BitArrayViewBase::getNumWords(CapacityT)
		+ MippedBitArrayBase::getNumMipWords(BitArrayViewBase::getNumWords(CapacityT) )
		> >;

	/// Dynamic mipped bit array with heap-allocated memory managed by an allocator.
	class MippedBitArray final : public MippedBitArrayT<DynamicStorageT<uint64_t>>
	{
	public:
		/// Default constructor is deleted; an allocator must be provided.
		MippedBitArray() = delete;

		/// Copying is not supported.
		MippedBitArray(const MippedBitArray&) = delete;

		/// Copying is not supported.
		MippedBitArray& operator=(const MippedBitArray&) = delete;

		/// Constructs an empty bit array with the specified allocator.
		///
		/// @param[in] _allocator Memory allocator.
		///
		MippedBitArray(AllocatorI* _allocator);

		/// Returns the allocator used by this bit array.
		AllocatorI* getAllocator() const;
	};


} // namespace bx

#include "inline/bitarray.inl"

#endif // BX_BITARRAY_H_HEADER_GUARD

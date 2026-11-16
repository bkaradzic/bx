/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_UNIQUEPTR_H_HEADER_GUARD
#	error "Must be included from bx/uniqueptr.h!"
#endif // BX_UNIQUEPTR_H_HEADER_GUARD

namespace bx
{
	template<typename Ty>
	inline UniquePtrT<Ty>::UniquePtrT()
	{
		debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty>::~UniquePtrT()
	{
		reset();
	}

	template<typename Ty>
	inline UniquePtrT<Ty>::UniquePtrT(AllocatorHandle _allocator)
	{
		m_blk.allocatorHandle = _allocator.idx;
		debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty>::UniquePtrT(Blk&& _blk)
		: m_blk(bx::move(_blk) )
	{
		debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty>::UniquePtrT(UniquePtrT&& _rhs)
		: m_blk(bx::move(_rhs.m_blk) )
	{
		_rhs.m_blk.allocatorHandle = m_blk.allocatorHandle;
		debugPtrUpdate();
		_rhs.debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty>& UniquePtrT<Ty>::operator=(UniquePtrT&& _rhs)
	{
		if (this != &_rhs)
		{
			reset();
			m_blk = bx::move(_rhs.m_blk);
			_rhs.m_blk.allocatorHandle = m_blk.allocatorHandle;
			debugPtrUpdate();
			_rhs.debugPtrUpdate();
		}

		return *this;
	}

	template<typename Ty>
	inline Ty& UniquePtrT<Ty>::operator*() const
	{
		return *(Ty*)m_blk;
	}

	template<typename Ty>
	inline Ty* UniquePtrT<Ty>::operator->() const
	{
		return (Ty*)m_blk;
	}

	template<typename Ty>
	inline Ty* UniquePtrT<Ty>::getPtr() const
	{
		return isValid() ? (Ty*)m_blk : NULL;
	}

	template<typename Ty>
	inline Blk UniquePtrT<Ty>::release()
	{
		Blk blk = bx::move(m_blk);
		m_blk.allocatorHandle = blk.allocatorHandle;
		debugPtrUpdate();
		return blk;
	}

	template<typename Ty>
	inline AllocatorHandle UniquePtrT<Ty>::getAllocatorHandle() const
	{
		return bx::getAllocatorHandle(m_blk);
	}

	template<typename Ty>
	inline void UniquePtrT<Ty>::reset(const Location& _location)
	{
		if (isValid() )
		{
			( (Ty*)m_blk)->~Ty();
			m_blk = free(bx::move(m_blk), _location);
			debugPtrUpdate();
		}
	}

	template<typename Ty>
	inline bool UniquePtrT<Ty>::isValid() const
	{
		return bx::isValid(m_blk);
	}

	template<typename Ty>
	inline void UniquePtrT<Ty>::debugPtrUpdate()
	{
#if BX_CONFIG_DEBUG
		m_debugPtr = bx::isValid(m_blk) ? (Ty*)m_blk : NULL;
#endif // BX_CONFIG_DEBUG
	}

	template<typename Ty>
	inline UniquePtrT<Ty[]>::UniquePtrT()
		: m_count(0)
	{
		debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty[]>::~UniquePtrT()
	{
		reset();
	}

	template<typename Ty>
	inline UniquePtrT<Ty[]>::UniquePtrT(AllocatorHandle _allocator)
		: m_count(0)
	{
		m_blk.allocatorHandle = _allocator.idx;
		debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty[]>::UniquePtrT(Blk&& _blk, uint32_t _count)
		: m_blk(bx::move(_blk) )
		, m_count(bx::isValid(m_blk) ? _count : 0)
	{
		debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty[]>::UniquePtrT(UniquePtrT&& _rhs)
		: m_blk(bx::move(_rhs.m_blk) )
		, m_count(_rhs.m_count)
	{
		_rhs.m_blk.allocatorHandle = m_blk.allocatorHandle;
		_rhs.m_count = 0;
		debugPtrUpdate();
		_rhs.debugPtrUpdate();
	}

	template<typename Ty>
	inline UniquePtrT<Ty[]>& UniquePtrT<Ty[]>::operator=(UniquePtrT&& _rhs)
	{
		if (this != &_rhs)
		{
			reset();
			m_blk   = bx::move(_rhs.m_blk);
			m_count = _rhs.m_count;
			_rhs.m_blk.allocatorHandle = m_blk.allocatorHandle;
			_rhs.m_count = 0;
			debugPtrUpdate();
			_rhs.debugPtrUpdate();
		}

		return *this;
	}

	template<typename Ty>
	inline Ty& UniquePtrT<Ty[]>::operator[](uint32_t _idx) const
	{
		BX_ASSERT(_idx < m_count, "Index out of bounds %d (count: %d)!", _idx, m_count);
		return ( (Ty*)m_blk)[_idx];
	}

	template<typename Ty>
	inline Ty* UniquePtrT<Ty[]>::getPtr() const
	{
		return isValid() ? (Ty*)m_blk : NULL;
	}

	template<typename Ty>
	inline uint32_t UniquePtrT<Ty[]>::getCount() const
	{
		return m_count;
	}

	template<typename Ty>
	inline Blk UniquePtrT<Ty[]>::release()
	{
		Blk blk = bx::move(m_blk);
		m_blk.allocatorHandle = blk.allocatorHandle;
		m_count = 0;
		debugPtrUpdate();
		return blk;
	}

	template<typename Ty>
	inline AllocatorHandle UniquePtrT<Ty[]>::getAllocatorHandle() const
	{
		return bx::getAllocatorHandle(m_blk);
	}

	template<typename Ty>
	inline void UniquePtrT<Ty[]>::reset(const Location& _location)
	{
		if (isValid() )
		{
			StorageTraits::destruct( (Ty*)m_blk, m_count);

			m_blk   = free(bx::move(m_blk), _location);
			m_count = 0;
			debugPtrUpdate();
		}
	}

	template<typename Ty>
	inline bool UniquePtrT<Ty[]>::isValid() const
	{
		return bx::isValid(m_blk);
	}

	template<typename Ty>
	inline void UniquePtrT<Ty[]>::debugPtrUpdate()
	{
#if BX_CONFIG_DEBUG
		m_debugPtr = bx::isValid(m_blk) ? (Ty*)m_blk : NULL;
#endif // BX_CONFIG_DEBUG
	}

	template<typename Ty, typename... ArgsT>
	requires (!isArray<Ty>() )
	inline UniquePtrT<Ty> makeUnique(AllocatorHandle _allocator, const Location& _location, ArgsT&&... _args)
	{
		Blk blk = alloc(_allocator, sizeof(Ty), alignof(Ty), _location);

		if (!isValid(blk) )
		{
			return UniquePtrT<Ty>(_allocator);
		}

		BX_PLACEMENT_NEW( (Ty*)blk, Ty)(bx::forward<ArgsT>(_args)...);
		return UniquePtrT<Ty>(bx::move(blk) );
	}

	template<typename Ty, typename... ArgsT>
	requires (isUnboundedArray<Ty>() )
	inline UniquePtrT<Ty> makeUnique(AllocatorHandle _allocator, const Location& _location, uint32_t _count, ArgsT&&... _args)
	{
		using ElementT = typename UniquePtrT<Ty>::ElementType;

		if (0 == _count)
		{
			return UniquePtrT<Ty>(_allocator);
		}

		Blk blk = alloc(_allocator, size_t(_count)*sizeof(ElementT), alignof(ElementT), _location);

		if (!isValid(blk) )
		{
			return UniquePtrT<Ty>(_allocator);
		}

		ElementT* ptr = (ElementT*)blk;

		if constexpr (0 == sizeof...(ArgsT) )
		{
			StorageTraits::constructDefault(ptr, _count);
			return UniquePtrT<Ty>(bx::move(blk), _count);
		}
		else
		{
			return UniquePtrT<Ty>(bx::move(blk), StorageTraits::constructEmplace(ptr, _count, bx::forward<ArgsT>(_args)...) );
		}
	}

} // namespace bx

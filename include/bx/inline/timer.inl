/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_TIMER_H_HEADER_GUARD
#	error "Must be included from bx/timer.h!"
#endif // BX_TIMER_H_HEADER_GUARD

namespace bx
{
	inline Ticks::Ticks(InitNoneTag)
	{
	}

	inline constexpr Ticks::Ticks(InitZeroTag)
		: Ticks(0)
	{
	}

	inline Ticks::Ticks(InitIdentityTag)
		: Ticks(getHPCounter())
	{
	}

	inline constexpr Ticks::Ticks(int64_t _ticks)
		: ticks(_ticks)
	{
	}

	inline constexpr Ticks Ticks::operator+(const Ticks &_rhs) const noexcept { return Ticks(ticks + _rhs.ticks); 							}
	inline constexpr Ticks Ticks::operator-(const Ticks &_rhs) const noexcept { return Ticks(ticks - _rhs.ticks); 							}
	inline constexpr Ticks Ticks::operator*(float _rhs) const noexcept { return Ticks(int64_t(double(ticks) * _rhs)); 						}
	inline constexpr Ticks &Ticks::operator+=(const Ticks &_rhs) noexcept { ticks += _rhs.ticks; return *this;							 	}
	inline constexpr Ticks &Ticks::operator-=(const Ticks &_rhs) noexcept { ticks -= _rhs.ticks; return *this; 							 	}
	inline constexpr Ticks &Ticks::operator*=(float _rhs) noexcept { ticks = int64_t(double(ticks) * _rhs); return *this; 					}
	inline constexpr bool Ticks::operator==(const Ticks &_rhs) const noexcept { return ticks == _rhs.ticks; 							 	}
	inline constexpr bool Ticks::operator!=(const Ticks &_rhs) const noexcept { return ticks != _rhs.ticks; 							 	}
	inline constexpr bool Ticks::operator<(const Ticks &_rhs) const noexcept { return ticks < _rhs.ticks; 							 		}
	inline constexpr bool Ticks::operator<=(const Ticks &_rhs) const noexcept { return ticks <= _rhs.ticks; 							 	}
	inline constexpr bool Ticks::operator>(const Ticks &_rhs) const noexcept { return ticks > _rhs.ticks; 									}
	inline constexpr bool Ticks::operator>=(const Ticks &_rhs) const noexcept { return ticks >= _rhs.ticks; 							 	}

	inline constexpr Ticks operator+(int64_t ticks, const Ticks &_rhs) noexcept { return Ticks(ticks + _rhs.ticks); 						}
	inline constexpr Ticks operator-(int64_t ticks, const Ticks &_rhs) noexcept { return Ticks(ticks - _rhs.ticks); 						}
	inline constexpr Ticks operator*(float ticks, const Ticks &_rhs) noexcept { return Ticks(int64_t(double(ticks * _rhs.ticks))); 			}
	inline constexpr Ticks &operator+=(int64_t ticks, Ticks &_rhs) noexcept { _rhs.ticks += ticks; return _rhs; 							}
	inline constexpr Ticks &operator-=(int64_t ticks, Ticks &_rhs) noexcept { _rhs.ticks -= ticks; return _rhs; 							}
	inline constexpr Ticks &operator*=(float ticks, Ticks &_rhs) noexcept { _rhs.ticks = int64_t(double(ticks * _rhs.ticks)); return _rhs;  }
	inline constexpr bool operator==(int64_t ticks, const Ticks &_rhs) noexcept { return ticks == _rhs.ticks; 							 	}
	inline constexpr bool operator!=(int64_t ticks, const Ticks &_rhs) noexcept { return ticks != _rhs.ticks; 							 	}
	inline constexpr bool operator<(int64_t ticks, const Ticks &_rhs) noexcept { return ticks < _rhs.ticks; 							 	}
	inline constexpr bool operator<=(int64_t ticks, const Ticks &_rhs) noexcept { return ticks <= _rhs.ticks; 							 	}
	inline constexpr bool operator>(int64_t ticks, const Ticks &_rhs) noexcept { return ticks > _rhs.ticks; 							 	}
	inline constexpr bool operator>=(int64_t ticks, const Ticks &_rhs) noexcept { return ticks >= _rhs.ticks; 							 	}

	inline Ticks getNow()
	{
		return Ticks(getHPCounter());
	}

	inline Ticks getTicksSinceStartup()
	{
		return getNow() - Ticks::s_kStartup;
	}

	template <typename Ty>
	inline constexpr Ty toSeconds(const Ticks &_ticks)
	{
		return Ty(double(_ticks.ticks * Ticks::s_kInvFreq));
	}

	template <typename Ty>
	inline constexpr Ty toMilliseconds(const Ticks &_ticks)
	{
		return Ty(double(_ticks.ticks * 1000 * Ticks::s_kInvFreq));
	}

} // namespace bx

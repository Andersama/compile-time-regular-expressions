#ifndef CTRE__PATTERN_MATCH_LIMITS__HPP
#define CTRE__PATTERN_MATCH_LIMITS__HPP

namespace ctre {
	//Stores the lower and upper bounds of characters a regex pattern can match
	struct pattern_limits {
		// -1 is considered INF, -2 is finite (but perhaps too large to store), all other values are exact counts
		size_t min = 0ULL;
		size_t max = 0ULL;
		constexpr pattern_limits(const size_t _min, const size_t _max) : min(_min), max(_max) {};
		static constexpr CTRE_FORCE_INLINE size_t saturate_limit(const size_t lhs, const size_t rhs) {
			const constexpr size_t inf = size_t{ 0 } - 1;
			const constexpr size_t lim = size_t{ 0 } - 2;
			size_t ret = inf;
			if (lhs == inf || rhs == inf) { //if eaither lhs or rhs are infinite we remain in infinite state
				return ret;
			} else {
				ret = lhs + rhs;
				//clamp to a finite state
				ret = (ret < lhs) ? lim : ((ret == inf) ? lim : ret);
				return ret;
			}
		}

		static constexpr CTRE_FORCE_INLINE size_t mult_saturate_limit(const size_t lhs, const size_t rhs) {
			const constexpr size_t inf = size_t{ 0 } - 1;
			const constexpr size_t lim = size_t{ 0 } - 2;
			size_t ret = inf;
			if (lhs == inf || rhs == inf) {
				return ret;
			} else if (lhs == 0ULL || rhs == 0ULL) { //inf * 0 == 0, eaither side being multiplied here results in 0
				return ret = 0ULL;
			} else {
				if (lhs > (inf / rhs)) //if we would overflow remain in finite state
					return ret = lim;
				ret = lhs * rhs;
				ret = ret == inf ? lim : ret;
				return ret;
			}
		}

		constexpr CTRE_FORCE_INLINE operator bool() const noexcept {
			return min;
		}
		constexpr CTRE_FORCE_INLINE pattern_limits operator+(pattern_limits other) const noexcept {
			pattern_limits ret{ 0ULL,0ULL };
			ret.min = saturate_limit(min, other.min);
			ret.max = saturate_limit(max, other.max);
			return ret;
		}
		constexpr CTRE_FORCE_INLINE pattern_limits operator||(pattern_limits other) const noexcept {
			pattern_limits ret{ 0ULL,0ULL };
			ret.min = std::min(min, other.min);
			ret.max = std::max(max, other.max);
			return ret;
		}
	};

	//anything we haven't matched already isn't supported and will contribute nothing to the pattern
	template<typename Pattern, typename R>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(Pattern, R captures) noexcept {
		return pattern_limits{ 0ULL, 0ULL };
	};

	//repeat multiplies minimum and maximum
	template<size_t A, size_t B, typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(repeat<A, B, Content...>, R captures) noexcept {
		pattern_limits ret{ 0ULL, 0ULL };
		if constexpr (sizeof...(Content)) {
			ret = pattern_limit_analysis(ctll::list<Content...>(), captures);
			ret.min = pattern_limits::mult_saturate_limit(ret.min, A);
			ret.max = pattern_limits::mult_saturate_limit(ret.max, B);
		}
		return ret;
	}

	//note: all * ? + operations are specialized variations of repeat {A,B}
	template<size_t A, size_t B, typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(lazy_repeat<A, B, Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<A, B, Content...>(), captures);
	}

	template<size_t A, size_t B, typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(possessive_repeat<A, B, Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<A, B, Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(star<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<0ULL, ~(0ULL), Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(lazy_star<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<0ULL, ~(0ULL), Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(possessive_star<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<0ULL, ~(0ULL), Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(plus<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<1ULL, ~(0ULL), Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(lazy_plus<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<1ULL, ~(0ULL), Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(possessive_plus<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<1ULL, ~(0ULL), Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(optional<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<0ULL, 1ULL, Content...>(), captures);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(lazy_optional<Content...>, R captures) noexcept {
		return pattern_limit_analysis(repeat<0ULL, 1ULL, Content...>(), captures);
	}

	//back_reference (needs to get the referenced pattern)
	template<size_t Id, typename R>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(back_reference<Id>, R captures) noexcept {
		const auto ref = captures.template get<Id>();
		pattern_limits ret{0ULL, 0ULL};
		if constexpr (size(ref.get_expression())) {
			ret = ((pattern_limit_analysis(ref.get_expression(), captures)) + ...);
		}
		return ret;
	}

	//back_reference_with_name (needs to get the referenced pattern)
	template<typename Name, typename R>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(back_reference_with_name<Name>, R captures) noexcept {
		const auto ref = captures.template get<Name>();
		pattern_limits ret{0ULL, 0ULL};
		if constexpr (size(ref.get_expression())) {
			ret = ((pattern_limit_analysis(ref.get_expression(), captures)) + ...);
		}
		return ret;
	}

	//select, this is specialized, we need to take the minimum of all minimums and maximum of all maximums
	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(select<Content...>, R captures) noexcept {
		return ((pattern_limit_analysis(ref.get_expression(), captures)) || ...);
	}

	//character, any character contributes exactly one to both counts
	template<auto C, typename R>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(character<C>, R captures) noexcept {
		return pattern_limits { 1ULL, 1ULL };
	}

	//strings, any string contributes the # of characters it contains (if we have an empty string that'll be 0)
	template<auto... Str, typename R>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(string<Str...>, R captures) noexcept {
		return pattern_limits {sizeof...(Str), sizeof...(Str)};
	}

	//we'll process anything that has sequential contents by adding their limits
	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(ctll::list<Content...>, R captures) noexcept {
		return ((pattern_limit_analysis(Content(), captures)) + ...);
	}

	template<typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(sequence<Content...>, R captures) noexcept {
		return ((pattern_limit_analysis(Content(), captures)) + ...);
	}

	template<size_t Id, typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(capture<Id, Content...>, R captures) noexcept {
		return ((pattern_limit_analysis(Content(), captures)) + ...);
	}

	template<size_t Id, typename Name, typename R, typename... Content>
	constexpr CTRE_FORCE_INLINE pattern_limits pattern_limit_analysis(capture_with_name<Id, Name, Content...>, R captures) noexcept {
		return ((pattern_limit_analysis(Content(), captures)) + ...);
	}
}

#endif
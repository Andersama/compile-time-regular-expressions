#ifndef CTRE__PATTERN_OPTIMIZER__HPP
#define CTRE__PATTERN_OPTIMIZER__HPP

namespace ctre {
	constexpr CTRE_FORCE_INLINE size_t saturate_repeat_limit(const size_t lhs, const size_t rhs) {
		const constexpr size_t inf = size_t{ 0 } - 1;
		const constexpr size_t lim = size_t{ 0 } - 2;
		size_t ret = inf;
		if (lhs == inf || rhs == inf) { //if eaither lhs or rhs are infinite we remain in infinite state
			return ret;
		}
		else {
			ret = lhs + rhs;
			//clamp to a finite state
			ret = (ret < lhs) ? lim : ((ret == inf) ? lim : ret);
			return ret;
		}
	}

	constexpr CTRE_FORCE_INLINE size_t mult_saturate_limit(const size_t lhs, const size_t rhs) {
		const constexpr size_t inf = size_t{ 0 } - 1;
		const constexpr size_t lim = size_t{ 0 } - 2;
		size_t ret = inf;
		if (lhs == inf || rhs == inf) {
			return ret;
		}
		else if (lhs == 0ULL || rhs == 0ULL) { //inf * 0 == 0, eaither side being multiplied here results in 0
			return ret = 0ULL;
		}
		else {
			if (lhs > (inf / rhs)) //if we would overflow remain in finite state
				return ret = lim;
			ret = lhs * rhs;
			ret = ret == inf ? lim : ret;
			return ret;
		}
	}
	//see https://www.rexegg.com/regex-optimizations.html

	template <typename Item, typename... Content>
	constexpr auto pattern_optimize(ctll::list<Item, Content...>) noexcept {
		return ctll::list<pattern_optimize(Content), ...>{};
	}

	//patterns we don't recognize are left as is
	template <typename Pattern>
	constexpr auto pattern_optimize(Pattern) -> Pattern noexcept {
		return {};
	}
	//optimize each section of a sequence
	template <typename... Content>
	constexpr auto pattern_optimize(ctll::list<Content...>) noexcept {
		return ctll::list<pattern_optimize(Content), ...>{};
	}
	template <typename... Content>
	constexpr auto pattern_optimize(sequence<Content...>) noexcept {
		return sequence<pattern_optimize(Content), ...>{};
	}
	//optimize each section of a capture (like a sequence)
	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(capture<Index, Seq...>) noexcept {
		return capture<Index, pattern_optimize(Seq), ...>{};
	}

	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(capture_with_name<Index, Seq...>) noexcept {
		return capture_with_name<Index, pattern_optimize(Seq),...>{};
	}
	//optimize each section of repeated groups (like a sequence)
	template <size_t A, size_t B, typename... Content>
	constexpr auto pattern_optimize(repeat<A, B, Content...>) noexcept {
		return repeat<A, B, pattern_optimize(Content), ...>{};
	}

	template <size_t A, size_t B, typename... Content>
	constexpr auto pattern_optimize(lazy_repeat<A, B, Content...>) noexcept {
		return lazy_repeat<A, B, pattern_optimize(Content),...>{};
	}

	template <size_t A, size_t B, typename... Content>
	constexpr auto pattern_optimize(possessive_repeat<A, B, Content...>) noexcept {
		return possessive_repeat<A, B, pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(star<Content...>) noexcept {
		return star<pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(lazy_star<Content...>) noexcept {
		return lazy_star<pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(possessive_star<Content...>) noexcept {
		return possesive_star<pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(plus<Content...>) noexcept {
		return plus<pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(lazy_plus<Content...>) noexcept {
		return lazy_plus<pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(possessive_plus<Content...>) noexcept {
		return possesive_plus<pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(optional<Content...>) noexcept {
		return optional<pattern_optimize(Content), ...>{};
	}

	template <typename... Content>
	constexpr auto pattern_optimize(lazy_optional<Content...>) noexcept {
		return lazy_optional<pattern_optimize(Content), ...>{};
	}

	//fold sequence into ctll::list
	template <typename... Seq, typename... Content>
	constexpr auto pattern_optimize(ctll::list<sequence<Seq...>, Content...>) noexcept {
		return pattern_optimize(ctll::list<Seq..., Content...>{});
	}
	template <typename... Seq, typename... Content>
	constexpr auto pattern_optimize(ctll::list<ctll::list<Seq...>, Content...>) noexcept {
		return pattern_optimize(ctll::list<Seq..., Content...>{});
	}
	//a sequence which consumes characters with starting or ending anchors interleaved must fail
	//assert_begin, consuming sequence, assert_begin -> reject
	//assert_end, consuming sequence, assert_end -> reject

	//unroll alternation (avoids an amount of backtracking) replace (A|B)*... with A*(?:B+A*)*...
	template <typename... Options, typename A, typename B>
	constexpr auto pattern_optimize(star<select<A, B>>) noexcept {
		return pattern_optimize(ctll::list<star<A>, star<plus<B>, star<A>>>{});
	}

	//replace A{n,m} with A....AnA{0,m-n}
	template <size_t A, size_t B, typename Ch>
	constexpr auto pattern_optimize(repeat<A, B, character<Ch>>) noexcept {
		if constexpr (A < 64) { //only going to bother optimizing under this # of characters for compiler's sake
			if constexpr (A > 8) {
				//0 is infinite and A<=B
				return ctll::list<character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>, 
					pattern_optimize(repeat<A - 8ULL, (B) ? B - 8ULL : 0ULL, character<Ch>>{})
				>{};
			} else if (A > 0) {
				return ctll::list < character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>, character<Ch>,
					pattern_optimize(repeat<A - 1ULL, (B) ? B - 1ULL : 0ULL, character<Ch>>{})
				> {};
			} else {
				return repeat<A, B, character<Ch>>{};
			}
		} else {
			//do nothing
			return repeat<A, B, character<Ch>>{};
		}
	}

	//replace a+a*, a{1,}a{0,}... etc with a{1,}...
	template <size_t A, size_t B, size_t A2, size_t B2, typename... Seq, typename... Content>
	constexpr auto pattern_optimize(ctll::list<repeat<A, B, Seq...>, repeat<A2, B2, Seq...>, Content...>) noexcept {
		return pattern_optimize(
			ctll::list<repeat<saturate_repeat_limit(A, A2), saturate_repeat_limit(B, B2), pattern_optimize(Seq...)>, pattern_optimize(Content...)>
		{});
	}

	//replace (a*)*... with (a*)... (reduce backtracking)
	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(star<capture<Index,star<Seq...>>>) {
		return capture<Index,star<pattern_optimize(Seq),...>>{};
	}
	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(star<capture_with_name<Index, star<Seq...>>>) {
		return capture_with_name<Index, star<pattern_optimize(Seq),...>>{};
	}
	template <typename... Seq>
	constexpr auto pattern_optimize(star<sequence<star<Seq...>>>) {
		return ctll::list<star<pattern_optimize(Seq),...>>{};
	}
	template <typename... Seq>
	constexpr auto pattern_optimize(star<ctll::list<star<Seq...>>>) {
		return ctll::list<star<pattern_optimize(Seq),...>>{};
	}

	//replace (a*?)*?... with (a*?)... (reduce backtracking)
	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(lazy_star<capture<Index, lazy_star<Seq...>>>) {
		return capture<Index, lazy_star<pattern_optimize(Seq),...>>{};
	}
	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(lazy_star<capture_with_name<Index, lazy_star<Seq...>>>) {
		return capture_with_name<Index, lazy_star<pattern_optimize(Seq),...>>{};
	}
	template <typename... Seq>
	constexpr auto pattern_optimize(lazy_star<sequence<lazy_star<Seq...>>>) {
		return ctll::list<lazy_star<pattern_optimize(Seq),...>>{};
	}
	template <typename... Seq>
	constexpr auto pattern_optimize(lazy_star<ctll::list<lazy_star<Seq...>>>) {
		return ctll::list<lazy_star<pattern_optimize(Seq),...>>{};
	}

	//replace (a*+)*+... with (a*+)... (simplify syntax)
	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(possesive_star<capture<Index, possesive_star<Seq...>>>) {
		return capture<Index, possesive_star<pattern_optimize(Seq),...>>{};
	}
	template <size_t Index, typename... Seq>
	constexpr auto pattern_optimize(possesive_star<capture_with_name<Index, possesive_star<Seq...>>>) {
		return capture_with_name<Index, possesive_star<pattern_optimize(Seq),...>>{};
	}
	template <typename... Seq>
	constexpr auto pattern_optimize(possesive_star<sequence<possesive_star<Seq...>>>) {
		return ctll::list<possesive_star<pattern_optimize(Seq),...>>{};
	}
	template <typename... Seq>
	constexpr auto pattern_optimize(possesive_star<ctll::list<possesive_star<Seq...>>>) {
		return ctll::list<possesive_star<pattern_optimize(Seq),...>>{};
	}
};

#endif  
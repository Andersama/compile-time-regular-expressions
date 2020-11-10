#ifndef CTRE__REGEX_COUNT__HPP
#define CTRE__REGEX_COUNT__HPP
#include "atoms.hpp"
#include "atoms_characters.hpp"
#include "atoms_unicode.hpp"
#include "starts_with_anchor.hpp"
#include "utility.hpp"
#include "return_type.hpp"
#include "find_captures.hpp"
#include "first.hpp"

namespace ctre {
	constexpr CTRE_FORCE_INLINE size_t regex_count_add(const size_t lhs, const size_t rhs) {
		const constexpr size_t inf = size_t{ 0 } - 1;
		const constexpr size_t lim = size_t{ 0 } - 2;
		size_t ret = inf;
		if (lhs == inf || rhs == inf) {
			return ret;
		} else {
			ret = lhs + rhs;
			ret = ret < lhs ? lim : ret == inf ? lim : ret;
			return ret;
		}
	}

	constexpr CTRE_FORCE_INLINE size_t regex_count_mul(const size_t lhs, const size_t rhs) {
		const constexpr size_t inf = size_t{ 0 } - 1;
		const constexpr size_t lim = size_t{ 0 } - 2;
		size_t ret = inf;
		if (lhs == inf || rhs == inf) {
			return ret;
		} else if (lhs == 0 || rhs == 0) {
			return ret = 0;
		} else {
			if (lhs > (inf / rhs))
				return ret = lim;
			ret = lhs * rhs;
			ret = ret == inf ? lim : ret;
			return ret;
		}
	}

	constexpr CTRE_FORCE_INLINE size_t regex_count_min(const size_t lhs, const size_t rhs) {
		return (lhs < rhs) ? lhs : rhs;
	}

	constexpr CTRE_FORCE_INLINE size_t regex_count_max(const size_t lhs, const size_t rhs) {
		return (rhs > lhs) ? rhs : lhs;
	}

	struct regex_count_result {
		size_t _Min = 0;
		size_t _Max = 0;
		constexpr regex_count_result(size_t _Min, size_t _Max) : _Min(_Min), _Max(_Max) {}
		constexpr regex_count_result CTRE_FORCE_INLINE operator+(const regex_count_result other) {
			return regex_count_result(regex_count_add(_Min, other._Min), regex_count_add(_Max, other._Max));
		}
		constexpr regex_count_result CTRE_FORCE_INLINE operator||(const regex_count_result other) {
			return regex_count_result(regex_count_min(_Min, other._Min), regex_count_max(_Max, other._Max));
		}
		constexpr regex_count_result CTRE_FORCE_INLINE operator*(const regex_count_result other) {
			return regex_count_result(regex_count_mul(_Min, other._Min), regex_count_mul(_Max, other._Max));
		}
	};

	template<typename Name>
	constexpr bool has_backreference(back_reference_with_name<Name>) {
		return true;
	}

	template<size_t Index>
	constexpr bool has_backreference(back_reference<Index>) {
		return true;
	}

	template<typename Pattern>
	constexpr bool has_backreference(Pattern) {
		return false;
	}
	
	template<typename Pattern>
	constexpr bool is_terminator(Pattern) {
		if constexpr (std::is_same_v<Pattern, accept> || std::is_same_v<Pattern, reject>) {
			return true;
		} else {
			return false;
		}
	}

	template<typename... Pattern>
	constexpr bool has_backreference(ctll::list<Pattern...>) {
		bool is_accepting = true; //short circuit when we hit a terminator
		return ((is_accepting && is_accepting = !is_terminator(Pattern{}) && has_backreference(ctll::list<Pattern>())) || ...);
	}

	template<typename... Pattern>
	constexpr bool has_backreference(sequence<Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<size_t Index, typename... Pattern>
	constexpr bool has_backreference(capture<Index, Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<size_t Index, typename Name, typename... Pattern>
	constexpr bool has_backreference(capture_with_name<Index, Name, Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<size_t A, size_t B, typename... Pattern>
	constexpr bool has_backreference(repeat<A, B, Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<size_t A, size_t B, typename... Pattern>
	constexpr bool has_backreference(lazy_repeat<A, B, Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<size_t A, size_t B, typename... Pattern>
	constexpr bool has_backreference(possessive_repeat<A, B, Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<typename... Pattern>
	constexpr bool has_backreference(lookahead_positive<Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<typename... Pattern>
	constexpr bool has_backreference(lookahead_negative<Pattern...>) {
		return has_backreference(ctll::list<Pattern...>());
	}

	template<typename... Pattern>
	constexpr bool has_backreference(select<Pattern...>) {
		return (has_backreference(ctll::list<Pattern>()) || ...);
	}

	//try to minimize number of function signatures we're dealing with across multiple regexs eg: (?:a|b) should memolize if inside (?:(?:a|b)c) and (?:(?:a|b)d)
	template<typename Pattern>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(Pattern p) noexcept {
		using captures_list = decltype(find_captures(Pattern{}));
		using return_type = decltype(regex_results(std::declval<std::basic_string_view<char>::iterator>(), captures_list()));
		if constexpr (size(captures_list{}) && has_backreference(ctll::list<Pattern>()))
			return regex_count(ctll::list<Pattern>(), return_type{}); //we have captures, need these as context
		else
			return regex_count(ctll::list<Pattern>());
	}

	template<typename R, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(ctll::list<Patterns...>, R captures) noexcept {
		return ((regex_count(Patterns{}, captures)) + ...);
	}

	template<typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(ctll::list<Patterns...>) noexcept {
		return ((regex_count(Patterns{})) + ...);
	}

	template<typename R, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(sequence<Patterns...>, R captures) noexcept {
		return regex_count(ctll::list<Patterns...>(), captures);
	}

	template<typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(sequence<Patterns...>) noexcept {
		return regex_count(ctll::list<Patterns...>());
	}
	
	template<size_t Index, typename R, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(capture<Index, Patterns...>, R captures) noexcept {
		return regex_count(ctll::list<Patterns...>(), captures);
	}

	template<size_t Index, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(capture<Index, Patterns...>) noexcept {
		return regex_count(ctll::list<Patterns...>());
	}

	template<size_t Index, typename Name, typename R, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(capture_with_name<Index, Name, Patterns...>, R captures) noexcept {
		return regex_count(ctll::list<Patterns...>(), captures);
	}

	template<size_t Index, typename Name, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(capture_with_name<Index, Name, Patterns...>) noexcept {
		return regex_count(ctll::list<Patterns...>());
	}

	//needs context of captures
	template<size_t Index, typename R>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(back_reference<Index>, R captures) noexcept {
		const auto ref = captures.template get<Index>();
		if constexpr (size(ref.get_expression())) {
			return regex_count(ref.get_expression(), captures);
		} else {
			return regex_count_result{ 0ULL, 0ULL };
		}
	}

	template<typename Name, typename R>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(back_reference_with_name<Name>, R captures) noexcept {
		const auto ref = captures.template get<Name>();
		if constexpr (size(ref.get_expression())) {
			return regex_count(ref.get_expression(), captures);
		} else {
			return regex_count_result{ 0ULL, 0ULL };
		}
	}

	template<auto... Str, typename R>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(string<Str...>, R captures) noexcept {
		return regex_count_result{ sizeof...(Str), sizeof...(Str) };
	}

	template<auto... Str>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(string<Str...>) noexcept {
		return regex_count_result{ sizeof...(Str), sizeof...(Str) };
	}

	template<typename R, typename CharacterLike, typename = std::enable_if_t<MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<std::string_view::iterator>())>, void>>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(CharacterLike, R captures) noexcept {
		return regex_count_result{ 1ULL, 1ULL };
	}

	template<typename CharacterLike, typename = std::enable_if_t<MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<std::string_view::iterator>())>, void>>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(CharacterLike) noexcept {
		return regex_count_result{ 1ULL, 1ULL };
	}

	template<typename R, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(select<Patterns...>, R captures) noexcept {
		return ((regex_count(Patterns{}, captures)) || ...);
	}

	template<typename R, typename... Patterns>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(select<Patterns...>) noexcept {
		return ((regex_count(Patterns{})) || ...);
	}

	template<size_t A, size_t B, typename R, typename... Content>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(repeat<A, B, Content...>, R captures) noexcept {
		regex_count_result ret{ 0ULL, 0ULL };
		if constexpr (sizeof...(Content)) {
			constexpr size_t MinA = A;
			constexpr size_t MaxA = (B) ? (B) : (size_t{0}-1); // 0 in the second part means inf
			ret = regex_count(ctll::list<Content...>(), captures) * regex_count_result{ MinA , MaxA };
		}
		return ret;
	}

	template<size_t A, size_t B, typename... Content>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(repeat<A, B, Content...>) noexcept {
		regex_count_result ret{ 0ULL, 0ULL };
		if constexpr (sizeof...(Content)) {
			constexpr size_t MinA = A;
			constexpr size_t MaxA = (B) ? (B) : (size_t{ 0 } - 1); // 0 in the second part means inf
			ret = regex_count(ctll::list<Content...>()) * regex_count_result { MinA, MaxA };
		}
		return ret;
	}

	template<size_t A, size_t B, typename R, typename... Content>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(lazy_repeat<A, B, Content...>, R captures) noexcept {
		return regex_count(repeat<A, B, Content...>(), captures);
	}

	template<size_t A, size_t B, typename... Content>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(lazy_repeat<A, B, Content...>) noexcept {
		return regex_count(repeat<A, B, Content...>());
	}

	template<size_t A, size_t B, typename R, typename... Content>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(possessive_repeat<A, B, Content...>, R captures) noexcept {
		return regex_count(repeat<A, B, Content...>(), captures);
	}

	template<size_t A, size_t B, typename... Content>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(possessive_repeat<A, B, Content...>) noexcept {
		return regex_count(repeat<A, B, Content...>());
	}
	
	//sink for any atom that shouldn't impact the result
	template<typename Pattern, typename R>
	static constexpr CTRE_FORCE_INLINE regex_count_result regex_count(Pattern, R) noexcept {
		return regex_count_result{0ULL, 0ULL};
	}
}

#endif
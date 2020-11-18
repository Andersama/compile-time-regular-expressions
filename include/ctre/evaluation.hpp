#ifndef CTRE__EVALUATION__HPP
#define CTRE__EVALUATION__HPP

#include "flags_and_modes.hpp"
#include "atoms.hpp"
#include "atoms_unicode.hpp"
#include "starts_with_anchor.hpp"
#include "utility.hpp"
#include "return_type.hpp"
#include "find_captures.hpp"
#include "first.hpp"
#include "dfa_util.hpp"
#include <emmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>
#include <iterator>

// remove me when MSVC fix the constexpr bug
#ifdef _MSC_VER
#ifndef CTRE_MSVC_GREEDY_WORKAROUND
#define CTRE_MSVC_GREEDY_WORKAROUND
#endif
#endif

namespace ctre {

template <size_t Limit> constexpr CTRE_FORCE_INLINE bool less_than_or_infinite(size_t i) {
	if constexpr (Limit == 0) {
		// infinite
		return true;
	} else {
		return i < Limit;
	}
}

template <size_t Limit> constexpr CTRE_FORCE_INLINE bool less_than(size_t i) {
	if constexpr (Limit == 0) {
		// infinite
		return false;
	} else {
		return i < Limit;
	}
}

constexpr bool is_bidirectional(const std::bidirectional_iterator_tag &) { return true; }
constexpr bool is_bidirectional(...) { return false; }

// sink for making the errors shorter
template <typename R, typename Iterator, typename EndIterator> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator, Iterator, const EndIterator, flags, R, ...) noexcept = delete;

// if we found "accept" object on stack => ACCEPT
template <typename R, typename Iterator, typename EndIterator> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator, Iterator, const EndIterator, flags, R captures, ctll::list<accept>) noexcept {
	return captures.matched();
}

// if we found "reject" object on stack => REJECT
template <typename R, typename... Rest, typename Iterator, typename EndIterator> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator, Iterator, const EndIterator, flags, R, ctll::list<reject, Rest...>) noexcept {
	return not_matched;
}

// mark start of outer capture
template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<start_mark, Tail...>) noexcept {
	return evaluate(begin, current, end, f, captures.set_start_mark(current), ctll::list<Tail...>());
}

// mark end of outer capture
template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<end_mark, Tail...>) noexcept {
	return evaluate(begin, current, end, f, captures.set_end_mark(current), ctll::list<Tail...>());
}

// mark end of cycle
template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator, Iterator current, const EndIterator, [[maybe_unused]] const flags & f, R captures, ctll::list<end_cycle_mark>) noexcept {
	if (cannot_be_empty_match(f)) {
		return not_matched;
	}
	
	return captures.set_end_mark(current).matched();
}

// matching everything which behave as a one character matcher

template <typename R, typename Iterator, typename EndIterator, typename CharacterLike, typename... Tail, typename = std::enable_if_t<(MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<Iterator>())>)>> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<CharacterLike, Tail...>) noexcept {
	if (current == end) return not_matched;
	if (!CharacterLike::match_char(*current)) return not_matched;
	
	return evaluate(begin, ++current, end, consumed_something(f), captures, ctll::list<Tail...>());
}

template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<any, Tail...>) noexcept {
	if (current == end) return not_matched;
	
	if (multiline_mode(f)) {
		// TODO add support for different line ending and unicode (in a future unicode mode)
		if (*current == '\n') return not_matched;
	}
	return evaluate(begin, ++current, end, consumed_something(f), captures, ctll::list<Tail...>());
}

// matching strings in patterns

template <typename Iterator> struct string_match_result {
	Iterator position;
	bool match;
};

template <typename CharT, typename Iterator, typename EndIterator> constexpr CTRE_FORCE_INLINE bool compare_character(CharT c, Iterator & it, const EndIterator & end) {
	if (it != end) {
		using char_type = decltype(*it);
		return *it++ == static_cast<char_type>(c);
	}
	return false;
}

struct zero_terminated_string_end_iterator;
template <auto... String, size_t... Idx, typename Iterator, typename EndIterator> constexpr CTRE_FORCE_INLINE string_match_result<Iterator> evaluate_match_string(Iterator current, [[maybe_unused]] const EndIterator end, std::index_sequence<Idx...>) noexcept {
	using char_type = typename std::iterator_traits<Iterator>::value_type;//decltype(*current);
#if __cpp_char8_t >= 201811
	if constexpr (sizeof...(String) && !std::is_same_v<Iterator, utf8_iterator> && is_random_accessible(typename std::iterator_traits<Iterator>::iterator_category{}) && !std::is_same_v<EndIterator, ctre::zero_terminated_string_end_iterator>) {
#else
	if constexpr (sizeof...(String) && is_random_accessible(typename std::iterator_traits<Iterator>::iterator_category{}) && !std::is_same_v<EndIterator, ctre::zero_terminated_string_end_iterator>) {
#endif
		bool same = ((size_t)std::distance(current, end) >= sizeof...(String)) && ((static_cast<char_type>(String) == *(current + Idx)) && ...);
		if (same) {
			return { current += sizeof...(String), same };
		} else {
			return { current, same };
		}
	} else {
		bool same = (compare_character(String, current, end) && ... && true);
		return { current, same };
	}
}

template <typename R, typename Iterator, typename EndIterator, auto... String, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, [[maybe_unused]] const flags & f, R captures, ctll::list<string<String...>, Tail...>) noexcept {
	auto result = evaluate_match_string<String...>(current, end, std::make_index_sequence<sizeof...(String)>());
	
	if (!result.match) {
		return not_matched;
	}
	
	return evaluate(begin, result.position, end, consumed_something(f, sizeof...(String) > 0), captures, ctll::list<Tail...>());
}

// matching select in patterns
template <typename R, typename Iterator, typename EndIterator, typename HeadOptions, typename... TailOptions, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<select<HeadOptions, TailOptions...>, Tail...>) noexcept {
	if (auto r = evaluate(begin, current, end, f, captures, ctll::list<HeadOptions, Tail...>())) {
		return r;
	} else {
		return evaluate(begin, current, end, f, captures, ctll::list<select<TailOptions...>, Tail...>());
	}
}

template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator, Iterator, const EndIterator, flags, R, ctll::list<select<>, Tail...>) noexcept {
	// no previous option was matched => REJECT
	return not_matched;
}

// matching sequence in patterns
template <typename R, typename Iterator, typename EndIterator, typename HeadContent, typename... TailContent, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<sequence<HeadContent, TailContent...>, Tail...>) noexcept {
	if constexpr (sizeof...(TailContent) > 0) {
		return evaluate(begin, current, end, f, captures, ctll::list<HeadContent, sequence<TailContent...>, Tail...>());
	} else {
		return evaluate(begin, current, end, f, captures, ctll::list<HeadContent, Tail...>());
	}
}

// matching empty in patterns
template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<empty, Tail...>) noexcept {
	return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
}

// matching asserts
template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<assert_subject_begin, Tail...>) noexcept {
	if (begin != current) {
		return not_matched;
	}
	return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
}

template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<assert_subject_end, Tail...>) noexcept {
	if (end != current) {
		return not_matched;
	}
	return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
}

template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<assert_subject_end_line, Tail...>) noexcept {
	if (multiline_mode(f)) {
		if (end == current) {
			return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
		} else if (*current == '\n' && std::next(current) == end) {
			return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
		} else {
			return not_matched;
		}
	} else {
		if (end != current) {
			return not_matched;
		}
		return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
	}
}

template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<assert_line_begin, Tail...>) noexcept {
	if (multiline_mode(f)) {
		if (begin == current) {
			return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
		} else if (*std::prev(current) == '\n') {
			return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
		} else {
			return not_matched;
		}
	} else {
		if (begin != current) {
			return not_matched;
		}
		return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
	}
}

template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<assert_line_end, Tail...>) noexcept {
	if (multiline_mode(f)) {
		if (end == current) {
			return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
		} else if (*current == '\n') {
			return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
		} else {
			return not_matched;
		}
	} else {
		if (end != current) {
			return not_matched;
		}
		return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
	}
	
	// TODO properly match line end
	if (end != current) {
		return not_matched;
	}
	return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
}

// matching boundary
template <typename R, typename Iterator, typename EndIterator, typename CharacterLike, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<boundary<CharacterLike>, Tail...>) noexcept {
	
	// reason why I need bidirectional iterators or some clever hack
	bool before = false;
	bool after = false;
	
	static_assert(is_bidirectional(typename std::iterator_traits<Iterator>::iterator_category{}), "To use boundary in regex you need to provide bidirectional iterator or range.");
	
	if (end != current) {
		after = CharacterLike::match_char(*current);
	}
	if (begin != current) {
		before = CharacterLike::match_char(*std::prev(current));
	}
	
	if (before == after) return not_matched;
	
	return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
}

// matching not_boundary
template <typename R, typename Iterator, typename EndIterator, typename CharacterLike, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<not_boundary<CharacterLike>, Tail...>) noexcept {
	
	// reason why I need bidirectional iterators or some clever hack
	bool before = false;
	bool after = false;
	
	static_assert(is_bidirectional(typename std::iterator_traits<Iterator>::iterator_category{}), "To use boundary in regex you need to provide bidirectional iterator or range.");
	
	if (end != current) {
		after = CharacterLike::match_char(*current);
	}
	if (begin != current) {
		before = CharacterLike::match_char(*std::prev(current));
	}
	
	if (before != after) return not_matched;
	
	return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
}

// lazy repeat
template <typename R, typename Iterator, typename EndIterator, size_t A, size_t B, typename... Content, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, [[maybe_unused]] const flags & f, R captures, ctll::list<lazy_repeat<A,B,Content...>, Tail...>) noexcept {

	if constexpr (B != 0 && A > B) {
		return not_matched;
	}
	
	const Iterator backup_current = current;
	
	size_t i{0};
	
	while (less_than<A>(i)) {
		auto outer_result = evaluate(begin, current, end, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());
		
		if (!outer_result) return not_matched;
		
		captures = outer_result.unmatch();
		current = outer_result.get_end_position();
		
		++i;
	}
	
	if (auto outer_result = evaluate(begin, current, end, consumed_something(f, backup_current != current), captures, ctll::list<Tail...>())) {
		return outer_result;
	}
	
	while (less_than_or_infinite<B>(i)) {
		auto inner_result = evaluate(begin, current, end, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());
		
		if (!inner_result) return not_matched;
		
		auto outer_result = evaluate(begin, inner_result.get_end_position(), end, consumed_something(f), inner_result.unmatch(), ctll::list<Tail...>());
		
		if (outer_result) {
			return outer_result;
		}
		
		captures = inner_result.unmatch();
		current = inner_result.get_end_position();
		
		++i;
	}
	
	// rest of regex
	return evaluate(begin, current, end, consumed_something(f), captures, ctll::list<Tail...>());
}

// possessive repeat
template <typename R, typename Iterator, typename EndIterator, size_t A, size_t B, typename... Content, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, [[maybe_unused]] const flags & f, R captures, ctll::list<possessive_repeat<A,B,Content...>, Tail...>) noexcept {

	if constexpr ((B != 0) && (A > B)) {
		return not_matched;
	}
	
	const auto backup_current = current;

	for (size_t i{0}; less_than_or_infinite<B>(i); ++i) {
		// try as many of inner as possible and then try outer once
		auto inner_result = evaluate(begin, current, end, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());
		
		if (!inner_result) {
			if (!less_than<A>(i)) break;
			return not_matched;
		}
		
		captures = inner_result.unmatch();
		current = inner_result.get_end_position();
	}
	
	return evaluate(begin, current, end, consumed_something(f, backup_current != current), captures, ctll::list<Tail...>());
}

// (gready) repeat
template <typename R, typename Iterator, typename EndIterator, size_t A, size_t B, typename... Content, typename... Tail> 
#ifdef CTRE_MSVC_GREEDY_WORKAROUND
constexpr inline void evaluate_recursive(R & result, size_t i, const Iterator begin, Iterator current, const EndIterator end, [[maybe_unused]] const flags & f, R captures, ctll::list<repeat<A,B,Content...>, Tail...> stack) {
#else
constexpr inline R evaluate_recursive(size_t i, const Iterator begin, Iterator current, const EndIterator end, [[maybe_unused]] const flags & f, R captures, ctll::list<repeat<A,B,Content...>, Tail...> stack) {
#endif
	if (less_than_or_infinite<B>(i)) {
		 
		// a*ab
		// aab
		
		if (auto inner_result = evaluate(begin, current, end, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>())) {
			// TODO MSVC issue:
			// if I uncomment this return it will not fail in constexpr (but the matching result will not be correct)
			//  return inner_result
			// I tried to add all constructors to R but without any success
			auto tmp_current = current;
			tmp_current = inner_result.get_end_position();
			#ifdef CTRE_MSVC_GREEDY_WORKAROUND
			evaluate_recursive(result, i+1, begin, tmp_current, end, f, inner_result.unmatch(), stack);
			if (result) {
				return;
			}
			#else
			if (auto rec_result = evaluate_recursive(i+1, begin, tmp_current, end, f, inner_result.unmatch(), stack)) {
				return rec_result;
			}
			#endif
		}
	}
	#ifdef CTRE_MSVC_GREEDY_WORKAROUND
	result = evaluate(begin, current, end, consumed_something(f), captures, ctll::list<Tail...>());
	#else
	return evaluate(begin, current, end, consumed_something(f), captures, ctll::list<Tail...>());
	#endif
}	



// (greedy) repeat 
template <typename R, typename Iterator, typename EndIterator, size_t A, size_t B, typename... Content, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, [[maybe_unused]] const flags & f, R captures, [[maybe_unused]] ctll::list<repeat<A,B,Content...>, Tail...> stack) {

	if constexpr ((B != 0) && (A > B)) {
		return not_matched;
	}

#ifndef CTRE_DISABLE_GREEDY_OPT
	if constexpr (!collides(calculate_first(Content{}...), calculate_first(Tail{}...))) {
		return evaluate(begin, current, end, f, captures, ctll::list<possessive_repeat<A,B,Content...>, Tail...>());
	}
#endif
	
	// A..B
	size_t i{0};
	while (less_than<A>(i)) {
		auto inner_result = evaluate(begin, current, end, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());
		
		if (!inner_result) return not_matched;
		
		captures = inner_result.unmatch();
		current = inner_result.get_end_position();
		
		++i;
	}
	
#ifdef CTRE_MSVC_GREEDY_WORKAROUND
	R result;
	evaluate_recursive(result, i, begin, current, end, f, captures, stack);
	return result;
#else
	return evaluate_recursive(i, begin, current, end, f, captures, stack);
#endif

}

// capture (numeric ID)
template <typename R, typename Iterator, typename EndIterator, size_t Id, typename... Content, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<capture<Id, Content...>, Tail...>) noexcept {
	return evaluate(begin, current, end, f, captures.template start_capture<Id>(current), ctll::list<sequence<Content...>, numeric_mark<Id>, Tail...>());
}

// capture end mark (numeric and string ID)
template <typename R, typename Iterator, typename EndIterator, size_t Id, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<numeric_mark<Id>, Tail...>) noexcept {
	return evaluate(begin, current, end, f, captures.template end_capture<Id>(current), ctll::list<Tail...>());
}

// capture (string ID)
template <typename R, typename Iterator, typename EndIterator, size_t Id, typename Name, typename... Content, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<capture_with_name<Id, Name, Content...>, Tail...>) noexcept {
	return evaluate(begin, current, end, f, captures.template start_capture<Id>(current), ctll::list<sequence<Content...>, numeric_mark<Id>, Tail...>());
}

// backreference support (match agains content of iterators)
template <typename Iterator, typename EndIterator> constexpr CTRE_FORCE_INLINE string_match_result<Iterator> match_against_range(Iterator current, const EndIterator end, Iterator range_current, const Iterator range_end, flags) noexcept {
	while (end != current && range_end != range_current) {
		if (*current == *range_current) {
			current++;
			range_current++;
		} else {
			return {current, false};
		}
	}
	return {current, range_current == range_end};
}

// backreference with name
template <typename R, typename Id, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<back_reference_with_name<Id>, Tail...>) noexcept {
	
	if (const auto ref = captures.template get<Id>()) {
		if (auto result = match_against_range(current, end, ref.begin(), ref.end(), f); result.match) {
			return evaluate(begin, result.position, end, consumed_something(f, ref.begin() != ref.end()), captures, ctll::list<Tail...>());
		}
	}
	return not_matched;
}

// backreference
template <typename R, size_t Id, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<back_reference<Id>, Tail...>) noexcept {
	
	if (const auto ref = captures.template get<Id>()) {
		if (auto result = match_against_range(current, end, ref.begin(), ref.end(), f); result.match) {
			return evaluate(begin, result.position, end, consumed_something(f, ref.begin() != ref.end()), captures, ctll::list<Tail...>());
		}
	}
	return not_matched;
}

// end of lookahead
template <typename R, typename Iterator, typename EndIterator, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator, Iterator, const EndIterator, flags, R captures, ctll::list<end_lookahead_mark>) noexcept {
	// TODO check interaction with non-empty flag
	return captures.matched();
}

// lookahead positive
template <typename R, typename Iterator, typename EndIterator, typename... Content, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<lookahead_positive<Content...>, Tail...>) noexcept {
	
	if (auto lookahead_result = evaluate(begin, current, end, f, captures, ctll::list<sequence<Content...>, end_lookahead_mark>())) {
		captures = lookahead_result.unmatch();
		return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
	} else {
		return not_matched;
	}
}

// lookahead negative
template <typename R, typename Iterator, typename EndIterator, typename... Content, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags & f, R captures, ctll::list<lookahead_negative<Content...>, Tail...>) noexcept {
	
	if (auto lookahead_result = evaluate(begin, current, end, f, captures, ctll::list<sequence<Content...>, end_lookahead_mark>())) {
		return not_matched;
	} else {
		return evaluate(begin, current, end, f, captures, ctll::list<Tail...>());
	}
}

template <typename T>
constexpr bool is_string(T) noexcept {
	return false;
}
template <auto... String>
constexpr bool is_string(string<String...>)noexcept {
	return true;
}

template <typename T>
constexpr bool is_string_like(T) noexcept {
	return false;
}
template <auto... String>
constexpr bool is_string_like(string<String...>) noexcept {
	return true;
}
template <typename CharacterLike, typename = std::enable_if_t<MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<std::string_view::iterator>())>>>
constexpr bool is_string_like(CharacterLike) noexcept {
	return true;
}

template <typename... Content>
constexpr auto extract_leading_string(ctll::list<Content...>) noexcept -> ctll::list<Content...> {
	return {};
}
template <typename... Content>
constexpr auto extract_leading_string(sequence<Content...>) noexcept -> sequence<Content...> {
	return {};
}

//concatenation
template <auto C, auto... String, typename... Content>
constexpr auto extract_leading_string(ctll::list<string<String...>, character<C>, Content...>) noexcept {
	return extract_leading_string(ctll::list<string<String..., C>, Content...>());
}

template <auto... StringA, auto... StringB, typename... Content>
constexpr auto extract_leading_string(ctll::list<string<StringA...>, string<StringB...>, Content...>) noexcept {
	return extract_leading_string(ctll::list<string<StringA..., StringB...>, Content...>());
}
//move things up out of sequences
template <typename... Content, typename... Tail>
constexpr auto extract_leading_string(ctll::list<sequence<Content...>, Tail...>) noexcept {
	return extract_leading_string(ctll::list<Content..., Tail...>());
}

template <typename T, typename... Content, typename... Tail>
constexpr auto extract_leading_string(ctll::list<T, sequence<Content...>, Tail...>) noexcept {
	return extract_leading_string(ctll::list<T, Content..., Tail...>());
}

template <typename... Content>
constexpr auto make_into_sequence(ctll::list<Content...>) noexcept -> sequence<Content...> {
	return{};
}
template <typename... Content>
constexpr auto make_into_sequence(sequence<Content...>) noexcept -> sequence<Content...> {
	return{};
}

//boyer moore utils
template<typename Ty>
constexpr bool is_prefix(Ty* word, size_t wordlen, ptrdiff_t pos) {
	ptrdiff_t suffixlen = wordlen - pos;
	for (int i = 0; i < suffixlen; i++) {
		if (word[i] != word[pos + i]) {
			return false;
		}
	}
	return true;
}

template<typename Ty>
constexpr size_t suffix_length(Ty* word, size_t wordlen, ptrdiff_t pos) {
	size_t i = 0;
	// increment suffix length i to the first mismatch or beginning of the word
	for (; (word[pos - i] == word[wordlen - 1 - i]) && (i < pos); i++);
	return i;
}
//MSVC workaround, array operator[] blows up in face if constexpr, use pointers instead
template<typename Ty, auto... String>
constexpr auto make_delta_2(string<String...>) {
	std::array<Ty, sizeof...(String)> chars{ String... };
	std::array<ptrdiff_t, sizeof...(String)> table;
	constexpr size_t patlen = sizeof...(String);
	size_t p = 0;
	size_t last_prefix_index = patlen - 1;

	for (p = patlen - 1; p < patlen; p--) {
		if (is_prefix(chars.data(), patlen, p + 1)) {
			last_prefix_index = p + 1;
		}
		table.data()[p] = last_prefix_index + (patlen - 1 - p);
	}

	for (p = 0; p < patlen - 1; p++) {
		size_t slen = suffix_length(chars.data(), patlen, p);
		if (chars.data()[p - slen] != chars.data()[patlen - 1 - slen]) {
			table.data()[patlen - 1 - slen] = patlen - 1 - p + slen;
		}
	}

	return table;
}

template <typename Iterator> struct string_search_result {
	Iterator position;
	Iterator end_position;
	bool match;
};

constexpr __m128i CTRE_FORCE_INLINE load_unaligned_m128i(const int8_t* src) {
	__m128i ret;
	if (::std::is_constant_evaluated()) {
		for (size_t i = 0; i < 16; i++)
			ret.m128i_u8[i] = *(src + i);
		return ret;
	} else {
		return ret = _mm_loadu_si128((const __m128i*)src);
	}
}
constexpr __m256i CTRE_FORCE_INLINE load_unaligned_m256i(const int8_t* src) {
	__m256i ret;
	if (::std::is_constant_evaluated()) {
		for (size_t i = 0; i < 31; i++)
			ret.m256i_u8[i] = *(src + i);
		return ret;
	} else {
		return ret = _mm256_loadu_epi8((const __m256i*)src);
	}
}

constexpr uint32_t CTRE_FORCE_INLINE countl_zeros(uint32_t x) noexcept {
	uint32_t n = 0;
	if (x <= 0x0000ffff) n += 16, x <<= 16;
	if (x <= 0x00ffffff) n += 8, x <<= 8;
	if (x <= 0x0fffffff) n += 4, x <<= 4;
	if (x <= 0x3fffffff) n += 2, x <<= 2;
	if (x <= 0x7fffffff) n++;
	return n;
}
constexpr uint32_t CTRE_FORCE_INLINE countl_zeros(uint16_t x) noexcept {
	uint32_t n = 0;
	if (x <= 0x00ff) n += 8, x <<= 8;
	if (x <= 0x0fff) n += 4, x <<= 4;
	if (x <= 0x3fff) n += 2, x <<= 2;
	if (x <= 0x7fff) n++;
	return n;
}
constexpr uint32_t CTRE_FORCE_INLINE countl_zeros(uint8_t x) noexcept {
	uint32_t n = 0;
	if (x <= 0x0f) n += 4, x <<= 4;
	if (x <= 0x3f) n += 2, x <<= 2;
	if (x <= 0x7f) n++;
	return n;
}

constexpr uint32_t CTRE_FORCE_INLINE reverse_bits(uint32_t v) noexcept {
	// swap odd and even bits
	v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
	// swap consecutive pairs
	v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
	// swap nibbles ... 
	v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
	// swap bytes
	v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
	// swap 2-byte long pairs
	v = (v >> 16) | (v << 16);
	return v;
}

template <typename Iterator, typename EndIterator, auto... String>
constexpr CTRE_FORCE_INLINE string_search_result<Iterator> search_for_string(Iterator current, const EndIterator end, string<String...>) noexcept {
#if __cpp_char8_t >= 201811
	//32
	if constexpr (true && sizeof...(String) > 1 && sizeof...(String) < 256 && !std::is_same_v<Iterator, utf8_iterator>) {
#else
	if constexpr (true && sizeof...(String) > 1 && sizeof...(String) < 256) {
#endif
		constexpr auto dfa_info = make_string_search_dfa<Iterator>(string<String...>());
		constexpr std::array<typename ::std::iterator_traits<Iterator>::value_type, sizeof...(String)> chars{ String... };
		//size_t str_size = std::distance(current, end);
		size_t state = 0;
		size_t i = 0;
		string_search_result<Iterator> result{};
		size_t string_left = std::distance(current, end);

		if (!std::is_constant_evaluated()) {
			if constexpr (dfa_info.first.inputs() <= 16) {
				const __m128i state_mask = _mm_set1_epi8(sizeof...(String)-1);
				__m128i states = _mm_set1_epi8(0);
				__m128i shift_states = _mm_set1_epi8(0);
				__m128i transitions[dfa_info.first.states()];
				for (size_t j = 0; j < dfa_info.first.states(); j++) {
					transitions[j] = _mm_set1_epi8(0);
					for (size_t c = 0; c < dfa_info.first.inputs(); c++) {
						transitions[j].m128i_u8[c] = dfa_info.first.table[j][c];
					}
				}
				for (; i < string_left; i++) {
					uint8_t c1 = *(current + i);
					uint8_t e1 = dfa_info.second[c1];
					uint8_t tmp = states.m128i_u8[0];
					states = _mm_bsrli_si128(states, 1);
					states = _mm_shuffle_epi8(transitions[e1], states);
					states.m128i_u8[1] = tmp;
					if (states.m128i_u8[i] >= sizeof...(String)) {
						return result = { current + (i + 1 - sizeof...(String)), current + (i + 1), true };
					}
				}
				return result = { current + string_left, current + string_left, false };
			} else if (dfa_info.first.states() <= 16) {
				const __m128i state_mask = _mm_set1_epi8(sizeof...(String) - 1);
				__m128i states = _mm_set1_epi8(0);
				__m128i transitions[256];
				for (size_t j = 0; j < 256; j++)
					transitions[j] = _mm_set1_epi8(0);
				for (size_t j = 0; j < dfa_info.first.states(); j++) {
					for (size_t c = 0; c < 256; c++) {
						uint8_t e1 = dfa_info.second[c];
						transitions[c].m128i_u8[j] = dfa_info.first.table[j][e1];
					}
				}
				for (; i < string_left; i++) {
					uint8_t c1 = *(current + i);
					states = _mm_shuffle_epi8(transitions[c1], states);
					uint8_t state = _mm_cvtsi128_si32(states);
					if (state >= sizeof...(String)) [[unlikely]] {
						return result = { current + (i + 1 - sizeof...(String)), current + (i + 1), true };
					}
					/*
					uint8_t tmp = states.m128i_u8[0];
					states = _mm_bsrli_si128(states, 1);
					states = _mm_shuffle_epi8(transitions[c1], states);
					states.m128i_u8[1] = tmp;
					if (states.m128i_u8[i] >= sizeof...(String)) {
						return result = { current + (i + 1 - sizeof...(String)), current + (i + 1), true };
					}
					*/
				}
				return result = { current + string_left, current + string_left, false };
			}
#if 0
			const __m128i cmp_mask = _mm_set1_epi8(sizeof...(String) - 1);
			__m128i cmp_mask2{};
			__m128i str_states{};
			__m128i str_chrs{};
			__m128i str_ecs{};
			for (size_t j = 0; j < 16; j++)
				cmp_mask2.m128i_u8[j] = j;
			for (size_t j = 0; j < 16; j++)
				str_states.m128i_u8[j] = j;

			//_mm_srli_si128(_16,1)
			for (; (i + 15) < string_left;) {
				str_chrs = load_unaligned_m128i((const int8_t*)&(*(current + i)));
				__m128i tmask = _mm_add_epi8(_mm_set1_epi8(state), cmp_mask2);
#if 0
				str_ecs = _mm_set_epi8(
					dfa_info.second[str_chrs.m128i_u8[0]],
					dfa_info.second[str_chrs.m128i_u8[1]],
					dfa_info.second[str_chrs.m128i_u8[2]],
					dfa_info.second[str_chrs.m128i_u8[3]],
					dfa_info.second[str_chrs.m128i_u8[4]],
					dfa_info.second[str_chrs.m128i_u8[5]],
					dfa_info.second[str_chrs.m128i_u8[6]],
					dfa_info.second[str_chrs.m128i_u8[7]],
					dfa_info.second[str_chrs.m128i_u8[8]],
					dfa_info.second[str_chrs.m128i_u8[9]],
					dfa_info.second[str_chrs.m128i_u8[10]],
					dfa_info.second[str_chrs.m128i_u8[11]],
					dfa_info.second[str_chrs.m128i_u8[12]],
					dfa_info.second[str_chrs.m128i_u8[13]],
					dfa_info.second[str_chrs.m128i_u8[14]],
					dfa_info.second[str_chrs.m128i_u8[15]]
				);
				str_states = _mm_set_epi8(
					dfa_info.first.table[str_states.m128i_u8[0]][str_ecs.m128i_u8[0]],
					dfa_info.first.table[str_states.m128i_u8[1]][str_ecs.m128i_u8[1]],
					dfa_info.first.table[str_states.m128i_u8[2]][str_ecs.m128i_u8[2]],
					dfa_info.first.table[str_states.m128i_u8[3]][str_ecs.m128i_u8[3]],
					dfa_info.first.table[str_states.m128i_u8[4]][str_ecs.m128i_u8[4]],
					dfa_info.first.table[str_states.m128i_u8[5]][str_ecs.m128i_u8[5]],
					dfa_info.first.table[str_states.m128i_u8[6]][str_ecs.m128i_u8[6]],
					dfa_info.first.table[str_states.m128i_u8[7]][str_ecs.m128i_u8[7]],
					dfa_info.first.table[str_states.m128i_u8[8]][str_ecs.m128i_u8[8]],
					dfa_info.first.table[str_states.m128i_u8[9]][str_ecs.m128i_u8[9]],
					dfa_info.first.table[str_states.m128i_u8[10]][str_ecs.m128i_u8[10]],
					dfa_info.first.table[str_states.m128i_u8[11]][str_ecs.m128i_u8[11]],
					dfa_info.first.table[str_states.m128i_u8[12]][str_ecs.m128i_u8[12]],
					dfa_info.first.table[str_states.m128i_u8[13]][str_ecs.m128i_u8[13]],
					dfa_info.first.table[str_states.m128i_u8[14]][str_ecs.m128i_u8[14]],
					dfa_info.first.table[str_states.m128i_u8[15]][str_ecs.m128i_u8[15]]
				);

#endif
				for (size_t j = 0; j < 16; j++) {
					str_ecs.m128i_u8[j] = dfa_info.second[str_chrs.m128i_u8[j]];
					str_states.m128i_u8[j] = dfa_info.first.table[str_states.m128i_u8[j]][str_ecs.m128i_u8[j]];
				}
#if 0
				for (size_t j = 0; j < 16; j++) {
					str_ecs.m128i_u8[j] = dfa_info.second[str_chrs.m128i_u8[j]];
					/*
					uint8_t hi = str_chrs.m128i_u8[i] >> 4;
					uint8_t lo = str_chrs.m128i_u8[i] & 0xf;
					str_ecs = _mm_insert_epi8(str_ecs, j, _mm_extract_epi8(ecs_table[hi], lo));
					*/
					//str_states = _mm_insert_epi8(str_states, j, _mm_extract_epi8(ecs_table[hi], lo));
					//str_states.m256i_u8[j] = ecs_table[hi][lo];
					//str_states.m128i_u8[j] = _mm_extract_epi8(ecs_table[hi], lo);
				}
				for (size_t j = 0; j < 16; j++) {
					str_states.m128i_u8[j] = dfa_info.table[str_states.m128i_u8[j]][str_ecs[j]];
				}
#endif
				__m128i cmask = _mm_cmpgt_epi8(str_states, cmp_mask);
				__m128i lmask = _mm_cmpgt_epi8(str_states, tmask);
				uint32_t mask0 = _mm_movemask_epi8(cmask);
				uint32_t mask1 = ~_mm_movemask_epi8(lmask);

				uint16_t flipped0 = reverse_bits(mask0) >> 16;
				uint32_t l0s0 = countl_zeros(flipped0);
				uint16_t flipped1 = reverse_bits(mask1) >> 16;
				uint32_t l0s1 = countl_zeros(flipped1);
				if (mask0 && ((l0s0+1) == l0s1)) [[unlikely]] {
					return { current + (i + l0s0 + 1 - sizeof...(String)), current + (i + l0s0 + 1), true };
				} else {
					state = l0s1 < 16 ? str_states.m128i_u8[l0s1] : state + 16;
					str_states = _mm_add_epi8(_mm_set1_epi8(state), cmp_mask2);
					i += l0s1+1;
					continue;
				}	
			}

			/*
			::std::array<uint8_t, 32> vals;

			__m256i str_chrs;
			__m256i str_ecs;
			__m256i str_states;
			//__m256i cmp_mask = _mm_set1_epi8(sizeof...(String) - 1);
			const __m256i cmp_mask = _mm256_set1_epi8(sizeof...(String) - 1);
			const __m256i eq_table = load_unaligned_m256i((const int8_t*)&dfa_info.second);
			
			//::std::array<uint8_t, 16> ecs;
			for (; (i + 31) < string_left;) {
				//str_chrs = load_unaligned(static_cast<typename ::std::iterator_traits<Iterator>::pointer>(current+i));
				str_chrs = load_unaligned_m256i((const int8_t*)&(*(current + i)));
				for (size_t j = 0; j < 32; j++) {
					uint8_t eq = dfa_info.second[str_chrs.m256i_u8[j]];
					state = str_states.m256i_u8[j] = dfa_info.first.table[state][eq];
				}
				__m256i cmask = _mm256_cmpgt_epi8(str_states, cmp_mask);
				uint32_t mask = _mm256_movemask_epi8(cmask);
				if (mask) [[unlikely]] {
					uint32_t flipped = reverse_bits(mask);
					uint32_t l0s = countl_zeros(flipped);
					return { current + (i + l0s + 1 - sizeof...(String)), current + (i + l0s + 1), true };
				} else {
					i += 32;
					continue;
				}
			}
			*/
			/*
			__m128i str_chrs;
			__m128i eq_cls;
			__m128i str_states;
			constexpr __m256i eq_tbl = load_unaligned_m128i((const int8_t*)&dfa_info.second);
			const __m128i cmp_mask = _mm_set1_epi8(sizeof...(String) - 1);
			
			//::std::array<uint8_t, 16> ecs;
			for (; (i + 15) < string_left;) {
				//str_chrs = load_unaligned(static_cast<typename ::std::iterator_traits<Iterator>::pointer>(current+i));
				str_chrs = load_unaligned_m128i((const int8_t*)&(*(current + i)));
				__m128i eqc = _mm256_permutexvar_epi8(str_chrs, eq_tbl);

				for (size_t j = 0; j < 16; j++)
					eq_cls.m128i_u8[j] = dfa_info.second[str_chrs.m128i_u8[j]];
				for (size_t j = 0; j < 16; j++)
					str_states.m128i_u8[j] = state = dfa_info.first.table[state][eq_cls.m128i_u8[j]];

				for (size_t j = 0; j < 16; j++) {
					uint8_t eq = dfa_info.second[str_chrs.m128i_u8[j]];
					state = str_states.m128i_u8[j] = dfa_info.first.table[state][eq];
				}

				__m128i cmask = _mm_cmpgt_epi8(str_states, cmp_mask);
				uint32_t mask = _mm_movemask_epi8(cmask);
				if (mask) [[unlikely]] {
					uint16_t flipped = reverse_bits(mask) >> 16;
					uint32_t l0s = countl_zeros(flipped);
					return { current + (i + l0s + 1 - sizeof...(String)), current + (i + l0s + 1), true };
				} else {
					i += 16;
					continue;
				}
			}
			*/
#endif
		}
		for (; i < string_left; i++) {
			uint8_t val = *(current + i);
			size_t eq = dfa_info.second[val];
			state = dfa_info.first.table[state][eq];
			if (state >= sizeof...(String)) [[unlikely]] {
				return result = { current+(i+1-sizeof...(String)), current+(i+1), true };
			}
		}
		return result = { current+string_left, current+string_left, false };
	}
#if __cpp_char8_t >= 201811
	//32
	else if constexpr (false && sizeof...(String) > 0 && !std::is_same_v<Iterator, utf8_iterator>) {
#else
	else if constexpr (false && sizeof...(String) > 0 ) {
#endif
		constexpr auto dfa_info = make_string_search_dfa<Iterator>(string<String...>());
		constexpr std::array<typename ::std::iterator_traits<Iterator>::value_type, sizeof...(String)> chars{ String... };
		//size_t str_size = std::distance(current, end);
		size_t state = 0;
		string_search_result<Iterator> result;
		std::array<Iterator, sizeof...(String)> prev;
		for (size_t i = 0; i < prev.size(); i++)
			prev[i] = current;

		for (; prev.back() != end;) {
			uint8_t val = *prev.back();
			size_t eq = dfa_info.second[val];
			state = dfa_info.first.table[state][eq];
			Iterator start = prev[0];
			for (size_t i = 0; i < (prev.size() - 1); i++)
				prev[i] = prev[i + 1];
			++prev.back();
			if (state >= sizeof...(String)) {
				return result = { start, prev.back(), true };
			}
		}
		return result = { current, current, false };

	}
#if __cpp_char8_t >= 201811
	else if constexpr (false && sizeof...(String) > 2 && !std::is_same_v<Iterator, utf8_iterator> && is_random_accessible(typename std::iterator_traits<Iterator>::iterator_category{})) {
#else
	else if constexpr (false && sizeof...(String) > 2 && is_random_accessible(typename std::iterator_traits<Iterator>::iterator_category{})) {
#endif
		constexpr std::array<typename ::std::iterator_traits<Iterator>::value_type, sizeof...(String)> chars{ String... };
		constexpr std::array<ptrdiff_t, sizeof...(String)> delta_2 = make_delta_2<typename ::std::iterator_traits<Iterator>::value_type>(string<String...>());

		size_t str_size = std::distance(current, end);
		if (str_size < sizeof...(String)) { //quick exit no way to match
			return { current + str_size, current + str_size, false };
		}

		size_t i = sizeof...(String) - 1; //index over to the starting location
		for (; i < str_size;) {
			size_t j = sizeof...(String) - 1;
			size_t m = i + 1;
			for (; *(current + i) == *(chars.data() + j); --i, --j) { //match string in reverse
				if (j == 0) {
					return { current + i, current + m, true };
				}
			}
			size_t shift = enumeration<String...>::match_char(*(current + i)) ? static_cast<size_t>(*(delta_2.data() + j)) : sizeof...(String);
			i += shift;
		}

		return { current + str_size, current + str_size, false };
	} else if constexpr (sizeof...(String)) {
		//fallback to plain string matching
		constexpr std::array<typename ::std::iterator_traits<Iterator>::value_type, sizeof...(String)> chars{ String... };
		constexpr typename ::std::iterator_traits<Iterator>::value_type first_char = chars.data()[0];
		while (current != end) {
			while (current != end && *current != first_char) {
				current++;
			}
			auto result = evaluate_match_string<String...>(current, end, std::make_index_sequence<sizeof...(String)>());
			if (result.match) {
				return { current, result.position, result.match };
			} else {
				++current;
			}
		}
		return { current, current, false };
	} else {
		return { current, current, true };
	}
}

}

#endif

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

template <auto... String, size_t... Idx, typename Iterator, typename EndIterator> constexpr CTRE_FORCE_INLINE string_match_result<Iterator> evaluate_match_string(Iterator current, [[maybe_unused]] const EndIterator end, std::index_sequence<Idx...>) noexcept {

	bool same = (compare_character(String, current, end) && ... && true);

	return {current, same};
}

template <typename R, typename Iterator, typename EndIterator, auto... String, typename... Tail> 
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, [[maybe_unused]] const flags & f, R captures, ctll::list<string<String...>, Tail...>) noexcept {
	auto result = evaluate_match_string<String...>(current, end, std::make_index_sequence<sizeof...(String)>());
	
	if (!result.match) {
		return not_matched;
	}
	
	return evaluate(begin, result.position, end, consumed_something(f, sizeof...(String) > 0), captures, ctll::list<Tail...>());
}

template <typename R, typename Iterator, typename EndIterator, size_t N, typename... Content, size_t... Idxs>
constexpr CTRE_FORCE_INLINE R evaluate_parallel(const Iterator begin, Iterator current, const EndIterator end, const flags& f, R captures, std::array<bool, N> match_mask, ctll::list<Content...>, std::index_sequence<Idxs...>) noexcept {
	static_assert(((Idxs < N) && ...) && "all indexs must be within bounds");
	bool some_previous_path_matched = ((match_mask[Idxs]) || ...);
	//fallback to serial when we can't step in parallel
	if (some_previous_path_matched) {
		return evaluate(begin, current, end, f, captures, ctll::list<Content...>{});
	} else {
		return not_matched;
	}
}

template <typename R, typename Iterator, typename EndIterator, size_t N, size_t Idx, typename HeadOptions, typename... TailOptions, typename... Tail, size_t... Idxs>
constexpr CTRE_FORCE_INLINE R evaluate_parallel(const Iterator begin, Iterator current, const EndIterator end, const flags& f, R captures, std::array<bool, N> match_mask, ctll::list<select<HeadOptions, TailOptions...>, Tail...>, std::index_sequence<Idx, Idxs...>) noexcept {
	static_assert(((Idxs < N) && ...) && "all indexs must be within bounds");
	using first_pair = decltype(calculate_first_and_trailing(HeadOptions{}));
	using first_element = decltype(first_pair().front);
	using first_trailing = decltype(first_pair().list);
	using char_type = ::std::remove_cvref_t<decltype(*current)>;

	if constexpr (sizeof...(TailOptions)) {
		constexpr bool has_leading_elements =
			(!is_empty_sequence<decltype(ctll::front(first_element{}))>) &&
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).front){}))>) && ...);
		constexpr bool has_same_leading_element =
			((std::is_same_v<first_element, decltype(calculate_first_and_trailing(TailOptions{}).front)>) && ...);

		constexpr bool has_trailing_elements =
			(!is_empty_sequence<decltype(ctll::front(first_trailing{}))>) &&
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).list){}))>) && ...);
		constexpr bool has_same_trailing_elements =
			((std::is_same_v<first_trailing, decltype(calculate_first_and_trailing(TailOptions{}).list)>) && ...);
		constexpr bool has_no_trailing_elements =
			(is_empty_sequence<decltype(ctll::front(first_trailing{}))>) &&
			((is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).list){}))>) && ...);

		constexpr bool leading_elements_are_characters = 
			(is_basic_character(first_element{})) &&
			((is_basic_character(decltype(calculate_first_and_trailing(TailOptions{}).front){})) && ...);
		constexpr bool leading_elements_are_character_like =
			(MatchesCharacter<decltype(ctll::front(first_element{}))>::template value<decltype(*std::declval<::std::string_view::iterator>())>) &&
			((MatchesCharacter<
				decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).front){}))
			>::template value<decltype(*std::declval<::std::string_view::iterator>())>) && ...);

		if constexpr (has_leading_elements && has_same_leading_element && has_same_trailing_elements) {
			//trivial case all select<> branches are the same
			return evaluate_parallel(begin, current, end, f, captures, match_mask, ctll::list<HeadOptions, Tail...>{}, std::index_sequence<Idx, Idxs...>{});
		} else if constexpr (has_leading_elements && has_same_leading_element && has_trailing_elements) {
			//first element is the same, we have more select<> to process
			return evaluate_parallel(begin, current, end, f, captures, match_mask,
			ctll::list<decltype(concatenate_into_sequence(first_element{})),
				select<
					decltype(concatenate_into_sequence(first_trailing{})),
					decltype(concatenate_into_sequence(calculate_first_and_trailing(TailOptions{}).list))...
				>,
				Tail...
			>{}, std::index_sequence<Idx, Idxs...>{});
		} else if constexpr (has_leading_elements && has_same_leading_element && has_no_trailing_elements) {
			return evaluate_parallel(begin, current, end, f, captures, match_mask, ctll::list<
				decltype(concatenate_into_sequence(first_element{})),
				Tail...
			>{}, std::index_sequence<Idx, Idxs...>{});
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_trailing_elements && has_same_trailing_elements) {
			return evaluate_parallel(begin, current, end, f, captures, match_mask, ctll::list<
				set<
					decltype(ctll::front(first_element{})),
					decltype(ctll::front(calculate_first_and_trailing(TailOptions{}).front))...
					//decltype(concatenate_into_sequence(calculate_first_and_trailing(TailOptions{}).list))...
				>,
				decltype(concatenate_into_sequence(first_trailing{})),
				Tail...
			>{}, std::index_sequence<Idx, Idxs...>{});
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_no_trailing_elements) {
			return evaluate_parallel(begin, current, end, f, captures, match_mask, ctll::list<
				set<
					decltype(ctll::front(first_element{})),
					decltype(ctll::front(calculate_first_and_trailing(TailOptions{}).front))...
				>,
				Tail...>{}, std::index_sequence<Idx, Idxs...>{});
		} else if constexpr (leading_elements_are_characters && has_trailing_elements) {
			//match each select branch's leading character (opportunity for sse instructions here)
			match_mask[Idx] = match_mask[Idx] && (current != end) && (*current == static_cast<char_type>(get_basic_character(first_element{})));
			((match_mask[Idxs] = match_mask[Idxs] && (current != end) && (*current == static_cast<char_type>(get_basic_character(calculate_first_and_trailing(TailOptions{}).front)))), ...);
			if (current != end)
				current++;
			return evaluate_parallel(begin, current, end, f, captures, match_mask, 
				ctll::list<select<
						decltype(concatenate_into_sequence(first_trailing{})),
						decltype(concatenate_into_sequence(calculate_first_and_trailing(TailOptions{}).list))...
					>, Tail...>{}, std::index_sequence<Idx, Idxs...>{});
		} else if constexpr (leading_elements_are_character_like && has_trailing_elements) {
			//match each select branch's leading character (opportunity for sse instructions here)
			match_mask[Idx] = match_mask[Idx] && (current != end) && (decltype(ctll::front(first_element{}))::match_char(*current));
			((match_mask[Idxs] = match_mask[Idxs] && (current != end) && (
					decltype(ctll::front(calculate_first_and_trailing(TailOptions{}).front))::match_char(*current)
				)), ...);
			if (current != end)
				current++;
			return evaluate_parallel(begin, current, end, f, captures, match_mask,
				ctll::list<select<
					decltype(concatenate_into_sequence(first_trailing{})),
					decltype(concatenate_into_sequence(calculate_first_and_trailing(TailOptions{}).list))...
				>, Tail...>{},
				std::index_sequence<Idx, Idxs...>{}
			);
		}
	}

	//if possible treat select as character class
	if constexpr (MatchesCharacter<HeadOptions>::template value<decltype(*std::declval<Iterator>())> &&
		((MatchesCharacter<TailOptions>::template value<decltype(*std::declval<Iterator>())>) && ...)) {
		return evaluate_parallel(begin, current, end, f, captures, match_mask, ctll::list<set<HeadOptions, TailOptions...>, Tail...>(), std::index_sequence<Idx, Idxs...>{});
	} else { //otherwise
		bool some_previous_path_matched = (match_mask[Idx] || ((match_mask[Idxs]) || ...));
		if (some_previous_path_matched) {
			if (auto r = evaluate(begin, current, end, f, captures, ctll::list<HeadOptions, Tail...>{})) {
				return r;
			} else {
				return evaluate(begin, current, end, f, captures, ctll::list<select<TailOptions...>, Tail...>{});
			}
		} else {
			return not_matched;
		}
	}
}

template<typename T, size_t N>
constexpr auto make_array(T value) -> std::array<T, N>
{
	std::array<T, N> a{};
	for (auto& x : a)
		x = value;
	return a;
}

template <typename R, typename Iterator, typename EndIterator, typename HeadOptions, typename... TailOptions, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const Iterator begin, Iterator current, const EndIterator end, const flags& f, R captures, ctll::list<select<HeadOptions, TailOptions...>, Tail...>) noexcept {
	using first_pair = decltype(calculate_first_and_trailing(HeadOptions{}));
	using first_element = decltype(first_pair().front);
	using first_trailing = decltype(first_pair().list);
	using char_type = ::std::remove_cvref_t<decltype(*current)>;

	if constexpr (sizeof...(TailOptions)) {
		constexpr bool has_leading_elements =
			(!is_empty_sequence<decltype(ctll::front(first_element{}))>) &&
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).front){}))>) && ...);
		constexpr bool has_same_leading_element =
			((std::is_same_v<first_element, decltype(calculate_first_and_trailing(TailOptions{}).front)>) && ...);

		constexpr bool has_trailing_elements =
			(!is_empty_sequence<decltype(ctll::front(first_trailing{}))>) &&
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).list){}))>) && ...);
		constexpr bool has_same_trailing_elements =
			((std::is_same_v<first_trailing, decltype(calculate_first_and_trailing(TailOptions{}).list)>) && ...);
		constexpr bool has_no_trailing_elements =
			(is_empty_sequence<decltype(ctll::front(first_trailing{}))>) &&
			((is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).list){}))>) && ...);

		constexpr bool leading_elements_are_characters = 
			(is_basic_character(first_element{})) &&
			((is_basic_character(decltype(calculate_first_and_trailing(TailOptions{}).front){})) && ...);
		constexpr bool leading_elements_are_character_like =
			(MatchesCharacter<decltype(ctll::front(first_element{}))>::template value<decltype(*std::declval<::std::string_view::iterator>())>) &&
			((MatchesCharacter<
				decltype(ctll::front(decltype(calculate_first_and_trailing(TailOptions{}).front){}))
			>::template value<decltype(*std::declval<::std::string_view::iterator>())>) && ...);

		if constexpr (has_leading_elements && has_same_leading_element && has_same_trailing_elements) {
			//trivial case all select<> branches are the same
			return evaluate(begin, current, end, f, captures, ctll::list<HeadOptions, Tail...>{});
		} else if constexpr (has_leading_elements && has_same_leading_element && has_trailing_elements) {
			//first element is the same, we have more select<> to process
			return evaluate(begin, current, end, f, captures,
			ctll::list<decltype(concatenate_into_sequence(first_element{})),
				select<
					decltype(concatenate_into_sequence(first_trailing{})),
					decltype(concatenate_into_sequence(calculate_first_and_trailing(TailOptions{}).list))...
				>,
				Tail...
			>{});
		} else if constexpr (has_leading_elements && has_same_leading_element && has_no_trailing_elements) {
			return evaluate(begin, current, end, f, captures, ctll::list<
				decltype(concatenate_into_sequence(first_element{})),
				Tail...
			>{});
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_trailing_elements && has_same_trailing_elements) {
			return evaluate(begin, current, end, f, captures, ctll::list<
				set<
					decltype(ctll::front(first_element{})),
					decltype(ctll::front(calculate_first_and_trailing(TailOptions{}).front))...
					//decltype(concatenate_into_sequence(calculate_first_and_trailing(TailOptions{}).list))...
				>,
				decltype(concatenate_into_sequence(first_trailing{})),
				Tail...
			>{});
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_no_trailing_elements) {
			return evaluate(begin, current, end, f, captures, ctll::list<
				set<
					decltype(ctll::front(first_element{})),
					decltype(ctll::front(calculate_first_and_trailing(TailOptions{}).front))...
				>,
				Tail...
			>{});
		} else if constexpr (leading_elements_are_characters && has_trailing_elements) {
			alignas(32) constexpr std::array<bool, sizeof...(TailOptions) + 1> match_mask = make_array<bool, sizeof...(TailOptions) + 1>(true);
			return evaluate_parallel(begin, current, end, f, captures, match_mask, ctll::list<select<HeadOptions, TailOptions...>, Tail...>{}, std::make_index_sequence<sizeof...(TailOptions) + 1>{});
		} else if constexpr (leading_elements_are_character_like && has_trailing_elements) {
			alignas(32) constexpr std::array<bool, sizeof...(TailOptions) + 1> match_mask = make_array<bool, sizeof...(TailOptions) + 1>(true);
			return evaluate_parallel(begin, current, end, f, captures, match_mask, ctll::list<select<HeadOptions, TailOptions...>, Tail...>{}, std::make_index_sequence<sizeof...(TailOptions) + 1>{});
		}
	}

	//if possible treat select as character class
	if constexpr (MatchesCharacter<HeadOptions>::template value<decltype(*std::declval<Iterator>())> &&
		((MatchesCharacter<TailOptions>::template value<decltype(*std::declval<Iterator>())>) && ...)) {
		return evaluate(begin, current, end, f, captures, ctll::list<set<HeadOptions, TailOptions...>, Tail...>());
	} else { //otherwise
		if (auto r = evaluate(begin, current, end, f, captures, ctll::list<HeadOptions, Tail...>{})) {
			return r;
		} else {
			return evaluate(begin, current, end, f, captures, ctll::list<select<TailOptions...>, Tail...>{});
		}
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


}

#endif

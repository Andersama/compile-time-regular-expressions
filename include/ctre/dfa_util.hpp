#ifndef CTRE__DFA_UTIL__HPP
#define CTRE__DFA_UTIL__HPP
#include <tuple>
#include <span>
#include <array>
#include <algorithm>
#include "evaluation.hpp"
#include "stack_vector.hpp"

namespace ctre {
	template<typename Ty, size_t N, size_t M = 256>
	struct quiet_basic_dfa {
		using state = Ty;
		//reserve additional error state
		std::array<std::array<Ty, M>, N + 1> transitions = {};
		state start_state = 0;
		template<typename Ty>
		constexpr quiet_basic_dfa(std::span<std::tuple<Ty, Ty, Ty>> edges, Ty start_state, Ty default_state) {
			start_state = start_state;
			for (size_t s = 0; s < transitions.size(); s++) {
				for (size_t t = 0; t < transitions[s].size(); t++) {
					transitions[s][t] = default_state;
				}
			}
			//fill error state to loop onto itself
			for (size_t t = 0; t < transitions[N].size(); t++) {
				transitions[N][t] = N;
			}
			for (size_t i = 0; i < edges.size(); i++) {
				//clamp from and to states, (anything wrong -> error state)
				state from = (std::get<0>(edges[i]) < N) ? std::get<0>(edges[i]) : N;
				state to = (std::get<2>(edges[i]) < N) ? std::get<2>(edges[i]) : N;
				if (std::get<1>(edges[i]) < M) //make sure input is valid*
					transitions[from][std::get<1>(edges[i])] = to;
			}
		}
		constexpr state apply(const Ty* ptr, size_t len, state s) {
			size_t i = 0;
			for (; (i + 7) < len; i += 8) {
				Ty c1 = ptr[i];
				Ty c2 = ptr[i + 1];
				Ty c3 = ptr[i + 2];
				Ty c4 = ptr[i + 3];
				Ty c5 = ptr[i + 4];
				Ty c6 = ptr[i + 5];
				Ty c7 = ptr[i + 6];
				Ty c8 = ptr[i + 7];
				s = transitions[s][c1];
				s = transitions[s][c2];
				s = transitions[s][c3];
				s = transitions[s][c4];
				s = transitions[s][c5];
				s = transitions[s][c6];
				s = transitions[s][c7];
				s = transitions[s][c8];
			}
			for (; i < len; ++i) {
				s = transitions[s][data[i]];
			}
			return s;
		}
	};
	/*
	template<typename Ty = uint8_t>
	constexpr std::array<stack_vector::stack_vector<uint8_t, 256>, sizeof(Ty)> make_equivalence_class() {
		
	}
	*/
	template<typename Ty, typename Pattern>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(Pattern) {
		return make_equivalence_class<Ty>(ctll::list<Pattern>());
	}

	template<typename Ty, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(ctll::list<Content...>) {
		//make everything the same equivalence class to start
		stack_vector::stack_vector<Ty, 256> ec{};
		for (size_t i = 0; i < ec.capacity(); i++)
			ec.emplace_back(0);
		((ec = make_equivalence_class(ec, ctll::list<Content>())), ...);
		return ec;
	}

	template<typename Ty>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, stack_vector::stack_vector<uint8_t, 256> equiv_class) {
		stack_vector::stack_vector<Ty, 256> ecs{};
		stack_vector::stack_vector<Ty, 256> new_ecs{};

		stack_vector::stack_vector<Ty, 256> current = previous;
		if (equiv_class.size() && (equiv_class.size() < equiv_class.max_size())) {
			std::sort(equiv_class.begin(), equiv_class.end());
			auto last = std::unique(equiv_class.begin(), equiv_class.end());
			equiv_class.erase(last, equiv_class.end());
			//if we have an equivalence class that would split
			if (equiv_class.size() != equiv_class.capacity()) {
				//make a mapping to collapse the equivalence classes together
				for (size_t i = 0; i < current.size(); i++) {
					size_t tmp = current[i];
					auto it = std::find(ecs.begin(), ecs.end(), tmp);
					size_t ec = std::distance(ecs.begin(), it);
					if (it == ecs.end())
						ecs.emplace_back(tmp);
					current[i] = ec;
				}
				//update the mapping
				for (size_t i = 0; i < ecs.size(); i++)
					ecs[i] = i;
				//add equivalence classes we're missing
				size_t lim = equiv_class.back() + 1;
				size_t last_size = ecs.size();
				for (size_t i = current.size(); i < lim; i++) {
					auto idx = std::find(equiv_class.begin(), equiv_class.end(), i);
					size_t tmp = last_size + (equiv_class.end() == idx);
					auto it = std::find(ecs.begin(), ecs.end(), tmp);
					size_t ec = std::distance(ecs.begin(), it);
					if (it == ecs.end())
						ecs.emplace_back(tmp);
					current.emplace_back(ec);
				}
				size_t last_size_2 = ecs.size();
				//revisit our equivalence classes
				for (size_t j = 0; j < equiv_class.size(); j++) {
					size_t i = equiv_class[j];
					auto it = std::find(new_ecs.begin(), new_ecs.end(), i);
					size_t ec = std::distance(new_ecs.begin(), it) + last_size_2;
					if (it == new_ecs.end())
						new_ecs.emplace_back(i);
					current[i] = ec;
				}
				//reshrink
				ecs.clear();
				for (size_t i = 0; i < current.size(); i++) {
					size_t tmp = current[i];
					auto it = std::find(ecs.begin(), ecs.end(), tmp);
					size_t ec = std::distance(ecs.begin(), it);
					if (it == ecs.end())
						ecs.emplace_back(tmp);
					current[i] = ec;
				}
			}
		}

		return current;
	}

	template<typename Ty, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<sequence<Content...>>) {
		stack_vector::stack_vector<Ty, 256> ec = previous;
		((ec = make_equivalence_class(ec, ctll::list<Content>())), ...);
		return ec;
	}

	template<typename Ty, auto A>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<character<A>>) {
		stack_vector::stack_vector<uint8_t, 256> equiv_class{ A };
		return make_equivalence_class(previous, equiv_class);
	}

	template<typename Ty>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<any>) {
		return previous;
	}

	template<typename Ty, auto... Str>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<string<Str...>>) {
		stack_vector::stack_vector<uint8_t, 256> equiv_class{};
		(equiv_class.emplace_back(Str), ...);
		std::sort(equiv_class.begin(), equiv_class.end());
		auto last = std::unique(equiv_class.begin(), equiv_class.end());
		equiv_class.erase(last, equiv_class.end());
		return make_equivalence_class(previous, equiv_class);
	}

	template<typename Ty, typename CharacterLike, typename = std::enable_if_t<MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<std::string_view::iterator>())>, void>>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<CharacterLike>) {
		stack_vector::stack_vector<uint8_t, 256> equiv_class{};
		for (size_t ch = 0; ch < previous.capacity(); ch++) {
			if (CharacterLike::match_char(ch))
				equiv_class.emplace_back(ch);
		}
		return make_equivalence_class(previous, equiv_class);
	}

	template<typename Ty, size_t A, size_t B, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<repeat<A,B,Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, size_t A, size_t B, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<possessive_repeat<A, B, Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, size_t A, size_t B, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<lazy_repeat<A, B, Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<atomic_group<Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<lookahead_negative<Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<lookahead_positive<Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, size_t Index, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<capture<Index, Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, size_t Index, typename Name, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<capture_with_name<Index, Name, Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty, typename... Content>
	constexpr stack_vector::stack_vector<Ty, 256> make_equivalence_class(stack_vector::stack_vector<Ty, 256> previous, ctll::list<select<Content...>>) {
		return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename Ty>
	constexpr stack_vector::stack_vector<Ty, 256> count_equivalence_classes(stack_vector::stack_vector<Ty, 256> current) {
		stack_vector::stack_vector<Ty, 256> ecs{};
		for (size_t i = 0; i < current.size(); i++) {
			size_t tmp = current[i];
			auto it = std::find(ecs.begin(), ecs.end(), tmp);
			if (it == ecs.end())
				ecs.emplace_back(tmp);
		}
		return ecs;
	}

	//returns the worst case # of states needed to implement this regex
	template<typename... Content>
	constexpr size_t dfa_max_states(ctll::list<Content...>) {
		return ((dfa_max_states(Content())) + ...);
	}

	template<typename... Content>
	constexpr size_t dfa_max_states(sequence<Content...>) {
		return dfa_max_states(ctll::list<Content...>());
	}

	template<size_t A, size_t B, typename... Content>
	constexpr size_t dfa_max_states(repeat<A, B, Content...>) {
		size_t count = ((dfa_max_states(Content())) + ...);
		return count;
		//return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<size_t A, size_t B, typename... Content>
	constexpr size_t dfa_max_states(possessive_repeat<A, B, Content...>) {
		return dfa_max_states(ctll::list<repeat<A, B, Content...>>());
	}

	template<size_t A, size_t B, typename... Content>
	constexpr size_t dfa_max_states(lazy_repeat<A, B, Content...>) {
		return dfa_max_states(ctll::list<repeat<A, B, Content...>>());
	}

	template<typename... Content>
	constexpr size_t dfa_max_states(lookahead_negative<Content...>) {
		return 1;
		//return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename... Content>
	constexpr size_t dfa_max_states(lookahead_positive<Content...>) {
		return 1;
		//return ((previous = make_equivalence_class(previous, ctll::list<Content>())), ...);
	}

	template<typename... Content>
	constexpr size_t dfa_max_states(atomic_group<Content...>) {
		return dfa_max_states(ctll::list<Content...>());
	}

	template<size_t Index, typename... Content>
	constexpr size_t dfa_max_states(capture<Index, Content...>) {
		return dfa_max_states(ctll::list<Content...>());
	}

	template<size_t Index, typename Name, typename... Content>
	constexpr size_t dfa_max_states(capture_with_name<Index, Name, Content...>) {
		return dfa_max_states(ctll::list<Content...>());
	}

	template<typename CharacterLike, typename = std::enable_if_t<MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<std::string_view::iterator>())>, void>>
	constexpr size_t dfa_max_states(CharacterLike) {
		return 1;
	}

	template<auto... Str>
	constexpr size_t dfa_max_states(string<Str...>) {
		return sizeof...(Str);
	}

	//action prototype
	template <typename R, typename Iterator, typename EndIterator>
	constexpr CTRE_FORCE_INLINE R process_action(const Iterator, Iterator prev, Iterator current, const EndIterator, [[maybe_unused]] const flags& f, R captures) {
		return R{};
	}

	template<typename Ty, size_t N, size_t M = 256>
	struct noisy_basic_dfa {
		using state = Ty;
		static constexpr size_t _alphabet_size = M;
		static constexpr size_t _states_size = N;
		//reserve additional error state
		std::array<std::array<Ty, _alphabet_size>, _states_size> table = {};
		std::array<Ty, N> state_partition = {};
		//stack_vector::stack_vector<uint8_t, E> alpha_remap = {};

		state start_state = 0;
		constexpr noisy_basic_dfa() {
			for (size_t s = 0; s < table.size(); s++) {
				for (size_t t = 0; t < table[s].size(); t++) {
					table[s][t] = 0;
				}
			}
		}
		constexpr noisy_basic_dfa(std::span<std::tuple<Ty, Ty, Ty>> edges, std::span<Ty> state_partitions, Ty start_state, Ty default_state) : start_state{ start_state } {
			for (size_t s = 0; s < table.size(); s++) {
				for (size_t t = 0; t < table[s].size(); t++) {
					table[s][t] = default_state;
				}
			}
			//mark partitions
			for (size_t i = 0; i < state_partition.size(); i++)
				state_partition[i] = 0; //default 0 "start / non-final" partition
			Ty p{ 1 };
			for (size_t i = 0; i < state_partitions.size(); i++)
				state_partition[state_partitions[i]] = p++;
			//build dfa
			for (size_t i = 0; i < edges.size(); i++) {
				mark_edge(edges[i]);
			}
		}

		template<typename _st, typename _in>
		constexpr std::pair<state, state> mark_edge(std::tuple<_st, _in, _st> edge) {
			state from = (std::get<0>(edges[i]) < N) ? std::get<0>(edges[i]) : N;
			state to = (std::get<2>(edges[i]) < N) ? std::get<2>(edges[i]) : N;
			if (std::get<1>(edges[i]) < M) { //make sure input is valid*
				transitions[from][std::get<1>(edges[i])] = to;	
			}
			return std::pair<state, state>(from, to);
		}
#if 0
		template<typename Iterator, typename EndIterator>
		constexpr std::pair<state, Iterator> apply(const Iterator begin, const EndIterator end, state s) {
			Iterator current = begin;
			for (; current != end; current++) {
				s = transitions[s][*current];
				if (s < N || state_partition[s]) //if anything but default 0 state
					break;
			}
			return { s, current };
		}
		
		template<typename Iterator, typename EndIterator>
		constexpr std::pair<state, Iterator> apply_with_classes(const Iterator begin, const EndIterator end, state s, const stack_vector::stack_vector<uint8_t> &equiv_class) {
			Iterator current = begin;
			for (; current != end; current++) {
				uint8_t ec = equiv_class[*current];
				s = transitions[s][ec];
				if (s < N || state_partition[s]) //if anything but default 0 state
					break;
			}
			return { s, current };
		}


		constexpr std::pair<state, size_t> apply(const Ty* ptr, size_t len, state s) {
			size_t i = 0;
			/*
			std::array<Ty, 8> chrs;
			std::array<state, 8> states;
			states.back() = start_state;
			for (; (i + (chrs.size() - 1)) < len; i += chrs.size()) {
				const Ty* current = ptr[i];
				for (size_t j = 0; j < chrs.size(); j++)
					chrs[j] = current[j];
				states[0] = transitions[states.back()][chrs.front()];
				for (size_t j = 1; j < ; j++)
					states[i] = transitions[states[i - 1]][chrs[i]];
				std::array<Ty, 8> part;
				for (size_t j = 0; j < chrs.size(); j++)
					part[i] = state_partition[states[i]];
				for (size_t j = 0; j < chrs.size(); j++)
					if (part[i])
						return { states[j], i + j };
			}
			s = states.back();
			*/
			for (; i < len; ++i) {
				s = transitions[s][data[i]];
				if (is_final_state[s])
					break;
			}
			return { s, i };
		}

		template<typename... Pattern>
		constexpr noisy_basic_dfa(ctll::list<Pattern...>) {
			size_t idx = 0;
			start_state = 0;
			alpha_remap = make_equivalence_class(ctll::list<Pattern...>());
			
			//((idx = append_states(Pattern, idx)), ...);
			process_pattern(ctll::list<Pattern...>());
		}

		template<typename... Pattern>
		constexpr size_t append_states(sequence<Pattern...>, size_t idx) {
			((idx = append_state(Pattern, idx)), ...);
			return idx;
		}

		template<typename CharacterLike, typename = std::enable_if_t<MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<std::string_view::iterator>())>, void>>
		constexpr size_t append_states(CharacterLike, size_t idx) {
			if (idx < transitions.size()) {
				for (size_t ch = 0; ch < transitions[0].size(); ch++) {
					transitions[idx][ch] = idx + 1 + (CharacterLike::match_char(ch) ? 1 : 0);
				}
				return idx + 2;
			} else {
				return idx;
			}
		}

		template<size_t A, size_t B, typename... Content>
		constexpr void append_state(repeat<A, B, Content...>, size_t idx) {
			if (idx < transitions.size()) {
				((idx = append_state(Content, idx)), ...);
				return idx;
			}
			else {
				return idx;
			}
		}
#endif
		/*
		states[0] = transitions[states[7]][c1]; //spin
		states[1] = transitions[states[0]][c2];
		states[2] = transitions[states[1]][c3];
		states[3] = transitions[states[2]][c4];
		states[4] = transitions[states[3]][c5];
		states[5] = transitions[states[4]][c6];
		states[6] = transitions[states[5]][c7];
		states[7] = transitions[states[6]][c8];
		if (is_final_state[states[0]] || is_final_state[states[1]] ||
			is_final_state[states[2]] || is_final_state[states[3]] ||
			is_final_state[states[4]] || is_final_state[states[5]] ||
			is_final_state[states[6]] || is_final_state[states[7]]) {
			for (size_t j = 0; j < states.size(); ++j)
				if (is_final_state[states[j]])
					return s = states[j];
		}
		*/
	};

	template<typename Pattern>
	constexpr auto setup_dfa(Pattern) {
		constexpr size_t mx = dfa_max_states(ctll::list<Pattern>());
		constexpr auto ec = make_equivalence_class<uint8_t>(ctll::list<Pattern>());
		constexpr auto ec_count = count_equivalence_classes(ec);
		if constexpr (mx <= ::std::numeric_limits<uint8_t>::max()) {
			return noisy_basic_dfa<uint8_t, mx, ec_count.size()>{};
		} else if (mx <= ::std::numeric_limits<uint16_t>::max()) {
			return noisy_basic_dfa<uint16_t, mx, ec_count.size()>{};
		} else if (mx <= ::std::numeric_limits<uint32_t>::max()) { //how did you manage this?
			return noisy_basic_dfa<uint32_t, mx, ec_count.size()>{};
		} else { //I'd expect your compiler to have exploded well before this point, how you doing?
			return noisy_basic_dfa<uint64_t, mx, ec_count.size()>{};
		}
	}
	/*
	template<typename Pattern>
	constexpr auto make_dfa(Pattern) {
		return make_dfa(begin, it, end, f, return_type{}, ctll::list<Pattern>());
	}
	*/

	template<typename Ty, size_t N, size_t M = 256>
	struct dfa_table {
		using state = Ty;
		Ty table[N][M];
		template<typename In>
		constexpr void set_transition(Ty from, In in, Ty to) noexcept {
			size_t _f = from;
			size_t _i = in;
			size_t _t = to;
			if (_i < N) {
				table[_f][_i] = _t;
			}
		}
		template<typename In, typename Classes>
		constexpr void set_ec_transition(Ty from, In in, Ty to, const Classes& classes) noexcept {
			size_t _f = from;
			size_t _i = in;
			size_t _t = to;
			if (_i < classes.size()) {
				size_t _c = classes[_i];
				if (_c < N) {
					table[_f][_c] = _t;
				}
			}
		}
		constexpr size_t inputs() noexcept {
			return M;
		}
		constexpr size_t states() noexcept {
			return N;
		}

		template<typename In>
		constexpr CTRE_FORCE_INLINE Ty step(Ty from, In in) noexcept {
			size_t _f = from;
			size_t _i = in;
			return table[_f][_i];
		}
	};

	template<auto... String>
	constexpr auto make_string_search_dfa(string<String...>) {
		//auto dfa = setup_dfa(string<String...>());
		constexpr size_t mx = dfa_max_states(ctll::list<string<String...>>());
		constexpr auto ec = make_equivalence_class<uint8_t>(ctll::list<string<String...>>());
		constexpr auto ec_count = count_equivalence_classes(ec);
		if constexpr (mx <= ::std::numeric_limits<uint8_t>::max()) {
			dfa_table<uint8_t, mx, ec_count.size()> dfa{};
			constexpr ::std::array<uint8_t, sizeof...(String)> chars{String...};
			//set the prefix state
			size_t x = 0;
			//loop to self
			for (size_t t = 0; t < dfa.inputs(); t++) {
				dfa.table[0][t] = 0;
			}
			//set first step forward
			dfa.set_ec_transition(0, chars[0], 1, ec);
			for (size_t i = 1; i < dfa.states(); i++) {
				for (size_t t = 0; t < dfa.inputs(); t++) {
					dfa.table[i][t] = dfa.table[x][t];
				}
				size_t eq = ec[chars[i]];
				dfa.set_ec_transition(i, chars[i], i + 1, ec);
				//step the prefix forward
				x = dfa.table[x][eq];
			}

			return std::pair<decltype(dfa), decltype(ec_array)>(dfa, ec_array);
		} else if (mx <= ::std::numeric_limits<uint16_t>::max()) {
			dfa_table<uint16_t, mx, ec_count.size()> dfa{};
			constexpr ::std::array<uint16_t, sizeof...(String)> chars{ String... };
			//set the prefix state
			size_t x = 0;
			//loop to self
			for (size_t t = 0; t < dfa.inputs(); t++) {
				dfa.table[0][t] = 0;
			}
			//set first step forward
			dfa.set_ec_transition(0, chars[0], 1, ec);
			for (size_t i = 1; i < dfa.states(); i++) {
				for (size_t t = 0; t < dfa.inputs(); t++) {
					dfa.table[i][t] = dfa.table[x][t];
				}
				size_t eq = ec[chars[i]];
				dfa.set_ec_transition(i, chars[i], i + 1, ec);
				//step the prefix forward
				x = dfa.table[x][eq];
			}
			return std::pair<decltype(dfa), decltype(ec_array)>(dfa, ec_array);
		} else if (mx <= ::std::numeric_limits<uint32_t>::max()) { //how did you manage this?
			dfa_table<uint32_t, mx, ec_count.size()> dfa{};
			constexpr ::std::array<uint32_t, sizeof...(String)> chars{ String... };
			//set the prefix state
			size_t x = 0;
			//loop to self
			for (size_t t = 0; t < dfa.inputs(); t++) {
				dfa.table[0][t] = 0;
			}
			//set first step forward
			dfa.set_ec_transition(0, chars[0], 1, ec);
			for (size_t i = 1; i < dfa.states(); i++) {
				for (size_t t = 0; t < dfa.inputs(); t++) {
					dfa.table[i][t] = dfa.table[x][t];
				}
				size_t eq = ec[chars[i]];
				dfa.set_ec_transition(i, chars[i], i + 1, ec);
				//step the prefix forward
				x = dfa.table[x][eq];
			}
			return std::pair<decltype(dfa), decltype(ec_array)>(dfa, ec_array);
		} else { //I'd expect your compiler to have exploded well before this point, how you doing?
			dfa_table<uint64_t, mx, ec_count.size()> dfa{};
			constexpr ::std::array<uint64_t, sizeof...(String)> chars{ String... };
			//set the prefix state
			size_t x = 0;
			//loop to self
			for (size_t t = 0; t < dfa.inputs(); t++) {
				dfa.table[0][t] = 0;
			}
			//set first step forward
			dfa.set_ec_transition(0, chars[0], 1, ec);
			for (size_t i = 1; i < dfa.states(); i++) {
				for (size_t t = 0; t < dfa.inputs(); t++) {
					dfa.table[i][t] = dfa.table[x][t];
				}
				size_t eq = ec[chars[i]];
				dfa.set_ec_transition(i, chars[i], i + 1, ec);
				//step the prefix forward
				x = dfa.table[x][eq];
			}
			return std::pair<decltype(dfa), decltype(ec_array)>(dfa, ec_array);
		}
	}

	/*
	template <typename Iterator, typename EndIterator, typename Pattern, typename ResultIterator = Iterator>
	constexpr CTRE_FORCE_INLINE auto match_dfa_re(const Iterator begin, const EndIterator end, Pattern, const flags& f = {}) noexcept {
		//return (begin, begin, end, f, return_type<ResultIterator, Pattern>{}, ctll::list<start_mark, Pattern, assert_end, end_mark, accept>());
	}
	*/
/*
	template <typename Iterator, typename EndIterator, typename Pattern, typename ResultIterator = Iterator>
	constexpr inline auto dfa_starts_with_re(const Iterator begin, const EndIterator end, Pattern) noexcept {
		static const constexpr noisy_basic_dfa dfa{ Pattern };
		return dfa_evaluate(begin, begin, end, return_type<ResultIterator, Pattern>{}, ctll::list<start_mark, Pattern, end_mark, accept>());
	}

	template <typename Iterator, typename EndIterator, typename Pattern, typename ResultIterator = Iterator>
	constexpr inline auto dfa_search_re(const Iterator begin, const EndIterator end, Pattern) noexcept {
		constexpr bool fixed = starts_with_anchor(ctll::list<Pattern>{});

		auto it = begin;
		for (; end != it && !fixed; ++it) {
			if (auto out = dfa_evaluate(begin, it, end, return_type<ResultIterator, Pattern>{}, ctll::list<start_mark, Pattern, end_mark, accept>())) {
				return out;
			}
		}

		// in case the RE is empty or fixed
		return dfa_evaluate(begin, it, end, return_type<ResultIterator, Pattern>{}, ctll::list<start_mark, Pattern, end_mark, accept>());
	}
	*/
}

#endif
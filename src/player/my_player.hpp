#pragma once

#include "core/game.hpp"
#include <array>
#include <vector>

namespace ttt::my_player {

using game::Event;
using game::IPlayer;
using game::Point;
using game::Sign;
using game::State;

class MyPlayer : public IPlayer {
  Sign m_sign = Sign::NONE;
  const char *m_name;

    public:
    struct PosScore {
        int x, y, score;
        bool operator<(const PosScore& other) const { return score > other.score; }
    };

    class SearchBrain {
        Sign root_player;
        Sign opp_player;
        int board_w, board_h;
        int win_len; 

        int cell_state(const State& st, int x, int y, Sign player) const;

        int score_line_pattern(const std::vector<int>& line) const;
        int assess_point(const State& st, int cx, int cy, Sign player) const;
        std::vector<PosScore> generate_moves(const State& st, Sign turn) const;
        int heuristic_board(const State& st, Sign turn) const;
        int negascout(State st, int depth, int alpha, int beta, Sign turn) const;

    public:
        SearchBrain(Sign me, int win_length) 
            : root_player(me), opp_player(me == Sign::X ? Sign::O : Sign::X)
            , board_w(0), board_h(0), win_len(win_length) {}

        Point find_best(const State& st);

        // Публичные методы для тестирования
        int test_score_line_pattern(const std::vector<int>& line) const {
            return score_line_pattern(line);
        }
        int test_assess_point(const State& st, int x, int y, Sign player) const {
            return assess_point(st, x, y, player);
        }
        std::vector<PosScore> test_generate_moves(const State& st, Sign turn) const {
            return generate_moves(st, turn);
        }

        // Обёртки для тестирования приватных методов
        int test_cell_state(const State& st, int x, int y, Sign player) const {
          return cell_state(st, x, y, player);
        }
        
        int test_heuristic_board(const State& st, Sign turn) const {
          return heuristic_board(st, turn);
        }
        
        int test_negascout(State st, int depth, int alpha, int beta, Sign turn) const {
          return negascout(st, depth, alpha, beta, turn);
        }
        
        int get_win_len() const { return win_len; }
    };

    MyPlayer(const char* name) : m_sign(Sign::NONE), m_name(name) {}
    void set_sign(Sign sign) override;
    Point make_move(const State& game) override;
    const char* get_name() const override;
};

}; // namespace ttt::my_player

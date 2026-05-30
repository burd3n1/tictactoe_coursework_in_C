#include "my_player.hpp"
#include <vector>
#include <algorithm>
#include <array>

namespace ttt::my_player {

namespace {
// Структура для хранения потенциальных ходов с оценкой
struct PosScore {
    int x, y, score;
    bool operator<(const PosScore& other) const { return score > other.score; }
};
// Внутренний класс для поиска лучшего хода
class SearchBrain {
    Sign root_player;
    Sign opp_player;
    int board_w, board_h;
// Получение состояния клетки относительно игрока (0 - пусто, 1 - свой, 2 - чужой/стена)
    int cell_state(const State& st, int x, int y, Sign player) const {
        if (x < 0 || x >= board_w || y < 0 || y >= board_h) return 2; // wall
        Sign s = st.get_value(x, y);
        if (s == Sign::NONE) return 0;
        return (s == player) ? 1 : 2;
    }
// Оценка линии из 9 клеток (центр - потенциальный ход)
    int score_line_pattern(const std::array<int, 9>& line) const {
        int w5 = 0, w4 = 0, w3 = 0, w2 = 0;
        for (int start = 0; start <= 4; ++start) {
            int friends = 0;
            bool blocked = false;
            for (int i = 0; i < 5; ++i) {
                if (line[start + i] == 2) { blocked = true; break; }
                if (line[start + i] == 1) friends++;
            }
            if (!blocked) {
                if (friends == 5) w5++;
                else if (friends == 4) w4++;
                else if (friends == 3) w3++;
                else if (friends == 2) w2++;
            }
        }
     // Эвристика на основе количества открытых линий разной длины   
        if (w5 > 0) return 1000000;
        if (w4 >= 2) return 50000;
        if (w4 == 1) return 10000;
        if (w3 >= 2) return 8000;
        if (w3 == 1) return 1000;
        if (w2 >= 2) return 500;
        if (w2 == 1) return 50;
        return 10;
    }
// Оценка позиции для игрока (чем выше - тем лучше)
    int assess_point(const State& st, int cx, int cy, Sign player) const {
        const int steps[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
        int total = 0;
        for (auto& step : steps) {
            std::array<int, 9> line;
            for (int i = -4; i <= 4; ++i) {
                if (i == 0) line[4] = 1;
                else line[i + 4] = cell_state(st, cx + i * step[0], cy + i * step[1], player);
            }
            total += score_line_pattern(line);
        }
        return total;
    }
// Генерация потенциальных ходов с оценкой (только рядом с существующими)
    std::vector<PosScore> generate_moves(const State& st, Sign turn) const {
        Sign opp = (turn == Sign::X) ? Sign::O : Sign::X;
        std::vector<PosScore> moves;
        
        for (int y = 0; y < board_h; ++y) {
            for (int x = 0; x < board_w; ++x) {
                if (st.get_value(x, y) != Sign::NONE) continue;
                
                // Проверяем, есть ли рядом уже занятые клетки (чтобы не рассматривать удаленные ходы)
                bool near = false;
                for (int dy = std::max(0, y - 2); dy <= std::min(board_h - 1, y + 2) && !near; ++dy) {
                    for (int dx = std::max(0, x - 2); dx <= std::min(board_w - 1, x + 2) && !near; ++dx) {
                        if (st.get_value(dx, dy) != Sign::NONE) near = true;
                    }
                }
                // Если рядом есть занятые клетки, оцениваем этот ход
                if (near) {
                    int att = assess_point(st, x, y, turn);
                    int def = assess_point(st, x, y, opp);
                    int priority = att + def;
                    // Если ход создает угрозу 5 в ряд, ставим ему очень высокий приоритет
                    if (def >= 1000000) priority += 2000000;
                    else if (def >= 50000) priority += 100000;
                    
                    moves.push_back({x, y, priority});
                }
            }
        }
        // Сортируем ходы по приоритету и оставляем только топ-8 для дальнейшего поиска
        std::sort(moves.begin(), moves.end());
        if (moves.size() > 8) moves.resize(8);
        return moves;
    }
// Эвристическая оценка всей доски для игрока (чем выше - тем лучше)
    int heuristic_board(const State& st, Sign turn) const {
        Sign opp = (turn == Sign::X) ? Sign::O : Sign::X;
        int diff = 0;
        for (int y = 0; y < board_h; ++y) {
            for (int x = 0; x < board_w; ++x) {
                if (st.get_value(x, y) == Sign::NONE) {
                    bool near = false;
                    for (int dy = std::max(0, y - 2); dy <= std::min(board_h - 1, y + 2) && !near; ++dy) {
                        for (int dx = std::max(0, x - 2); dx <= std::min(board_w - 1, x + 2) && !near; ++dx) {
                            if (st.get_value(dx, dy) != Sign::NONE) near = true;
                        }
                    }
                    if (near) {
                        diff += assess_point(st, x, y, turn) - assess_point(st, x, y, opp);
                    }
                }
            }
        }
        return diff;
    }
// Негаскут с альфа-бета отсечением для поиска лучшего хода
    int negascout(State st, int depth, int alpha, int beta, Sign turn) const {
        if (st.get_status() == game::Status::ENDED) {
            Sign winner = st.get_winner();
            if (winner == turn) return 50000000 + depth;
            if (winner == Sign::NONE) return 0;
            return -50000000 - depth;
        }
        
        if (depth == 0) return heuristic_board(st, turn);
        
        auto moves = generate_moves(st, turn);
        if (moves.empty()) return heuristic_board(st, turn);
        
        Sign next_turn = (turn == Sign::X) ? Sign::O : Sign::X;
        int max_sc = -2000000000;
        
        for (const auto& m : moves) {
            State next_st = st;
            next_st.process_move(turn, m.x, m.y);
            
            // basic negamax recursive path
            int score = -negascout(next_st, depth - 1, -beta, -alpha, next_turn);
            
            max_sc = std::max(max_sc, score);
            alpha = std::max(alpha, score);
            if (alpha >= beta) break;
        }
        return max_sc;
    }
// Конструктор и основной метод для поиска лучшего хода
public:
    SearchBrain(Sign me) : root_player(me), opp_player(me == Sign::X ? Sign::O : Sign::X), board_w(0), board_h(0) {}

    Point find_best(const State& st) {
        board_w = st.get_opts().cols;
        board_h = st.get_opts().rows;
        
        if (st.get_move_no() == 0 && st.get_value(board_w / 2, board_h / 2) == Sign::NONE) {
            return {board_w / 2, board_h / 2};
        }
        
        auto moves = generate_moves(st, root_player);
        if (moves.empty()) {
            for (int y = 0; y < board_h; ++y) {
                for (int x = 0; x < board_w; ++x) {
                    if (st.get_value(x, y) == Sign::NONE) return {x, y};
                }
            }
        }
        
        Point best_p = {moves[0].x, moves[0].y};
        int max_score = -2000000000;
        int alpha = -2000000000;
        int beta = 2000000000;
        
        for (const auto& m : moves) {
            State next_st = st;
            next_st.process_move(root_player, m.x, m.y);
            
            if (next_st.get_status() == game::Status::ENDED && next_st.get_winner() == root_player) {
                return {m.x, m.y};
            }
            
            int score = -negascout(next_st, 2, -beta, -alpha, opp_player);
            
            if (score > max_score) {
                max_score = score;
                best_p = {m.x, m.y};
            }
            alpha = std::max(alpha, score);
        }
        
        return best_p;
    }
};

} // namespace
// Реализация методов MyPlayer
void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

Point MyPlayer::make_move(const State &state) {
    SearchBrain sb(m_sign);
    return sb.find_best(state);
}

} // namespace ttt::my_player

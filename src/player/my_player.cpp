#include "my_player.hpp"
#include <vector>
#include <algorithm>
#include <array>

namespace ttt::my_player {

int MyPlayer::SearchBrain::cell_state(const State& st, int x, int y, Sign player) const {
    if (x < 0 || x >= st.get_opts().cols || y < 0 || y >= st.get_opts().rows) return 2;
    Sign s = st.get_value(x, y);
    if (s == Sign::NONE) return 0;
    return (s == player) ? 1 : 2;
}

// 1. Оценка линии (адаптировано под win_len)
    int MyPlayer::SearchBrain::score_line_pattern(const std::vector<int>& line) const {
    int max_friends = 0;
    
    // Проверяем все возможные окна длины win_len
    for (int start = 0; start <= (int)line.size() - win_len; ++start) {
        int friends = 0;
        bool blocked = false;
        for (int i = 0; i < win_len; ++i) {
            if (line[start + i] == 2) { 
                blocked = true; 
                break; 
            }
            if (line[start + i] == 1) friends++;
        }
        if (!blocked && friends == win_len) {
            return 1000000;
        }
        if (!blocked) {
            max_friends = std::max(max_friends, friends);
        }
    }

    // Эвристика для угрозы: чем больше друзей в ряду, тем выше оценка
    int score = 0;
    int base_score = 10000;
    for (int i = 1; i <= 5 && i < win_len; ++i) {
        if (max_friends >= win_len - i) {
            score = base_score;
            break;
        }
        base_score /= 10;  // 10000 → 1000 → 100 → 10 → 1
    }
    
    return score > 0 ? score : 10;
}

// 2. Оценка точки (насколько она способствует победе)
    int MyPlayer::SearchBrain::assess_point(const State& st, int cx, int cy, Sign player) const {
    const int steps[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    int total = 0;
    
    for (auto& step : steps) {
        // Создаём линию достаточной длины: win_len * 2 - 1
        int line_len = win_len * 2 - 1;
        std::vector<int> line(line_len);
        int center = win_len - 1;
        
        for (int i = 0; i < line_len; ++i) {
            if (i == center) {
                line[i] = 1; // текущая клетка считается принадлежащей player
            } else {
                int offset = i - center;
                line[i] = cell_state(st, cx + offset * step[0], cy + offset * step[1], player);
            }
        }

        total += score_line_pattern(line);

    }
    return total;
}
// 3. Генерация ходов (только рядом с уже занятыми клетками, с приоритетом по оценке)
    std::vector<MyPlayer::PosScore> MyPlayer::SearchBrain::generate_moves(const State& st, Sign turn) const {
        Sign opp = (turn == Sign::X) ? Sign::O : Sign::X;
        std::vector<PosScore> moves;
        int w = st.get_opts().cols;
        int h = st.get_opts().rows;
        
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (st.get_value(x, y) != Sign::NONE) continue;
                
                // Проверяем, есть ли рядом уже занятые клетки (чтобы не рассматривать удаленные ходы)
                bool near = false;
                for (int dy = std::max(0, y - 2); dy <= std::min(h - 1, y + 2) && !near; ++dy) {
                    for (int dx = std::max(0, x - 2); dx <= std::min(w - 1, x + 2) && !near; ++dx) {
                        if (st.get_value(dx, dy) != Sign::NONE) near = true;
                    }
                }
                // Если рядом есть занятые клетки, оцениваем этот ход
                
                if (near) {
                    // 1. Проверяем, приводит ли этот ход к немедленной победе
                    State test_st = st;
                    test_st.process_move(turn, x, y);
                    bool is_winning_move = (test_st.get_status() == game::Status::ENDED && test_st.get_winner() == turn);
                    // 2. Проверяем, блокирует ли этот ход немедленную победу противника
                    State opp_test_st = st;
                    opp_test_st.process_move(opp, x, y);
                    bool blocks_winning_move = (opp_test_st.get_status() == game::Status::ENDED && opp_test_st.get_winner() == opp);
                    int att = assess_point(st, x, y, turn);
                    int def = assess_point(st, x, y, opp);
                    int priority = att + def;
                // 3. Назначаем абсолютные приоритеты
                if (is_winning_move) {
                    priority = 1000000000; // Абсолютный приоритет: своя победа
                } else if (blocks_winning_move) {
                    priority = 500000000;  // Высокий приоритет: блокировка победы противника
                } else if (def >= 10000) {
                    priority += 100000;    // Блокировка серьезной угрозы
                }
    
                moves.push_back({x, y, priority});
                }
            }
        }
        // Сортируем ходы по приоритету и оставляем только топ-8 для дальнейшего поиска
        std::sort(moves.begin(), moves.end());
        if (moves.size() > 8) moves.resize(8);
        return moves;
    }
// 4. Эвристика доски (сумма оценок всех пустых клеток)
    int MyPlayer::SearchBrain::heuristic_board(const State& st, Sign turn) const {
        Sign opp = (turn == Sign::X) ? Sign::O : Sign::X;
        int w = st.get_opts().cols;
        int h = st.get_opts().rows;
        int diff = 0;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (st.get_value(x, y) == Sign::NONE) {
                    bool near = false;
                    for (int dy = std::max(0, y - 2); dy <= std::min(h - 1, y + 2) && !near; ++dy) {
                        for (int dx = std::max(0, x - 2); dx <= std::min(w - 1, x + 2) && !near; ++dx) {
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
// 5. Негаскут с альфа-бета отсечением для поиска лучшего хода(глубина 2)
    int MyPlayer::SearchBrain::negascout(State st, int depth, int alpha, int beta, Sign turn) const {
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

    // 6. Поиск лучшего хода
    Point MyPlayer::SearchBrain::find_best(const State& st) {
    int w = st.get_opts().cols;
    int h = st.get_opts().rows;


    // Первый ход — в центр (если доска свободна)
    if (st.get_move_no() == 0 && st.get_value(w / 2, h / 2) == Sign::NONE) {
        return {w / 2, h / 2};
    }

    auto moves = generate_moves(st, root_player);
    if (moves.empty()) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
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

// 7. Реализация методов MyPlayer
void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

Point MyPlayer::make_move(const State &state) {
    int win_len = state.get_opts().win_len;
    SearchBrain sb(m_sign, win_len);
    return sb.find_best(state);
}

} // namespace ttt::my_player

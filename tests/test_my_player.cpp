#include "player/my_observer.hpp"
#include "player/my_player.hpp"

#ifdef HAS_BASELINE
#include "core/baseline.hpp"
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <chrono>

using namespace ttt;
using namespace ttt::game;
using namespace ttt::my_player;

// ─────────────────────────────────────────────────────────────
// Структура статистики
// ─────────────────────────────────────────────────────────────
struct GameStats {
    int total = 0;
    int wins_my = 0;
    int wins_bot = 0;
    int draws = 0;
};

// Новые тесты для SearchBrain
bool test_score_line_win4() {
    MyPlayer p("Test");
    MyPlayer::SearchBrain sb(Sign::X, 4); // win_len = 4

    std::vector<int> line = {0,0,1,1,1,1,0,0,0}; // 4 подряд — выигрыш!
    int score = sb.test_score_line_pattern(line);
    bool ok = (score == 1000000);
    std::cout << (ok ? "✅" : "❌") << " score_line win4\n";
    return ok;
}

bool test_score_line_threat_win4() {
    MyPlayer p("Test");
    MyPlayer::SearchBrain sb(Sign::X, 4);

    std::vector<int> line = {0,0,1,1,1,0,0,0,0}; // 3 подряд — угроза
    int score = sb.test_score_line_pattern(line);
    bool ok = (score >= 10000); // должно быть высоко
    std::cout << (ok ? "✅" : "❌") << " score_line threat win4 (" << score << ")\n";
    return ok;
}

bool test_assess_center_win3() {
    State::Opts o{}; o.rows = o.cols = 5; o.win_len = 3; o.max_moves = 0;
    State st(o);
    st.process_move(Sign::X, 2, 1);
    st.process_move(Sign::X, 2, 2);

    MyPlayer::SearchBrain sb(Sign::X, 3);
    int score = sb.test_assess_point(st, 2, 3, Sign::X); // завершает вертикаль 3-в-ряд
    bool ok = (score >= 10000);
    std::cout << (ok ? "✅" : "❌") << " assess_center win3 (" << score << ")\n";
    return ok;
}

bool test_generate_moves_respects_winlen() {
    State::Opts o{}; o.rows = o.cols = 10; o.win_len = 6; o.max_moves = 0;
    State st(o);
    st.process_move(Sign::X, 5, 5);

    MyPlayer::SearchBrain sb(Sign::X, 6);
    auto moves = sb.test_generate_moves(st, Sign::X);
    bool ok = !moves.empty();
    std::cout << (ok ? "✅" : "❌") << " generate_moves win6 (count=" << moves.size() << ")\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────
// Тест для cell_state
// ─────────────────────────────────────────────────────────────
bool test_cell_state() {
    State::Opts o{}; o.rows = o.cols = 5; o.win_len = 3; o.max_moves = 0;
    State st(o);
    st.process_move(Sign::X, 2, 2);  // своя клетка для X
    st.process_move(Sign::O, 2, 3);  // клетка противника для X

    MyPlayer::SearchBrain sb(Sign::X, 3);
    
    // Пустая клетка → 0
    bool ok1 = (sb.test_cell_state(st, 0, 0, Sign::X) == 0);
    // Своя клетка → 1
    bool ok2 = (sb.test_cell_state(st, 2, 2, Sign::X) == 1);
    // Клетка противника → 2
    bool ok3 = (sb.test_cell_state(st, 2, 3, Sign::X) == 2);
    // За пределами доски → 2
    bool ok4 = (sb.test_cell_state(st, -1, 0, Sign::X) == 2);
    bool ok5 = (sb.test_cell_state(st, 10, 10, Sign::X) == 2);
    
    bool ok = ok1 && ok2 && ok3 && ok4 && ok5;
    std::cout << (ok ? "✅" : "❌") << " cell_state\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────
// Тест для heuristic_board
// ─────────────────────────────────────────────────────────────
bool test_heuristic_board() {
    State::Opts o{}; o.rows = o.cols = 5; o.win_len = 3; o.max_moves = 0;
    State st(o);
    
    MyPlayer::SearchBrain sb(Sign::X, 3);
    
    // Пустая доска — оценка должна быть около 0
    int score_empty = sb.test_heuristic_board(st, Sign::X);
    
    // Добавим ход для X в центре
    st.process_move(Sign::X, 2, 2);
    int score_after_x = sb.test_heuristic_board(st, Sign::X);
    
    // Оценка должна стать положительной (позиция X улучшилась)
    bool ok = (score_after_x > score_empty);
    std::cout << (ok ? "✅" : "❌") << " heuristic_board (empty=" << score_empty 
              << ", after_x=" << score_after_x << ")\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────
// Тест для negascout
// ─────────────────────────────────────────────────────────────
bool test_negascout() {
    State::Opts o{}; o.rows = o.cols = 5; o.win_len = 3; o.max_moves = 0;
    State st(o);
    
    st.process_move(Sign::X, 2, 1);
    st.process_move(Sign::O, 0, 0);
    st.process_move(Sign::X, 2, 2);
    st.process_move(Sign::O, 0, 1);
    MyPlayer::SearchBrain sb(Sign::X, 3);
    
    // Оцениваем позицию с точки зрения X на глубине 2
    int score = sb.test_negascout(st, 2, -2000000000, 2000000000, Sign::X);
    
    // Оценка должна быть высокой (близкой к выигрышной), так как X может выиграть
    bool ok = (score > 10000);
    std::cout << (ok ? "✅" : "❌") << " negascout (score=" << score << ")\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────
// Тест для find_best (находит ли бот выигрышный ход)
// ─────────────────────────────────────────────────────────────
bool test_find_best_wins() {
    State::Opts o{}; o.rows = o.cols = 5; o.win_len = 3; o.max_moves = 0;
    State st(o);
    
    
    st.process_move(Sign::X, 1, 2);
    st.process_move(Sign::O, 0, 0);
    st.process_move(Sign::X, 2, 2); 
    st.process_move(Sign::O, 0, 1);
    
    if (st.get_value(0, 2) != Sign::NONE || st.get_value(3, 2) != Sign::NONE) {
        std::cout << "❌ find_best_wins: test setup error - target cells not empty\n";
        return false;
    }
    
    MyPlayer p("Test");
    p.set_sign(Sign::X);
    Point m = p.make_move(st);
    
    if (st.get_value(m.x, m.y) != Sign::NONE) {
        std::cout << "❌ find_best_wins: bot tried to play on occupied cell (" << m.x << "," << m.y << ")\n";
        return false;
    }
    
    // Бот должен сделать выигрышный ход: y == 2 и x ∈ {0, 3}
    bool ok = (m.y == 2 && (m.x == 0 || m.x == 3));
    std::cout << (ok ? "✅" : "❌") << " find_best_wins (move=" << m.x << "," << m.y << ")\n";
    return ok;
}

void print_stats(const GameStats& s, const char* my_name, const char* bot_name) {
    std::cout << "\n=== Statistics ===\n";
    std::cout << "Total games:  " << s.total << "\n";
    std::cout << my_name << " wins:  " << s.wins_my << " ("
              << std::fixed << std::setprecision(1) << (100.0 * s.wins_my / s.total) << "%)\n";
    std::cout << bot_name << " wins: " << s.wins_bot << " ("
              << std::fixed << std::setprecision(1) << (100.0 * s.wins_bot / s.total) << "%)\n";
    std::cout << "Draws:        " << s.draws << " ("
              << std::fixed << std::setprecision(1) << (100.0 * s.draws / s.total) << "%)\n";
    
    int decisive = s.wins_my + s.wins_bot;
    double win_rate = decisive > 0 ? (100.0 * s.wins_my / decisive) : 0.0;
    std::cout << "Win Rate (no draws): " << win_rate << "%\n";
    std::cout << "=====================\n";
}

// ─────────────────────────────────────────────────────────────
// Юнит-тесты (проверка работоспособности алгоритма)
// ─────────────────────────────────────────────────────────────
bool test_move_in_bounds() {
    MyPlayer p("Test"); p.set_sign(Sign::X);
    State::Opts o{}; o.rows = o.cols = 20; o.win_len = 5; o.max_moves = 0;
    State st(o);
    Point m = p.make_move(st);
    bool ok = (m.x >= 0 && m.x < 20 && m.y >= 0 && m.y < 20 && st.get_value(m.x, m.y) == Sign::NONE);
    std::cout << (ok ? "✅" : "❌") << " move_in_bounds\n"; return ok;
}

bool test_no_play_on_occupied() {
    MyPlayer p("Test"); p.set_sign(Sign::O);
    State::Opts o{}; o.rows = o.cols = 20; o.win_len = 5; o.max_moves = 0;
    State st(o);
    st.process_move(Sign::X, 10, 10); st.process_move(Sign::O, 10, 11); st.process_move(Sign::X, 11, 10);
    Point m = p.make_move(st);
    bool ok = (st.get_value(m.x, m.y) == Sign::NONE);
    std::cout << (ok ? "✅" : "❌") << " no_play_on_occupied\n"; return ok;
}

bool test_respect_obstacles() {
    MyPlayer p("Test"); p.set_sign(Sign::X);
    State::Opts o{}; o.rows = o.cols = 20; o.win_len = 5; o.max_moves = 0;
    RandomObstaclesFI init(0.75, 50, 1);
    State st(o, &init);
    Point m = p.make_move(st);
    bool ok = (st.get_value(m.x, m.y) == Sign::NONE);
    std::cout << (ok ? "✅" : "❌") << " respect_obstacles\n"; return ok;
}

bool test_move_time_limit() {
    MyPlayer p("Test"); p.set_sign(Sign::X);
    State::Opts o{}; o.rows = o.cols = 20; o.win_len = 5; o.max_moves = 0;
    RandomObstaclesFI init(0.75, 50, 1);
    State st(o, &init);
    for(int i=0; i<2; ++i) { 
        (void)p.make_move(st); 
        if(st.get_status()==Status::ACTIVE){ 
            st.process_move(Sign::X,0,0); 
            if(st.get_status()==Status::ACTIVE) st.process_move(Sign::O,1,0); 
        } 
        st.reset(); 
    }
    auto t0 = std::chrono::high_resolution_clock::now();
    Point m = p.make_move(st);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count();
    bool ok = (ms <= 100 && st.get_value(m.x, m.y) == Sign::NONE);
    std::cout << (ok ? "✅" : "") << " move_time_limit: " << ms << " ms\n"; return ok;
}

bool test_basic_strategy() {
    MyPlayer p("Test"); p.set_sign(Sign::X);
    State::Opts o{}; o.rows = o.cols = 20; o.win_len = 5; o.max_moves = 0;
    State st(o);
    st.process_move(Sign::X, 5, 10); st.process_move(Sign::O, 0, 0);
    st.process_move(Sign::X, 6, 10); st.process_move(Sign::O, 0, 1);
    st.process_move(Sign::X, 7, 10); st.process_move(Sign::O, 0, 2);
    st.process_move(Sign::X, 8, 10);
    Point m = p.make_move(st);
    bool ok = (m.y == 10 && m.x >= 3 && m.x <= 10);
    std::cout << (ok ? "✅" : "⚠️") << " basic_strategy: (" << m.x << "," << m.y << ")\n"; return ok;
}

int run_unit_tests() {
    std::cout << "\n=== Unit Tests ===\n";
    int passed = 0, total = 0;
    auto run = [&](bool (*f)(), const char* n){ 
        total++; 
        try{ if(f()) passed++; }catch(...){ std::cout<<"❌ "<<n<<"\n"; } 
    };
    run(test_move_in_bounds, "bounds");
    run(test_no_play_on_occupied, "occupied");
    run(test_respect_obstacles, "obstacles");
    run(test_move_time_limit, "time");
    run(test_basic_strategy, "strategy");
    run(test_score_line_win4, "score_line_win4");
    run(test_score_line_threat_win4, "score_line_threat_win4");
    run(test_assess_center_win3, "assess_center_win3");
    run(test_generate_moves_respects_winlen, "gen_moves_win6");
    run(test_cell_state, "cell_state");
    run(test_heuristic_board, "heuristic_board");
    run(test_negascout, "negascout");
    run(test_find_best_wins, "find_best_wins");
    std::cout << "=== " << passed << "/" << total << " passed ===\n";
    return passed == total ? 0 : 1;
}

// ─────────────────────────────────────────────────────────────
// Тесты против baseline (статистика)
// ─────────────────────────────────────────────────────────────
#ifdef HAS_BASELINE
void run_vs_baseline(int level, int num_games) {
    std::cout << "Running " << num_games << " games vs Baseline (level " << level << ")...\n";
    GameStats stats{};
    State::Opts opts{}; opts.rows = opts.cols = 20; opts.win_len = 5; opts.max_moves = 0;

    for (int i = 0; i < num_games; ++i) {
        RandomObstaclesFI init(0.75, 50, 1);
        MyPlayer my_bot("MyPlayer");
        
        // ✅ Правильные вызовы согласно baseline.hpp
        ttt::baseline::IPlayer* baseline = nullptr;
        if (level == 0) {
            baseline = ttt::baseline::get_easy_player("BaselineEasy");
        } else {
            baseline = ttt::baseline::get_harder_player("BaselineHard");
        }
        
        Game game(opts, &init);
        game.add_player(Sign::X, &my_bot);
        game.add_player(Sign::O, baseline);  // baseline возвращает сырой указатель

        while (game.process() == MoveResult::OK) {}

        Sign w = game.get_state().get_winner();
        if (w == Sign::X) stats.wins_my++;
        else if (w == Sign::O) stats.wins_bot++;
        else stats.draws++;
        stats.total++;

        if ((i + 1) % 10 == 0) std::cout << "Progress: " << (i + 1) << "/" << num_games << "\n";
    }
    print_stats(stats, "MyPlayer", level == 0 ? "BaselineEasy" : "BaselineHard");
}
#endif

// ────────────────────────────────────────────────────────────
// main
// ────────────────────────────────────────────────────────────
int main(int argc, char *argv[]) {
    std::string mode = "game";
    int num_games = 20;
    int baseline_level = 0;  // 0 = easy, 1 = hard
    unsigned seed = 0;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--test") mode = "test";
        else if (a == "--baseline" || a == "-b") {
            mode = "baseline";
            if (i+1 < argc) baseline_level = std::atoi(argv[++i]);
            if (i+1 < argc) num_games = std::atoi(argv[++i]);
        } else if (a == "--seed" || a == "-s") {
            if (i+1 < argc) seed = std::atoi(argv[++i]);
        } else if (a == "--help" || a == "-h") {
            std::cout << "Usage: " << argv[0] << " [--test] [--baseline [LEVEL] [GAMES]] [--seed S]\n"
                      << "  --test             Run unit tests only\n"
                      << "  --baseline L N     Play N games vs baseline level L (0=easy, 1=hard)\n";
            return 0;
        }
    }
    if (seed > 0) std::srand(seed);

    if (mode == "test") return run_unit_tests();

    if (mode == "baseline") {
    #ifdef HAS_BASELINE
        run_vs_baseline(baseline_level, num_games);
        return 0;
    #else
        std::cerr << "Error: Baseline library not linked.\n"
                  << "Add to tests/CMakeLists.txt:\n"
                  << "  target_compile_definitions(test_my_player PRIVATE HAS_BASELINE)\n"
                  << "Then rebuild: cmake .. -DBUILD_TTTCORE=PREBUILT && make\n";
        return 1;
    #endif
    }

    // -- интерактивная игра ──
    std::cout << "Hello! (Interactive mode)\n";
    State::Opts opts; opts.rows = opts.cols = 20; opts.win_len = 5; opts.max_moves = 0;
    RandomObstaclesFI field_initializer(0.75, 50, 1);
    MyPlayer p1("p1"), p2("p2");
    ConsoleWriter obs;
    Game game(opts, &field_initializer);
    game.add_player(Sign::X, &p1);
    game.add_player(Sign::O, &p2);
    game.add_observer(&obs);
    obs.print_game_state(game.get_state());
    while (game.process() == MoveResult::OK) obs.print_game_state(game.get_state());
    obs.print_game_state(game.get_state());
    return 0;
    
}
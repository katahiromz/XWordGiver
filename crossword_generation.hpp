// (Japanese, UTF-8)
#pragma once

#define CROSSWORD_GENERATION 25 // crossword_generation version

#define _GNU_SOURCE
#include <cstdio>               // 標準入出力。
#include <cstdint>              // 標準整数。
#include <ctime>                // 時間。
#include <cassert>              // assertion。
#include <vector>               // std::vector
#include <unordered_set>        // std::unordered_set
#include <unordered_map>        // std::unordered_map
#include <queue>                // 待ち行列（std::queue）
#include <thread>               // std::thread
#include <mutex>                // std::mutex
#include <atomic>               // std::atomic
#include <algorithm>            // std::shuffle
#include <utility>              // ????
#include <random>               // 新しい乱数生成。
#ifdef _WIN32
    #include <windows.h>        // Windowsヘッダ。
#else
    // Linux/Mac用の定義。
    #include <unistd.h>
    #include <sys/types.h>
    inline uint64_t GetTickCount64(void) {
        using namespace std;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t tick = ts.tv_nsec / 1000000;
        tick += ts.tv_sec * 1000;
        return tick;
    }
    inline unsigned GetCurrentThreadId(void) {
        using namespace std;
        return gettid();
    }
#endif

// マスの位置を定義する。
namespace crossword_generation {
    struct pos_t {
        int m_x, m_y;
        constexpr pos_t(int x, int y) noexcept : m_x(x), m_y(y) { }
        constexpr bool operator==(const pos_t& pos) const noexcept {
            return m_x == pos.m_x && m_y == pos.m_y;
        }
    };
} // namespace crossword_generation

namespace std {
    template <>
    struct hash<crossword_generation::pos_t> {
        size_t operator()(const crossword_generation::pos_t& pos) const noexcept {
            return static_cast<uint16_t>(pos.m_x) | (static_cast<uint16_t>(pos.m_y) << 16);
        }
    };
} // namespace std

// クロスワード生成用の名前空間。現状は「単語群から自動生成」のみ実装。
// 将来的にはその他の生成方法もこのようなモダンな方向に移行する。
namespace crossword_generation {
inline static std::atomic<bool> s_generated{false};     // 生成済みか？
inline static std::atomic<bool> s_canceled{false};      // キャンセルされたか？
inline static std::mutex s_mutex;           // ミューテックス（排他処理用）。

// ルールを表すフラグ群。
struct RULES {
    enum {
        DONTDOUBLEBLACK = (1 << 0),     // 連黒禁。
        DONTCORNERBLACK = (1 << 1),     // 四隅黒禁。
        DONTTRIDIRECTIONS = (1 << 2),   // 三方黒禁。
        DONTDIVIDE = (1 << 3),          // 分断禁。
        DONTFOURDIAGONALS = (1 << 4),   // 黒斜四連禁。
        POINTSYMMETRY = (1 << 5),       // 黒マス点対称。
        DONTTHREEDIAGONALS = (1 << 6),  // 黒斜三連禁。
        LINESYMMETRYV = (1 << 7),       // 黒マス上下対称。
        LINESYMMETRYH = (1 << 8),       // 黒マス左右対称。
    };
};

// 文字マスか？
template <typename t_char>
inline bool is_letter(t_char ch) noexcept {
    return (ch != '#' && ch != '?');
}

// プロセッサの数を返す関数。
inline uint32_t get_num_processors(void) noexcept {
#ifdef XWORDGIVER
    return xg_dwThreadCount;
#elif defined(_WIN32)
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
#else
    return std::thread::hardware_concurrency();
#endif
}

// replacement of std::random_shuffle
// 従来型の乱数生成（std::rand、std::random_shuffle）は推奨されない。
template <typename t_elem>
inline void random_shuffle(const t_elem& begin, const t_elem& end) {
#ifndef NO_RANDOM
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(begin, end, g);
#endif
}

// 生成前に初期化。
inline void reset() noexcept {
    s_generated = s_canceled = false;
#ifdef XWORDGIVER
    for (auto& info : xg_aThreadInfo) {
        info.m_count = 0;
    }
#endif
}

// 連結性を判定。
template <typename t_string>
inline bool
check_connectivity(const std::unordered_set<t_string>& words,
                   t_string& nonconnected)
{
    if (words.size() <= 1)
        return true;

    std::vector<t_string> vec_words(words.begin(), words.end()); // 単語群。
    std::queue<size_t> queue; // 待ち行列。
    std::unordered_set<size_t> indexes;
    queue.emplace(0); // 待ち行列に初期の種を添える。。

    while (!queue.empty()) {
        const size_t index0 = queue.front();
        indexes.insert(index0);
        queue.pop(); // 種を取り除く。

        auto& w0 = vec_words[index0];
        for (size_t index1 = 0; index1 < vec_words.size(); ++index1) {
            if (index0 == index1)
                continue;

            auto& w1 = vec_words[index1];
            for (auto ch0 : w0) {
                for (auto ch1 : w1) {
                    if (ch0 == ch1) {
                        if (indexes.count(index1) == 0) {
                            queue.emplace(index1);
                            goto skip;
                        }
                    }
                }
            }
skip:;
        }
    }

    for (size_t i = 0; i < vec_words.size(); ++i) {
        if (indexes.count(i) == 0) {
            nonconnected = vec_words[i];
            return false;
        }
    }

    return true;
}

// 候補。位置情報と単語と縦横の向きの情報を持つ。
template <typename t_string>
struct candidate_t {
    int m_x, m_y;
    t_string m_word;
    bool m_vertical;
};

// 交差候補。
template <typename t_string>
struct cross_candidate_t {
    candidate_t<t_string> m_cand_x, m_cand_y;
    cross_candidate_t() {
        m_cand_x.m_vertical = false;
        m_cand_y.m_vertical = true;
    }
};

// 盤面データ。中身は文字列。配置方法はサブクラス board_t によって決まる。
template <typename t_string>
struct board_data_t {
    using t_char = typename t_string::value_type;
    t_string m_data;

    // コンストラクタによる初期化。文字で埋める。
    board_data_t(int cx = 1, int cy = 1, t_char ch = ' ') {
        resize(cx, cy, ch);
    }

    // マス数。
    int size() const noexcept {
        return static_cast<int>(m_data.size());
    }

    // マス数の増減。
    void resize(int cx, int cy, t_char ch = ' ') {
        m_data.assign(cx * cy, ch);
    }

    // 特定の文字で埋める。
    void fill(t_char ch = ' ') {
        std::fill(m_data.begin(), m_data.end(), ch);
    }

    // 文字を置換により置き換える。
    void replace(t_char chOld, t_char chNew) {
        std::replace(m_data.begin(), m_data.end(), chOld, chNew);
    }

    // 特定の文字の個数を数える。
    int count(t_char ch) const noexcept {
        int ret = 0;
        for (int xy = 0; xy < size(); ++xy) {
            if (m_data[xy] == ch)
                ++ret;
        }
        return ret;
    }

    // 空か？
    bool is_empty() const noexcept {
        return count('?') == size();
    }
    // 未知のマスがないか？
    bool is_full() const noexcept {
        return count('?') == 0;
    }
    // 文字マスがあるか。
    bool has_letter() const noexcept {
        for (int xy = 0; xy < size(); ++xy) {
            if (is_letter(m_data[xy]))
                return true;
        }
        return false;
    }
};

// board_data_t のサブクラス。盤面データ。継承されて文字の配置が規定されている。
template <typename t_string, bool t_fixed>
struct board_t : board_data_t<t_string> {
    using t_char = typename t_string::value_type;
    using self_type = board_t<t_string, t_fixed>;
    using super_type = board_data_t<t_string>;

    int m_cx, m_cy; // サイズ。
    int m_rules; // ルール群。
    int m_x0, m_y0;

    // board_tのコンストラクタ。
    board_t(int cx = 1, int cy = 1, t_char ch = ' ', int rules = 0, int x0 = 0, int y0 = 0) noexcept
        : super_type(cx, cy, ch), m_cx(cx), m_cy(cy)
        , m_rules(rules), m_x0(x0), m_y0(y0)
    {
    }
    board_t(const self_type& b) = default;
    self_type& operator=(const self_type& b) = default;

    // インデックス位置の文字を返す。
    t_char get(int xy) const {
        if (0 <= xy && xy < this->size())
            return super_type::m_data[xy];
        return t_fixed ? '#' : '?';
    }
    // インデックス位置に文字をセット。
    void set(int xy, t_char ch) {
        if (0 <= xy && xy < this->size())
            super_type::m_data[xy] = ch;
    }

    // マス(x, y)は盤面の範囲内か？
    // x, y: absolute coordinate
    bool in_range(int x, int y) const noexcept {
        return (0 <= x && x < m_cx && 0 <= y && y < m_cy);
    }

    // マス(x, y)を取得する。範囲チェックあり。
    // x, y: absolute coordinate
    t_char real_get_at(int x, int y) const noexcept {
        if (in_range(x, y))
            return super_type::m_data[y * m_cx + x];
        return ' ';
    }
    // マス(x, y)を取得する。範囲チェックあり。範囲外なら'#'か'?'を返す。
    // x, y: absolute coordinate
    t_char get_at(int x, int y) const noexcept {
        if (in_range(x, y))
            return super_type::m_data[y * m_cx + x];
        return t_fixed ? '#' : '?';
    }
    // マス(x, y)をセットする。範囲チェックあり。範囲外なら無視。
    // x, y: absolute coordinate
    void set_at(int x, int y, t_char ch) noexcept {
        if (in_range(x, y))
            super_type::m_data[y * m_cx + x] = ch;
    }
    // マス(x, y)をセットする。ただしルールにより反射する。範囲チェックあり。範囲外なら無視。
    // x, y: absolute coordinate
    void mirror_set_black_at(int x, int y) {
        if (!in_range(x, y))
            return;
        set_at(x, y, '#');
        if (m_rules == 0) {
            return;
        }
        if (m_rules & RULES::POINTSYMMETRY) {
            set_at(m_cx - (x + 1), m_cy - (y + 1), '#');
        } else if (m_rules & RULES::LINESYMMETRYV) {
            set_at(m_cx - (x + 1), y, '#');
        } else if (m_rules & RULES::LINESYMMETRYH) {
            set_at(x, m_cy - (y + 1), '#');
        }
    }

    // ルールにより黒マスを反射する。
    void do_mirror() {
        if (m_rules == 0)
            return;
        if (m_rules & RULES::POINTSYMMETRY) {
            for (int y = 0; y < m_cy; ++y) {
                for (int x = 0; x < m_cx; ++x) {
                    if (get_at(x, y) == '#') {
                        set_at(m_cx - (x + 1), m_cy - (y + 1), '#');
                    }
                }
            }
        } else if (m_rules & RULES::LINESYMMETRYV) {
            for (int y = 0; y < m_cy; ++y) {
                for (int x = 0; x < m_cx; ++x) {
                    if (get_at(x, y) == '#') {
                        set_at(m_cx - (x + 1), y, '#');
                    }
                }
            }
        } else if (m_rules & RULES::LINESYMMETRYH) {
            for (int y = 0; y < m_cy; ++y) {
                for (int x = 0; x < m_cx; ++x) {
                    if (get_at(x, y) == '#') {
                        set_at(x, m_cy - (y + 1), '#');
                    }
                }
            }
        }
    }

    // マス(x, y)から横向きパターンを取得する。
    // x, y: absolute coordinate
    t_string get_pat_x(int x, int y, int *px0 = nullptr) const {
        t_string pat;
        if (!in_range(x, y) || get_at(x, y) == '#')
            return pat;

        int x0, x1;
        x0 = x1 = x;
        while (get_at(x0 - 1, y) != '#') {
            --x0;
        }
        while (get_at(x1 + 1, y) != '#') {
            ++x1;
        }

        for (int x2 = x0; x2 <= x1; ++x2) {
            pat += get_at(x2, y);
        }
        if (px0)
            *px0 = x0;
        return pat;
    }

    // マス(x, y)から縦向きパターンを取得する。
    // x, y: absolute coordinate
    t_string get_pat_y(int x, int y, int *py0 = nullptr) const {
        t_string pat;
        if (!in_range(x, y) || get_at(x, y) == '#')
            return pat;

        int y0, y1;
        y0 = y1 = y;
        while (get_at(x, y0 - 1) != '#') {
            --y0;
        }
        while (get_at(x, y1 + 1) != '#') {
            ++y1;
        }

        for (int y2 = y0; y2 <= y1; ++y2) {
            pat += get_at(x, y2);
        }
        if (py0)
            *py0 = y0;
        return pat;
    }

    // マス(x, y)は、コーナー（四隅）か？
    bool is_corner(int x, int y) const {
        if (y == 0 || y == m_cy - 1) {
            if (x == 0 || x == m_cx - 1)
                return true;
        }
        return false;
    }

    // マス(x, y)に黒マスを置くと、連黒禁に抵触するか？
    bool can_make_double_black(int x, int y) const {
        return (real_get_at(x - 1, y) == '#' || real_get_at(x + 1, y) == '#' ||
                real_get_at(x, y - 1) == '#' || real_get_at(x, y + 1) == '#');
    }

    bool can_make_tri_direction(int x, int y) {
        auto ch = get_at(x, y);
        set_at(x, y, '#');
        bool ret = false;
        for (int i = y - 4; i <= y + 4; ++y) {
            for (int j = x - 4; i <= x + 4; ++i) {
                int sum = 0;
                sum += (real_get_at(j, i - 1) == '#');
                sum += (real_get_at(j, i + 1) == '#');
                sum += (real_get_at(j - 1, i) == '#');
                sum += (real_get_at(j + 1, i) == '#');
                if (sum >= 3) {
                    ret = true;
                    goto skip;
                }
            }
        }
skip:;
        set_at(x, y, ch);
        return ret;
    }

    // マス(x, y)に黒マスがあるとき、黒斜三連禁に抵触するか？
    bool can_make_three_diagonals(int x, int y) const {
        // center (right down)
        if (real_get_at(x - 1, y - 1) == '#' || real_get_at(x + 1, y + 1) == '#') {
            return true;
        }
        // center (right up)
        if (real_get_at(x + 1, y - 1) == '#' || real_get_at(x - 1, y + 1) == '#') {
            return true;
        }
        // upper left
        if (real_get_at(x - 2, y - 2) == '#' || real_get_at(x - 1, y - 1) == '#') {
            return true;
        }
        // lower right
        if (real_get_at(x + 1, y + 1) == '#' || real_get_at(x + 2, y + 2) == '#') {
            return true;
        }
        // upper right
        if (real_get_at(x + 2, y - 2) == '#' || real_get_at(x + 1, y - 1) == '#') {
            return true;
        }
        // lower left
        if (real_get_at(x - 2, y + 1) == '#' || real_get_at(x - 1, y + 2) == '#') {
            return true;
        }
        return false;
    }

    // マス(x, y)に黒マスがあるとき、黒斜四連禁に抵触するか？
    bool can_make_four_diagonals(int x, int y) {
        auto ch = get_at(x, y);
        set_at(x, y, '#'); // 一時的にセット。後で戻す。
        bool ret = false;
        for (int i = y - 3; i <= y + 3; ++y) {
            for (int j = x - 3; i <= x + 3; ++i) {
                if (real_get_at(j, i) != '#')
                    continue;
                if (real_get_at(j + 1, i + 1) != '#')
                    continue;
                if (real_get_at(j + 2, i + 2) != '#')
                    continue;
                if (real_get_at(j + 3, i + 3) != '#')
                    continue;
                ret = true;
                goto skip;
            }
        }
        for (int i = y - 3; i <= y + 3; ++y) {
            for (int j = x - 3; i <= x + 3; ++i) {
                if (real_get_at(j, i) != '#')
                    continue;
                if (real_get_at(j - 1, i + 1) != '#')
                    continue;
                if (real_get_at(j - 2, i + 2) != '#')
                    continue;
                if (real_get_at(j - 3, i + 3) != '#')
                    continue;
                ret = true;
                goto skip;
            }
        }
skip:;
        set_at(x, y, ch); // 元に戻す。
        return ret;
    }

    // マス(x, y)に黒マスをセットできるかどうか？
    // NOTE: This method doesn't check divided_by_black.
    bool can_set_black_at(int x, int y) {
        if (get_at(x, y) == '#')
            return true;
        if (!in_range(x, y) || get_at(x, y) != '?')
            return false;
        if (m_rules == 0) {
            return true;
        }

        if (m_rules & RULES::DONTCORNERBLACK) {
            if (is_corner(x, y))
                return false;
        }

        if (m_rules & RULES::DONTDOUBLEBLACK) {
            if (m_rules & RULES::POINTSYMMETRY) {
                if (can_make_double_black(x, y) ||
                    can_make_double_black(m_cx - (x + 1), m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYV) {
                if (can_make_double_black(x, y) ||
                    can_make_double_black(x, m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYH) {
                if (can_make_double_black(x, y) ||
                    can_make_double_black(m_cx - (x + 1), y))
                {
                    return false;
                }
            } else {
                if (can_make_double_black(x, y))
                    return false;
            }
        }

        if (m_rules & RULES::DONTTRIDIRECTIONS) {
            if (m_rules & RULES::POINTSYMMETRY) {
                if (can_make_tri_direction(x, y) ||
                    can_make_tri_direction(m_cx - (x + 1), m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYV) {
                if (can_make_tri_direction(x, y) ||
                    can_make_tri_direction(x, m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYH) {
                if (can_make_tri_direction(x, y) ||
                    can_make_tri_direction(m_cx - (x + 1), y))
                {
                    return false;
                }
            } else {
                if (can_make_tri_direction(x, y))
                    return false;
            }
        }

        if (m_rules & RULES::DONTTHREEDIAGONALS) {
            if (m_rules & RULES::POINTSYMMETRY) {
                if (can_make_three_diagonals(x, y) ||
                    can_make_three_diagonals(m_cx - (x + 1), m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYV) {
                if (can_make_three_diagonals(x, y) ||
                    can_make_three_diagonals(x, m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYH) {
                if (can_make_three_diagonals(x, y) ||
                    can_make_three_diagonals(m_cx - (x + 1), y))
                {
                    return false;
                }
            } else {
                if (can_make_three_diagonals(x, y))
                    return false;
            }
        } else if (m_rules & RULES::DONTFOURDIAGONALS) {
            if (m_rules & RULES::POINTSYMMETRY) {
                if (can_make_four_diagonals(x, y) ||
                    can_make_four_diagonals(m_cx - (x + 1), m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYV) {
                if (can_make_four_diagonals(x, y) ||
                    can_make_four_diagonals(x, m_cy - (y + 1)))
                {
                    return false;
                }
            } else if (m_rules & RULES::LINESYMMETRYH) {
                if (can_make_four_diagonals(x, y) ||
                    can_make_four_diagonals(m_cx - (x + 1), y))
                {
                    return false;
                }
            } else {
                if (can_make_four_diagonals(x, y))
                    return false;
            }
        }

        if (m_rules & RULES::POINTSYMMETRY) {
            auto ch = real_get_at(m_cx - (x + 1), m_cy - (y + 1));
            if (is_letter(ch))
                return false;
        } else if (m_rules & RULES::LINESYMMETRYV) {
            auto ch = real_get_at(x, m_cy - (y + 1));
            if (is_letter(ch))
                return false;
        } else if (m_rules & RULES::LINESYMMETRYH) {
            auto ch = real_get_at(m_cx - (x + 1), y);
            if (is_letter(ch))
                return false;
        } else {
            ;
        }

        return true;
    }

    // 盤面が固定サイズならサイズに収まるかどうか判定し、
    // 盤面が固定サイズでなければ、収まるように拡張する。
    // x: relative coordinate
    bool ensure_x(int x) {
        if (t_fixed) {
            return (0 <= x && x < m_cx);
        } else {
            if (x < m_x0) {
                grow_x0(m_x0 - x, '?');
            } else if (m_cx + m_x0 <= x) {
                grow_x1(x - (m_cx + m_x0) + 1, '?');
            }
            return true;
        }
    }
    // 盤面が固定サイズならサイズに収まるかどうか判定し、
    // 盤面が固定サイズでなければ、収まるように拡張する。
    // y: relative coordinate
    bool ensure_y(int y) {
        if (t_fixed) {
            return (0 <= y && y < m_cy);
        } else {
            if (y < m_y0) {
                grow_y0(m_y0 - y, '?');
            } else if (m_cy + m_y0 <= y) {
                grow_y1(y - (m_cy + m_y0) + 1, '?');
            }
            return true;
        }
    }
    // 盤面が固定サイズならサイズに収まるかどうか判定し、
    // 盤面が固定サイズでなければ、収まるように拡張する。
    // x, y: relative coordinate
    bool ensure(int x, int y) {
        if (t_fixed) {
            return in_range(x, y);
        } else {
            ensure_x(x);
            ensure_y(y);
            return true;
        }
    }

    // マス(x, y)の文字を取得する。リリース時の範囲チェックなし。
    // x, y: relative coordinate
    t_char get_on(int x, int y) const noexcept {
        assert(m_x0 <= 0);
        assert(m_y0 <= 0);
        return get_at(x - m_x0, y - m_y0);
    }
    // マス(x, y)の文字をセットする。リリース時の範囲チェックなし。
    // x, y: relative coordinate
    void set_on(int x, int y, t_char ch) noexcept {
        assert(m_x0 <= 0);
        assert(m_y0 <= 0);
        set_at(x - m_x0, y - m_y0, ch);
    }

    // 列を挿入する。指定された文字と幅で。
    // x0: absolute coordinate
    void insert_x(int x0, int cx = 1, t_char ch = ' ') {
        assert(0 <= x0 && x0 <= m_cx);

        board_t<t_string, t_fixed> data(m_cx + cx, m_cy, ch);

        for (int y = 0; y < m_cy; ++y) {
            for (int x = 0; x < m_cx; ++x) {
                auto ch0 = get_at(x, y);
                if (x < x0)
                    data.set_at(x, y, ch0);
                else
                    data.set_at(x + cx, y, ch0);
            }
        }

        this->m_data = std::move(data.m_data);
        m_cx += cx;
    }

    // 行を挿入する。指定された文字と高さで。
    // y0: absolute coordinate
    void insert_y(int y0, int cy = 1, t_char ch = ' ') {
        assert(0 <= y0 && y0 <= m_cy);

        board_t<t_string, t_fixed> data(m_cx, m_cy + cy, ch);

        for (int y = 0; y < m_cy; ++y) {
            for (int x = 0; x < m_cx; ++x) {
                auto ch0 = get_at(x, y);
                if (y < y0)
                    data.set_at(x, y, ch0);
                else
                    data.set_at(x, y + cy, ch0);
            }
        }

        this->m_data = std::move(data.m_data);
        m_cy += cy;
    }

    // 列を削除する。
    // x0: absolute coordinate
    void delete_x(int x0) {
        assert(0 <= x0 && x0 < m_cx);

        board_t<t_string, t_fixed> data(m_cx - 1, m_cy, ' ');

        for (int y = 0; y < m_cy; ++y) {
            for (int x = 0; x < m_cx - 1; ++x) {
                if (x < x0)
                    data.set_at(x, y, get_at(x, y));
                else
                    data.set_at(x, y, get_at(x + 1, y));
            }
        }

        this->m_data = std::move(data.m_data);
        --m_cx;
    }

    // 行を削除する。
    // y0: absolute coordinate
    void delete_y(int y0) {
        assert(0 <= y0 && y0 < m_cy);

        board_t<t_string, t_fixed> data(m_cx, m_cy - 1, ' ');

        for (int y = 0; y < m_cy - 1; ++y) {
            for (int x = 0; x < m_cx; ++x) {
                if (y < y0)
                    data.set_at(x, y, get_at(x, y));
                else
                    data.set_at(x, y, get_at(x, y + 1));
            }
        }

        this->m_data = std::move(data.m_data);
        --m_cy;
    }

    // 左側に列を挿入する。
    void grow_x0(int cx, t_char ch = ' ') {
        assert(cx > 0);
        insert_x(0, cx, ch);
        m_x0 -= cx;
    }
    // 右側に列を挿入する。
    void grow_x1(int cx, t_char ch = ' ') {
        assert(cx > 0);
        insert_x(m_cx, cx, ch);
    }

    // 上側に行を挿入する。
    void grow_y0(int cy, t_char ch = ' ') {
        assert(cy > 0);
        insert_y(0, cy, ch);
        m_y0 -= cy;
    }
    // 下側に行を挿入する。
    void grow_y1(int cy, t_char ch = ' ') {
        assert(cy > 0);
        insert_y(m_cy, cy, ch);
    }

    // 文字マスが見つからない列を左右端からカットする。
    void trim_x() {
        bool found;
        int x, y;

        while (m_cx > 0) {
            found = false;
            x = 0;
            for (y = 0; y < m_cy; ++y) {
                t_char ch = get_at(x, y);
                if (is_letter(ch)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                delete_x(0);
                ++m_x0;
            } else {
                break;
            }
        }

        while (m_cx > 0) {
            found = false;
            x = m_cx - 1;
            for (y = 0; y < m_cy; ++y) {
                t_char ch = get_at(x, y);
                if (is_letter(ch)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                delete_x(m_cx - 1);
            } else {
                break;
            }
        }

        m_x0 = 0;
    }

    // 文字マスが見つからない行を上下端からカットする。
    void trim_y() {
        bool found;
        int x, y;

        while (m_cy > 0) {
            found = false;
            y = 0;
            for (x = 0; x < m_cx; ++x) {
                t_char ch = get_at(x, y);
                if (is_letter(ch)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                delete_y(0);
            } else {
                break;
            }
        }

        while (m_cy > 0) {
            found = false;
            y = m_cy - 1;
            for (x = 0; x < m_cx; ++x) {
                t_char ch = get_at(x, y);
                if (is_letter(ch)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                delete_y(m_cy - 1);
            } else {
                break;
            }
        }

        m_y0 = 0;
    }

    // 文字マスが見つからない行と列を端からカットする。
    void trim() {
        trim_y();
        trim_x();
    }

    // 文字列として標準出力。デバッグ用。
    void print() const {
        std::printf("dx:%d, dy:%d, cx:%d, cy:%d\n", m_x0, m_y0, m_cx, m_cy);
        for (int y = m_y0; y < m_y0 + m_cy; ++y) {
            std::printf("%3d: ", y);
            for (int x = m_x0; x < m_x0 + m_cx; ++x) {
                auto ch = get_on(x, y);
                std::putchar(ch);
            }
            std::printf("\n");
        }
        std::fflush(stdout);
    }

    // マス(x, y)は横向き交差可能か？
    bool is_crossable_x(int x, int y) const noexcept {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x - 1, y);
        ch2 = get_on(x + 1, y);
        return (ch1 == '?' || ch2 == '?');
    }
    // マス(x, y)は縦向き交差可能か？
    bool is_crossable_y(int x, int y) const noexcept {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x, y - 1);
        ch2 = get_on(x, y + 1);
        return (ch1 == '?' || ch2 == '?');
    }

    bool must_be_cross(int x, int y) const noexcept {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x - 1, y);
        ch2 = get_on(x + 1, y);
        const bool flag1 = (is_letter(ch1) || is_letter(ch2));
        ch1 = get_on(x, y - 1);
        ch2 = get_on(x, y + 1);
        const bool flag2 = (is_letter(ch1) || is_letter(ch2));
        return flag1 && flag2;
    }

    // 候補に対して十分なサイズを確保する。
    void apply_size(const candidate_t<t_string>& cand) {
        auto& word = cand.m_word;
        const int x = cand.m_x, y = cand.m_y;
        if (cand.m_vertical) {
            ensure(x, y - 1);
            ensure(x, y + static_cast<int>(word.size()));
        }
        else {
            ensure(x - 1, y);
            ensure(x + static_cast<int>(word.size()), y);
        }
    }

    // ルールに適合しているか？
    bool rules_ok() const {
        if (m_rules == 0)
            return true;
        if ((m_rules & RULES::DONTDOUBLEBLACK) && double_black())
            return false;
        if ((m_rules & RULES::DONTCORNERBLACK) && corner_black())
            return false;
        if ((m_rules & RULES::DONTTRIDIRECTIONS) && tri_black_around())
            return false;
        if ((m_rules & RULES::DONTTHREEDIAGONALS) && three_diagonals())
            return false;
        else if ((m_rules & RULES::DONTFOURDIAGONALS) && four_diagonals())
            return false;
        if ((m_rules & RULES::POINTSYMMETRY) && !is_point_symmetry()) {
            return false;
        } else {
            if ((m_rules & RULES::LINESYMMETRYH) && !is_line_symmetry_h())
                return false;
            if ((m_rules & RULES::LINESYMMETRYV) && !is_line_symmetry_v())
                return false;
        }
        if ((m_rules & RULES::DONTDIVIDE) && divided_by_black())
            return false;
        return true;
    }

    // 四隅黒禁。
    bool corner_black() const noexcept {
        return get_at(0, 0) == '#' ||
               get_at(m_cx - 1, 0) == '#' ||
               get_at(m_cx - 1, m_cy - 1) == '#' ||
               get_at(0, m_cy - 1) == '#';
    }

    // 連黒禁。
    bool double_black() const noexcept {
        const int n1 = m_cx - 1;
        const int n2 = m_cy - 1;
        int i = m_cy;
        for (--i; i >= 0; --i) {
            for (int j = 0; j < n1; j++) {
                if (get_at(j, i) == '#' && get_at(j + 1, i) == '#')
                    return true;
            }
        }
        int j = m_cx;
        for (--j; j >= 0; --j) {
            for (int i0 = 0; i0 < n2; i0++) {
                if (get_at(j, i0) == '#' && get_at(j, i0 + 1) == '#')
                    return true;
            }
        }
        return false;
    }

    // 三方黒禁。
    bool tri_black_around() const noexcept {
        for (int i = m_cy - 2; i >= 1; --i) {
            for (int j = m_cx - 2; j >= 1; --j) {
                if ((get_at(j, i - 1) == '#') + (get_at(j, i + 1) == '#') + 
                    (get_at(j - 1, i) == '#') + (get_at(j + 1, i) == '#') >= 3)
                {
                    return true;
                }
            }
        }
        return false;
    }

    // 分断禁。
    bool divided_by_black() const {
        int count = m_cx * m_cy; // マスの個数。
        std::vector<uint8_t> pb(count, 0); // 各位置に対応するフラグ群。
        std::queue<pos_t> positions; // 位置情報群の待ち行列。

        // 待ち行列に種を置く。
        if (get_at(0, 0) != '#') {
            positions.emplace(0, 0);
        } else {
            for (int i = 0; i < m_cy; ++i) {
                for (int j = 0; j < m_cx; ++j) {
                    if (get_at(j, i) != '#') {
                        positions.emplace(j, i);
                        goto skip;
                    }
                }
            }
skip:;
        }

        // 待ち行列が空になるまで、、、
        while (!positions.empty()) {
            pos_t pos = positions.front(); // 待ち行列の先頭を取得。
            positions.pop(); // 先頭を待ち行列から取り除く。
            if (!pb[pos.m_y * m_cx + pos.m_x]) {
                // 種から次々と広がっていく。
                pb[pos.m_y * m_cx + pos.m_x] = 1;
                // above
                if (pos.m_y > 0 && get_at(pos.m_x, pos.m_y - 1) != '#')
                    positions.emplace(pos.m_x, pos.m_y - 1);
                // below
                if (pos.m_y < m_cy - 1 && get_at(pos.m_x, pos.m_y + 1) != '#')
                    positions.emplace(pos.m_x, pos.m_y + 1);
                // left
                if (pos.m_x > 0 && get_at(pos.m_x - 1, pos.m_y) != '#')
                    positions.emplace(pos.m_x - 1, pos.m_y);
                // right
                if (pos.m_x < m_cx - 1 && get_at(pos.m_x + 1, pos.m_y) != '#')
                    positions.emplace(pos.m_x + 1, pos.m_y);
            }
        }

        // 未到達の場所があれば分断禁に抵触する。
        while (count-- > 0) {
            if (pb[count] == 0 && get(count) != '#') {
                return true;
            }
        }

        // さもなければ分断禁に抵触しない。
        return false;
    }

    // 黒斜四連禁。
    bool four_diagonals() const noexcept {
        // 斜め四つ黒マスがあればtrueを返す。
        for (int i = 0; i < m_cy - 3; i++) {
            for (int j = 0; j < m_cx - 3; j++) {
                if (get_at(j, i) != '#')
                    continue;
                if (get_at(j + 1, i + 1) != '#')
                    continue;
                if (get_at(j + 2, i + 2) != '#')
                    continue;
                if (get_at(j + 3, i + 3) != '#')
                    continue;
                return true;
            }
        }
        for (int i = 0; i < m_cy - 3; i++) {
            for (int j = 3; j < m_cx; j++) {
                if (get_at(j, i) != '#')
                    continue;
                if (get_at(j - 1, i + 1) != '#')
                    continue;
                if (get_at(j - 2, i + 2) != '#')
                    continue;
                if (get_at(j - 3, i + 3) != '#')
                    continue;
                return true;
            }
        }
        return false;
    }

    // 黒斜三連禁。
    bool three_diagonals() const noexcept {
        // 斜め三つ黒マスがあればtrueを返す。
        for (int i = 0; i < m_cy - 2; i++) {
            for (int j = 0; j < m_cx - 2; j++) {
                if (get_at(j, i) != '#')
                    continue;
                if (get_at(j + 1, i + 1) != '#')
                    continue;
                if (get_at(j + 2, i + 2) != '#')
                    continue;
                return true;
            }
        }
        for (int i = 0; i < m_cy - 2; i++) {
            for (int j = 2; j < m_cx; j++) {
                if (get_at(j, i) != '#')
                    continue;
                if (get_at(j - 1, i + 1) != '#')
                    continue;
                if (get_at(j - 2, i + 2) != '#')
                    continue;
                return true;
            }
        }
        return false;
    }

    // 黒マス点対称か？
    bool is_point_symmetry() const noexcept {
        for (int i = 0; i < m_cy; i++) {
            for (int j = 0; j < m_cx; j++) {
                if (get_at(j, i) == '#') {
                    auto ch = get_at(m_cx - (j + 1), m_cy - (i + 1));
                    if (ch != '#' && ch != '?')
                        return false;
                }
            }
        }
        return true;
    }

    // 黒マス左右対称か？
    bool is_line_symmetry_h() const noexcept {
        for (int j = 0; j < m_cx; j++) {
            for (int i = 0; i < m_cy; i++) {
                if (get_at(j, i) == '#') {
                    auto ch = get_at(m_cx - (j + 1), i);
                    if (ch != '#' && ch != '?')
                        return false;
                }
            }
        }
        return true;
    }

    // 黒マス上下対称か？
    bool is_line_symmetry_v() const noexcept {
        for (int i = 0; i < m_cy; i++) {
            for (int j = 0; j < m_cx; j++) {
                if (get_at(j, i) == '#') {
                    auto ch = get_at(j, m_cy - (i + 1));
                    if (ch != '#' && ch != '?')
                        return false;
                }
            }
        }
        return true;
    }

    // 単体テスト。
    static void unittest() {
#ifndef NDEBUG
        board_t<t_string, t_fixed> b(3, 3, '#');
        // 挿入テスト。
        b.insert_x(1, 1, '|');
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(1, 0) == '|');
        assert(b.get_at(2, 0) == '#');
        assert(b.get_at(3, 0) == '#');
        b.insert_y(1, 1, '-');
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(0, 1) == '-');
        assert(b.get_at(0, 2) == '#');
        assert(b.get_at(0, 3) == '#');
        // 削除テスト。
        b.delete_y(1);
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(0, 1) == '#');
        assert(b.get_at(0, 2) == '#');
        b.delete_x(1);
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(1, 0) == '#');
        assert(b.get_at(2, 0) == '#');
        // 成長テスト。
        b.grow_x0(1, '|');
        assert(b.get_on(-1, 1) == '|');
        assert(b.get_on(0, 1) == '#');
        assert(b.get_on(1, 1) == '#');
        assert(b.get_on(2, 1) == '#');
        assert(b.get_at(0, 1) == '|');
        assert(b.get_at(1, 1) == '#');
        assert(b.get_at(2, 1) == '#');
        assert(b.get_at(3, 1) == '#');
        // 削除と生成テスト。
        b.delete_x(0);
        b.m_x0 = 0;
        b.grow_y0(1, '-');
        assert(b.get_on(1, -1) == '-');
        assert(b.get_on(1, 0) == '#');
        assert(b.get_on(1, 1) == '#');
        assert(b.get_on(1, 2) == '#');
        assert(b.get_at(1, 0) == '-');
        assert(b.get_at(1, 1) == '#');
        assert(b.get_at(1, 2) == '#');
        assert(b.get_at(1, 3) == '#');
        // 削除と生成テスト。
        b.delete_y(0);
        b.m_y0 = 0;
        b.grow_x1(1, '|');
        assert(b.get_on(0, 1) == '#');
        assert(b.get_on(1, 1) == '#');
        assert(b.get_on(2, 1) == '#');
        assert(b.get_on(3, 1) == '|');
        b.delete_x(3);
        b.grow_y1(1, '-');
        assert(b.get_on(1, 0) == '#');
        assert(b.get_on(1, 1) == '#');
        assert(b.get_on(1, 2) == '#');
        assert(b.get_on(1, 3) == '-');
        // いろいろテスト。
        b.delete_y(3);
        b.resize(3, 3, '?');
        b.grow_x0(1, '?');
        b.set_on(1, 1, 'A');
        b.trim_x();
        b.trim_y();
        assert(b.get_at(0, 0) == 'A');
        b.ensure(2, 2);
        b.set_on(2, 2, 'A');
        assert(b.get_at(0, 0) == 'A');
        assert(b.get_at(2, 2) == 'A');
        assert(b.get_on(0, 0) == 'A');
        assert(b.get_on(2, 2) == 'A');
        b.grow_x0(1, '|');
        assert(b.get_at(0, 0) == '|');
        assert(b.get_at(1, 0) == 'A');
        assert(b.get_at(2, 0) == '?');
        assert(b.get_at(3, 0) == '?');
        assert(b.get_on(-1, 0) == '|');
        assert(b.get_on(0, 0) == 'A');
        assert(b.get_on(1, 0) == '?');
        assert(b.get_on(2, 0) == '?');
        b.delete_x(0);
        b.m_x0 = 0;
        b.grow_y0(1, '-');
        assert(b.get_at(0, 0) == '-');
        assert(b.get_at(0, 1) == 'A');
        assert(b.get_at(0, 2) == '?');
        assert(b.get_at(0, 3) == '?');
        assert(b.get_on(0, -1) == '-');
        assert(b.get_on(0, 0) == 'A');
        assert(b.get_on(0, 1) == '?');
        assert(b.get_on(0, 2) == '?');
        b.delete_y(0);
        b.m_y0 = 0;
#endif
    }
};

// 「単語群から自動生成」を実装するクラス。
template <typename t_string, bool t_fixed>
struct from_words_t {
    using t_char = typename t_string::value_type;

    inline static board_t<t_string, t_fixed> s_solution; // 解の盤面データ。
    board_t<t_string, t_fixed> m_board; // 盤面データ。
    std::unordered_set<t_string> m_words, m_dict; // 単語群と辞書データ。
    std::unordered_set<pos_t> m_crossable_x, m_crossable_y;
    int m_iThread; // スレッドのインデックス。

    // 候補を適用する。
    bool apply_candidate(const candidate_t<t_string>& cand) {
        auto& word = cand.m_word; // 候補から単語を取得。
        m_words.erase(word); // 使用済みとして単語群から除去する。
        const int x = cand.m_x, y = cand.m_y; // 候補のマス位置。
        if (cand.m_vertical) { // 候補が縦方向か？
            // 盤面が固定サイズならサイズを確認する。固定サイズでなければ、サイズを拡張する。
            if (t_fixed) {
                if (!m_board.ensure_y(y) || !m_board.ensure_y(y + static_cast<int>(word.size()) - 1))
                    return false;
            } else {
                m_board.ensure(x, y - 1);
                m_board.ensure(x, y + static_cast<int>(word.size()));
            }
            // 単語の上下に可能ならば黒マスを配置。
            m_board.set_on(x, y - 1, '#');
            m_board.set_on(x, y + static_cast<int>(word.size()), '#');
            // 単語を適用しながら交差可能性を更新する。
            for (size_t ich = 0; ich < word.size(); ++ich) {
                const int y0 = y + static_cast<int>(ich);
                m_board.set_on(x, y0, word[ich]);
                m_crossable_y.erase({x, y0});
                if (m_board.is_crossable_x(x, y0))
                    m_crossable_x.insert({ x, y0 });
            }
        } else { // 候補がヨコ方向か？
            // 盤面が固定サイズならサイズを確認する。固定サイズでなければ、サイズを拡張する。
            if (t_fixed) {
                if (!m_board.ensure_x(x) || !m_board.ensure_x(x + static_cast<int>(word.size()) - 1))
                    return false;
            } else {
                m_board.ensure(x - 1, y);
                m_board.ensure(x + static_cast<int>(word.size()), y);
            }
            // 単語の左右に可能ならば黒マスを配置。
            m_board.set_on(x - 1, y, '#');
            m_board.set_on(x + static_cast<int>(word.size()), y, '#');
            // 単語を適用しながら交差可能性を更新する。
            for (size_t ich = 0; ich < word.size(); ++ich) {
                const int x0 = x + static_cast<int>(ich);
                m_board.set_on(x0, y, word[ich]);
                m_crossable_x.erase({x0, y});
                if (m_board.is_crossable_y(x0, y))
                    m_crossable_y.insert({ x0, y });
            }
        }
        return true;
    }

    // ヨコ方向の候補群を取得する。
    std::vector<candidate_t<t_string> >
    get_candidates_x(int x, int y) const {
        std::vector<candidate_t<t_string> > cands; // 候補群。

        // マス(x, y)に着目する。
        t_char ch0 = m_board.get_on(x, y); // (x, y)の文字を取得。
        assert(is_letter(ch0));

        // その左右のマスを取得する。
        t_char ch1 = m_board.get_on(x - 1, y);
        t_char ch2 = m_board.get_on(x + 1, y);
        if (!is_letter(ch1) && !is_letter(ch2)) {
            t_char sz[2] = { ch0, 0 };
            cands.push_back({ x, y, sz, false });
        }

        // 各単語について。
        for (auto& word : m_words) {
            if (s_canceled || s_generated) { // キャンセル済みか生成済みなら終了。
                cands.clear();
                return cands;
            }

            // 単語中の各位置について、、、
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0) // 文字が一致しなければスキップ。
                    continue;

                // 境界のヨコ位置を取得。
                const int x0 = x - static_cast<int>(ich);
                const int x1 = x0 + static_cast<int>(word.size());
                bool matched = true; // 一致していると仮定。
                if (matched) {
                    // ヨコ向きの単語について、境界の２マスについて
                    t_char tch1 = m_board.get_on(x0 - 1, y);
                    t_char tch2 = m_board.get_on(x1, y);
                    if (is_letter(tch1) || is_letter(tch2)) { // 文字マスなら
                        matched = false; // 一致していない！
                    }
                }
                if (matched) {
                    for (size_t k = 0; k < word.size(); ++k) { // 各マスについて
                        t_char ch3 = m_board.get_on(x0 + static_cast<int>(k), y);
                        if (ch3 != '?' && word[k] != ch3) { // 未知のマスでなく一致しなければ
                            matched = false; // 一致していない！
                            break;
                        }
                    }
                }
                if (matched) {
                    cands.push_back({x0, y, word, false}); // 一致していれば候補を追加。
                }
            }
        }

        return cands; // 候補群を返す。
    }

    // タテ方向の候補群を取得する。
    std::vector<candidate_t<t_string> >
    get_candidates_y(int x, int y) const {
        std::vector<candidate_t<t_string> > cands;

        // マス(x, y)に着目する。
        t_char ch0 = m_board.get_on(x, y); // (x, y)の文字を取得。
        assert(is_letter(ch0));

        // その上下のマスを取得する。
        t_char ch1 = m_board.get_on(x, y - 1);
        t_char ch2 = m_board.get_on(x, y + 1);
        if (!is_letter(ch1) && !is_letter(ch2)) { // 両方とも文字マスでなければ
            t_char sz[2] = { ch0, 0 };
            cands.push_back({ x, y, sz, true }); // 1マスの候補 { ch0 } を追加。
        }

        // 各単語について、、、
        for (auto& word : m_words) {
            if (s_canceled || s_generated) { // キャンセル済みか生成済みなら終了。
                cands.clear();
                return cands;
            }

            // 単語中の各位置について、、、
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0) // 文字が一致しなければスキップ。
                    continue;

                // 境界のタテ位置を取得。
                const int y0 = y - static_cast<int>(ich);
                const int y1 = y0 + static_cast<int>(word.size());
                bool matched = true; // 一致していると仮定。
                if (matched) {
                    // 縦向きの単語について、境界の２マスについて
                    t_char tch1 = m_board.get_on(x, y0 - 1);
                    t_char tch2 = m_board.get_on(x, y1);
                    if (is_letter(tch1) || is_letter(tch2)) { // 両方とも文字マスなら
                        matched = false; // 一致していない！
                    }
                }
                if (matched) {
                    for (size_t k = 0; k < word.size(); ++k) { // 対応する各マスについて
                        t_char ch3 = m_board.get_on(x, y0 + static_cast<int>(k)); // 文字を取得し、
                        if (ch3 != '?' && word[k] != ch3) { // 未知マスでなく、一致していないなら
                            matched = false; // 一致していない！
                            break;
                        }
                    }
                }
                if (matched) { // 一致している。
                    cands.push_back({x, y0, word, true}); // 候補を追加。
                }
            }
        }

        return cands; // 候補群を返す。
    }

    bool fixup_candidates(std::vector<candidate_t<t_string> >& candidates) {
        std::vector<candidate_t<t_string> > cands; // 適用すべき候補。
        std::unordered_set<pos_t> positions; // 使用した位置。

        // 各候補について、、、
        for (auto& cand : candidates) {
            if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                return s_generated;
            // 候補が１マスであれば
            if (cand.m_word.size() == 1) {
                cands.push_back(cand); // 適用すべき候補を追加。
                positions.insert( {cand.m_x, cand.m_y} ); // この位置は使用済みとする。
            }
        }

        // 各候補について、、、
        for (auto& cand : candidates) {
            if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                return s_generated;
            // 候補が１マスでなければ、未使用のマスがあるか確認し、
            if (cand.m_word.size() != 1) {
                if (positions.count(pos_t(cand.m_x, cand.m_y)) == 0)
                    return false; // 未使用のマスがなければ失敗。
            }
        }

        // 適用すべき各候補について、、、
        for (auto& cand : cands) {
            if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                return s_generated;
            apply_candidate(cand); // 適用すべき候補を適用する。
        }
        return true;
    }

    // 生成の再帰関数。
    bool generate_recurse() {
        if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
            return s_generated; // 生成済みなら成功。

        // 交差可能性がなければ失敗。
        if (m_crossable_x.empty() && m_crossable_y.empty())
            return false;

#ifdef XWORDGIVER
        xg_aThreadInfo[m_iThread].m_count = static_cast<int>(m_dict.size() - m_words.size());
#endif

        std::vector<candidate_t<t_string> > candidates; // 候補群。

        // 横向きの交差可能性について、、、
        for (auto& cross : m_crossable_x) {
            if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                return s_generated;
            // 横向きの候補群を取得。
            auto cands = get_candidates_x(cross.m_x, cross.m_y);
            if (cands.empty()) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else if (cands.size() == 1 && cands[0].m_word.size() == 1) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else {
                // 候補群を追加。
                candidates.insert(candidates.end(), cands.begin(), cands.end());
            }
        }

        // タテ向きの交差可能性について、、、
        for (auto& cross : m_crossable_y) {
            if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                return s_generated;
            // タテ向きの候補群を取得。
            auto cands = get_candidates_y(cross.m_x, cross.m_y);
            if (cands.empty()) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else if (cands.size() == 1 && cands[0].m_word.size() == 1) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else {
                // 候補群を追加。
                candidates.insert(candidates.end(), cands.begin(), cands.end());
            }
        }

        if (m_words.empty()) { // 残りの単語がなければ
            if (fixup_candidates(candidates)) {
                board_t<t_string, t_fixed> board0 = m_board;
                board0.trim();
                board0.replace('?', '#');
                if (is_solution(board0)) { // 盤面が解ならば
                    std::lock_guard<std::mutex> lock(s_mutex); // 排他制御しながら
                    // キャンセルされていなければ解をセット
                    if (!s_canceled) {
                        s_generated = true;
                        s_solution = board0;
                    }
                    return !s_canceled; // キャンセルされていなければ成功。
                }
            }
            return s_generated; // 生成済みなら成功。
        }

#ifdef XWORDGIVER
        if (m_dict.size() <= 50 && m_words.size() < m_dict.size() / 2 && !t_fixed) {
            std::sort(candidates.begin(), candidates.end(),
                [&](const candidate_t<t_string>& cand0, const candidate_t<t_string>& cand1) {
                    board_t<t_string, false> board0 = m_board;
                    board0.apply_size(cand0);
                    board_t<t_string, false> board1 = m_board;
                    board1.apply_size(cand1);
                    const int cxy0 = (board0.m_cx + board0.m_cy) + std::abs(board0.m_cy - board0.m_cx) / 4;
                    const int cxy1 = (board1.m_cx + board1.m_cy) + std::abs(board1.m_cy - board1.m_cx) / 4;
                    return cxy0 < cxy1;
                }
            );
        } else {
            crossword_generation::random_shuffle(candidates.begin(), candidates.end());
        }
#else
        crossword_generation::random_shuffle(candidates.begin(), candidates.end());
#endif

        // 各候補について、、、
        for (auto& cand : candidates) {
            if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                return s_generated;
            // 複製して候補を適用して再帰・分岐。
            from_words_t<t_string, t_fixed> copy(*this);
            if (copy.apply_candidate(cand) && copy.generate_recurse()) {
                return true;
            }
        }

        // すべての候補を適用したが、失敗した。
        return false;
    }

    // 使用済み単語をチェックする。
    bool check_used_words(const board_t<t_string, t_fixed>& board) const
    {
        std::unordered_set<t_string> used; // 使用済み単語を保持。

        // 各行の各マスの並びについて、、、
        for (int y = board.m_y0; y < board.m_y0 + board.m_cy; ++y) {
            for (int x = board.m_x0; x < board.m_x0 + board.m_cx - 1; ++x) {
                // 隣り合う２マスを取得。
                auto ch0 = board.get_on(x, y);
                auto ch1 = board.get_on(x + 1, y);
                t_string word;
                word += ch0;
                word += ch1;
                // 両方とも文字マスならば、、、
                if (is_letter(ch0) && is_letter(ch1)) {
                    // 単語を構築する。
                    ++x;
                    for (;;) {
                        ++x;
                        ch1 = board.get_on(x, y);
                        if (!is_letter(ch1))
                            break;
                        word += ch1;
                    }
                    // 使用済みがあるか、辞書にない単語があれば、失敗。
                    if (used.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    // 構築した単語を使用済みと見なす。
                    used.insert(word);
                }
            }
        }

        // 各列の各マスの並びについて、、、
        for (int x = board.m_x0; x < board.m_x0 + board.m_cx; ++x) {
            for (int y = board.m_y0; y < board.m_y0 + board.m_cy - 1; ++y) {
                // 上下に並んだ２マスを取得。
                auto ch0 = board.get_on(x, y);
                auto ch1 = board.get_on(x, y + 1);
                t_string word;
                word += ch0;
                word += ch1;
                // 両方文字マスならば、、、
                if (is_letter(ch0) && is_letter(ch1)) {
                    // 単語を構築。
                    ++y;
                    for (;;) {
                        ++y;
                        ch1 = board.get_on(x, y);
                        if (!is_letter(ch1))
                            break;
                        word += ch1;
                    }
                    // 使用済みがあるか、辞書にない単語があれば、失敗。
                    if (used.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    // 構築した単語を使用済みと見なす。
                    used.insert(word);
                }
            }
        }

        return used.size() == m_dict.size(); // 使用済みと辞書単語数が一致すれば成功。
    }

    // 盤面データは解か？
    bool is_solution(const board_t<t_string, t_fixed>& board) const {
        if (board.count('?') > 0)
            return false; // 未知マスがあれば失敗。
        if (!board.rules_ok())
            return false; // ルールに反していれば失敗。
        return check_used_words(board); // 使用済み単語をチェックする。
    }

    // 生成する。
    bool generate() {
        if (m_words.empty())
            return false; // 単語群が空であれば失敗。

        // 最初の単語を候補として適用する。これが生成の種となる。
        auto word = *m_words.begin();
        candidate_t<t_string> cand = { 0, 0, word, false };
        apply_candidate(cand);
        // 生成の再帰を開始。
        return generate_recurse();
    }

    // 生成スレッドのプロシージャ。
    static bool generate_proc(const std::unordered_set<t_string> *words, int iThread) {
        // 乱数の種をセットする。
        std::srand(static_cast<uint32_t>(::GetTickCount64()) ^ ::GetCurrentThreadId());
#ifdef _WIN32
        // 性能を重視してスレッドの優先度を指定する。
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
        // 生成用のデータを初期化。
        from_words_t<t_string, t_fixed> data;
        data.m_iThread = iThread;
        data.m_words = *words;
        data.m_dict = *words;
        delete words; // 単語の所有権はスレッドのプロシージャに渡されているのでここで破棄する。
        return data.generate(); // 生成を開始する。
    }

    // 生成用のヘルパー関数。
    static bool
    do_generate(const std::unordered_set<t_string>& words,
                int num_threads = get_num_processors())
    {
#ifdef SINGLETHREADDEBUG // シングルスレッドテスト用。
        auto clone = new std::unordered_set<t_string>(words);
        generate_proc(clone, 0);
#else // 複数スレッド。
        // 各スレッドについて、、、
        for (int i = 0; i < num_threads; ++i) {
            // スレッドに所有権を譲渡したいので汚いがnewを使わせていただきたい。
            auto clone = new std::unordered_set<t_string>(words);
            try {
                // スレッドを生成。切り離す。
                std::thread t(generate_proc, clone, i);
                t.detach();
            } catch (std::system_error&) {
                delete clone;
            }
        }
#endif
        return s_generated;
    }
}; // struct from_words_t

template <typename t_string>
struct non_add_block_t {
    using t_char = typename t_string::value_type;
    enum { t_fixed = 1 }; // 固定か？

    inline static board_t<t_string, t_fixed> s_solution; // 解の盤面データ。
    board_t<t_string, t_fixed> m_board; // 盤面データ。
    std::unordered_set<t_string> m_words, m_dict; // 単語群と辞書データ。
    std::unordered_set<pos_t> m_checked_x, m_checked_y;
    int m_iThread;

    // パターンから候補群を取得する。
    std::vector<candidate_t<t_string>>
    get_candidates_from_pat(int x, int y, const t_string& pat, bool vertical) const {
        std::vector<candidate_t<t_string>> ret; // 候補群が戻り値。
        assert(pat.size() > 0); // 候補があると仮定。
        if (pat.find('?') == pat.npos) { // 未知のマスがなければ
            if (m_words.count(pat) > 0)
                ret.push_back({ x, y, pat, vertical }); // その候補が戻り値の一つ。
            return ret;
        }
        
        // 候補数の上限（メモリとパフォーマンスのバランス）。
        constexpr size_t MAX_CANDIDATES = 1000;
        
        // 速度のために前もって予約して確保（より正確な見積もり）。
        ret.reserve(std::min(m_words.size() >> 3, MAX_CANDIDATES));
        
        for (auto& word : m_words) { // 各単語について、、、
            // 早期終了: 候補数が上限に達したら打ち切る。
            if (ret.size() >= MAX_CANDIDATES)
                break;
                
            if (word.size() != pat.size()) // 単語とパターンの長さが不一致ならばスキップ。
                continue;
            bool matched = true; // 一致していると仮定。
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (pat[ich] != '?' && pat[ich] != word[ich]) { // 未知のマスか同一か？
                    matched = false; // 一致していない！
                    break;
                }
            }
            if (matched)
                ret.push_back({ x, y, word, vertical}); // 一致すればその候補が戻り値の一つ。
        }
        return ret; // 候補群を返す。
    }

    // 単語群をチェックする。
    bool check_words() {
        // 各行の各マスについて、、、
        for (int y = 0; y < m_board.m_cy; ++y) {
            for (int x = 0; x < m_board.m_cx; ++x) {
                if (m_checked_x.count(pos_t(x, y)) > 0)
                    continue; // チェック済みならスキップ。

                int x0;
                auto pat = m_board.get_pat_x(x, y, &x0); // 横向きのパターンを取得。
                if (pat.find('?') != pat.npos) // 未知のマスがあればスキップ。
                    continue;

                if (pat.size() <= 1) { // パターンが１マスであれば、
                    m_checked_x.emplace(x, y); // チェック済みと見なす。
                    continue;
                }

                if (m_dict.count(pat) == 0) // 単語が登録されていなければ失敗。
                    return false;

                // パターンの各マス位置について
                for (size_t i = 0; i < pat.size(); ++i, ++x0) {
                    m_checked_x.emplace(x0, y); // チェック済みとする。
                }
            }
        }

        // 各列の各マスについて、、、
        for (int x = 0; x < m_board.m_cx; ++x) {
            for (int y = 0; y < m_board.m_cy; ++y) {
                if (m_checked_y.count(pos_t(x, y)) > 0)
                    continue; // チェック済みならスキップ。

                int y0;
                auto pat = m_board.get_pat_y(x, y, &y0); // 縦向きのパターンを取得。
                if (pat.find('?') != pat.npos) // 未知のマスがあればスキップ。
                    continue;

                if (pat.size() <= 1) { // パターンが１マスであれば、
                    m_checked_y.emplace(x, y); // チェック済みと見なす。
                    continue;
                }

                if (m_dict.count(pat) == 0) // 単語が登録されていなければ失敗。
                    return false;

                // パターンの各マス位置について
                for (size_t i = 0; i < pat.size(); ++i, ++y0) {
                    m_checked_y.emplace(x, y0); // チェック済みとする。
                }
            }
        }

        return true;
    }

    // x向きに候補を適用する。
    bool apply_candidate_x(const candidate_t<t_string>& cand) {
        auto& word = cand.m_word;
        m_words.erase(word); // 単語群から単語を取り除く。
        int x = cand.m_x, y = cand.m_y;
        for (size_t ich = 0; ich < word.size(); ++ich, ++x) {
            m_checked_x.emplace(x, y); // チェック済みとする。
            m_board.set_at(x, y, word[ich]); // マス(x, y)に適用する。
        }
        return true;
    }
    // y向きに候補を適用する。
    bool apply_candidate_y(const candidate_t<t_string>& cand) {
        auto& word = cand.m_word;
        m_words.erase(word); // 単語群から単語を取り除く。
        int x = cand.m_x, y = cand.m_y;
        for (size_t ich = 0; ich < word.size(); ++ich, ++y) {
            m_checked_y.emplace(x, y); // チェック済みとする。
            m_board.set_at(x, y, word[ich]); // マス(x, y)に適用する。
        }
        return true;
    }

    // 生成再帰用の関数。
    bool generate_recurse() {
        // キャンセル済み、生成済み、もしくは単語チェックに失敗ならば終了。
        if (s_canceled || s_generated || !check_words())
            return s_generated; // 生成済みなら成功。

        // 各行のマスの並びについて。
        for (int y = 0; y < m_board.m_cy; ++y) {
            for (int x = 0; x < m_board.m_cx - 1; ++x) {
                // キャンセル済みか生成済みなら終了。
                if (s_canceled || s_generated)
                    return s_generated; // 生成済みなら成功。

                // 横向きの２マスの並びを見る。
                t_char ch0 = m_board.get_at(x, y);
                t_char ch1 = m_board.get_at(x + 1, y);
                // 文字マスと未知のマス、もしくは、未知のマスと文字マスが並んでいれば、、、
                if ((is_letter(ch0) && ch1 == '?') || (ch0 == '?' && is_letter(ch1))) {
                    int x0;
                    auto pat = m_board.get_pat_x(x, y, &x0); // x方向にパターンを取得。
                    auto cands = get_candidates_from_pat(x0, y, pat, false); // パターンから候補群を取得。
                    if (cands.empty())
                        return false; // 候補がなければ失敗。
                    // 候補群をランダムシャッフル。
                    crossword_generation::random_shuffle(cands.begin(), cands.end());
                    // 各候補について、、、
                    for (auto& cand : cands) {
                        if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                            return s_generated; // 生成済みなら成功。

                        // 複製して候補を適用して再帰・分岐する。
                        non_add_block_t<t_string> copy(*this);
                        copy.apply_candidate_x(cand);
                        if (copy.generate_recurse())
                            break;
                    }
                    return s_generated; // 生成済みなら成功。
                }
            }
        }

        // 各列のマスの並びについて、、、
        for (int x = 0; x < m_board.m_cx; ++x) {
            for (int y = 0; y < m_board.m_cy - 1; ++y) {
                // キャンセル済みか生成済みなら終了。
                if (s_canceled || s_generated)
                    return s_generated; // 生成済みなら成功。

                // タテ向きの２マスの並びを見る。
                t_char ch0 = m_board.get_at(x, y);
                t_char ch1 = m_board.get_at(x, y + 1);
                // 文字マスと未知のマス、もしくは、未知のマスと文字マスが並んでいれば、、、
                if ((is_letter(ch0) && ch1 == '?') || (ch0 == '?' && is_letter(ch1))) {
                    int y0;
                    auto pat = m_board.get_pat_y(x, y, &y0); // y方向にパターンを取得。
                    auto cands = get_candidates_from_pat(x, y0, pat, true); // パターンから候補群を取得。
                    if (cands.empty())
                        return false; // 候補がなければ失敗。
                    // 候補群をランダムシャッフル。
                    crossword_generation::random_shuffle(cands.begin(), cands.end());
                    // 各候補について、、、
                    for (auto& cand : cands) {
                        if (s_canceled || s_generated) // キャンセル済みか生成済みなら終了。
                            return s_generated; // 生成済みなら成功。

                        // 複製して候補を適用して再帰・分岐。
                        non_add_block_t<t_string> copy(*this);
                        copy.apply_candidate_y(cand);
                        if (copy.generate_recurse())
                            break;
                    }
                    return s_generated; // 生成済みなら成功。
                }
            }
        }

        // 盤面が解ならば成功。
        if (is_solution(m_board)) {
            std::lock_guard<std::mutex> lock(s_mutex);
            // キャンセルされていなければ解をセット
            if (!s_canceled) {
                s_generated = true;
                s_solution = m_board;
            }
            return !s_canceled; // キャンセルされていなければ成功。
        }

        return s_generated; // 生成済みなら成功。
    }

    // 盤面は解か？
    bool is_solution(const board_t<t_string, t_fixed>& board) {
        return (board.count('?') == 0);
    }

    // 生成を行う関数。
    bool generate() {
        // 単語がなければ生成できない。
        if (m_words.empty())
            return false;

        // 生成中はルールに適合していると仮定する。
        assert(m_board.rules_ok());

        // 文字マスがあれば、再帰用の関数を呼び出す。
        if (m_board.has_letter())
            return generate_recurse();

        // 単語群をランダムシャッフル。
        std::vector<t_string> words(m_words.begin(), m_words.end());
        crossword_generation::random_shuffle(words.begin(), words.end());

        // 盤面の各マスについて。
        for (int y = 0; y < m_board.m_cy; ++y) {
            for (int x = 0; x < m_board.m_cx - 1; ++x) {
                // 生成済みもしくはキャンセル済みならば終了。
                if (s_canceled || s_generated)
                    return s_generated;
                // 未知のマスがあれば、、、
                if (m_board.get_at(x, y) == '?' && m_board.get_at(x + 1, y) == '?') {
                    int x0;
                    auto pat = m_board.get_pat_x(x, y, &x0); // x方向のパターンを取得し、
                    auto cands = get_candidates_from_pat(x0, y, pat, false); // パターンから候補を取得。
                    for (auto& cand : cands) { // 各候補について
                        // キャンセル済みもしくは生成済みなら終了。
                        if (s_canceled || s_generated)
                            return s_generated; // 生成済みなら成功。

                        // 複製して候補を適用して再帰。
                        non_add_block_t<t_string> copy(*this);
                        copy.apply_candidate_x(cand);
                        if (copy.generate_recurse())
                            return true; // 生成に成功。
                    }
                    // パターンの長さだけx方向にスキップ。
                    x += static_cast<int>(pat.size());
                }
            }
        }

        return false;
    }

    // 生成スレッドのプロシージャ。
    static bool
    generate_proc(board_t<t_string, t_fixed> *pboard,
                  std::unordered_set<t_string> *pwords, int iThread)
    {
        std::srand(uint32_t(::GetTickCount64()) ^ ::GetCurrentThreadId());
#ifdef _WIN32
        //::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
        non_add_block_t<t_string> data;
        data.m_iThread = iThread;
        data.m_board = std::move(*pboard);
        delete pboard;
        data.m_words = *pwords;
        data.m_dict = std::move(*pwords);
        delete pwords;
        return data.generate();
    }

    // 生成を行うヘルパー関数。
    static bool
    do_generate(const board_t<t_string, t_fixed>& board,
                const std::unordered_set<t_string>& words,
                int num_threads = get_num_processors())
    {
        board_t<t_string, t_fixed> *pboard = nullptr;
        std::unordered_set<t_string> *pwords = nullptr;
#ifdef SINGLETHREADDEBUG // シングルスレッドテスト用。
        pboard = new board_t<t_string, t_fixed>(board);
        pwords = new std::unordered_set<t_string>(words);
        generate_proc(pboard, pwords, 0);
#else // 複数スレッド。
        for (int i = 0; i < num_threads; ++i) {
            // スレッドに所有権を譲渡したいので汚いがnewを使わせていただきたい。
            pboard = new board_t<t_string, t_fixed>(board);
            pwords = new std::unordered_set<t_string>(words);
            try {
                // スレッドを生成。切り離す。
                std::thread t(generate_proc, pboard, pwords, i);
                xg_ahThreads[i] = (HANDLE)t.native_handle();
                t.detach();
            } catch (std::system_error&) {
                delete pboard;
                delete pwords;
            }
        }
#endif
        return s_generated;
    }
}; // struct non_add_block_t

} // namespace crossword_generation

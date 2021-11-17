#pragma once

#define CROSSWORD_GENERATION 17 // crossword_generation version

#define _GNU_SOURCE
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <cassert>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <algorithm>
#include <utility>
#include <random>
#ifdef _WIN32
    #include <windows.h>
#else
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

namespace crossword_generation {
    struct pos_t {
        int m_x, m_y;
        pos_t(int x, int y) : m_x(x), m_y(y) { }
        bool operator==(const pos_t& pos) const {
            return m_x == pos.m_x && m_y == pos.m_y;
        }
    };
} // namespace crossword_generation

namespace std {
    template <>
    struct hash<crossword_generation::pos_t> {
        size_t operator()(const crossword_generation::pos_t& pos) const {
            return uint16_t(pos.m_x) | (uint16_t(pos.m_y) << 16);
        }
    };
} // namespace std

namespace crossword_generation {
inline static bool s_generated = false;
inline static bool s_canceled = false;
inline static std::mutex s_mutex;

struct RULES {
    enum {
        DONTDOUBLEBLACK = (1 << 0),
        DONTCORNERBLACK = (1 << 1),
        DONTTRIDIRECTIONS = (1 << 2),
        DONTDIVIDE = (1 << 3),
        DONTFOURDIAGONALS = (1 << 4),
        POINTSYMMETRY = (1 << 5),
        DONTTHREEDIAGONALS = (1 << 6),
        LINESYMMETRYV = (1 << 7),
        LINESYMMETRYH = (1 << 8),
    };
};

template <typename t_char>
inline bool is_letter(t_char ch) {
    return (ch != '#' && ch != '?');
}

inline uint32_t get_num_processors(void) {
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
template <typename t_elem>
inline void random_shuffle(const t_elem& begin, const t_elem& end) {
#ifndef NO_RANDOM
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(begin, end, g);
#endif
}

inline void reset() {
    s_generated = s_canceled = false;
#ifdef XWORDGIVER
    for (auto& info : xg_aThreadInfo) {
        info.m_count = 0;
    }
#endif
}

inline void wait_for_threads(int num_threads = get_num_processors(), int retry_count = 3) {
    const int INTERVAL = 100;
    for (int i = 0; i < retry_count; ++i) {
        if (s_generated || s_canceled)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL));
    }
}

template <typename t_char>
inline bool
check_connectivity(const std::unordered_set<std::basic_string<t_char> >& words,
                   std::basic_string<t_char>& nonconnected)
{
    typedef std::basic_string<t_char> t_string;
    if (words.size() <= 1)
        return true;

    std::vector<t_string> vec_words(words.begin(), words.end());
    std::queue<size_t> queue;
    std::unordered_set<size_t> indexes;
    queue.emplace(0);

    while (!queue.empty()) {
        size_t index0 = queue.front();
        indexes.insert(index0);
        queue.pop();

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

template <typename t_char>
struct candidate_t {
    typedef std::basic_string<t_char> t_string;
    int m_x, m_y;
    t_string m_word;
    bool m_vertical;
};

template <typename t_char>
struct cross_candidate_t {
    candidate_t<t_char> m_cand_x, m_cand_y;
    cross_candidate_t() {
        m_cand_x.m_vertical = false;
        m_cand_y.m_vertical = true;
    }
};

template <typename t_char>
struct board_data_t {
    typedef std::basic_string<t_char> t_string;
    t_string m_data;

    board_data_t(int cx = 1, int cy = 1, t_char ch = ' ') {
        resize(cx, cy, ch);
    }

    int size() const {
        return int(m_data.size());
    }

    void resize(int cx, int cy, t_char ch = ' ') {
        m_data.assign(cx * cy, ch);
    }

    void fill(t_char ch = ' ') {
        std::fill(m_data.begin(), m_data.end(), ch);
    }

    void replace(t_char chOld, t_char chNew) {
        std::replace(m_data.begin(), m_data.end(), chOld, chNew);
    }

    int count(t_char ch) const {
        int ret = 0;
        for (int xy = 0; xy < size(); ++xy) {
            if (m_data[xy] == ch)
                ++ret;
        }
        return ret;
    }

    bool is_empty() const {
        return count('?') == size();
    }
    bool is_full() const {
        return count('?') == 0;
    }
    bool has_letter() const {
        for (size_t xy = 0; xy < size(); ++xy) {
            if (is_letter(m_data[xy]))
                return true;
        }
        return false;
    }
};

template <typename t_char, bool t_fixed>
struct board_t : board_data_t<t_char> {
    typedef std::basic_string<t_char> t_string;

    int m_cx, m_cy;
    int m_rules;
    int m_x0, m_y0;

    board_t(int cx = 1, int cy = 1, t_char ch = ' ', int rules = 0, int x0 = 0, int y0 = 0)
        : board_data_t<t_char>(cx, cy, ch), m_cx(cx), m_cy(cy)
        , m_rules(rules), m_x0(x0), m_y0(y0)
    {
    }
    board_t(const board_t<t_char, t_fixed>& b) = default;
    board_t<t_char, t_fixed>& operator=(const board_t<t_char, t_fixed>& b) = default;

    t_char get(int xy) const {
        if (0 <= xy && xy < this->size())
            return board_data_t<t_char>::m_data[xy];
        return t_fixed ? '#' : '?';
    }
    void set(int xy, t_char ch) {
        if (0 <= xy && xy < this->size())
            board_data_t<t_char>::m_data[xy] = ch;
    }

    // x, y: absolute coordinate
    bool in_range(int x, int y) const {
        return (0 <= x && x < m_cx && 0 <= y && y < m_cy);
    }

    // x, y: absolute coordinate
    t_char real_get_at(int x, int y) const {
        if (in_range(x, y))
            return board_data_t<t_char>::m_data[y * m_cx + x];
        return ' ';
    }
    // x, y: absolute coordinate
    t_char get_at(int x, int y) const {
        if (in_range(x, y))
            return board_data_t<t_char>::m_data[y * m_cx + x];
        return t_fixed ? '#' : '?';
    }
    // x, y: absolute coordinate
    void set_at(int x, int y, t_char ch) {
        if (in_range(x, y))
            board_data_t<t_char>::m_data[y * m_cx + x] = ch;
    }
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
        int 
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

    bool is_corner(int x, int y) const {
        if (y == 0 || y == m_cy - 1) {
            if (x == 0 || x == m_cx - 1)
                return true;
        }
        return false;
    }

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

    bool can_make_four_diagonals(int x, int y) {
        auto ch = get_at(x, y);
        set_at(x, y, '#');
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
        set_at(x, y, ch);
        return ret;
    }

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

    // x, y: relative coordinate
    t_char get_on(int x, int y) const {
        assert(m_x0 <= 0);
        assert(m_y0 <= 0);
        return get_at(x - m_x0, y - m_y0);
    }
    // x, y: relative coordinate
    void set_on(int x, int y, t_char ch) {
        assert(m_x0 <= 0);
        assert(m_y0 <= 0);
        set_at(x - m_x0, y - m_y0, ch);
    }

    // x0: absolute coordinate
    void insert_x(int x0, int cx = 1, t_char ch = ' ') {
        assert(0 <= x0 && x0 <= m_cx);

        board_t<t_char, t_fixed> data(m_cx + cx, m_cy, ch);

        for (int y = 0; y < m_cy; ++y) {
            for (int x = 0; x < m_cx; ++x) {
                auto ch = get_at(x, y);
                if (x < x0)
                    data.set_at(x, y, ch);
                else
                    data.set_at(x + cx, y, ch);
            }
        }

        this->m_data = std::move(data.m_data);
        m_cx += cx;
    }

    // y0: absolute coordinate
    void insert_y(int y0, int cy = 1, t_char ch = ' ') {
        assert(0 <= y0 && y0 <= m_cy);

        board_t<t_char, t_fixed> data(m_cx, m_cy + cy, ch);

        for (int y = 0; y < m_cy; ++y) {
            for (int x = 0; x < m_cx; ++x) {
                auto ch = get_at(x, y);
                if (y < y0)
                    data.set_at(x, y, ch);
                else
                    data.set_at(x, y + cy, ch);
            }
        }

        this->m_data = std::move(data.m_data);
        m_cy += cy;
    }

    // x0: absolute coordinate
    void delete_x(int x0) {
        assert(0 <= x0 && x0 < m_cx);

        board_t<t_char, t_fixed> data(m_cx - 1, m_cy, ' ');

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

    // y0: absolute coordinate
    void delete_y(int y0) {
        assert(0 <= y0 && y0 < m_cy);

        board_t<t_char, t_fixed> data(m_cx, m_cy - 1, ' ');

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

    void grow_x0(int cx, t_char ch = ' ') {
        assert(cx > 0);
        insert_x(0, cx, ch);
        m_x0 -= cx;
    }
    void grow_x1(int cx, t_char ch = ' ') {
        assert(cx > 0);
        insert_x(m_cx, cx, ch);
    }

    void grow_y0(int cy, t_char ch = ' ') {
        assert(cy > 0);
        insert_y(0, cy, ch);
        m_y0 -= cy;
    }
    void grow_y1(int cy, t_char ch = ' ') {
        assert(cy > 0);
        insert_y(m_cy, cy, ch);
    }

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

    void trim() {
        trim_y();
        trim_x();
    }

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

    bool is_crossable_x(int x, int y) const {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x - 1, y);
        ch2 = get_on(x + 1, y);
        return (ch1 == '?' || ch2 == '?');
    }
    bool is_crossable_y(int x, int y) const {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x, y - 1);
        ch2 = get_on(x, y + 1);
        return (ch1 == '?' || ch2 == '?');
    }

    bool must_be_cross(int x, int y) const {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x - 1, y);
        ch2 = get_on(x + 1, y);
        bool flag1 = (is_letter(ch1) || is_letter(ch2));
        ch1 = get_on(x, y - 1);
        ch2 = get_on(x, y + 1);
        bool flag2 = (is_letter(ch1) || is_letter(ch2));
        return flag1 && flag2;
    }

    void apply_size(const candidate_t<t_char>& cand) {
        auto& word = cand.m_word;
        int x = cand.m_x, y = cand.m_y;
        if (cand.m_vertical) {
            ensure(x, y - 1);
            ensure(x, y + int(word.size()));
        }
        else {
            ensure(x - 1, y);
            ensure(x + int(word.size()), y);
        }
    }

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

    bool corner_black() const {
        return get_at(0, 0) == '#' ||
               get_at(m_cx - 1, 0) == '#' ||
               get_at(m_cx - 1, m_cy - 1) == '#' ||
               get_at(0, m_cy - 1) == '#';
    }

    bool double_black() const {
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
            for (int i = 0; i < n2; i++) {
                if (get_at(j, i) == '#' && get_at(j, i + 1) == '#')
                    return true;
            }
        }
        return false;
    }

    bool tri_black_around() const {
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

    bool divided_by_black() const {
        int count = m_cx * m_cy;

        std::vector<uint8_t> pb(count, 0);
        std::queue<pos_t> positions;
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

        while (!positions.empty()) {
            pos_t pos = positions.front();
            positions.pop();
            if (!pb[pos.m_y * m_cx + pos.m_x]) {
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

        while (count-- > 0) {
            if (pb[count] == 0 && get(count) != '#') {
                return true;
            }
        }

        return false;
    }

    bool four_diagonals() const {
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

    bool three_diagonals() const {
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

    bool is_point_symmetry() const {
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

    bool is_line_symmetry_h() const {
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

    bool is_line_symmetry_v() const {
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

    static void unittest() {
#ifndef NDEBUG
        board_t<t_char, t_fixed> b(3, 3, '#');
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
        b.delete_y(1);
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(0, 1) == '#');
        assert(b.get_at(0, 2) == '#');
        b.delete_x(1);
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(1, 0) == '#');
        assert(b.get_at(2, 0) == '#');
        b.grow_x0(1, '|');
        assert(b.get_on(-1, 1) == '|');
        assert(b.get_on(0, 1) == '#');
        assert(b.get_on(1, 1) == '#');
        assert(b.get_on(2, 1) == '#');
        assert(b.get_at(0, 1) == '|');
        assert(b.get_at(1, 1) == '#');
        assert(b.get_at(2, 1) == '#');
        assert(b.get_at(3, 1) == '#');
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

template <typename t_char, bool t_fixed>
struct from_words_t {
    typedef std::basic_string<t_char> t_string;

    inline static board_t<t_char, t_fixed> s_solution;
    board_t<t_char, t_fixed> m_board;
    std::unordered_set<t_string> m_words, m_dict;
    std::unordered_set<pos_t> m_crossable_x, m_crossable_y;
    int m_iThread;

    bool apply_candidate(const candidate_t<t_char>& cand) {
        auto& word = cand.m_word;
        m_words.erase(word);
        int x = cand.m_x, y = cand.m_y;
        if (cand.m_vertical) {
            if (t_fixed) {
                if (!m_board.ensure_y(y) || !m_board.ensure_y(y + int(word.size()) - 1))
                    return false;
            } else {
                m_board.ensure(x, y - 1);
                m_board.ensure(x, y + int(word.size()));
            }
            m_board.set_on(x, y - 1, '#');
            m_board.set_on(x, y + int(word.size()), '#');
            for (size_t ich = 0; ich < word.size(); ++ich) {
                int y0 = y + int(ich);
                m_board.set_on(x, y0, word[ich]);
                m_crossable_y.erase({x, y0});
                if (m_board.is_crossable_x(x, y0))
                    m_crossable_x.insert({ x, y0 });
            }
        } else {
            if (t_fixed) {
                if (!m_board.ensure_x(x) || !m_board.ensure_x(x + int(word.size()) - 1))
                    return false;
            } else {
                m_board.ensure(x - 1, y);
                m_board.ensure(x + int(word.size()), y);
            }
            m_board.set_on(x - 1, y, '#');
            m_board.set_on(x + int(word.size()), y, '#');
            for (size_t ich = 0; ich < word.size(); ++ich) {
                int x0 = x + int(ich);
                m_board.set_on(x0, y, word[ich]);
                m_crossable_x.erase({x0, y});
                if (m_board.is_crossable_y(x0, y))
                    m_crossable_y.insert({ x0, y });
            }
        }
        return true;
    }

    std::vector<candidate_t<t_char> >
    get_candidates_x(int x, int y) const {
        std::vector<candidate_t<t_char> > cands;

        t_char ch0 = m_board.get_on(x, y);
        assert(is_letter(ch0));

        t_char ch1 = m_board.get_on(x - 1, y);
        t_char ch2 = m_board.get_on(x + 1, y);
        if (!is_letter(ch1) && !is_letter(ch2)) {
            t_char sz[2] = { ch0, 0 };
            cands.push_back({ x, y, sz, false });
        }

        for (auto& word : m_words) {
            if (s_canceled || s_generated) {
                cands.clear();
                return cands;
            }

            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0)
                    continue;

                int x0 = x - int(ich);
                int x1 = x0 + int(word.size());
                bool matched = true;
                if (matched) {
                    t_char ch1 = m_board.get_on(x0 - 1, y);
                    t_char ch2 = m_board.get_on(x1, y);
                    if (is_letter(ch1) || is_letter(ch2)) {
                        matched = false;
                    }
                }
                if (matched) {
                    for (size_t k = 0; k < word.size(); ++k) {
                        t_char ch3 = m_board.get_on(x0 + int(k), y);
                        if (ch3 != '?' && word[k] != ch3) {
                            matched = false;
                            break;
                        }
                    }
                }
                if (matched) {
                    cands.push_back({x0, y, word, false});
                }
            }
        }

        return cands;
    }

    std::vector<candidate_t<t_char> >
    get_candidates_y(int x, int y) const {
        std::vector<candidate_t<t_char> > cands;

        t_char ch0 = m_board.get_on(x, y);
        assert(is_letter(ch0));

        t_char ch1 = m_board.get_on(x, y - 1);
        t_char ch2 = m_board.get_on(x, y + 1);
        if (!is_letter(ch1) && !is_letter(ch2)) {
            t_char sz[2] = { ch0, 0 };
            cands.push_back({ x, y, sz, true });
        }

        for (auto& word : m_words) {
            if (s_canceled || s_generated) {
                cands.clear();
                return cands;
            }

            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0)
                    continue;

                int y0 = y - int(ich);
                int y1 = y0 + int(word.size());
                bool matched = true;
                if (matched) {
                    t_char ch1 = m_board.get_on(x, y0 - 1);
                    t_char ch2 = m_board.get_on(x, y1);
                    if (is_letter(ch1) || is_letter(ch2)) {
                        matched = false;
                    }
                }
                if (matched) {
                    for (size_t k = 0; k < word.size(); ++k) {
                        t_char ch3 = m_board.get_on(x, y0 + int(k));
                        if (ch3 != '?' && word[k] != ch3) {
                            matched = false;
                            break;
                        }
                    }
                }
                if (matched) {
                    cands.push_back({x, y0, word, true});
                }
            }
        }

        return cands;
    }

    bool fixup_candidates(std::vector<candidate_t<t_char> >& candidates) {
        std::vector<candidate_t<t_char> > cands;
        std::unordered_set<pos_t> positions;
        for (auto& cand : candidates) {
            if (s_canceled || s_generated)
                return s_generated;
            if (cand.m_word.size() == 1) {
                cands.push_back(cand);
                positions.insert( {cand.m_x, cand.m_y} );
            }
        }
        for (auto& cand : candidates) {
            if (s_canceled || s_generated)
                return s_generated;
            if (cand.m_word.size() != 1) {
                if (positions.count(pos_t(cand.m_x, cand.m_y)) == 0)
                    return false;
            }
        }
        for (auto& cand : cands) {
            if (s_canceled || s_generated)
                return s_generated;
            apply_candidate(cand);
        }
        return true;
    }

    bool generate_recurse() {
        if (s_canceled || s_generated)
            return s_generated;

        if (s_generated)
            return true;

        if (m_crossable_x.empty() && m_crossable_y.empty())
            return false;

#ifdef XWORDGIVER
        xg_aThreadInfo[m_iThread].m_count = int(m_dict.size() - m_words.size());
#endif

        std::vector<candidate_t<t_char> > candidates;

        for (auto& cross : m_crossable_x) {
            if (s_canceled || s_generated)
                return s_generated;
            auto cands = get_candidates_x(cross.m_x, cross.m_y);
            if (cands.empty()) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else if (cands.size() == 1 && cands[0].m_word.size() == 1) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else {
                candidates.insert(candidates.end(), cands.begin(), cands.end());
            }
        }

        for (auto& cross : m_crossable_y) {
            if (s_canceled || s_generated)
                return s_generated;
            auto cands = get_candidates_y(cross.m_x, cross.m_y);
            if (cands.empty()) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else if (cands.size() == 1 && cands[0].m_word.size() == 1) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else {
                candidates.insert(candidates.end(), cands.begin(), cands.end());
            }
        }

        if (m_words.empty()) {
            if (fixup_candidates(candidates)) {
                board_t<t_char, t_fixed> board0 = m_board;
                board0.trim();
                board0.replace('?', '#');
                if (is_solution(board0)) {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_generated = true;
                    s_solution = board0;
                    return true;
                }
            }
            return s_generated;
        }

#ifdef XWORDGIVER
        if (m_words.size() < m_dict.size() / 2 && !t_fixed) {
            std::sort(candidates.begin(), candidates.end(),
                [&](const candidate_t<t_char>& cand0, const candidate_t<t_char>& cand1) {
                    board_t<t_char, false> board0 = m_board;
                    board0.apply_size(cand0);
                    board_t<t_char, false> board1 = m_board;
                    board1.apply_size(cand1);
                    int cxy0 = (board0.m_cx + board0.m_cy) + std::abs(board0.m_cy - board0.m_cx) / 4;
                    int cxy1 = (board1.m_cx + board1.m_cy) + std::abs(board1.m_cy - board1.m_cx) / 4;
                    return cxy0 < cxy1;
                }
            );
        } else {
            crossword_generation::random_shuffle(candidates.begin(), candidates.end());
        }
#else
        crossword_generation::random_shuffle(candidates.begin(), candidates.end());
#endif

        for (auto& cand : candidates) {
            if (s_canceled || s_generated)
                return s_generated;
            from_words_t<t_char, t_fixed> copy(*this);
            if (copy.apply_candidate(cand) && copy.generate_recurse()) {
                return true;
            }
        }

        return false;
    }

    bool check_used_words(const board_t<t_char, t_fixed>& board) const
    {
        std::unordered_set<t_string> used;

        for (int y = board.m_y0; y < board.m_y0 + board.m_cy; ++y) {
            for (int x = board.m_x0; x < board.m_x0 + board.m_cx - 1; ++x) {
                auto ch0 = board.get_on(x, y);
                auto ch1 = board.get_on(x + 1, y);
                t_string word;
                word += ch0;
                word += ch1;
                if (is_letter(ch0) && is_letter(ch1)) {
                    ++x;
                    for (;;) {
                        ++x;
                        ch1 = board.get_on(x, y);
                        if (!is_letter(ch1))
                            break;
                        word += ch1;
                    }
                    if (used.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    used.insert(word);
                }
            }
        }

        for (int x = board.m_x0; x < board.m_x0 + board.m_cx; ++x) {
            for (int y = board.m_y0; y < board.m_y0 + board.m_cy - 1; ++y) {
                auto ch0 = board.get_on(x, y);
                auto ch1 = board.get_on(x, y + 1);
                t_string word;
                word += ch0;
                word += ch1;
                if (is_letter(ch0) && is_letter(ch1)) {
                    ++y;
                    for (;;) {
                        ++y;
                        ch1 = board.get_on(x, y);
                        if (!is_letter(ch1))
                            break;
                        word += ch1;
                    }
                    if (used.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    used.insert(word);
                }
            }
        }

        return used.size() == m_dict.size();
    }

    bool is_solution(const board_t<t_char, t_fixed>& board) const {
        if (board.count('?') > 0)
            return false;
        if (!board.rules_ok())
            return false;
        return check_used_words(board);
    }

    bool generate() {
        if (m_words.empty())
            return false;

        auto word = *m_words.begin();
        candidate_t<t_char> cand = { 0, 0, word, false };
        apply_candidate(cand);
        if (!generate_recurse())
            return false;

        return true;
    }

    static bool generate_proc(const std::unordered_set<t_string> *words, int iThread) {
        std::srand(uint32_t(::GetTickCount64()) ^ ::GetCurrentThreadId());
#ifdef _WIN32
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
        from_words_t<t_char, t_fixed> data;
        data.m_iThread = iThread;
        data.m_words = data.m_dict = *words;
        bool flag = data.generate();
        delete words;
        return flag;
    }

    static bool
    do_generate(const std::unordered_set<t_string>& words,
                int num_threads = get_num_processors())
    {
#ifdef SINGLETHREADDEBUG
        auto clone = new std::unordered_set<t_string>(words);
        generate_proc(clone, 0);
#else
        for (int i = 0; i < num_threads; ++i) {
            auto clone = new std::unordered_set<t_string>(words);
            try {
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

} // namespace crossword_generation

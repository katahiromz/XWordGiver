#pragma once
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
#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    inline unsigned GetTickCount(void) {
        using namespace std;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned tick = ts.tv_nsec / 1000000;
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

template <typename t_char>
struct candidate_t {
    typedef std::basic_string<t_char> t_string;
    int m_x = 0, m_y = 0;
    bool m_vertical = false;
    t_string m_word;
};

template <typename t_char>
inline bool is_letter(t_char ch) {
    return (ch != ' ' && ch != '#' && ch != '?');
}

inline uint32_t get_num_processors(void) {
#if defined(_WIN32)
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
#else
    return std::thread::hardware_concurrency();
#endif
}

template <typename t_char>
struct board_data_t {
    typedef std::basic_string<t_char> t_string;
    t_string m_data;

    board_data_t(int cx = 1, int cy = 1, t_char ch = ' ') {
        allocate(cx, cy, ch);
    }

    void allocate(int cx, int cy, t_char ch = ' ') {
        m_data.assign(cx * cy, ch);
    }

    void fill(t_char ch = ' ') {
        std::fill(m_data.begin(), m_data.end(), ch);
    }

    void replace(t_char chOld, t_char chNew) {
        std::replace(m_data.begin(), m_data.end(), chOld, chNew);
    }
};

template <typename t_char, bool t_fixed>
struct board_t : board_data_t<t_char> {
    typedef std::basic_string<t_char> t_string;

    int m_x0, m_y0;
    int m_cx, m_cy;

    board_t(int cx = 1, int cy = 1, t_char ch = ' ', int x0 = 0, int y0 = 0)
        : board_data_t<t_char>(cx, cy, ch), m_x0(x0), m_y0(y0), m_cx(cx), m_cy(cy)
    {
    }
    board_t(const board_t<t_char, t_fixed>& b) = default;
    board_t<t_char, t_fixed>& operator=(const board_t<t_char, t_fixed>& b) = default;

    // x, y: absolute coordinate
    bool in_range(int x, int y) const {
        return (0 <= x && x < m_cx && 0 <= y && y < m_cy);
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
        if (ch1 == '?' || ch2 == '?')
            return true;
        return false;
    }
    bool is_crossable_y(int x, int y) const {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x, y - 1);
        ch2 = get_on(x, y + 1);
        if (ch1 == '?' || ch2 == '?')
            return true;
        return false;
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
        b.allocate(3, 3, '?');
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
struct generation_t {
    typedef std::basic_string<t_char> t_string;

    inline static bool s_generated = false;
    inline static bool s_canceled = false;
    inline static int s_count = 0;
    inline static board_t<t_char, t_fixed> s_solution;
    inline static std::mutex s_mutex;
    board_t<t_char, t_fixed> m_board;
    std::unordered_set<t_string> m_words, m_dict;
    std::unordered_set<pos_t> m_crossable_x, m_crossable_y;

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
            cands.push_back({ x, y, false, sz });
        }

        for (auto& word : m_words) {
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0)
                    continue;

                int x0 = x - int(ich);
                int x1 = x0 + int(word.size());
                bool matched = true;
                if (matched) {
                    t_char ch1 = m_board.get_on(x0 - 1, y);
                    t_char ch2 = m_board.get_on(x1, y);
                    if (is_letter(ch1) || ch1 == ' ') {
                        matched = false;
                    } else if (is_letter(ch2) || ch2 == ' ') {
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
                    cands.push_back({x0, y, false, word});
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
            cands.push_back({ x, y, true, sz });
        }

        for (auto& word : m_words) {
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0)
                    continue;

                int y0 = y - int(ich);
                int y1 = y0 + int(word.size());
                bool matched = true;
                if (matched) {
                    t_char ch1 = m_board.get_on(x, y0 - 1);
                    t_char ch2 = m_board.get_on(x, y1);
                    if (is_letter(ch1) || ch1 == ' ') {
                        matched = false;
                    } else if (is_letter(ch2) || ch2 == ' ') {
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
                    cands.push_back({x, y0, true, word});
                }
            }
        }

        return cands;
    }

    bool fixup_candidates(std::vector<candidate_t<t_char> >& candidates) {
        std::vector<candidate_t<t_char> > cands;
        std::unordered_set<pos_t> positions;
        for (auto& cand : candidates) {
            if (cand.m_word.size() == 1) {
                cands.push_back(cand);
                positions.insert( {cand.m_x, cand.m_y} );
            }
        }
        for (auto& cand : candidates) {
            if (cand.m_word.size() != 1) {
                if (positions.count(pos_t(cand.m_x, cand.m_y)) == 0)
                    return false;
            }
        }
        for (auto& cand : cands) {
            apply_candidate(cand);
        }
        return true;
    }

    bool generate_recurse() {
        if (s_canceled)
            return s_generated;

        if (s_generated)
            return true;

        if (m_crossable_x.empty() && m_crossable_y.empty())
            return false;

        std::vector<candidate_t<t_char> > candidates;

        for (auto& cross : m_crossable_x) {
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

        std::random_shuffle(candidates.begin(), candidates.end());

        for (auto& cand : candidates) {
            if (s_canceled)
                return s_generated;
            generation_t<t_char, t_fixed> copy(*this);
            if (copy.apply_candidate(cand) && copy.generate_recurse()) {
                return true;
            }
        }

        return false;
    }

    bool generate() {
        auto words = m_words;
        if (words.empty())
            return false;

        for (auto& word : m_words) {
            if (word.size() <= 1) {
                words.erase(word);
            }
        }

        if (words.empty())
            return false;

        auto word = *words.begin();
        candidate_t<t_char> cand = { 0, 0, false, word };
        apply_candidate(cand);
        if (!generate_recurse())
            return false;

        return true;
    }

    bool is_solution(const board_t<t_char, t_fixed>& board) const {
        for (int y = board.m_y0; y < board.m_y0 + board.m_cy; ++y) {
            for (int x = board.m_x0; x < board.m_x0 + board.m_cx; ++x) {
                auto ch = board.get_on(x, y);
                if (!is_letter(ch) && ch != '#')
                    return false;
            }
        }

        std::unordered_set<t_string> words;

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
                    if (words.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    words.insert(word);
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
                    if (words.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    words.insert(word);
                }
            }
        }

        return words.size() == m_dict.size();
    }

    static bool generate_from_words_proc(const std::unordered_set<t_string> *words, int iThread) {
        std::srand(::GetTickCount() ^ ::GetCurrentThreadId());
#if defined(_WIN32)
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
        generation_t<t_char, t_fixed> data;
        data.m_words = data.m_dict = *words;
        bool flag = data.generate();
        ++s_count;
        delete words;
        return flag;
    }

    static bool generate_from_words(const std::unordered_set<t_string>& words) {
#ifdef XWORDGIVER
        int num_threads = (int)xg_dwThreadCount;
#else
        int num_threads = (int)get_num_processors();
#endif
        //printf("num_threads: %d\n", int(num_threads));
        const int RETRY_COUNT = 3;
        const int INTERVAL = 100;

        s_count = 0;
        s_generated = s_canceled = false;

        for (int i = 0; i < num_threads; ++i) {
            auto clone = new std::unordered_set<t_string>(words);
            try {
                std::thread t(generate_from_words_proc, clone, i);
                t.detach();
            } catch (std::system_error& e) {
                delete clone;
                s_count++;
            }
        }

#if defined(_WIN32)
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

        for (size_t i = 0; i < RETRY_COUNT; ++i) {
            if (s_generated || s_canceled)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL));
        }

        if (!s_generated)
            s_canceled = true;

        for (size_t i = 0; i < RETRY_COUNT; ++i) {
            if (s_count >= num_threads)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL));
        }

#if defined(_WIN32)
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif

        return s_generated;
    }
};

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

} // namespace crossword_generation

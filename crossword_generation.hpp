#pragma once

#define CROSSWORD_GENERATION 24 // crossword_generation version

#define _GNU_SOURCE
#include <cstdio>               // �W�����o�́B
#include <cstdint>              // �W�������B
#include <ctime>                // ���ԁB
#include <cassert>              // assertion�B
#include <vector>               // std::vector
#include <unordered_set>        // std::unordered_set
#include <unordered_map>        // std::unordered_map
#include <queue>                // �҂��s��istd::queue�j
#include <thread>               // std::thread
#include <mutex>                // std::mutex
#include <algorithm>            // std::shuffle
#include <utility>              // ????
#include <random>               // �V�������������B
#ifdef _WIN32
    #include <windows.h>        // Windows�w�b�_�B
#else
    // Linux/Mac�p�̒�`�B
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

// �}�X�̈ʒu���`����B
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

// �N���X���[�h�����p�̖��O��ԁB����́u�P��Q���玩�������v�̂ݎ����B
// �����I�ɂ͂��̑��̐������@�����̂悤�ȃ��_���ȕ����Ɉڍs����B
namespace crossword_generation {
inline static bool s_generated = false;     // �����ς݂��H
inline static bool s_canceled = false;      // �L�����Z�����ꂽ���H
inline static std::mutex s_mutex;           // �~���[�e�b�N�X�i�r�������p�j�B

// ���[����\���t���O�Q�B
struct RULES {
    enum {
        DONTDOUBLEBLACK = (1 << 0),     // �A���ցB
        DONTCORNERBLACK = (1 << 1),     // �l�����ցB
        DONTTRIDIRECTIONS = (1 << 2),   // �O�����ցB
        DONTDIVIDE = (1 << 3),          // ���f�ցB
        DONTFOURDIAGONALS = (1 << 4),   // ���Ύl�A�ցB
        POINTSYMMETRY = (1 << 5),       // ���}�X�_�Ώ́B
        DONTTHREEDIAGONALS = (1 << 6),  // ���ΎO�A�ցB
        LINESYMMETRYV = (1 << 7),       // ���}�X�㉺�Ώ́B
        LINESYMMETRYH = (1 << 8),       // ���}�X���E�Ώ́B
    };
};

// �����}�X���H
template <typename t_char>
inline bool is_letter(t_char ch) {
    return (ch != '#' && ch != '?');
}

// �v���Z�b�T�̐���Ԃ��֐��B
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
// �]���^�̗��������istd::rand�Astd::random_shuffle�j�͐�������Ȃ��B
template <typename t_elem>
inline void random_shuffle(const t_elem& begin, const t_elem& end) {
#ifndef NO_RANDOM
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(begin, end, g);
#endif
}

// �����O�ɏ������B
inline void reset() {
    s_generated = s_canceled = false;
#ifdef XWORDGIVER
    for (auto& info : xg_aThreadInfo) {
        info.m_count = 0;
    }
#endif
}

// �����ς݂��L�����Z���ς݂���҂B���̂悤�ȑ҂����̓��_���ł͂Ȃ��B
// �����̃R�[�h�̓��_���ȕ��@�ɒu����������ׂ��B
inline void wait_for_threads(int num_threads = get_num_processors(), int retry_count = 3) {
    const int INTERVAL = 100;
    for (int i = 0; i < retry_count; ++i) {
        if (s_generated || s_canceled)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL));
    }
}

// �A�����𔻒�B
template <typename t_char>
inline bool
check_connectivity(const std::unordered_set<std::basic_string<t_char> >& words,
                   std::basic_string<t_char>& nonconnected)
{
    typedef std::basic_string<t_char> t_string;
    if (words.size() <= 1)
        return true;

    std::vector<t_string> vec_words(words.begin(), words.end()); // �P��Q�B
    std::queue<size_t> queue; // �҂��s��B
    std::unordered_set<size_t> indexes;
    queue.emplace(0); // �҂��s��ɏ����̎��Y����B�B

    while (!queue.empty()) {
        size_t index0 = queue.front();
        indexes.insert(index0);
        queue.pop(); // �����菜���B

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

// ���B�ʒu���ƒP��Əc���̌����̏������B
template <typename t_char>
struct candidate_t {
    typedef std::basic_string<t_char> t_string;
    int m_x, m_y;
    t_string m_word;
    bool m_vertical;
};

// �������B
template <typename t_char>
struct cross_candidate_t {
    candidate_t<t_char> m_cand_x, m_cand_y;
    cross_candidate_t() {
        m_cand_x.m_vertical = false;
        m_cand_y.m_vertical = true;
    }
};

// �Ֆʃf�[�^�B���g�͕�����B�z�u���@�̓T�u�N���X board_t �ɂ���Č��܂�B
template <typename t_char>
struct board_data_t {
    typedef std::basic_string<t_char> t_string;
    t_string m_data;

    // �R���X�g���N�^�ɂ�鏉�����B�����Ŗ��߂�B
    board_data_t(int cx = 1, int cy = 1, t_char ch = ' ') {
        resize(cx, cy, ch);
    }

    // �}�X���B
    int size() const {
        return int(m_data.size());
    }

    // �}�X���̑����B
    void resize(int cx, int cy, t_char ch = ' ') {
        m_data.assign(cx * cy, ch);
    }

    // ����̕����Ŗ��߂�B
    void fill(t_char ch = ' ') {
        std::fill(m_data.begin(), m_data.end(), ch);
    }

    // ������u���ɂ��u��������B
    void replace(t_char chOld, t_char chNew) {
        std::replace(m_data.begin(), m_data.end(), chOld, chNew);
    }

    // ����̕����̌��𐔂���B
    int count(t_char ch) const {
        int ret = 0;
        for (int xy = 0; xy < size(); ++xy) {
            if (m_data[xy] == ch)
                ++ret;
        }
        return ret;
    }

    // �󂩁H
    bool is_empty() const {
        return count('?') == size();
    }
    // ���m�̃}�X���Ȃ����H
    bool is_full() const {
        return count('?') == 0;
    }
    // �����}�X�����邩�B
    bool has_letter() const {
        for (int xy = 0; xy < size(); ++xy) {
            if (is_letter(m_data[xy]))
                return true;
        }
        return false;
    }
};

// board_data_t �̃T�u�N���X�B�Ֆʃf�[�^�B�p������ĕ����̔z�u���K�肳��Ă���B
template <typename t_char, bool t_fixed>
struct board_t : board_data_t<t_char> {
    typedef std::basic_string<t_char> t_string; // ������^�B

    int m_cx, m_cy; // �T�C�Y�B
    int m_rules; // ���[���Q�B
    int m_x0, m_y0;

    // board_t�̃R���X�g���N�^�B
    board_t(int cx = 1, int cy = 1, t_char ch = ' ', int rules = 0, int x0 = 0, int y0 = 0)
        : board_data_t<t_char>(cx, cy, ch), m_cx(cx), m_cy(cy)
        , m_rules(rules), m_x0(x0), m_y0(y0)
    {
    }
    board_t(const board_t<t_char, t_fixed>& b) = default;
    board_t<t_char, t_fixed>& operator=(const board_t<t_char, t_fixed>& b) = default;

    // �C���f�b�N�X�ʒu�̕�����Ԃ��B
    t_char get(int xy) const {
        if (0 <= xy && xy < this->size())
            return board_data_t<t_char>::m_data[xy];
        return t_fixed ? '#' : '?';
    }
    // �C���f�b�N�X�ʒu�ɕ������Z�b�g�B
    void set(int xy, t_char ch) {
        if (0 <= xy && xy < this->size())
            board_data_t<t_char>::m_data[xy] = ch;
    }

    // �}�X(x, y)�͔Ֆʂ͈͓̔����H
    // x, y: absolute coordinate
    bool in_range(int x, int y) const {
        return (0 <= x && x < m_cx && 0 <= y && y < m_cy);
    }

    // �}�X(x, y)���擾����B�͈̓`�F�b�N����B
    // x, y: absolute coordinate
    t_char real_get_at(int x, int y) const {
        if (in_range(x, y))
            return board_data_t<t_char>::m_data[y * m_cx + x];
        return ' ';
    }
    // �}�X(x, y)���擾����B�͈̓`�F�b�N����B�͈͊O�Ȃ�'#'��'?'��Ԃ��B
    // x, y: absolute coordinate
    t_char get_at(int x, int y) const {
        if (in_range(x, y))
            return board_data_t<t_char>::m_data[y * m_cx + x];
        return t_fixed ? '#' : '?';
    }
    // �}�X(x, y)���Z�b�g����B�͈̓`�F�b�N����B�͈͊O�Ȃ疳���B
    // x, y: absolute coordinate
    void set_at(int x, int y, t_char ch) {
        if (in_range(x, y))
            board_data_t<t_char>::m_data[y * m_cx + x] = ch;
    }
    // �}�X(x, y)���Z�b�g����B���������[���ɂ�蔽�˂���B�͈̓`�F�b�N����B�͈͊O�Ȃ疳���B
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

    // ���[���ɂ�荕�}�X�𔽎˂���B
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

    // �}�X(x, y)���牡�����p�^�[�����擾����B
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

    // �}�X(x, y)����c�����p�^�[�����擾����B
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

    // �}�X(x, y)�́A�R�[�i�[�i�l���j���H
    bool is_corner(int x, int y) const {
        if (y == 0 || y == m_cy - 1) {
            if (x == 0 || x == m_cx - 1)
                return true;
        }
        return false;
    }

    // �}�X(x, y)�ɍ��}�X��u���ƁA�A���ւɒ�G���邩�H
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

    // �Ֆʂ��Œ�T�C�Y�Ȃ�T�C�Y�Ɏ��܂邩�ǂ������肵�A
    // �Ֆʂ��Œ�T�C�Y�łȂ���΁A���܂�悤�Ɋg������B
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
    // �Ֆʂ��Œ�T�C�Y�Ȃ�T�C�Y�Ɏ��܂邩�ǂ������肵�A
    // �Ֆʂ��Œ�T�C�Y�łȂ���΁A���܂�悤�Ɋg������B
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
    // �Ֆʂ��Œ�T�C�Y�Ȃ�T�C�Y�Ɏ��܂邩�ǂ������肵�A
    // �Ֆʂ��Œ�T�C�Y�łȂ���΁A���܂�悤�Ɋg������B
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

    // �}�X(x, y)�̕������擾����B�����[�X���͈̔̓`�F�b�N�Ȃ��B
    // x, y: relative coordinate
    t_char get_on(int x, int y) const {
        assert(m_x0 <= 0);
        assert(m_y0 <= 0);
        return get_at(x - m_x0, y - m_y0);
    }
    // �}�X(x, y)�̕������Z�b�g����B�����[�X���͈̔̓`�F�b�N�Ȃ��B
    // x, y: relative coordinate
    void set_on(int x, int y, t_char ch) {
        assert(m_x0 <= 0);
        assert(m_y0 <= 0);
        set_at(x - m_x0, y - m_y0, ch);
    }

    // ���}������B�w�肳�ꂽ�����ƕ��ŁB
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

    // �s��}������B�w�肳�ꂽ�����ƍ����ŁB
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

    // ����폜����B
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

    // �s���폜����B
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

    // ������Ƃ��ĕW���o�́B�f�o�b�O�p�B
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

    // �}�X(x, y)�͉����������\���H
    bool is_crossable_x(int x, int y) const {
        assert(is_letter(get_on(x, y)));
        t_char ch1, ch2;
        ch1 = get_on(x - 1, y);
        ch2 = get_on(x + 1, y);
        return (ch1 == '?' || ch2 == '?');
    }
    // �}�X(x, y)�͏c���������\���H
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

    // ���ɑ΂��ď\���ȃT�C�Y���m�ۂ���B
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

    // ���[���ɓK�����Ă��邩�H
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

    // �l�����ցB
    bool corner_black() const {
        return get_at(0, 0) == '#' ||
               get_at(m_cx - 1, 0) == '#' ||
               get_at(m_cx - 1, m_cy - 1) == '#' ||
               get_at(0, m_cy - 1) == '#';
    }

    // �A���ցB
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

    // �O�����ցB
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

    // ���f�ցB
    bool divided_by_black() const {
        int count = m_cx * m_cy; // �}�X�̌��B
        std::vector<uint8_t> pb(count, 0); // �e�ʒu�ɑΉ�����t���O�Q�B
        std::queue<pos_t> positions; // �ʒu���Q�̑҂��s��B

        // �҂��s��Ɏ��u���B
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

        // �҂��s�񂪋�ɂȂ�܂ŁA�A�A
        while (!positions.empty()) {
            pos_t pos = positions.front(); // �҂��s��̐擪���擾�B
            positions.pop(); // �擪��҂��s�񂩂��菜���B
            if (!pb[pos.m_y * m_cx + pos.m_x]) {
                // �킩�玟�X�ƍL�����Ă����B
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

        // �����B�̏ꏊ������Ε��f�ւɒ�G����B
        while (count-- > 0) {
            if (pb[count] == 0 && get(count) != '#') {
                return true;
            }
        }

        // �����Ȃ���Ε��f�ւɒ�G���Ȃ��B
        return false;
    }

    // ���Ύl�A�ցB
    bool four_diagonals() const {
        // �΂ߎl���}�X�������true��Ԃ��B
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

    // ���ΎO�A�ցB
    bool three_diagonals() const {
        // �΂ߎO���}�X�������true��Ԃ��B
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

    // ���}�X�_�Ώ̂��H
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

    // ���}�X���E�Ώ̂��H
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

    // ���}�X�㉺�Ώ̂��H
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

    // �P�̃e�X�g�B
    static void unittest() {
#ifndef NDEBUG
        board_t<t_char, t_fixed> b(3, 3, '#');
        // �}���e�X�g�B
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
        // �폜�e�X�g�B
        b.delete_y(1);
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(0, 1) == '#');
        assert(b.get_at(0, 2) == '#');
        b.delete_x(1);
        assert(b.get_at(0, 0) == '#');
        assert(b.get_at(1, 0) == '#');
        assert(b.get_at(2, 0) == '#');
        // �����e�X�g�B
        b.grow_x0(1, '|');
        assert(b.get_on(-1, 1) == '|');
        assert(b.get_on(0, 1) == '#');
        assert(b.get_on(1, 1) == '#');
        assert(b.get_on(2, 1) == '#');
        assert(b.get_at(0, 1) == '|');
        assert(b.get_at(1, 1) == '#');
        assert(b.get_at(2, 1) == '#');
        assert(b.get_at(3, 1) == '#');
        // �폜�Ɛ����e�X�g�B
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
        // �폜�Ɛ����e�X�g�B
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
        // ���낢��e�X�g�B
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

// �u�P��Q���玩�������v����������N���X�B
template <typename t_char, bool t_fixed>
struct from_words_t {
    typedef std::basic_string<t_char> t_string; // ������N���X�B

    inline static board_t<t_char, t_fixed> s_solution; // ���̔Ֆʃf�[�^�B
    board_t<t_char, t_fixed> m_board; // �Ֆʃf�[�^�B
    std::unordered_set<t_string> m_words, m_dict; // �P��Q�Ǝ����f�[�^�B
    std::unordered_set<pos_t> m_crossable_x, m_crossable_y;
    int m_iThread; // �X���b�h�̃C���f�b�N�X�B

    // ����K�p����B
    bool apply_candidate(const candidate_t<t_char>& cand) {
        auto& word = cand.m_word; // ��₩��P����擾�B
        m_words.erase(word); // �g�p�ς݂Ƃ��ĒP��Q���珜������B
        int x = cand.m_x, y = cand.m_y; // ���̃}�X�ʒu�B
        if (cand.m_vertical) { // ��₪�c�������H
            // �Ֆʂ��Œ�T�C�Y�Ȃ�T�C�Y���m�F����B�Œ�T�C�Y�łȂ���΁A�T�C�Y���g������B
            if (t_fixed) {
                if (!m_board.ensure_y(y) || !m_board.ensure_y(y + int(word.size()) - 1))
                    return false;
            } else {
                m_board.ensure(x, y - 1);
                m_board.ensure(x, y + int(word.size()));
            }
            // �P��̏㉺�ɉ\�Ȃ�΍��}�X��z�u�B
            m_board.set_on(x, y - 1, '#');
            m_board.set_on(x, y + int(word.size()), '#');
            // �P���K�p���Ȃ�������\�����X�V����B
            for (size_t ich = 0; ich < word.size(); ++ich) {
                int y0 = y + int(ich);
                m_board.set_on(x, y0, word[ich]);
                m_crossable_y.erase({x, y0});
                if (m_board.is_crossable_x(x, y0))
                    m_crossable_x.insert({ x, y0 });
            }
        } else { // ��₪���R�������H
            // �Ֆʂ��Œ�T�C�Y�Ȃ�T�C�Y���m�F����B�Œ�T�C�Y�łȂ���΁A�T�C�Y���g������B
            if (t_fixed) {
                if (!m_board.ensure_x(x) || !m_board.ensure_x(x + int(word.size()) - 1))
                    return false;
            } else {
                m_board.ensure(x - 1, y);
                m_board.ensure(x + int(word.size()), y);
            }
            // �P��̍��E�ɉ\�Ȃ�΍��}�X��z�u�B
            m_board.set_on(x - 1, y, '#');
            m_board.set_on(x + int(word.size()), y, '#');
            // �P���K�p���Ȃ�������\�����X�V����B
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

    // ���R�����̌��Q���擾����B
    std::vector<candidate_t<t_char> >
    get_candidates_x(int x, int y) const {
        std::vector<candidate_t<t_char> > cands; // ���Q�B

        // �}�X(x, y)�ɒ��ڂ���B
        t_char ch0 = m_board.get_on(x, y); // (x, y)�̕������擾�B
        assert(is_letter(ch0));

        // ���̍��E�̃}�X���擾����B
        t_char ch1 = m_board.get_on(x - 1, y);
        t_char ch2 = m_board.get_on(x + 1, y);
        if (!is_letter(ch1) && !is_letter(ch2)) {
            t_char sz[2] = { ch0, 0 };
            cands.push_back({ x, y, sz, false });
        }

        // �e�P��ɂ��āB
        for (auto& word : m_words) {
            if (s_canceled || s_generated) { // �L�����Z���ς݂������ς݂Ȃ�I���B
                cands.clear();
                return cands;
            }

            // �P�ꒆ�̊e�ʒu�ɂ��āA�A�A
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0) // ��������v���Ȃ���΃X�L�b�v�B
                    continue;

                // ���E�̃��R�ʒu���擾�B
                int x0 = x - int(ich);
                int x1 = x0 + int(word.size());
                bool matched = true; // ��v���Ă���Ɖ���B
                if (matched) {
                    // ���R�����̒P��ɂ��āA���E�̂Q�}�X�ɂ���
                    t_char ch1 = m_board.get_on(x0 - 1, y);
                    t_char ch2 = m_board.get_on(x1, y);
                    if (is_letter(ch1) || is_letter(ch2)) { // �����}�X�Ȃ�
                        matched = false; // ��v���Ă��Ȃ��I
                    }
                }
                if (matched) {
                    for (size_t k = 0; k < word.size(); ++k) { // �e�}�X�ɂ���
                        t_char ch3 = m_board.get_on(x0 + int(k), y);
                        if (ch3 != '?' && word[k] != ch3) { // ���m�̃}�X�łȂ���v���Ȃ����
                            matched = false; // ��v���Ă��Ȃ��I
                            break;
                        }
                    }
                }
                if (matched) {
                    cands.push_back({x0, y, word, false}); // ��v���Ă���Ό���ǉ��B
                }
            }
        }

        return cands; // ���Q��Ԃ��B
    }

    // �^�e�����̌��Q���擾����B
    std::vector<candidate_t<t_char> >
    get_candidates_y(int x, int y) const {
        std::vector<candidate_t<t_char> > cands;

        // �}�X(x, y)�ɒ��ڂ���B
        t_char ch0 = m_board.get_on(x, y); // (x, y)�̕������擾�B
        assert(is_letter(ch0));

        // ���̏㉺�̃}�X���擾����B
        t_char ch1 = m_board.get_on(x, y - 1);
        t_char ch2 = m_board.get_on(x, y + 1);
        if (!is_letter(ch1) && !is_letter(ch2)) { // �����Ƃ������}�X�łȂ����
            t_char sz[2] = { ch0, 0 };
            cands.push_back({ x, y, sz, true }); // 1�}�X�̌�� { ch0 } ��ǉ��B
        }

        // �e�P��ɂ��āA�A�A
        for (auto& word : m_words) {
            if (s_canceled || s_generated) { // �L�����Z���ς݂������ς݂Ȃ�I���B
                cands.clear();
                return cands;
            }

            // �P�ꒆ�̊e�ʒu�ɂ��āA�A�A
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (word[ich] != ch0) // ��������v���Ȃ���΃X�L�b�v�B
                    continue;

                // ���E�̃^�e�ʒu���擾�B
                int y0 = y - int(ich);
                int y1 = y0 + int(word.size());
                bool matched = true; // ��v���Ă���Ɖ���B
                if (matched) {
                    // �c�����̒P��ɂ��āA���E�̂Q�}�X�ɂ���
                    t_char ch1 = m_board.get_on(x, y0 - 1);
                    t_char ch2 = m_board.get_on(x, y1);
                    if (is_letter(ch1) || is_letter(ch2)) { // �����Ƃ������}�X�Ȃ�
                        matched = false; // ��v���Ă��Ȃ��I
                    }
                }
                if (matched) {
                    for (size_t k = 0; k < word.size(); ++k) { // �Ή�����e�}�X�ɂ���
                        t_char ch3 = m_board.get_on(x, y0 + int(k)); // �������擾���A
                        if (ch3 != '?' && word[k] != ch3) { // ���m�}�X�łȂ��A��v���Ă��Ȃ��Ȃ�
                            matched = false; // ��v���Ă��Ȃ��I
                            break;
                        }
                    }
                }
                if (matched) { // ��v���Ă���B
                    cands.push_back({x, y0, word, true}); // ����ǉ��B
                }
            }
        }

        return cands; // ���Q��Ԃ��B
    }

    bool fixup_candidates(std::vector<candidate_t<t_char> >& candidates) {
        std::vector<candidate_t<t_char> > cands; // �K�p���ׂ����B
        std::unordered_set<pos_t> positions; // �g�p�����ʒu�B

        // �e���ɂ��āA�A�A
        for (auto& cand : candidates) {
            if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                return s_generated;
            // ��₪�P�}�X�ł����
            if (cand.m_word.size() == 1) {
                cands.push_back(cand); // �K�p���ׂ�����ǉ��B
                positions.insert( {cand.m_x, cand.m_y} ); // ���̈ʒu�͎g�p�ς݂Ƃ���B
            }
        }

        // �e���ɂ��āA�A�A
        for (auto& cand : candidates) {
            if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                return s_generated;
            // ��₪�P�}�X�łȂ���΁A���g�p�̃}�X�����邩�m�F���A
            if (cand.m_word.size() != 1) {
                if (positions.count(pos_t(cand.m_x, cand.m_y)) == 0)
                    return false; // ���g�p�̃}�X���Ȃ���Ύ��s�B
            }
        }

        // �K�p���ׂ��e���ɂ��āA�A�A
        for (auto& cand : cands) {
            if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                return s_generated;
            apply_candidate(cand); // �K�p���ׂ�����K�p����B
        }
        return true;
    }

    // �����̍ċA�֐��B
    bool generate_recurse() {
        if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
            return s_generated; // �����ς݂Ȃ琬���B

        // �����\�����Ȃ���Ύ��s�B
        if (m_crossable_x.empty() && m_crossable_y.empty())
            return false;

#ifdef XWORDGIVER
        xg_aThreadInfo[m_iThread].m_count = int(m_dict.size() - m_words.size());
#endif

        std::vector<candidate_t<t_char> > candidates; // ���Q�B

        // �������̌����\���ɂ��āA�A�A
        for (auto& cross : m_crossable_x) {
            if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                return s_generated;
            // �������̌��Q���擾�B
            auto cands = get_candidates_x(cross.m_x, cross.m_y);
            if (cands.empty()) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else if (cands.size() == 1 && cands[0].m_word.size() == 1) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else {
                // ���Q��ǉ��B
                candidates.insert(candidates.end(), cands.begin(), cands.end());
            }
        }

        // �^�e�����̌����\���ɂ��āA�A�A
        for (auto& cross : m_crossable_y) {
            if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                return s_generated;
            // �^�e�����̌��Q���擾�B
            auto cands = get_candidates_y(cross.m_x, cross.m_y);
            if (cands.empty()) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else if (cands.size() == 1 && cands[0].m_word.size() == 1) {
                if (m_board.must_be_cross(cross.m_x, cross.m_y))
                    return false;
            } else {
                // ���Q��ǉ��B
                candidates.insert(candidates.end(), cands.begin(), cands.end());
            }
        }

        if (m_words.empty()) { // �c��̒P�ꂪ�Ȃ����
            if (fixup_candidates(candidates)) {
                board_t<t_char, t_fixed> board0 = m_board;
                board0.trim();
                board0.replace('?', '#');
                if (is_solution(board0)) { // �Ֆʂ����Ȃ��
                    std::lock_guard<std::mutex> lock(s_mutex); // �r�����䂵�Ȃ���
                    // �����Z�b�g����
                    s_generated = true;
                    s_solution = board0;
                    return true; // �����B
                }
            }
            return s_generated; // �����ς݂Ȃ琬���B
        }

#ifdef XWORDGIVER
        if (m_dict.size() <= 50 && m_words.size() < m_dict.size() / 2 && !t_fixed) {
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

        // �e���ɂ��āA�A�A
        for (auto& cand : candidates) {
            if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                return s_generated;
            // �������Č���K�p���čċA�E����B
            from_words_t<t_char, t_fixed> copy(*this);
            if (copy.apply_candidate(cand) && copy.generate_recurse()) {
                return true;
            }
        }

        // ���ׂĂ̌���K�p�������A���s�����B
        return false;
    }

    // �g�p�ςݒP����`�F�b�N����B
    bool check_used_words(const board_t<t_char, t_fixed>& board) const
    {
        std::unordered_set<t_string> used; // �g�p�ςݒP���ێ��B

        // �e�s�̊e�}�X�̕��тɂ��āA�A�A
        for (int y = board.m_y0; y < board.m_y0 + board.m_cy; ++y) {
            for (int x = board.m_x0; x < board.m_x0 + board.m_cx - 1; ++x) {
                // �ׂ荇���Q�}�X���擾�B
                auto ch0 = board.get_on(x, y);
                auto ch1 = board.get_on(x + 1, y);
                t_string word;
                word += ch0;
                word += ch1;
                // �����Ƃ������}�X�Ȃ�΁A�A�A
                if (is_letter(ch0) && is_letter(ch1)) {
                    // �P����\�z����B
                    ++x;
                    for (;;) {
                        ++x;
                        ch1 = board.get_on(x, y);
                        if (!is_letter(ch1))
                            break;
                        word += ch1;
                    }
                    // �g�p�ς݂����邩�A�����ɂȂ��P�ꂪ����΁A���s�B
                    if (used.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    // �\�z�����P����g�p�ς݂ƌ��Ȃ��B
                    used.insert(word);
                }
            }
        }

        // �e��̊e�}�X�̕��тɂ��āA�A�A
        for (int x = board.m_x0; x < board.m_x0 + board.m_cx; ++x) {
            for (int y = board.m_y0; y < board.m_y0 + board.m_cy - 1; ++y) {
                // �㉺�ɕ��񂾂Q�}�X���擾�B
                auto ch0 = board.get_on(x, y);
                auto ch1 = board.get_on(x, y + 1);
                t_string word;
                word += ch0;
                word += ch1;
                // ���������}�X�Ȃ�΁A�A�A
                if (is_letter(ch0) && is_letter(ch1)) {
                    // �P����\�z�B
                    ++y;
                    for (;;) {
                        ++y;
                        ch1 = board.get_on(x, y);
                        if (!is_letter(ch1))
                            break;
                        word += ch1;
                    }
                    // �g�p�ς݂����邩�A�����ɂȂ��P�ꂪ����΁A���s�B
                    if (used.count(word) > 0 || m_dict.count(word) == 0) {
                        return false;
                    }
                    // �\�z�����P����g�p�ς݂ƌ��Ȃ��B
                    used.insert(word);
                }
            }
        }

        return used.size() == m_dict.size(); // �g�p�ς݂Ǝ����P�ꐔ����v����ΐ����B
    }

    // �Ֆʃf�[�^�͉����H
    bool is_solution(const board_t<t_char, t_fixed>& board) const {
        if (board.count('?') > 0)
            return false; // ���m�}�X������Ύ��s�B
        if (!board.rules_ok())
            return false; // ���[���ɔ����Ă���Ύ��s�B
        return check_used_words(board); // �g�p�ςݒP����`�F�b�N����B
    }

    // ��������B
    bool generate() {
        if (m_words.empty())
            return false; // �P��Q����ł���Ύ��s�B

        // �ŏ��̒P������Ƃ��ēK�p����B���ꂪ�����̎�ƂȂ�B
        auto word = *m_words.begin();
        candidate_t<t_char> cand = { 0, 0, word, false };
        apply_candidate(cand);
        // �����̍ċA���J�n�B
        return generate_recurse();
    }

    // �����X���b�h�̃v���V�[�W���B
    static bool generate_proc(const std::unordered_set<t_string> *words, int iThread) {
        // �����̎���Z�b�g����B
        std::srand(uint32_t(::GetTickCount64()) ^ ::GetCurrentThreadId());
#ifdef _WIN32
        // ���\���d�����ăX���b�h�̗D��x���w�肷��B
        ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
        // �����p�̃f�[�^���������B
        from_words_t<t_char, t_fixed> data;
        data.m_iThread = iThread;
        data.m_words = *words;
        data.m_dict = std::move(*words);
        delete words; // �P��̏��L���̓X���b�h�̃v���V�[�W���ɓn����Ă���̂ł����Ŕj������B
        return data.generate(); // �������J�n����B
    }

    // �����p�̃w���p�[�֐��B
    static bool
    do_generate(const std::unordered_set<t_string>& words,
                int num_threads = get_num_processors())
    {
#ifdef SINGLETHREADDEBUG // �V���O���X���b�h�e�X�g�p�B
        auto clone = new std::unordered_set<t_string>(words);
        generate_proc(clone, 0);
#else // �����X���b�h�B
        // �e�X���b�h�ɂ��āA�A�A
        for (int i = 0; i < num_threads; ++i) {
            // �X���b�h�ɏ��L�������n�������̂ŉ�����new���g�킹�Ă������������B
            auto clone = new std::unordered_set<t_string>(words);
            try {
                // �X���b�h�𐶐��B�؂藣���B
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

template <typename t_char>
struct non_add_block_t {
    typedef std::basic_string<t_char> t_string; // ������B
    enum { t_fixed = 1 }; // �Œ肩�H

    inline static board_t<t_char, t_fixed> s_solution; // ���̔Ֆʃf�[�^�B
    board_t<t_char, t_fixed> m_board; // �Ֆʃf�[�^�B
    std::unordered_set<t_string> m_words, m_dict; // �P��Q�Ǝ����f�[�^�B
    std::unordered_set<pos_t> m_checked_x, m_checked_y;
    int m_iThread;

    // �p�^�[��������Q���擾����B
    std::vector<candidate_t<t_char>>
    get_candidates_from_pat(int x, int y, const t_string& pat, bool vertical) const {
        std::vector<candidate_t<t_char>> ret; // ���Q���߂�l�B
        assert(pat.size() > 0); // ��₪����Ɖ���B
        if (pat.find('?') == pat.npos) { // ���m�̃}�X���Ȃ����
            if (m_words.count(pat) > 0)
                ret.push_back({ x, y, pat, vertical }); // ���̌�₪�߂�l�̈�B
            return ret;
        }
        ret.reserve(m_words.size() >> 4); // ���x�̂��߂ɑO�����ė\�񂵂Ċm�ہB
        for (auto& word : m_words) { // �e�P��ɂ��āA�A�A
            if (word.size() != pat.size()) // �P��ƃp�^�[���̒������s��v�Ȃ�΃X�L�b�v�B
                continue;
            bool matched = true; // ��v���Ă���Ɖ���B
            for (size_t ich = 0; ich < word.size(); ++ich) {
                if (pat[ich] != '?' && pat[ich] != word[ich]) { // ���m�̃}�X�����ꂩ�H
                    matched = false; // ��v���Ă��Ȃ��I
                    break;
                }
            }
            if (matched)
                ret.push_back({ x, y, word, vertical}); // ��v����΂��̌�₪�߂�l�̈�B
        }
        return ret; // ���Q��Ԃ��B
    }

    // �P��Q���`�F�b�N����B
    bool check_words() {
        // �e�s�̊e�}�X�ɂ��āA�A�A
        for (int y = 0; y < m_board.m_cy; ++y) {
            for (int x = 0; x < m_board.m_cx; ++x) {
                if (m_checked_x.count(pos_t(x, y)) > 0)
                    continue; // �`�F�b�N�ς݂Ȃ�X�L�b�v�B

                int x0;
                auto pat = m_board.get_pat_x(x, y, &x0); // �������̃p�^�[�����擾�B
                if (pat.find('?') != pat.npos) // ���m�̃}�X������΃X�L�b�v�B
                    continue;

                if (pat.size() <= 1) { // �p�^�[�����P�}�X�ł���΁A
                    m_checked_x.emplace(x, y); // �`�F�b�N�ς݂ƌ��Ȃ��B
                    continue;
                }

                if (m_dict.count(pat) == 0) // �P�ꂪ�o�^����Ă��Ȃ���Ύ��s�B
                    return false;

                // �p�^�[���̊e�}�X�ʒu�ɂ���
                for (size_t i = 0; i < pat.size(); ++i, ++x0) {
                    m_checked_x.emplace(x0, y); // �`�F�b�N�ς݂Ƃ���B
                }
            }
        }

        // �e��̊e�}�X�ɂ��āA�A�A
        for (int x = 0; x < m_board.m_cx; ++x) {
            for (int y = 0; y < m_board.m_cy; ++y) {
                if (m_checked_y.count(pos_t(x, y)) > 0)
                    continue; // �`�F�b�N�ς݂Ȃ�X�L�b�v�B

                int y0;
                auto pat = m_board.get_pat_y(x, y, &y0); // �c�����̃p�^�[�����擾�B
                if (pat.find('?') != pat.npos) // ���m�̃}�X������΃X�L�b�v�B
                    continue;

                if (pat.size() <= 1) { // �p�^�[�����P�}�X�ł���΁A
                    m_checked_y.emplace(x, y); // �`�F�b�N�ς݂ƌ��Ȃ��B
                    continue;
                }

                if (m_dict.count(pat) == 0) // �P�ꂪ�o�^����Ă��Ȃ���Ύ��s�B
                    return false;

                // �p�^�[���̊e�}�X�ʒu�ɂ���
                for (size_t i = 0; i < pat.size(); ++i, ++y0) {
                    m_checked_y.emplace(x, y0); // �`�F�b�N�ς݂Ƃ���B
                }
            }
        }

        return true;
    }

    // x�����Ɍ���K�p����B
    bool apply_candidate_x(const candidate_t<t_char>& cand) {
        auto& word = cand.m_word;
        m_words.erase(word); // �P��Q����P�����菜���B
        int x = cand.m_x, y = cand.m_y;
        for (size_t ich = 0; ich < word.size(); ++ich, ++x) {
            m_checked_x.emplace(x, y); // �`�F�b�N�ς݂Ƃ���B
            m_board.set_at(x, y, word[ich]); // �}�X(x, y)�ɓK�p����B
        }
        return true;
    }
    // y�����Ɍ���K�p����B
    bool apply_candidate_y(const candidate_t<t_char>& cand) {
        auto& word = cand.m_word;
        m_words.erase(word); // �P��Q����P�����菜���B
        int x = cand.m_x, y = cand.m_y;
        for (size_t ich = 0; ich < word.size(); ++ich, ++y) {
            m_checked_y.emplace(x, y); // �`�F�b�N�ς݂Ƃ���B
            m_board.set_at(x, y, word[ich]); // �}�X(x, y)�ɓK�p����B
        }
        return true;
    }

    // �����ċA�p�̊֐��B
    bool generate_recurse() {
        // �L�����Z���ς݁A�����ς݁A�������͒P��`�F�b�N�Ɏ��s�Ȃ�ΏI���B
        if (s_canceled || s_generated || !check_words())
            return s_generated; // �����ς݂Ȃ琬���B

        // �e�s�̃}�X�̕��тɂ��āB
        for (int y = 0; y < m_board.m_cy; ++y) {
            for (int x = 0; x < m_board.m_cx - 1; ++x) {
                // �L�����Z���ς݂������ς݂Ȃ�I���B
                if (s_canceled || s_generated)
                    return s_generated; // �����ς݂Ȃ琬���B

                // �������̂Q�}�X�̕��т�����B
                t_char ch0 = m_board.get_at(x, y);
                t_char ch1 = m_board.get_at(x + 1, y);
                // �����}�X�Ɩ��m�̃}�X�A�������́A���m�̃}�X�ƕ����}�X������ł���΁A�A�A
                if ((is_letter(ch0) && ch1 == '?') || (ch0 == '?' && is_letter(ch1))) {
                    int x0;
                    auto pat = m_board.get_pat_x(x, y, &x0); // x�����Ƀp�^�[�����擾�B
                    auto cands = get_candidates_from_pat(x0, y, pat, false); // �p�^�[��������Q���擾�B
                    if (cands.empty())
                        return false; // ��₪�Ȃ���Ύ��s�B
                    // ���Q�������_���V���b�t���B
                    crossword_generation::random_shuffle(cands.begin(), cands.end());
                    // �e���ɂ��āA�A�A
                    for (auto& cand : cands) {
                        if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                            return s_generated; // �����ς݂Ȃ琬���B

                        // �������Č���K�p���čċA�E���򂷂�B
                        non_add_block_t<t_char> copy(*this);
                        copy.apply_candidate_x(cand);
                        if (copy.generate_recurse())
                            break;
                    }
                    return s_generated; // �����ς݂Ȃ琬���B
                }
            }
        }

        // �e��̃}�X�̕��тɂ��āA�A�A
        for (int x = 0; x < m_board.m_cx; ++x) {
            for (int y = 0; y < m_board.m_cy - 1; ++y) {
                // �L�����Z���ς݂������ς݂Ȃ�I���B
                if (s_canceled || s_generated)
                    return s_generated; // �����ς݂Ȃ琬���B

                // �^�e�����̂Q�}�X�̕��т�����B
                t_char ch0 = m_board.get_at(x, y);
                t_char ch1 = m_board.get_at(x, y + 1);
                // �����}�X�Ɩ��m�̃}�X�A�������́A���m�̃}�X�ƕ����}�X������ł���΁A�A�A
                if ((is_letter(ch0) && ch1 == '?') || (ch0 == '?' && is_letter(ch1))) {
                    int y0;
                    auto pat = m_board.get_pat_y(x, y, &y0); // y�����Ƀp�^�[�����擾�B
                    auto cands = get_candidates_from_pat(x, y0, pat, true); // �p�^�[��������Q���擾�B
                    if (cands.empty())
                        return false; // ��₪�Ȃ���Ύ��s�B
                    // ���Q�������_���V���b�t���B
                    crossword_generation::random_shuffle(cands.begin(), cands.end());
                    // �e���ɂ��āA�A�A
                    for (auto& cand : cands) {
                        if (s_canceled || s_generated) // �L�����Z���ς݂������ς݂Ȃ�I���B
                            return s_generated; // �����ς݂Ȃ琬���B

                        // �������Č���K�p���čċA�E����B
                        non_add_block_t<t_char> copy(*this);
                        copy.apply_candidate_y(cand);
                        if (copy.generate_recurse())
                            break;
                    }
                    return s_generated; // �����ς݂Ȃ琬���B
                }
            }
        }

        // �Ֆʂ����Ȃ�ΐ����B
        if (is_solution(m_board)) {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_generated = true;
            s_solution = m_board;
            return true;
        }

        return s_generated; // �����ς݂Ȃ琬���B
    }

    // �Ֆʂ͉����H
    bool is_solution(const board_t<t_char, t_fixed>& board) {
        return (board.count('?') == 0);
    }

    // �������s���֐��B
    bool generate() {
        // �P�ꂪ�Ȃ���ΐ����ł��Ȃ��B
        if (m_words.empty())
            return false;

        // �������̓��[���ɓK�����Ă���Ɖ��肷��B
        assert(m_board.rules_ok());

        // �����}�X������΁A�ċA�p�̊֐����Ăяo���B
        if (m_board.has_letter())
            return generate_recurse();

        // �P��Q�������_���V���b�t���B
        std::vector<t_string> words(m_words.begin(), m_words.end());
        crossword_generation::random_shuffle(words.begin(), words.end());

        // �Ֆʂ̊e�}�X�ɂ��āB
        for (int y = 0; y < m_board.m_cy; ++y) {
            for (int x = 0; x < m_board.m_cx - 1; ++x) {
                // �����ς݂������̓L�����Z���ς݂Ȃ�ΏI���B
                if (s_canceled || s_generated)
                    return s_generated;
                // ���m�̃}�X������΁A�A�A
                if (m_board.get_at(x, y) == '?' && m_board.get_at(x + 1, y) == '?') {
                    int x0;
                    auto pat = m_board.get_pat_x(x, y, &x0); // x�����̃p�^�[�����擾���A
                    auto cands = get_candidates_from_pat(x0, y, pat, false); // �p�^�[����������擾�B
                    for (auto& cand : cands) { // �e���ɂ���
                        // �L�����Z���ς݂������͐����ς݂Ȃ�I���B
                        if (s_canceled || s_generated)
                            return s_generated; // �����ς݂Ȃ琬���B

                        // �������Č���K�p���čċA�B
                        non_add_block_t<t_char> copy(*this);
                        copy.apply_candidate_x(cand);
                        if (copy.generate_recurse())
                            return true; // �����ɐ����B
                    }
                    // �p�^�[���̒�������x�����ɃX�L�b�v�B
                    x += int(pat.size());
                }
            }
        }

        return false;
    }

    // �����X���b�h�̃v���V�[�W���B
    static bool
    generate_proc(board_t<t_char, t_fixed> *pboard,
                  std::unordered_set<t_string> *pwords, int iThread)
    {
        std::srand(uint32_t(::GetTickCount64()) ^ ::GetCurrentThreadId());
#ifdef _WIN32
        //::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
        non_add_block_t<t_char> data;
        data.m_iThread = iThread;
        data.m_board = std::move(*pboard);
        delete pboard;
        data.m_words = *pwords;
        data.m_dict = std::move(*pwords);
        delete pwords;
        return data.generate();
    }

    // �������s���w���p�[�֐��B
    static bool
    do_generate(const board_t<t_char, t_fixed>& board,
                const std::unordered_set<t_string>& words,
                int num_threads = get_num_processors())
    {
        board_t<t_char, t_fixed> *pboard = nullptr;
        std::unordered_set<t_string> *pwords = nullptr;
#ifdef SINGLETHREADDEBUG // �V���O���X���b�h�e�X�g�p�B
        pboard = new board_t<t_char, t_fixed>(board);
        pwords = new std::unordered_set<t_string>(words);
        generate_proc(pboard, pwords, 0);
#else // �����X���b�h�B
        for (int i = 0; i < num_threads; ++i) {
            // �X���b�h�ɏ��L�������n�������̂ŉ�����new���g�킹�Ă������������B
            pboard = new board_t<t_char, t_fixed>(board);
            pwords = new std::unordered_set<t_string>(words);
            try {
                // �X���b�h�𐶐��B�؂藣���B
                std::thread t(generate_proc, pboard, pwords, i);
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

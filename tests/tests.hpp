#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>

#include <vector>
#include <algorithm>
#include <queue>
#include <unordered_set>

#ifndef ARRAYSIZE
    #define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

typedef std::vector<char> BOARD;
typedef std::pair<int, int> POS;

extern int g_rows;
extern int g_columns;

inline char
get_at(const BOARD& board, int i, int j)
{
    return board[i * g_columns + j];
}

inline void
set_at(BOARD& board, int i, int j, char ch)
{
    board[i * g_columns + j] = ch;
}

inline bool in_range(int i, int j)
{
    return i < g_rows && j < g_columns;
}

inline bool corner_black(const BOARD& board)
{
    if (get_at(board, 0, 0) != ' ')
        return true;
    if (get_at(board, g_rows - 1, 0) != ' ')
        return true;
    if (get_at(board, g_rows - 1, g_columns - 1) != ' ')
        return true;
    if (get_at(board, 0, g_columns - 1) != ' ')
        return true;
    return false;
}

inline bool double_black(const BOARD& board)
{
    for (int i = 0; i < g_rows; ++i)
    {
        for (int j = 0; j < g_columns - 1; ++j)
        {
            if (get_at(board, i, j) != ' ' &&
                get_at(board, i, j + 1) != ' ')
            {
                return true;
            }
        }
    }
    for (int j = 0; j < g_columns; ++j)
    {
        for (int i = 0; i < g_rows - 1; ++i)
        {
            if (get_at(board, i, j) != ' ' &&
                get_at(board, i + 1, j) != ' ')
            {
                return true;
            }
        }
    }

    return false;
}

inline bool tri_black_around(const BOARD& board)
{
    for (int i = g_rows - 2; i >= 1; --i) {
        for (int j = g_columns - 2; j >= 1; --j) {
            if ((get_at(board, i - 1, j) == '#') + (get_at(board, i + 1, j) == '#') + 
                (get_at(board, i, j - 1) == '#') + (get_at(board, i, j + 1) == '#') >= 3)
            {
                return true;
            }
        }
    }
    return false;
}

inline bool divided_by_black(const BOARD& board)
{
    int count = g_rows * g_columns;

    char *pb = new char[count];

    memset(pb, 0, count);

    std::queue<POS> positions;
    positions.emplace(0, 0);

    while (!positions.empty())
    {
        POS pos = positions.front();
        positions.pop();
        if (!pb[pos.first * g_columns + pos.second])
        {
            pb[pos.first * g_columns + pos.second] = 1;
            if (pos.first > 0 && get_at(board, pos.first - 1, pos.second) != '#')
                positions.emplace(pos.first - 1, pos.second);
            if (pos.first < g_rows - 1 && get_at(board, pos.first + 1, pos.second) != '#')
                positions.emplace(pos.first + 1, pos.second);
            if (pos.second > 0 && get_at(board, pos.first, pos.second - 1) != '#')
                positions.emplace(pos.first, pos.second - 1);
            if (pos.second < g_columns - 1 && get_at(board, pos.first, pos.second + 1) != '#')
                positions.emplace(pos.first, pos.second + 1);
        }
    }

    while (count-- > 0)
    {
        if (pb[count] == 0 && board[count] != '#')
        {
            delete[] pb;
            return true;
        }
    }

    delete[] pb;

    return false;
}

inline bool is_ok(const BOARD& board)
{
    if (corner_black(board))
        return false;

    if (double_black(board))
        return false;

    if (tri_black_around(board))
        return false;

    if (divided_by_black(board))
        return false;

    return true;
}

inline bool check_space(const BOARD& board, int max_count)
{
    for (int i = 0; i < g_rows; ++i)
    {
        int count = 0;
        for (int j = 0; j < g_columns; ++j)
        {
            if (get_at(board, i, j) == ' ')
            {
                ++count;
            }
            else
            {
                count = 0;
            }
            if (count > max_count)
                return false;
        }
    }

    for (int j = 0; j < g_columns; ++j)
    {
        int count = 0;
        for (int i = 0; i < g_rows; ++i)
        {
            if (get_at(board, i, j) == ' ')
            {
                ++count;
            }
            else
            {
                count = 0;
            }
            if (count > max_count)
                return false;
        }
    }

    return true;
}

inline bool check_space(const BOARD& board, int max_count, POS& pos)
{
    for (int i = 0; i < g_rows; ++i)
    {
        int count = 0;
        for (int j = 0; j < g_columns; ++j)
        {
            if (get_at(board, i, j) == ' ')
            {
                ++count;
            }
            else
            {
                count = 0;
            }
            if (count > max_count)
            {
                pos = { i, j };
                return false;
            }
        }
    }

    for (int j = 0; j < g_columns; ++j)
    {
        int count = 0;
        for (int i = 0; i < g_rows; ++i)
        {
            if (get_at(board, i, j) == ' ')
            {
                ++count;
            }
            else
            {
                count = 0;
            }
            if (count > max_count)
            {
                pos = { i, j };
                return false;
            }
        }
    }

    return true;
}

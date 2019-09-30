#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>

#include <vector>
#include <algorithm>
#include <queue>

typedef std::vector<char> BOARD;
typedef std::pair<int, int> POS;

int g_rows;
int g_columns;
int g_trial_count;

bool g_solved;
BOARD g_answer;

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

bool corner_black(const BOARD& board)
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

bool double_black(const BOARD& board)
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

bool tri_black_around(const BOARD& board)
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

bool divided_by_black(const BOARD& board)
{
    int nCount = g_rows * g_columns;

    char *pb = new char[nCount];

    memset(pb, 0, nCount);

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

    while (nCount-- > 0)
    {
        if (pb[nCount] == 0 && board[nCount] != '#')
        {
            delete[] pb;
            return true;
        }
    }

    delete[] pb;

    return false;
}

inline bool is_valid(const BOARD& board)
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

bool do_recurse(const BOARD& board)
{
    if (g_solved)
        return true;

    if (!is_valid(board))
        return false;

    for (int i = 0; i < g_rows; ++i)
    {
        for (int j = 0; j < g_columns; ++j)
        {
            if (g_solved)
                return true;

            if (get_at(board, i, j) == ' ')
            {
                int lo = j;
                while (lo > 0) {
                    if (get_at(board, i, lo - 1) != ' ')
                        break;
                    lo--;
                }
                while (j + 1 < g_columns) {
                    if (get_at(board, i, j + 1) != ' ')
                        break;
                    j++;
                }
                const int hi = j;
                j++;

                if (lo + 4 <= hi)
                {
                    char a[] = {0, 1, 2, 3};
                    std::random_shuffle(std::begin(a), std::end(a));

                    for (int k = 0; k < 4; ++k)
                    {
                        if (g_solved)
                            return true;
                        BOARD copy(board);
                        set_at(copy, i, lo + a[k], '#');
                        if (is_valid(copy) && do_recurse(copy))
                            return true;
                    }
                    return false;
                }
            }
        }
    }

    for (int j = 0; j < g_columns; ++j)
    {
        for (int i = 0; i < g_rows; ++i)
        {
            if (g_solved)
                return true;

            if (get_at(board, i, j) == ' ')
            {
                int lo = i;
                while (lo > 0) {
                    if (get_at(board, lo - 1, j) != ' ')
                        break;
                    lo--;
                }
                while (i + 1 < g_rows) {
                    if (get_at(board, i + 1, j) != ' ')
                        break;
                    i++;
                }
                const int hi = i;
                i++;

                if (lo + 4 <= hi)
                {
                    char a[] = {0, 1, 2, 3};
                    std::random_shuffle(std::begin(a), std::end(a));

                    for (int k = 0; k < 4; ++k)
                    {
                        if (g_solved)
                            return true;
                        BOARD copy(board);
                        set_at(copy, lo + a[k], j, '#');
                        if (is_valid(copy) && do_recurse(copy))
                            return true;
                    }
                    return false;
                }
            }
        }
    }

    g_solved = true;
    g_answer = board;
    return true;
}

int main(int argc, char **argv)
{
    std::srand(std::time(NULL));
    g_solved = false;

    g_rows = 10;
    g_columns = 10;
    g_trial_count = 10;

    if (argc > 1)
    {
        g_rows = atoi(argv[1]);
    }
    if (argc > 2)
    {
        g_columns = atoi(argv[2]);
    }
    if (argc > 3)
    {
        g_trial_count = atoi(argv[3]);
    }

    for (int i = 0; i < g_trial_count; ++i)
    {
        BOARD board(g_rows * g_columns, ' ');
        do_recurse(board);

        puts("answer:");
        for (int i = 0; i < g_rows; ++i)
        {
            putchar('|');
            for (int j = 0; j < g_columns; ++j)
            {
                putchar(get_at(g_answer, i, j));
            }
            putchar('|');
            putchar('\n');
        }
    }

    return 0;
}

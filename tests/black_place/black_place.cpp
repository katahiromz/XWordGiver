#include "../tests.hpp"

int g_rows;
int g_columns;
int g_trial_count;

bool g_solved;
BOARD g_answer;

bool do_recurse(const BOARD& board)
{
    if (g_solved)
        return true;

    if (!is_ok(board))
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
                        if (is_ok(copy) && do_recurse(copy))
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
                        if (is_ok(copy) && do_recurse(copy))
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

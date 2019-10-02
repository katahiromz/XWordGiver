#include "../tests.hpp"

int g_rows;
int g_columns;
int g_trial_count;

bool g_solved;
BOARD g_answer;

typedef char TEMPLATE[3][4];

TEMPLATE g_templates[] =
{
    {
        "# #",
        "   ",
        "   ",
    },
    {
        "#  ",
        " # ",
        "   ",
    },
    {
        "#  ",
        "  #",
        "   ",
    },
    {
        "#  ",
        "   ",
        "#  ",
    },
    {
        "#  ",
        "   ",
        " # ",
    },
    {
        "#  ",
        "   ",
        "  #",
    },
    {
        " # ",
        "  #",
        "   ",
    },
    {
        " # ",
        "#  ",
        "   ",
    },
    {
        " # ",
        "  #",
        "   ",
    },
    {
        " # ",
        "   ",
        "#  ",
    },
    {
        " # ",
        "   ",
        " # ",
    },
    {
        " # ",
        "   ",
        "  #",
    },
    {
        "  #",
        "   ",
        "#  ",
    },
    {
        "  #",
        "   ",
        " # ",
    },
    {
        "  #",
        "   ",
        "  #",
    },
    {
        "   ",
        "# #",
        "   ",
    },
    {
        "   ",
        "#  ",
        "  #",
    },
    {
        "   ",
        " # ",
        "   ",
    },
    {
        "# #",
        "   ",
        "#  ",
    },
    {
        "#  ",
        "  #",
        "#  ",
    },
    {
        " # ",
        "  #",
        "#  ",
    },
    {
        "#  ",
        "  #",
        " # ",
    },
    {
        "  #",
        "#  ",
        "  #",
    },
    {
        " # ",
        "#  ",
        "  #",
    },
    {
        "  #",
        "#  ",
        " # ",
    },
    {
        "# #",
        "   ",
        "# #",
    },
};

int max_count = 4;

bool do_recurse_2(const BOARD& board, int level)
{
    if (g_solved)
        return true;

    if (!is_ok(board))
        return false;

    printf("level:%d\n", level);
    fflush(stdout);

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
                std::vector<int> a;
                for (int k = 0; k < count; ++k)
                {
                    a.push_back(k);
                }
                std::random_shuffle(a.begin(), a.end());

                for (int k = 0; k < count; ++k)
                {
                    BOARD copy(board);
                    //printf("(%d, %d)\n", i, j - count + 1 + k);
                    //fflush(stdout);
                    set_at(copy, i, j - count + 1 + k, '#');
                    if (do_recurse_2(copy, level + 1))
                        return true;
                }
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
                std::vector<int> a;
                for (int k = 0; k < count; ++k)
                {
                    a.push_back(k);
                }
                std::random_shuffle(a.begin(), a.end());

                for (int k = 0; k < count; ++k)
                {
                    BOARD copy(board);
                    //printf("(%d, %d)\n", i - count + 1 + k, j);
                    //fflush(stdout);
                    set_at(copy, i - count + 1 + k, j, '#');
                    if (do_recurse_2(copy, level + 1))
                        return true;
                }
            }
        }
    }

    if (is_ok(board))
    {
        g_solved = true;
        g_answer = board;
        return true;
    }

    return false;
}

bool do_recurse(const BOARD& board)
{
    if (g_solved)
        return true;

    if (!is_ok(board))
        return false;

    BOARD copy(board);
    for (int i = 0; i < (g_rows + 2) / 3; ++i)
    {
        for (int j = 0; j < (g_columns + 2) / 3; ++j)
        {
            do
            {
                TEMPLATE& t = g_templates[rand() % ARRAYSIZE(g_templates)];
                for (int m = 0; m < 3; ++m)
                {
                    for (int n = 0; n < 3; ++n)
                    {
                        int p = 3 * i + m;
                        int q = 3 * j + n;
                        if (in_range(p, q))
                        {
                            set_at(copy, p, q, t[m][n]);
                        }
                    }
                }
            } while (corner_black(copy) || double_black(copy) || tri_black_around(copy));
        }
    }

    return do_recurse_2(copy, 0);
}

int main(int argc, char **argv)
{
    std::srand(std::time(NULL));
    g_solved = false;

    g_rows = 10;
    g_columns = 10;
    g_trial_count = 1;

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
        if (do_recurse(board))
        {
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
        else
        {
            puts("failed");
        }
    }

    return 0;
}

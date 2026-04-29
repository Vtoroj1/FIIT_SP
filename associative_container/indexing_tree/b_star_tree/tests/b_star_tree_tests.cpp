#include "gtest/gtest.h"

#include <random>
#include <vector>
#include <b_star_tree.h>

template<typename tkey, typename tvalue>
bool compare_results(
        std::vector<typename BS_tree<tkey, tvalue>::value_type> const &expected,
        std::vector<typename BS_tree<tkey, tvalue>::value_type> const &actual)
{
    if (expected.size() != actual.size())
    {
        return false;
    }

    for (size_t i = 0; i < expected.size(); ++i)
    {
        if (expected[i].key != actual[i].key)
        {
            return false;
        }

        if (expected[i].value != actual[i].value)
        {
            return false;
        }
    }

    return true;
}

template<typename tvalue>
bool compare_obtain_results(
        std::vector<tvalue> const &expected,
        std::vector<tvalue> const &actual)
{
    if (expected.size() != actual.size())
    {
        return false;
    }

    for (size_t i = 0; i < expected.size(); ++i)
    {
        if (expected[i] != actual[i])
        {
            return false;
        }
    }

    return true;
}

template <typename tkey, typename tvalue>
struct test_data
{
    tkey key;
    tvalue value;
    size_t depth, index;

    test_data(size_t d, size_t i, tkey k, tvalue v) : depth(d), index(i), key(k), value(v) {}
};

template<typename tkey, typename tvalue, typename comp, size_t t>
bool infix_const_iterator_test(
        BS_tree<tkey, tvalue, comp, t> const &tree,
        std::vector<test_data<tkey, tvalue>> const &expected_result)
{
    auto end_infix = tree.cend();
    auto it = tree.cbegin();

    for (auto const &item : expected_result)
    {
        auto data = *it;

        if (it->first != item.key || it->second != item.value || it.depth() != item.depth || it.index() != item.index)
        {
            return false;
        }
        ++it;
    }

    return true;
}

TEST(bTreePositiveTests, test_empty_tree)
{
    /* При создании дерева, оно должно быть пустое */
    std::vector<test_data<int, std::string>> expected_result =
            {

            };

    BS_tree<int, std::string, std::less<int>, 1024> tree(std::less<int>(), nullptr);

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test_split_root_with_t_equal_2)
{
    /* Переполнение корня */
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(1, 0, -10, "g"),
                    test_data<int, std::string>(1, 1, 0, "h"),
                    test_data<int, std::string>(1, 2, 1, "a"),
                    test_data<int, std::string>(1, 3, 2, "b"),
                    test_data<int, std::string>(0, 0, 3, "d"),
                    test_data<int, std::string>(1, 0, 4, "e"),
                    test_data<int, std::string>(1, 1, 15, "c"),
                    test_data<int, std::string>(1, 2, 27, "f"),
            };

    BS_tree<int, std::string, std::less<int>, 2> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(27, std::string("f"));
    tree.emplace(-10, std::string("g"));
    tree.emplace(0, std::string("h"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test_split_root_with_t_equal_3)
{
    /* Переполнение корня с большим значением t */
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(1, 0, 1, "a"),
                    test_data<int, std::string>(1, 1, 2, "b"),
                    test_data<int, std::string>(1, 2, 3, "d"),
                    test_data<int, std::string>(1, 3, 4, "e"),
                    test_data<int, std::string>(1, 4, 15, "c"),
                    test_data<int, std::string>(1, 5, 24, "g"),
                    test_data<int, std::string>(0, 0, 45, "k"),
                    test_data<int, std::string>(1, 0, 100, "f"),
                    test_data<int, std::string>(1, 1, 101, "j"),
                    test_data<int, std::string>(1, 2, 193, "l"),
                    test_data<int, std::string>(1, 3, 456, "h"),
                    test_data<int, std::string>(1, 4, 534, "m")
            };

    BS_tree<int, std::string, std::less<int>, 3> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test_not_split_root)
{
    /* корень не переполняется */
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(0, 0, 1, "a"),
                    test_data<int, std::string>(0, 1, 2, "b"),
                    test_data<int, std::string>(0, 2, 3, "d"),
                    test_data<int, std::string>(0, 3, 4, "e"),
                    test_data<int, std::string>(0, 4, 15, "c"),
                    test_data<int, std::string>(0, 5, 24, "g"),
                    test_data<int, std::string>(0, 6, 45, "k"),
                    test_data<int, std::string>(0, 7, 100, "f"),
                    test_data<int, std::string>(0, 8, 101, "j"),
                    test_data<int, std::string>(0, 9, 193, "l"),
                    test_data<int, std::string>(0, 10, 456, "h"),
                    test_data<int, std::string>(0, 11, 534, "m")
            };

    BS_tree<int, std::string, std::less<int>, 7> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test_erase_from_root)
{
    /* Удаление из корня */
    std::vector<test_data<int, std::string>> expected_result =
            {
                test_data<int, std::string>(0, 0, 1, "a"),
                test_data<int, std::string>(0, 1, 3, "d"),
                test_data<int, std::string>(0, 2, 15, "c"),
            };

    BS_tree<int, std::string, std::less<int>, 2> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));

    tree.erase(2);
    tree.erase(4);

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test_erase_from_leaves_without_merge)
{
    /* Удаляет элементы из листьев, при этом никакие операции для балансировки выполнять не нужно */
    std::vector<test_data<int, std::string>> expected_result_before_erase =
                {
                    test_data<int, std::string>(1, 0, 1, "a"),
                    test_data<int, std::string>(1, 1, 2, "b"),
                    test_data<int, std::string>(1, 2, 3, "d"),
                    test_data<int, std::string>(1, 3, 4, "e"),
                    test_data<int, std::string>(1, 4, 15, "c"),
                    test_data<int, std::string>(0, 0, 24, "g"),
                    test_data<int, std::string>(1, 0, 45, "k"),
                    test_data<int, std::string>(1, 1, 100, "f"),
                    test_data<int, std::string>(1, 2, 101, "j"),
                    test_data<int, std::string>(1, 3, 193, "l"),
                    test_data<int, std::string>(1, 4, 456, "h"),
            };

    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(1, 0, 2, "b"),
                    test_data<int, std::string>(1, 1, 3, "d"),
                    test_data<int, std::string>(1, 2, 15, "c"),
                    test_data<int, std::string>(0, 0, 24, "g"),
                    test_data<int, std::string>(1, 0, 45, "k"),
                    test_data<int, std::string>(1, 1, 101, "j"),
                    test_data<int, std::string>(1, 2, 456, "h"),
            };

    BS_tree<int, std::string, std::less<int>, 2> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result_before_erase));

    tree.erase(1);
    tree.erase(100);
    tree.erase(193);
    tree.erase(4);

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test_erase_with_merge_root)
{
    /* Удаляет элементы из листьев, при этом после удаления 15, нужно будет объединить все узлы в один */
    std::vector<test_data<int, std::string>> expected_result_before_erase =
                {
                    test_data<int, std::string>(1, 0, 1, "a"),
                    test_data<int, std::string>(1, 1, 2, "b"),
                    test_data<int, std::string>(1, 2, 3, "d"),
                    test_data<int, std::string>(1, 3, 4, "e"),
                    test_data<int, std::string>(1, 4, 15, "c"),
                    test_data<int, std::string>(0, 0, 24, "g"),
                    test_data<int, std::string>(1, 0, 45, "k"),
                    test_data<int, std::string>(1, 1, 100, "f"),
                    test_data<int, std::string>(1, 2, 101, "j"),
                    test_data<int, std::string>(1, 3, 193, "l"),
                    test_data<int, std::string>(1, 4, 456, "h"),
            };

    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(0, 0, 2, "b"),
                    test_data<int, std::string>(0, 1, 3, "d"),
                    test_data<int, std::string>(0, 2, 24, "g"),
                    test_data<int, std::string>(0, 3, 45, "k"),
                    test_data<int, std::string>(0, 4, 101, "j"),
                    test_data<int, std::string>(0, 5, 456, "h"),
            };

    BS_tree<int, std::string, std::less<int>, 2> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result_before_erase));

    tree.erase(1);
    tree.erase(100);
    tree.erase(193);
    tree.erase(4);
    tree.erase(15);

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test_element_access)
{
    std::vector<std::string> expected_result =
            {
                    "g",
                    "d",
                    "e",
                    " ",
                    "l",
                    "a",
                    "b",
                    "y"
            };

    BS_tree<int, std::string, std::less<int>, 5> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::move(std::string("a")));
    tree.emplace(2, std::move(std::string("b")));
    tree.emplace(15, std::move(std::string("c")));
    tree.emplace(3, std::move(std::string("d")));
    tree.emplace(4, std::move(std::string("e")));
    tree.emplace(100, std::move(std::string(" ")));
    tree.emplace(24, std::move(std::string("g")));
    tree.emplace(-456, std::move(std::string("h")));
    tree.emplace(101, std::move(std::string("j")));
    tree.emplace(-45, std::move(std::string("k")));
    tree.emplace(-193, std::move(std::string("l")));
    tree.emplace(534, std::move(std::string("m")));
    tree.emplace(1000, std::move(std::string("y")));

    std::vector<std::string> actual_result =
            {
                    tree.at(24),
                    tree.at(3),
                    tree.at(4),
                    tree.at(100),
                    tree.at(-193),
                    tree.at(1),
                    tree.at(2),
                    tree.at(1000)
            };

    EXPECT_TRUE(compare_obtain_results(expected_result, actual_result));
}

TEST(bTreePositiveTests, test_element_access_2)
{
    std::vector<std::string> expected_result =
            {
                    "y",
                    "l",
                    "a",
                    "g",
                    "k",
                    "b",
                    "c",
                    "h"
            };

    BS_tree<int, std::string, std::less<int>, 4> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string(" "));
    tree.emplace(24, std::string("g"));
    tree.emplace(-456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(-45, std::string("k"));
    tree.emplace(-193, std::string("l"));
    tree.emplace(534, std::string("m"));
    tree.emplace(1000, std::string("y"));

    std::vector<std::string> actual_result =
            {
                    tree.at(1000),
                    tree.at(-193),
                    tree.at(1),
                    tree.at(24),
                    tree.at(-45),
                    tree.at(2),
                    tree.at(15),
                    tree.at(-456)
            };

    EXPECT_TRUE(compare_obtain_results(expected_result, actual_result));
}

TEST(bTreePositiveTests, test_lower_and_upper_bound)
{
    std::vector<BS_tree<int, std::string>::value_type> expected_result =
            {
                    { 4, "e" },
                    { 15, "c" },
                    { 24, "g" },
                    { 45, "k" },
                    { 100, "f" },
                    { 101, "j" },
            };

    BS_tree<int, std::string, std::less<int>, 2> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));

    auto b = tree.lower_bound(4);
    auto e = tree.upper_bound(110);
    std::vector<decltype(tree)::value_type> actual_result(b, e);

    EXPECT_TRUE(compare_obtain_results(expected_result, actual_result));
}

TEST(bTreeNegativeTests, test1)
{
    BS_tree<int, std::string, std::less<int>, 3> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));

    EXPECT_EQ(tree.erase(45), tree.end());
}

TEST(bTreeNegativeTests, test3)
{
    BS_tree<int, std::string, std::less<int>, 4> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string(" "));
    tree.emplace(24, std::string("g"));
    tree.emplace(-456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(-45, std::string("k"));
    tree.emplace(-193, std::string("l"));
    tree.emplace(534, std::string("m"));
    tree.emplace(1000, std::string("y"));

    EXPECT_EQ(tree.erase(1001), tree.end());
}

int main(
        int argc,
        char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
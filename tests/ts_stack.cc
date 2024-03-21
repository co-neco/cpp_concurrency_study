#include <ts_stack.hpp>

#include <gtest/gtest.h>
#include <iostream>
#include <thread>

TEST(stack, ts) {

    ts::stack<std::string> s;

    std::thread t1([&s]{
        int times = 0;
        for (int i = 0; i < 10000; ++i)
            s.push(std::to_string(12 + i));

        std::cout << "t1 end" << "\n";
    });

    std::thread t2([&s]{
        for (int i = 20000; i < 30000; ++i)
            s.push(std::to_string(12 + i));

        std::cout << "t2 end" << "\n";
    });

    t1.join();
    t2.join();
    ASSERT_FALSE(s.empty());
    ASSERT_EQ(s.size(), 20000);
}

TEST(stack, lock_free) {

    ts::lock_free::stack<std::string> s;
    s.push("abc");
    auto str = s.pop();
    ASSERT_TRUE(!str->compare("abc"));
    ASSERT_TRUE(s.empty());
}
#include <ts_stack.hpp>

#include <gtest/gtest.h>
#include <iostream>
#include <thread>

TEST(stack, ts) {

    ts::stack<std::string> s;

    std::thread t1([&s]{
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

    std::thread t1([&s]{
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

    for (int i = 0; i < 1001; ++i) {
        s.push(std::to_string(i));
    }

    std::atomic<int> total_num = 0;
    std::vector<std::thread> threads;
    // free 1000 nodes.
    for (int i = 0; i < 10; ++i) {
        std::thread t([&s, &total_num] {
            for (int j = 0; j < 100; ++j) {
                auto rs = s.pop();
                ++total_num;
            }
        });
        threads.push_back(std::move(t));
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_TRUE(*s.pop() == "0");
}
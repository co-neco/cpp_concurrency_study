#include "../ts_quque.hpp"
#include "../ts_tuned_queue.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <thread>

// using ::testing::TestWithParam;
// using ::testing::Values;

// enum class queue_type {
//     ts_queue,
//     ts_fine_grained_queue
// }

// define an interface to use both ts_queue and ts_tuned_queue
// class iqueue {}

// class queueParam : TestWithParam<queue_type> { }

// TEST_P(queueParam, testQueueWithLog) {

//     switch (GetParam()) {
//         case queue_type::ts_queue:

//     }

// }

// INSTANTIATE_TEST_SUITE_P(queueSuite, queueParam, 
//     testing::Values(queue_type::ts_queue, queue_type::ts_fine_grained_queue));

TEST(ts_queue, multithreadrun_log) {

    ts_queue<std::string> q;

    std::thread t1([&q]{
        int times = 0;
        for (int i = 0; i < 10000; ++i)
            q.push(std::to_string(12 + i));

        std::cout << "t1 end" << "\n";
    });

    std::thread t2([&q]{
        for (int i = 20000; i < 30000; ++i)
            q.push(std::to_string(12 + i));

        std::cout << "t2 end" << "\n";
    });

    t1.join();
    t2.join();
    ASSERT_FALSE(q.empty());
    ASSERT_EQ(q.size(), 20000);

    q.clear();

    t1 = std::thread([&q] {
        std::cout << "push item\n";
        q.push("first string");
    });

    t2 = std::thread([&q] {
        std::cout << "wait_and_pop item\n";
        std::string str;
        ASSERT_TRUE(q.wait_and_pop(str));
        std::cout << "item string: " << str << "\n";
        std::cout << "pop again\n";
        auto s_str = q.wait_and_pop();
        ASSERT_TRUE(s_str);
        std::cout << "wait_and_pop string: " << *s_str << "\n";
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::thread t3([&q] {
        std::cout << "push one another item\n";
        q.push("second string");
        std::cout << "push done...\n";
    });

    t1.join();
    t2.join();
    t3.join();
}

TEST(ts_queue, multithreadrun) {

    ts_queue<std::string> q;

    q.push("test");
    std::string str;
    ASSERT_TRUE(q.pop(str));
    ASSERT_TRUE(str == "test");
    q.push("abc");
    auto s_str = q.pop();
    ASSERT_TRUE(s_str);
    ASSERT_TRUE(s_str->compare("abc") == 0);

    std::thread t1([&q] {
        for (int i = 0; i < 1000; ++i)
            q.push("string index: " + std::to_string(i+100));
    });

    std::thread t2([&q] {
        for (int i = 0; i < 500; ++i)
            ASSERT_TRUE(q.wait_and_pop());
    });

    std::thread t3([&q] {
        for (int i = 0; i < 500; ++i) {
            auto s_str = q.wait_and_pop();
            ASSERT_TRUE(s_str);
        }
    });
    
    t1.join();
    t2.join();
    t3.join();

    ASSERT_TRUE(q.empty());
    q.push("1");
    q.push("2");
    q.push("3");
    ASSERT_TRUE(q.size() == 3);
    q.clear();
    ASSERT_TRUE(q.size() == 0);
    ASSERT_TRUE(q.empty());
}

TEST(ts_fine_tuned_queue, multithreadrun) {

    ts::fine_tuned::queue<std::string> q;

    q.push("test");
    std::string str;
    ASSERT_TRUE(q.try_pop(str));
    ASSERT_TRUE(str == "test");
    q.push("abc");
    auto s_str = q.try_pop();
    ASSERT_TRUE(s_str);
    ASSERT_TRUE(s_str->compare("abc") == 0);

    std::thread t1([&q] {
        for (int i = 0; i < 1000; ++i)
            q.push("string index: " + std::to_string(i+100));
    });

    std::thread t2([&q] {
        for (int i = 0; i < 500; ++i)
            ASSERT_TRUE(q.wait_and_pop());
    });

    std::thread t3([&q] {
        for (int i = 0; i < 500; ++i) {
            auto s_str = q.wait_and_pop();
            ASSERT_TRUE(s_str);
        }
    });
    
    t1.join();
    t2.join();
    t3.join();

    ASSERT_TRUE(q.empty());
    q.push("1");
    q.push("2");
    q.push("3");
    ASSERT_TRUE(q.size() == 3);
    q.clear();
    ASSERT_TRUE(q.size() == 0);
    ASSERT_TRUE(q.empty());
}

TEST(ts_fine_tuned_queue, multithreadrun_log) {

    ts::fine_tuned::queue<std::string> q;

    std::thread t1([&q]{
        int times = 0;
        for (int i = 0; i < 10000; ++i)
            q.push(std::to_string(12 + i));

        std::cout << "t1 end" << "\n";
    });

    std::thread t2([&q]{
        for (int i = 20000; i < 30000; ++i)
            q.push(std::to_string(12 + i));

        std::cout << "t2 end" << "\n";
    });

    t1.join();
    t2.join();
    ASSERT_FALSE(q.empty());
    ASSERT_EQ(q.size(), 20000);

    q.clear();

    t1 = std::thread([&q] {
        std::cout << "push item\n";
        q.push("first string");
    });

    t2 = std::thread([&q] {
        std::cout << "wait_and_pop item\n";
        std::string str;
        q.wait_and_pop(str);
        std::cout << "item string: " << str << "\n";
        std::cout << "pop again\n";
        auto s_str = q.wait_and_pop();
        ASSERT_TRUE(s_str);
        std::cout << "wait_and_pop string: " << *s_str << "\n";
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::thread t3([&q] {
        std::cout << "push one another item\n";
        q.push("second string");
        std::cout << "push done...\n";
    });

    t1.join();
    t2.join();
    t3.join();
}
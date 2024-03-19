#include <gtest/gtest.h>

#include "../ts_map.hpp"

TEST(ts_map, multithreadrun) {

    ts::ts_map<int, std::string> m;

    std::thread t1([&m]{
        int times = 0;
        for (int i = 0; i < 30; ++i)
            m.insert(i, std::to_string(i));

        std::cout << "t1 end" << "\n";
    });

    t1.join();
    ASSERT_TRUE(m.size() == 30);
    ASSERT_FALSE(m.empty());

    auto map = m.get_map();
    for (auto& pair : map) {
        std::cout << pair.first << ": " << pair.second << "\n";
    }

    int index = 2;
    ASSERT_TRUE(m.find(index));

    auto str = m.get(index);
    ASSERT_TRUE(str == "2");

    m.erase(index);
    ASSERT_TRUE(m.size() == 29);
    str = m.get(index);
    ASSERT_TRUE(str.empty());

    ASSERT_FALSE(m.find(index));

    // erase even if m had no 'index' key
    m.erase(2);
    m.clear();

    std::thread t2([&m]{
        int times = 0;
        for (int i = 0; i < 1000; ++i) {
            int k = i + 12;
            m.insert(k, std::to_string(k));
        }
    });

    std::thread t3([&m]{
        int times = 0;
        for (int i = 1000; i < 2000; ++i) {
            int k = i + 12;
            m.insert(k, std::to_string(k));
        }
    });

    t2.join();
    t3.join();
    ASSERT_TRUE(m.size() == 2000);

    std::thread t4([&m]{
        int times = 0;
        for (int i = 0; i < 1000; ++i) {
            int k = i + 12;
            m.erase(k);
        }
    });

    std::thread t5([&m]{
        int times = 0;
        for (int i = 1000; i < 2000; ++i) {
            int k = i + 12;
            m.erase(k);
        }
    });

    t4.join();
    t5.join();
    ASSERT_TRUE(m.empty());
}
#include "ts_stack.hpp"
#include "ts_tuned_queue.hpp"

int main(int argc, char* argv[]) {

    ts::lock_free::stack<int> s;
    s.push(3);
    s.push(1);
    s.push(11);
    s.push(13);

    auto a = s.pop();
    std::cout << "first pop: " << *a << "\n";

    auto b = s.pop();
    std::cout << "second pop: " << *b << "\n";

    ts::fine_tuned::queue<std::string> q;
    q.push("111");
    std::cout << *q.wait_and_pop() << "\n";
    std::cout.flush();

    getchar();
    return 0;
}
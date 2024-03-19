#include "ts_stack.hpp"
#include "ts_tuned_queue.hpp"

int main(int argc, char* argv[]) {

    ts_stack<int> s;
    s.push(3);
    s.push(1);
    s.push(11);
    s.push(13);

    int a;
    s.pop(a);
    std::cout << "first pop: " << a << "\n";

    auto b = s.pop();
    std::cout << "second pop: " << *b.get() << "\n";

    ts::fine_tuned::queue<std::string> q;
    q.push("111");
    std::cout << *q.wait_and_pop() << "\n";
    std::cout.flush();

    getchar();
    return 0;
}
#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

namespace ts { namespace lock_free {
    
        class hp_owner {
        private:
            struct hazard_pointer {
                std::atomic<std::thread::id> id;
                std::atomic<void*> pointer;
            };

            static const unsigned int _max_hazard_pointers = 100;
            static std::vector<hazard_pointer> _hazard_pointers;

        private:
            hazard_pointer* hp;

        private:
            static std::vector<hazard_pointer> initialize_hazard_pointers();

        public:
            hp_owner(const hp_owner&) = delete;
            hp_owner& operator=(const hp_owner&) = delete;
            hp_owner() : hp(nullptr) {

                for (int i = 0; i < _max_hazard_pointers; ++i) {
                    std::thread::id empty_id;
                    if (_hazard_pointers[i].id.compare_exchange_strong(
                        empty_id, std::this_thread::get_id())) {
                        hp = &_hazard_pointers[i];
                        break;
                    }
                }

                if (!hp)
                    throw std::runtime_error("No hazard pointers available");
            }

            std::atomic<void*>& get_pointer() {
                return hp->pointer;
            }
        };
}}
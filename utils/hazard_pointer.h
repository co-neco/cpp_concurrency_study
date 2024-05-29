#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>

namespace ts { namespace lock_free {
    
    class hp_owner {
    private:
        struct hazard_pointer {
            std::atomic<std::thread::id> id;
            std::atomic<void*> pointer;
            inline hazard_pointer(): pointer(nullptr) { }
        };

        struct data_to_reclaim {
            void* _data;
            std::function<void(void*)> _deleter;
            data_to_reclaim* _next;

            template < typename T >
            static void do_delete(void* data) {
                delete static_cast<T*>(data);
            }

            template < typename T >
            data_to_reclaim(T* data): 
                _data(data), _deleter(&do_delete<T>), _next(nullptr) { }
            inline ~data_to_reclaim() {
                _deleter(_data);
            }
        };

        static const unsigned int _max_hazard_pointers = 100;
        static std::vector<hazard_pointer> _hazard_pointers;

    private:
        hazard_pointer* _hp;
        std::atomic<data_to_reclaim*> _nodes_to_reclaim;

    private:
        static std::vector<hazard_pointer> initialize_hazard_pointers();

    public:
        hp_owner(const hp_owner&) = delete;
        hp_owner& operator=(const hp_owner&) = delete;
        inline hp_owner() : _hp(nullptr) {

            for (int i = 0; i < _max_hazard_pointers; ++i) {
                std::thread::id empty_id;
                if (_hazard_pointers[i].id.compare_exchange_strong(
                    empty_id, std::this_thread::get_id())) {
                    _hp = &_hazard_pointers[i];
                    break;
                }
            }

            if (!_hp)
                throw std::runtime_error("No hazard pointers available");
        }
        inline ~hp_owner() {
            _hp->pointer.store(nullptr);
            _hp->id.store(std::thread::id());
        }

        inline std::atomic<void*>& get_pointer() {
            return _hp->pointer;
        }

    public:
        static hp_owner& get_hazard_pointer_for_cur_thread();
        static bool outstanding_hazard_pointer_for(void* p);

        void add_to_reclaim_list(data_to_reclaim* data);

        template < typename T >
        void reclaim_later(T* n) {
            add_to_reclaim_list(new data_to_reclaim(n));
        }
        void delete_nodes_with_no_hazards();
    };
}}
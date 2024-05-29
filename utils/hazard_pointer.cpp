#include "hazard_pointer.h"

namespace ts { namespace lock_free {

    std::vector<hp_owner::hazard_pointer> hp_owner::_hazard_pointers = initialize_hazard_pointers();

    std::vector<hp_owner::hazard_pointer> hp_owner::initialize_hazard_pointers() {
        
        std::vector<hazard_pointer> hazard_pointers(_max_hazard_pointers);
        return hazard_pointers;
    }

    hp_owner& hp_owner::get_hazard_pointer_for_cur_thread() {
        thread_local static hp_owner hp;
        return hp;
    }

    bool hp_owner::outstanding_hazard_pointer_for(void* p) {
        for (const auto& hp : _hazard_pointers) {
            if (hp.pointer == p)
                return true;
        }
        return false;
    }

    void hp_owner::add_to_reclaim_list(data_to_reclaim* data) {
        data->_next = _nodes_to_reclaim.load();
        while (!_nodes_to_reclaim.compare_exchange_weak(data->_next, data));
    }

    void hp_owner::delete_nodes_with_no_hazards() {

        data_to_reclaim* cur = _nodes_to_reclaim.exchange(nullptr);

        while (cur) {
            auto next = cur->_next;
            if (outstanding_hazard_pointer_for(cur->_data))
                add_to_reclaim_list(cur);
            else
                delete cur;

            cur = next;
        }
    }
}}
#include "hazard_pointer.h"

namespace ts { namespace lock_free {

    std::vector<hp_owner::hazard_pointer> hp_owner::_hazard_pointers = initialize_hazard_pointers();

    std::vector<hp_owner::hazard_pointer> hp_owner::initialize_hazard_pointers() {
        
        std::vector<hazard_pointer> hazard_pointers(_max_hazard_pointers);
        return hazard_pointers;
    }
}}
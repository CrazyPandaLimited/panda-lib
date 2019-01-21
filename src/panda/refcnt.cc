#include "refcnt.h"

namespace panda {

iptr<helper::weak_storage> panda::Refcnt::get_weak() {
    if (!_weak) {
        _weak = new helper::weak_storage(this);
    }
    return _weak;
}

Refcnt::~Refcnt() {
    if (_weak) {
        _weak->object = nullptr;
    }
}

}

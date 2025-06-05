//
// Created by richard on 5/15/25.
//
#include "ffi/container/array.h"
#include "ffi/container/map.h"
#include "ffi/container/shape.h"
#include "ffi/function.h"

namespace litetvm {
namespace ffi {

TVM_FFI_REGISTER_GLOBAL("ffi.Array").set_body_packed([](PackedArgs args, Any* ret) {
    *ret = Array<Any>(args.data(), args.data() + args.size());
});

TVM_FFI_REGISTER_GLOBAL("ffi.ArrayGetItem").set_body_typed([](const ArrayObj* n, int64_t i) -> Any {
    return n->at(i);
});

TVM_FFI_REGISTER_GLOBAL("ffi.ArraySize").set_body_typed([](const ArrayObj* n) -> int64_t {
    return n->size();
});

// Map
TVM_FFI_REGISTER_GLOBAL("ffi.Map").set_body_packed([](PackedArgs args, Any* ret) {
    TVM_FFI_ICHECK_EQ(args.size() % 2, 0);
    Map<Any, Any> data;
    for (int i = 0; i < args.size(); i += 2) {
        data.Set(args[i], args[i + 1]);
    }
    *ret = data;
});

TVM_FFI_REGISTER_GLOBAL("ffi.MapSize").set_body_typed([](const MapObj* n) -> int64_t {
    return n->size();
});

TVM_FFI_REGISTER_GLOBAL("ffi.MapGetItem").set_body_typed([](const MapObj* n, const Any& k) -> Any {
    return n->at(k);
});

TVM_FFI_REGISTER_GLOBAL("ffi.MapCount").set_body_typed([](const MapObj* n, const Any& k) -> int64_t {
    return n->count(k);
});

// Favor struct outside function scope as MSVC may have bug for in fn scope struct.
class MapForwardIterFunctor {
public:
    MapForwardIterFunctor(MapObj::iterator iter, MapObj::iterator end)
        : iter_(iter), end_(end) {}
    // 0 get current key
    // 1 get current value
    // 2 move to next: return true if success, false if end
    Any operator()(int command) const {
        if (command == 0) {
            return (*iter_).first;
        }

        if (command == 1) {
            return (*iter_).second;
        }

        ++iter_;
        if (iter_ == end_) {
            return false;
        }
        return true;
    }

private:
    mutable MapObj::iterator iter_;
    MapObj::iterator end_;
};

TVM_FFI_REGISTER_GLOBAL("ffi.MapForwardIterFunctor").set_body_typed([](const MapObj* n) -> Function {
            return Function::FromTyped(MapForwardIterFunctor(n->begin(), n->end()));
        });

}// namespace ffi
}// namespace litetvm
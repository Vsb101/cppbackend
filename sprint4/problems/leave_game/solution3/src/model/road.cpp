#include "road.h"

namespace model {

bool operator==(Road a, Road b) {
    return a.GetStart() == b.GetStart() && a.GetEnd() == b.GetEnd();
}

bool operator!=(Road a, Road b) {
    return !(a == b);
}

}  // namespace model

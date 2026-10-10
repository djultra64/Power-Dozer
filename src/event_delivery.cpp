#include "event_delivery.h"
#include "ultramodern/ultramodern.hpp"
namespace bc {
void initialize_event_delivery() {
    // Match librecomp's normal startup. Our direct preinit path otherwise
    // leaves the SDK's global retry bitset empty, losing critical completions
    // when a guest queue is temporarily full. VI/AI/PI keep SDK defaults.
    ultramodern::set_message_queue_control({});
}
}

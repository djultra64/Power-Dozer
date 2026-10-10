#pragma once
#include "input_model.h"
#include <optional>
namespace bc {
struct RemapRequest {InputAction action=InputAction::Confirm;InputSource source=InputSource::Keyboard;bool reset=false;};
struct RemapResult {bool success=false,cancelled=false;std::string status;};
class InputCapture {
 RemapRequest request_;
 bool active_=false,ready_=false,cancelled_=false;
 std::optional<InputBinding> binding_;
public:
 void begin(RemapRequest,const InputSnapshot&);
 void update(const InputSnapshot&);
 void press(InputBinding);
 void cancel(){active_=ready_=false;binding_.reset();}
 bool active() const{return active_;}
 bool ready() const{return ready_;}
 const RemapRequest& request() const{return request_;}
 std::optional<InputBinding> take_binding(){auto out=binding_;binding_.reset();return out;}
 bool take_cancelled(){auto out=cancelled_;cancelled_=false;return out;}
};
}

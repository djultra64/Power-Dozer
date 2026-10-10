#pragma once
#include "input_capture.h"
namespace bc {
class ControlsMenu {
public:
 enum class Page {Devices,Actions,Help,Capture,Reset,Saving,Closed};
 explicit ControlsMenu(bool help):page_(help?Page::Help:Page::Devices){}
 Page page() const{return page_;}
 bool closed() const{return page_==Page::Closed;}
 std::string status() const{return status_;}
 uint64_t revision() const{return revision_;}
 bool interactive(unsigned row) const;
 std::string title(const InputModel&) const;
 std::vector<std::string> rows(const InputModel&) const;
 void select(int);
 void back();
 void acknowledge(const RemapResult&);
 std::optional<RemapRequest> take_request(){auto out=request_;request_.reset();return out;}
 bool take_cancel_request(){auto out=cancel_;cancel_=false;return out;}
private:
 Page page_;
 InputSource source_=InputSource::Keyboard;
 unsigned part_=0;
 InputAction action_=InputAction::Confirm;
 bool cancel_=false;
 std::string status_;
 std::optional<RemapRequest> request_;
 uint64_t revision_=0;
};
}

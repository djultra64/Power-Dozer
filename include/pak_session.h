#pragma once
#include "pak_store.h"
namespace bc {
// Reject the explicit unfinished marker from older prototypes; other games
// and newly allocated/unwritten files keep the generic Pak store behavior.
bool blast_pak_committed(const PakImage&);
class PakSession {
 PakImage working_,committed_;
 std::array<bool,16> pending_{};
 void persist_file(const PakImage&,int,const PakStore&);
public:
 explicit PakSession(PakImage initial):working_(initial),committed_(std::move(initial)){}
 const PakImage& image() const{return working_;}
 bool pending(int index) const{return index>=0&&index<16&&pending_[index];}
 int allocate(const PakKey&,int bytes,int& index,const PakStore&);
 int remove(const PakKey&,const PakStore&);
 int write(int index,int offset,std::span<const uint8_t>,const PakStore&);
};
}

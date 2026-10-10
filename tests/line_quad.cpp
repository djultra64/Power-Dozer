#include "line_quad.h"
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){
 std::array<bc::LinePoint,4> q;
 check(bc::line_quad({10,20},{30,20},0,q));
 check(q[0].x==10&&q[0].y==20.75f&&q[2].x==30&&q[2].y==19.25f);
 check(bc::line_quad({10,20},{10,40},1,q));
 check(q[0].x==9&&q[1].x==11&&q[2].y==40);
 check(bc::line_xy({-1.25f,2.5f})==0xFFFB000A);
 check(!bc::line_quad({0,0},{0,0},0,q));
 check(!bc::line_quad({0,0},{10,0},-3,q));
 check(!bc::line_quad({0,0},{9000,0},0,q));
}

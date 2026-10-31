#include "../native/frame_pool.h"
#include <cassert>
#include <thread>
using namespace rchase2;
int main() {
    NativeFramePool pool;
    auto frame=std::make_unique<NativeFrame>();frame->sequence=5;
    frame->background.assign(496*384,0x123456);frame->blobs.push_back({"texture0",std::vector<uint8_t>(1048576,42)});
    auto pixels=frame->background.data();auto texture=frame->blobs[0].bytes.data();
    auto retained=pool.publish(std::move(frame));
    // A render-thread reader prevents reuse, even after the producer moves on.
    auto next=pool.acquire();assert(next.background.empty());
    assert(retained->background[0]==0x123456&&retained->blobs[0].bytes[0]==42);
    std::thread release([p=std::move(retained)]()mutable{p.reset();});release.join();
    auto reused=pool.acquire();
    assert(reused.sequence==5&&reused.background.data()==pixels&&reused.blobs[0].bytes.data()==texture);
    std::shared_ptr<const NativeFrame> late;
    {NativeFramePool temporary;auto f=std::make_unique<NativeFrame>();f->sequence=9;late=temporary.publish(std::move(f));}
    assert(late->sequence==9);late.reset(); // Deleter remains valid after pool facade destruction.
}

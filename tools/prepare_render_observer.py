#!/usr/bin/env python3
"""Instrument local copies of exactly pinned RT64 queue sources."""
import argparse
import hashlib
from pathlib import Path

EXPECTED={'present':'90df4e9c97136fa9662ee003393ad657fcb3e58cade6a24052012cf54a124c90','workload':'6e53030e8899c42347b51e67879fcb9679ad487ac067c192612c432d85bca4a4'}
def prepare(source,destination):
    destination=Path(destination);destination.mkdir(parents=True,exist_ok=True)
    for kind,digest in EXPECTED.items():
        original=Path(source)/'src/hle'/('rt64_'+kind+'_queue.cpp');raw=original.read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('RT64 observer source fingerprint changed: '+kind)
        text=raw.decode()
        if kind=='present':
            site='                swapChainValid = ext.swapChain->present(swapChainIndex, &waitSemaphore, 1);'
            replacement=site+'\n                bc_render_present(swapChainValid);'
        else:
            site='                    framesRendered++;'
            replacement='''                    if (bc_render_observing()) {
                        uint32_t mapped = 0, transforms = 0;
                        for (const auto &map : curFrame.frameMap.workloads) {
                            transforms += uint32_t(map.transforms.size());
                            for (const auto &transform : map.transforms) mapped += transform.mapped;
                        }
                        bc_render_generated(generateInterpolatedFrames, curFrameWeight, mapped, transforms);
                    }
'''+site
        if text.count(site)!=1:raise ValueError('RT64 observer hook site is not unique: '+kind)
        text=text.replace(site,replacement)
        if kind=='present':
            selection='''                RenderTargetKey colorTargetKey(presentFb->addressStart, presentFb->width, presentFb->siz, Framebuffer::Type::Color);'''
            if text.count(selection)!=1:raise ValueError('RT64 selection hook site is not unique')
            probe='''                if (bc_render_observing()) {
                    int count = 0, available = 0;
                    { std::lock_guard<std::mutex> lock(ext.sharedResources->interpolatedMutex); count = frameCounters.count; available = frameCounters.available; }
                    bool modified = false;
                    for (auto address : ext.sharedResources->colorImageAddressVector) modified |= address == presentFb->addressStart;
                    bc_render_selection(present.screenVI.fbAddress(), presentFb->addressStart, framesToPresent, count, available,
                        presentFb->interpolationEnabled, int(ext.sharedResources->colorImageAddressVector.size()), modified);
                }
'''
            text=text.replace(selection,probe+selection)
        payload='#include "render_observer.h"\n'+text
        output=destination/('blastcorps_'+kind+'_queue.cpp')
        if not output.exists() or output.read_text()!=payload:output.write_text(payload)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('source',type=Path);parser.add_argument('destination',type=Path);args=parser.parse_args();prepare(args.source,args.destination)

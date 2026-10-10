// RT64 core wiring adapted from the user's Tetrisphere prototype.
#include "prototype.h"
#include "l3d.h"
#include "settings.h"
#include "pc_options.h"
#include "render_observer.h"
#include "hle/rt64_application.h"
#include "hle/rt64_workload_queue.h"
#include "ultramodern/ultramodern.hpp"
#include <array>
#include <cstdio>
#include <algorithm>
#include <cmath>
namespace {
std::array<uint8_t,0x40> rom_header{};
std::array<uint8_t,0x1000> dmem{}, imem{};
bool task_swap=false;
uint32_t mi_intr=0,dpc_start=0,dpc_end=0,dpc_current=0,dpc_status=0,dpc_clock=0,dpc_bufbusy=0,dpc_pipebusy=0,dpc_tmem=0;
void bc_check_interrupts() {if(mi_intr&0x20){bc_dp_sync=task_swap;mi_intr&=~0x20u;}}
class Context final : public ultramodern::renderer::RendererContext {
std::unique_ptr<RT64::Application> app_;
public:
Context(uint8_t* rdram,ultramodern::renderer::WindowHandle window) {
        RT64::Application::Core core{};
#ifdef _WIN32
        core.window = window.window;
#else
        core.window = window;
#endif
        core.checkInterrupts = bc_check_interrupts;
        core.HEADER = rom_header.data();
        core.RDRAM = rdram;
        core.DMEM = dmem.data();
        core.IMEM = imem.data();
        core.MI_INTR_REG = &mi_intr;
        core.DPC_START_REG = &dpc_start;
        core.DPC_END_REG = &dpc_end;
        core.DPC_CURRENT_REG = &dpc_current;
        core.DPC_STATUS_REG = &dpc_status;
        core.DPC_CLOCK_REG = &dpc_clock;
        core.DPC_BUFBUSY_REG = &dpc_bufbusy;
        core.DPC_PIPEBUSY_REG = &dpc_pipebusy;
        core.DPC_TMEM_REG = &dpc_tmem;
        const auto* vi = ultramodern::renderer::get_vi_regs();
        core.VI_STATUS_REG = const_cast<std::uint32_t*>(&vi->VI_STATUS_REG);
        core.VI_ORIGIN_REG = const_cast<std::uint32_t*>(&vi->VI_ORIGIN_REG);
        core.VI_WIDTH_REG = const_cast<std::uint32_t*>(&vi->VI_WIDTH_REG);
        core.VI_INTR_REG = const_cast<std::uint32_t*>(&vi->VI_INTR_REG);
        core.VI_V_CURRENT_LINE_REG = const_cast<std::uint32_t*>(&vi->VI_V_CURRENT_LINE_REG);
        core.VI_TIMING_REG = const_cast<std::uint32_t*>(&vi->VI_TIMING_REG);
        core.VI_V_SYNC_REG = const_cast<std::uint32_t*>(&vi->VI_V_SYNC_REG);
        core.VI_H_SYNC_REG = const_cast<std::uint32_t*>(&vi->VI_H_SYNC_REG);
        core.VI_LEAP_REG = const_cast<std::uint32_t*>(&vi->VI_LEAP_REG);
        core.VI_H_START_REG = const_cast<std::uint32_t*>(&vi->VI_H_START_REG);
        core.VI_V_START_REG = const_cast<std::uint32_t*>(&vi->VI_V_START_REG);
        core.VI_V_BURST_REG = const_cast<std::uint32_t*>(&vi->VI_V_BURST_REG);
        core.VI_X_SCALE_REG = const_cast<std::uint32_t*>(&vi->VI_X_SCALE_REG);
        core.VI_Y_SCALE_REG = const_cast<std::uint32_t*>(&vi->VI_Y_SCALE_REG);

        RT64::ApplicationConfiguration config{};
        config.appId = "blastcorps-feasibility";
        config.useConfigurationFile = false;
        app_ = std::make_unique<RT64::Application>(core, config);
        app_->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::Vulkan;
        app_->userConfig.resolution = RT64::UserConfiguration::Resolution::Manual;
        app_->userConfig.resolutionMultiplier = bc_scale;
        app_->userConfig.aspectRatio = bc_wide ? RT64::UserConfiguration::AspectRatio::Manual : RT64::UserConfiguration::AspectRatio::Original;
        app_->userConfig.aspectTarget = 16.0/9.0;
        app_->userConfig.refreshRate = bc_refresh ? RT64::UserConfiguration::RefreshRate::Manual : RT64::UserConfiguration::RefreshRate::Original;
        app_->userConfig.refreshRateTarget = bc_refresh;
        using UC=RT64::UserConfiguration;
        app_->userConfig.antialiasing=bc::settings.msaa==8?UC::Antialiasing::MSAA8X:bc::settings.msaa==4?UC::Antialiasing::MSAA4X:bc::settings.msaa==2?UC::Antialiasing::MSAA2X:UC::Antialiasing::None;
        app_->userConfig.filtering=bc::settings.presentation=="nearest"?UC::Filtering::Nearest:bc::settings.presentation=="pixel"?UC::Filtering::AntiAliasedPixelScaling:UC::Filtering::Linear;
        app_->userConfig.threePointFiltering=bc::settings.texture=="three_point";
        app_->userConfig.developerMode = false;
        // Blast Corps' VI points at the preceding buffered image. Select the
        // newly drawn, VI-compatible image so RT64 presents its interpolation
        // batch rather than a single stale image per original frame.
        app_->enhancementConfig.presentation.mode = RT64::EnhancementConfiguration::Presentation::Mode::SkipBuffering;
        auto result=app_->setup(0);
        setup_result=static_cast<ultramodern::renderer::SetupResult>(result);
        chosen_api=ultramodern::renderer::GraphicsApi::Vulkan;
        if (result!=RT64::Application::SetupResult::Success) bc_fatal("rt64_setup",static_cast<uint32_t>(result));
        fprintf(stderr,"{\"event\":\"rt64_device_ready\",\"gpu\":\"%s\"}\n",app_->device->getDescription().name.c_str());
        fflush(stderr);
    }
    bool valid() override { return bool(app_); }
    bool update_config(const ultramodern::renderer::GraphicsConfig&,const ultramodern::renderer::GraphicsConfig&) override { return true; }
    void enable_instant_present() override {}
    void send_dl(const OSTask* task) override {
        if(auto filters=bc::take_pc_filter_change()){
            using UC=RT64::UserConfiguration;
            app_->userConfig.filtering=filters->presentation=="nearest"?UC::Filtering::Nearest:filters->presentation=="pixel"?UC::Filtering::AntiAliasedPixelScaling:UC::Filtering::Linear;
            app_->userConfig.threePointFiltering=filters->texture=="three_point";
            app_->updateUserConfig(false);
            fprintf(stderr,"{\"event\":\"pc_filters_applied\",\"presentation\":\"%s\",\"texture\":\"%s\"}\n",filters->presentation.c_str(),filters->texture.c_str());
        }
        task_swap=bc_take_task_swap(task->t.data_ptr);bc_dp_sync=false;
        auto count=++bc_gfx;
        if(count<=12){fprintf(stderr,"gfx begin seq=%llu ucode=%08X list=%08X\n",(unsigned long long)count,uint32_t(task->t.ucode),uint32_t(task->t.data_ptr));fflush(stderr);}
        app_->state->rsp->reset();
        app_->interpreter->loadUCodeGBI(task->t.ucode & 0x3FFFFFF,task->t.ucode_data & 0x3FFFFFF,true);
        bc_load_l3d(*app_->interpreter,task->t.ucode & 0x3FFFFFF,task->t.ucode_data & 0x3FFFFFF);
        bc_load_hud_annotations(*app_->interpreter,task->t.ucode & 0x3FFFFFF);
        if(!app_->interpreter->hleGBI || app_->interpreter->hleGBI->ucode==RT64::GBIUCode::Unknown)bc_fatal("unsupported_graphics_ucode",task->t.ucode);
        if(count<=12) fprintf(stderr,"{\"event\":\"gfx_task\",\"seq\":%llu,\"ucode\":%u,\"list\":%u}\n",(unsigned long long)count,(uint32_t)task->t.ucode,(uint32_t)task->t.data_ptr);
        app_->processDisplayLists(app_->core.RDRAM,task->t.data_ptr & 0x3FFFFFF,0,true);
    }
    void send_dummy_workload(uint32_t framebuffer) override {
        app_->state->listProcessBegin();
        app_->state->rdp->setColorImage(G_IM_FMT_RGBA,G_IM_SIZ_16b,320,framebuffer);
        app_->state->rdp->setOtherMode(0x382C30,0);
        app_->state->rdp->fillRect(0,0,320<<2,240<<2);
        app_->state->fullSync();
        app_->state->listProcessEnd();
    }
    void update_screen() override {
        app_->updateScreen();
        static uint64_t last=0;auto vi=bc_vis.load();if(vi<last+120)return;last=vi;
        auto& shared=*app_->sharedQueueResources;
        uint32_t width=0,height=0,target=0,original=0,presented=0,count=0;
        {std::lock_guard lock(shared.configurationMutex);target=shared.targetRate;original=shared.viOriginalRate;}
        {std::lock_guard lock(shared.managerMutex);for(auto& [key,fb]:shared.renderTargetManager.targetMap){if(fb->width*fb->height>width*height){width=fb->width;height=fb->height;}}}
        {std::lock_guard lock(shared.interpolatedMutex);for(auto& frames:shared.interpolatedFrames){presented+=frames.presented;count+=frames.count;}}
        fprintf(stderr,"{\"event\":\"rt64_metrics\",\"width\":%u,\"height\":%u,\"target_rate\":%u,\"original_rate\":%u,\"interpolated_count_snapshot\":%u,\"presented_snapshot\":%u}\n",width,height,target,original,count,presented);
        // RT64 writes these histories within threadRenderFrame's workloadMutex.
        // Copy under that same lock; these are rendering costs, not output FPS.
        std::vector<double> gpu,cpu;
        {std::lock_guard lock(shared.workloadMutex);gpu=app_->workloadQueue->rendererGPUProfiler.history;cpu=app_->workloadQueue->rendererCPUProfiler.history;}
        auto summarize=[](std::vector<double> samples){
            samples.erase(std::remove_if(samples.begin(),samples.end(),[](double value){return !std::isfinite(value)||value<=0;}),samples.end());
            std::sort(samples.begin(),samples.end());double sum=0;for(double value:samples)sum+=value;
            if(samples.empty())return std::array<double,4>{};
            return std::array<double,4>{double(samples.size()),sum/samples.size(),samples[size_t(std::ceil(samples.size()*.95))-1],samples.back()};
        };
        auto g=summarize(std::move(gpu)),c=summarize(std::move(cpu));
        fprintf(stderr,"{\"event\":\"rt64_render_cost\",\"vi\":%llu,\"gpu_samples\":%.0f,\"gpu_mean_ms\":%.4f,\"gpu_p95_ms\":%.4f,\"gpu_max_ms\":%.4f,\"cpu_samples\":%.0f,\"cpu_mean_ms\":%.4f,\"cpu_p95_ms\":%.4f,\"cpu_max_ms\":%.4f}\n",(unsigned long long)vi,g[0],g[1],g[2],g[3],c[0],c[1],c[2],c[3]);
        bc_render_report(vi);
    }
    void shutdown() override { app_->end(); }
    uint32_t get_display_framerate() const override { return app_->presentQueue->ext.sharedResources->swapChainRate; }
    float get_resolution_scale() const override { return bc_scale; }
};
}
std::unique_ptr<ultramodern::renderer::RendererContext> bc_create_context(uint8_t* rdram,ultramodern::renderer::WindowHandle window,bool) { return std::make_unique<Context>(rdram,window); }

#include "pc_settings_renderer.h"
#include "control_atlas_data.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace bc {
namespace {
constexpr unsigned atlas_width=512,atlas_height=1024,controls_y=224;
using Color=std::array<float,4>;
Color color(const std::array<uint8_t,4>& c){return {c[0]/255.f,c[1]/255.f,c[2]/255.f,c[3]/255.f};}
int font_index(unsigned char c){
 constexpr std::string_view chars="0123456789ABCD\x7f" "EFGHIJKLMNOPQRSTUVWXYZ";
 auto i=chars.find(char(std::toupper(c)));if(i!=chars.npos)return int(i);
 constexpr std::string_view punctuation="'):,$!.-(%?#/";
 i=punctuation.find(char(c));return i!=punctuation.npos?int(38+i):48;
}
struct DrawBuilder {
 const PcSettingsAssets& assets;
 PcSettingsDrawData out;
 float scale;
 void vertex(float x,float y,float u,float v,Color c){out.vertices.push_back({x,y,u,v,c[0],c[1],c[2],c[3]});}
 void quad(PcSettingsRect r,float u,float v,float uw,float vh,Color c){
  vertex(r.x,r.y,u,v,c);vertex(r.x+r.width,r.y,u+uw,v,c);vertex(r.x+r.width,r.y+r.height,u+uw,v+vh,c);
  vertex(r.x,r.y,u,v,c);vertex(r.x+r.width,r.y+r.height,u+uw,v+vh,c);vertex(r.x,r.y+r.height,u,v+vh,c);
 }
 void batch(uint32_t first,PcSettingsTexture t,PcSettingsPrimitive p,std::optional<GlyphSprite> glyph={}){
  if(out.vertices.size()>first)out.batches.push_back({t,p,first,uint32_t(out.vertices.size()-first),glyph});
 }
 void box(PcSettingsRect r){
  auto first=uint32_t(out.vertices.size());
  int minx=32767,miny=32767,maxx=-32768,maxy=-32768;
  for(auto v:assets.shadow_vertices){minx=std::min(minx,int(v.x));maxx=std::max(maxx,int(v.x));miny=std::min(miny,int(v.y));maxy=std::max(maxy,int(v.y));}
  for(auto t:assets.shadow_triangles)for(auto i:t){const auto v=assets.shadow_vertices.at(i);
   vertex(r.x+(v.x-minx)*r.width/std::max(1,maxx-minx),r.y+(v.y-miny)*r.height/std::max(1,maxy-miny),511.5f/atlas_width,1023.5f/atlas_height,{0,0,0,v.alpha/255.f});
  }
  batch(first,PcSettingsTexture::White,PcSettingsPrimitive::BoxShadow);
 }
 float text_width(std::string_view text,float size)const{
  float width=0;for(unsigned char c:text)width+=c==' '?.6f:assets.glyph_advances[font_index(c)];
  return (width+(text.empty()?0:.4f))*size;
 }
 // Each run has a half-alpha shadow offset by the original -3,+3 at 720p.
 void text(std::string_view value,PcSettingsRect r,float size,Color tint,int align=0){
  if(value.empty()||r.width<=0)return;
  size*=scale;size=std::min(size,r.width/std::max(.01f,text_width(value,1)));
  const float x=r.x+(align==1?(r.width-text_width(value,size))/2:align==2?r.width-text_width(value,size):0);
  for(bool shadow:{true,false}){
   auto first=uint32_t(out.vertices.size());float pen=x;
   for(unsigned char c:value){
    if(c!=' '){int g=font_index(c);quad({pen+(shadow?-2*scale:0),r.y+(shadow?2*scale:0),size,size},float(g%8*32)/atlas_width,float(g/8*32)/atlas_height,32.f/atlas_width,32.f/atlas_height,shadow?Color{0,0,0,tint[3]*.5f}:tint);}
    pen+=size*(c==' '?.6f:assets.glyph_advances[font_index(c)]);
   }
   batch(first,PcSettingsTexture::Font,shadow?PcSettingsPrimitive::TextShadow:PcSettingsPrimitive::Text);
  }
 }
 void glyph(GlyphSprite sprite,PcSettingsRect r){
  const auto first=uint32_t(out.vertices.size());
  quad(r,float(sprite.index%16*32)/atlas_width,float(controls_y+sprite.index/16*32)/atlas_height,32.f/atlas_width,32.f/atlas_height,{1,1,1,1});
  batch(first,PcSettingsTexture::Controls,PcSettingsPrimitive::Glyph,sprite);
 }
 void binding(const InputPrompt& prompt,ControllerFamily family,PcSettingsRect r,Color tint){
  if(auto sprite=input_glyph(prompt,family)){
   const float size=22*scale;glyph(*sprite,{r.x,r.y,size,size});r.x+=size+3*scale;r.width-=size+3*scale;
  }
  text(prompt.label,r,16,tint,2);
 }
};
std::atomic<std::shared_ptr<const PcSettingsSnapshot>> published_snapshot;
}
void publish_pc_settings_snapshot(std::shared_ptr<const PcSettingsSnapshot> snapshot){published_snapshot.store(std::move(snapshot),std::memory_order_release);}
PcSettingsDrawData pc_settings_draw_data(const PcSettingsSnapshot& snapshot,const PcSettingsAssets& assets,int width,int height){
 if(width<=0||height<=0)return {};
 const auto layout=pc_settings_layout(width,height);const float scale=layout.safe_area.width/640;
 DrawBuilder b{assets,{},scale};b.out.scissor={0,0,float(width),float(height)};
 auto rect=[&](float x,float y,float w,float h){return PcSettingsRect{layout.safe_area.x+x*scale,layout.safe_area.y+y*scale,w*scale,h*scale};};
 const auto white=color(assets.palette[7]),red=color(assets.palette[4]),grey=color(assets.palette[8]);
 b.box(rect(16,100,608,308));
 b.text("PC SETTINGS",layout.title,32,white,1);
 constexpr std::array sections={SettingsMenu::Page::Display,SettingsMenu::Page::Graphics,SettingsMenu::Page::Audio,SettingsMenu::Page::Controls};
 constexpr std::array tabs={"DISPLAY","GRAPHICS","AUDIO","CONTROLS"};
 for(size_t i=0;i<tabs.size();++i){
  const bool selected=snapshot.section==sections[i];
  const bool focused=snapshot.focus_target==PcSettingsFocusTarget::Tab&&snapshot.focus==i;
  b.text(tabs[i],layout.tabs[i],20,selected||focused?red:white,1);
  if(focused)b.text("-",{layout.tabs[i].x,layout.tabs[i].y,12*scale,layout.tabs[i].height},18,red);
 }
 // Keep the view state intact, but expose only the game backdrop through the
 // original translucent modal shadow instead of overlapping option text.
 const bool show_options=snapshot.modal==PcSettingsModal::None;
 for(size_t i=0;show_options&&i<snapshot.rows.size();++i){
  const auto& row=snapshot.rows[i];const bool controls=snapshot.section==SettingsMenu::Page::Controls;
  if(i>=(controls?layout.actions.size():layout.rows.size()))break;
  const auto r=controls?layout.actions[i]:layout.rows[i];
  const bool focused=snapshot.focus_target==PcSettingsFocusTarget::Row&&snapshot.focus==i;
  const auto tint=focused?red:white;
  if(focused)b.text("-",{r.x-12*scale,r.y,12*scale,r.height},18,red);
  auto label=r;label.width=(controls?158:340)*scale;
  b.text(row.label,label,controls?18:22,tint);
  auto value=r;value.x+=(controls?166:350)*scale;value.width-=(controls?166:350)*scale;
  if(row.binding)b.binding(*row.binding,snapshot.controller_family,value,tint);
  else b.text(row.value,value,22,tint,2);
  if(row.restart_required)b.text("RESTART REQUIRED",{r.x,r.y+25*scale,r.width,14*scale},12,grey);
 }
 for(size_t i=0;show_options&&i<std::min(snapshot.controls.size(),layout.controls.size());++i){
  const auto& c=snapshot.controls[i];auto r=layout.controls[i];
  const auto tint=snapshot.focus_target==PcSettingsFocusTarget::Control&&snapshot.focus==i?red:white;
  const auto label=i==0?"DEVICE: "+c.value:i==1?"FAMILY: "+c.value:c.label;
  if(snapshot.focus_target==PcSettingsFocusTarget::Control&&snapshot.focus==i)b.text("-",{r.x-12*scale,r.y,12*scale,r.height},18,red);
  b.text(label,r,18,tint,i%2?2:0);
 }
 auto status=snapshot.status;
 if(status.empty())status=snapshot.section==SettingsMenu::Page::Controls?"SELECT AN ACTION TO REMAP":snapshot.section==SettingsMenu::Page::Audio?"CHANGES APPLY IMMEDIATELY":"RESTART MARKED CHANGES APPLY NEXT LAUNCH";
 b.text(status,layout.status,16,snapshot.status=="SAVE FAILED"?red:grey,1);
 // Both names and glyphs come from the immutable active-device prompts.
 for(size_t i=0;i<2;++i){
  auto r=rect(i==0?360:32,444,248,24);b.binding(snapshot.prompts[i],snapshot.controller_family,{r.x,r.y,135*scale,r.height},white);
  r.x+=143*scale;r.width-=143*scale;b.text(i==0?"SELECT":"BACK",r,16,white);
 }
 if(snapshot.modal!=PcSettingsModal::None){
  b.box(layout.modal);
  b.text(snapshot.modal==PcSettingsModal::Capture?"PRESS AN INPUT":"RESTORE DEFAULTS?",rect(96,172,448,32),24,red,1);
  if(snapshot.modal==PcSettingsModal::Capture){
   b.text("ESC: CANCEL",rect(96,254,448,24),18,white,1);
   if(snapshot.focus<snapshot.rows.size())b.text(snapshot.rows[snapshot.focus].label,rect(96,216,448,24),20,white,1);
  }else{
   for(size_t i=0;i<2;++i){
    auto r=rect(104+i*224,230,208,26);
    b.binding(snapshot.prompts[i],snapshot.controller_family,{r.x,r.y,110*scale,r.height},white);
    r.x+=118*scale;r.width-=118*scale;b.text(i==0?"CONFIRM":"CANCEL",r,16,white);
   }
   // Fixed keyboard recovery remains available even after remapping Back.
   b.text("ESC: CANCEL",rect(96,292,448,24),16,grey,1);
  }
 }
 return std::move(b.out);
}
}

#ifndef BC_PC_SETTINGS_DRAW_DATA_ONLY
#include "rhi/rt64_render_hooks.h"
#include "pc_settings_vs_spirv.h"
#include "pc_settings_ps_spirv.h"
#ifdef _WIN32
#include "pc_settings_vs_dxil.h"
#include "pc_settings_ps_dxil.h"
#endif
namespace bc {
namespace {
// All GPU objects belong to RT64's init/draw/deinit lifecycle. No SDL or guest
// state is consulted here. Application::end joins the presentation queue first.
struct Renderer {
 PcSettingsAssets assets;
 RenderDevice* device=nullptr;
 std::unique_ptr<RenderTexture> atlas;
 std::unique_ptr<RenderSampler> sampler;
 std::unique_ptr<RenderDescriptorSet> descriptors;
 std::unique_ptr<RenderPipelineLayout> layout;
 std::unique_ptr<RenderPipeline> pipeline;
 std::unique_ptr<RenderBuffer> vertices;
 size_t vertex_capacity=0;
};
std::unique_ptr<Renderer> renderer;
void init(RenderInterface* rhi,RenderDevice* device){
 auto& r=*renderer;r.device=device;
 std::vector<uint8_t> pixels(atlas_width*atlas_height*4);
 for(unsigned y=0;y<PcSettingsAssets::font_height;++y)
  std::memcpy(pixels.data()+y*atlas_width*4,r.assets.font_rgba.data()+y*PcSettingsAssets::font_width*4,PcSettingsAssets::font_width*4);
 for(size_t i=0;i<control_atlas.size();++i)for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x){
  auto rgba=control_atlas[i].pixels[y*32+x];size_t at=((controls_y+i/16*32+y)*atlas_width+i%16*32+x)*4;
  pixels[at]=uint8_t(rgba>>24);pixels[at+1]=uint8_t(rgba>>16);pixels[at+2]=uint8_t(rgba>>8);pixels[at+3]=uint8_t(rgba);
 }
 std::fill(pixels.end()-4,pixels.end(),255);
 r.atlas=device->createTexture(RenderTextureDesc::Texture2D(atlas_width,atlas_height,1,RenderFormat::R8G8B8A8_UNORM));
 auto upload=device->createBuffer(RenderBufferDesc::UploadBuffer(pixels.size()));
 std::memcpy(upload->map(),pixels.data(),pixels.size());upload->unmap();
 auto queue=device->createCommandQueue(RenderCommandListType::DIRECT);
 auto list=queue->createCommandList();auto fence=device->createCommandFence();
 list->begin();list->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(r.atlas.get(),RenderTextureLayout::COPY_DEST));
 list->copyTextureRegion(RenderTextureCopyLocation::Subresource(r.atlas.get()),RenderTextureCopyLocation::PlacedFootprint(upload.get(),RenderFormat::R8G8B8A8_UNORM,atlas_width,atlas_height,1,atlas_width));
 list->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(r.atlas.get(),RenderTextureLayout::SHADER_READ));list->end();
 queue->executeCommandLists(list.get(),fence.get());queue->waitForCommandFence(fence.get());
 RenderSamplerDesc sampler;sampler.addressU=sampler.addressV=sampler.addressW=RenderTextureAddressMode::CLAMP;
 r.sampler=device->createSampler(sampler);
 RenderDescriptorRange ranges[]={ {RenderDescriptorRangeType::TEXTURE,0,1},{RenderDescriptorRangeType::SAMPLER,1,1} };
 RenderDescriptorSetDesc descriptors(ranges,2);
 r.descriptors=device->createDescriptorSet(descriptors);
 r.descriptors->setTexture(0,r.atlas.get(),RenderTextureLayout::SHADER_READ);r.descriptors->setSampler(1,r.sampler.get());
 RenderPushConstantRange push(0,0,0,2*sizeof(float),RenderShaderStageFlag::VERTEX);
 r.layout=device->createPipelineLayout(RenderPipelineLayoutDesc(&push,1,&descriptors,1,false,true));
 const auto format=rhi->getCapabilities().shaderFormat;
 std::unique_ptr<RenderShader> vs,ps;
 if(format==RenderShaderFormat::SPIRV){
  // DXC's C header is byte-aligned; Vulkan requires uint32_t-aligned code.
  auto shader=[&](const auto& bytes,const char* entry){
   std::vector<uint32_t> words(sizeof(bytes)/sizeof(uint32_t));
   std::memcpy(words.data(),bytes,sizeof(bytes));
   return device->createShader(words.data(),sizeof(bytes),entry,format);
  };
  vs=shader(pc_settings_vs_spirv,"VSMain");ps=shader(pc_settings_ps_spirv,"PSMain");
 }
#ifdef _WIN32
 else if(format==RenderShaderFormat::DXIL){
  vs=device->createShader(pc_settings_vs_dxil,sizeof(pc_settings_vs_dxil),"VSMain",format);
  ps=device->createShader(pc_settings_ps_dxil,sizeof(pc_settings_ps_dxil),"PSMain",format);
 }
#endif
 else throw std::runtime_error("Unsupported PC settings shader format");
 const RenderInputSlot slot(0,sizeof(PcSettingsVertex));
 const RenderInputElement elements[]={
  {"POSITION",0,0,RenderFormat::R32G32_FLOAT,0,offsetof(PcSettingsVertex,x)},
  {"TEXCOORD",0,1,RenderFormat::R32G32_FLOAT,0,offsetof(PcSettingsVertex,u)},
  {"COLOR",0,2,RenderFormat::R32G32B32A32_FLOAT,0,offsetof(PcSettingsVertex,r)}
 };
 RenderGraphicsPipelineDesc pipeline;pipeline.pipelineLayout=r.layout.get();pipeline.vertexShader=vs.get();pipeline.pixelShader=ps.get();
 // Matches RT64 Application::setup's swap-chain format, also used by VI.
 pipeline.renderTargetFormat[0]=RenderFormat::B8G8R8A8_UNORM;pipeline.renderTargetBlend[0]=RenderBlendDesc::AlphaBlend();pipeline.renderTargetCount=1;
 pipeline.inputSlots=&slot;pipeline.inputSlotsCount=1;pipeline.inputElements=elements;pipeline.inputElementsCount=3;
 r.pipeline=device->createGraphicsPipeline(pipeline);
}
void draw(RenderCommandList* list,RenderFramebuffer* framebuffer){
 const auto snapshot=published_snapshot.load(std::memory_order_acquire);
 if(!snapshot||!renderer||!renderer->device)return;
 auto& r=*renderer;
 const auto width=framebuffer->getWidth(),height=framebuffer->getHeight();
 auto data=pc_settings_draw_data(*snapshot,r.assets,int(width),int(height));
 if(data.vertices.empty())return;
 const auto bytes=data.vertices.size()*sizeof(PcSettingsVertex);
 // RT64 waits its presentGraphicsWorker fence immediately after every draw
 // submission, before another hook or swap-chain resize. This upload buffer is
 // therefore no longer in use at the next hook; no framebuffer pointer survives.
 if(bytes>r.vertex_capacity){r.vertex_capacity=bytes*2;r.vertices=r.device->createBuffer(RenderBufferDesc::VertexBuffer(r.vertex_capacity,RenderHeapType::UPLOAD));}
 std::memcpy(r.vertices->map(),data.vertices.data(),bytes);r.vertices->unmap();
 const RenderInputSlot slot(0,sizeof(PcSettingsVertex));const RenderVertexBufferView view(r.vertices->at(0),uint32_t(bytes));
 list->setFramebuffer(framebuffer);list->setViewports(RenderViewport(0,0,float(width),float(height)));
 list->setScissors(RenderRect(0,0,int32_t(width),int32_t(height)));
 list->setGraphicsPipelineLayout(r.layout.get());list->setPipeline(r.pipeline.get());
 const float extent[]={float(width),float(height)};list->setGraphicsPushConstants(0,extent);
 list->setGraphicsDescriptorSet(r.descriptors.get(),0);list->setVertexBuffers(0,&view,1,&slot);
 list->drawInstanced(uint32_t(data.vertices.size()),1,0,0);
}
void deinit(){renderer.reset();}
}
void initialize_pc_settings_renderer(const PcSettingsAssets& assets){
 if(renderer)throw std::logic_error("PC settings renderer already initialized");
 if(assets.font_rgba.size()!=PcSettingsAssets::font_width*PcSettingsAssets::font_height*4)throw std::invalid_argument("PC settings font atlas is incomplete");
 renderer=std::make_unique<Renderer>();renderer->assets=assets;
 RT64::SetRenderHooks(init,draw,deinit);
}
}
#endif

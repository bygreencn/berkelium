
#include "berkelium/Platform.hpp"
#include "MemoryRenderViewHost.hpp"

#include <stdio.h>

#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/render_messages.h"

namespace Berkelium {

MemoryRenderViewHost::MemoryRenderViewHost(
        SiteInstance* instance,
        RenderViewHostDelegate* delegate,
        int routing_id,
        base::WaitableEvent* modal_dialog_event)
    : RenderViewHost(instance, delegate, routing_id, modal_dialog_event) {
}

MemoryRenderViewHost::~MemoryRenderViewHost() {
}

void MemoryRenderViewHost::OnMessageReceived(const IPC::Message& msg) {
  bool msg_is_ok = true;
  IPC_BEGIN_MESSAGE_MAP_EX(MemoryRenderViewHost, msg, msg_is_ok)
    IPC_MESSAGE_HANDLER(ViewHostMsg_PaintRect, Memory_OnMsgPaintRect)
    IPC_MESSAGE_UNHANDLED(RenderViewHost::OnMessageReceived(msg))
  IPC_END_MESSAGE_MAP_EX()

      ;
  if (msg.type() == ViewHostMsg_PaintRect::ID) {
      RenderViewHost::OnMessageReceived(msg);
  }
          
  if (!msg_is_ok) {
    // The message had a handler, but its de-serialization failed.
    // Kill the renderer.
    process()->ReceivedBadMessage(msg.type());
  }
}

void MemoryRenderViewHost::Memory_OnMsgPaintRect(
    const ViewHostMsg_PaintRect_Params&params)
{
/*
    // Update our knowledge of the RenderWidget's size.
    current_size_ = params.view_size;

    bool is_resize_ack =
        ViewHostMsg_PaintRect_Flags::is_resize_ack(params.flags);

    // resize_ack_pending_ needs to be cleared before we call DidPaintRect, since
    // that will end up reaching GetBackingStore.
    if (is_resize_ack) {
        DCHECK(resize_ack_pending_);
        resize_ack_pending_ = false;
        in_flight_size_.SetSize(0, 0);
    }

    bool is_repaint_ack =
        ViewHostMsg_PaintRect_Flags::is_repaint_ack(params.flags);
    if (is_repaint_ack) {
        repaint_ack_pending_ = false;
        TimeDelta delta = TimeTicks::Now() - repaint_start_time_;
        UMA_HISTOGRAM_TIMES("MPArch.RWH_RepaintDelta", delta);
    }
*/
    DCHECK(!params.bitmap_rect.IsEmpty());
    DCHECK(!params.view_size.IsEmpty());

    const size_t size = params.bitmap_rect.height() *
        params.bitmap_rect.width() * 4;
    TransportDIB* dib = process()->GetTransportDIB(params.bitmap);
    if (dib) {
        if (dib->size() < size) {
            DLOG(WARNING) << "Transport DIB too small for given rectangle";
            process()->ReceivedBadMessage(ViewHostMsg_PaintRect__ID);
        } else {
            // Paint the backing store. This will update it with the renderer-supplied
            // bits. The view will read out of the backing store later to actually
            // draw to the screen.
            Memory_PaintBackingStoreRect(dib, params.bitmap_rect, params.view_size);
        }
    }

/*
    Send(new ViewMsg_PaintRect_ACK(routing_id_));
*/
}

void MemoryRenderViewHost::Memory_PaintBackingStoreRect(
    TransportDIB* bitmap,
    const gfx::Rect& bitmap_rect,
    const gfx::Size& view_size)
{
    static int call_count = 0;
    FILE *outfile;
    {
        std::ostringstream os;
        os << "/tmp/chromium_render_" << time(NULL) << "_" << (call_count++) << ".ppm";
        std::string str (os.str());
        outfile = fopen(str.c_str(), "wb");
    }
    const int width = bitmap_rect.width();
    const int height = bitmap_rect.height();
    const uint32_t* bitmap_in =
        static_cast<const uint32_t*>(bitmap->memory());

    fprintf(outfile, "P6 %d %d 255\n", width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint32_t pixel = *(bitmap_in++);
            fputc((pixel >> 16) & 0xff, outfile);  // Red
            fputc((pixel >> 8) & 0xff, outfile);  // Green
            fputc(pixel & 0xff, outfile);  // Blue
            //(pixel >> 24) & 0xff;  // Alpha
        }
    }
    fclose(outfile);
}


MemoryRenderViewHostFactory::MemoryRenderViewHostFactory() {
    RenderViewHostFactory::RegisterFactory(this);
}

MemoryRenderViewHostFactory::~MemoryRenderViewHostFactory() {
    RenderViewHostFactory::UnregisterFactory();
}

RenderViewHost* MemoryRenderViewHostFactory::CreateRenderViewHost(
    SiteInstance* instance,
    RenderViewHostDelegate* delegate,
    int routing_id,
    base::WaitableEvent* modal_dialog_event)
{
    return new MemoryRenderViewHost(instance, delegate,
                                    routing_id, modal_dialog_event);
}

}

#ifndef __WebWidgetAPIInstance__
#define __WebWidgetAPIInstance__

#if defined(TIZEN_DEVICE_API) && defined(STARFISH_TIZEN_WEARABLE_WIDGET)
#include <EscargotPublic.h>
#include <bundle.h>
#include <bundle_internal.h>

#ifndef __MODULE__
#define __MODULE__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define __LOG__(prio, fmt, args...)                                           \
    dlog_print(prio, "WebWidgetAPIInstance", "%s: %s(%d) > " fmt, __MODULE__, \
               __func__, __LINE__, ##args);

#define WIDGET_APP_API_LOG_INFO(fmt, args...) __LOG__(DLOG_INFO, fmt, ##args)
#define WIDGET_APP_API_LOG_ERROR(fmt, args...) __LOG__(DLOG_ERROR, fmt, ##args)
#define WIDGET_APP_API_LOG_WARN(fmt, args...) __LOG__(DLOG_WARN, fmt, ##args)

namespace DeviceAPI {
class WebWidgetAPIInstance : public gc {
public:
    WebWidgetAPIInstance();
    ~WebWidgetAPIInstance();
    Escargot::ObjectRef* createWebWidgetAPIObject(Escargot::ContextRef*);
    Escargot::FunctionObjectRef* receiveContentListener()
    {
        return m_receiveContentListener;
    }
    void setReceiveContentListener(Escargot::FunctionObjectRef* fobj)
    {
        m_receiveContentListener = fobj;
    }
    void invokeReceiveContentListener(Escargot::ContextRef* ctx,
                                      const void* data);

private:
    Escargot::FunctionObjectRef* m_receiveContentListener;
};
}

#endif // TIZEN_DEVICE_API
#endif // __WebWidgetAPIInstance__

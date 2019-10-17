#if defined(TIZEN_DEVICE_API) && defined(STARFISH_TIZEN_WEARABLE_WIDGET)

#include "StarfishConfig.h"
#include "TizenDeviceAPILoaderForEscargot.h"
#include "WebWidgetAPIInstance.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "platform/window/PlatformWindow.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "Starfish.h"
#include "LWEWebView.h"

using namespace Escargot;

typedef int (*sfwebWidgetAPISetContentInfoOfContext_cb)(const void* ctx,
                                                        const void* data);
typedef int (*sfwebWidgetAPIGetContentInfoOfContext_cb)(const void* ctx,
                                                        void** out);

namespace DeviceAPI {

#define THROW_WEB_API_EXCEPTION(state, type, message)                          \
    {                                                                          \
        ValueRef* exceptionValue = state->context()->globalObject()->get(      \
            state, ValueRef::create(StringRef::fromASCII("WebAPIException"))); \
        if (!exceptionValue->isObject()) {                                     \
            ValueRef* xwalk = state->context()->globalObject()->get(           \
                state, ValueRef::create(StringRef::fromASCII("xwalk")));       \
            xwalk->toObject(state)->get(                                       \
                state, ValueRef::create(StringRef::fromASCII("utils")));       \
            exceptionValue = state->context()->globalObject()->get(            \
                state,                                                         \
                ValueRef::create(StringRef::fromASCII("WebAPIException")));    \
        }                                                                      \
        if (!exceptionValue->isCallable()) {                                   \
            state->throwException(ValueRef::create(                            \
                StringRef::fromASCII("invalid WebAPIException")));             \
        }                                                                      \
        ValueRef* typeValue = exceptionValue->toObject(state)->get(            \
            state, ValueRef::create(StringRef::fromASCII(type)));              \
        ValueRef* arguments[] = {                                              \
            typeValue, ValueRef::create(StringRef::fromASCII(message))         \
        };                                                                     \
        state->throwException(ValueRef::create(                                \
            exceptionValue->asObject()->construct(state, 2, arguments)));      \
    }

struct IterateToConvertBundleToESObjectData {
    ObjectRef* obj;
    ExecutionStateRef* state;
};
void iterateToConvertBundleToESObject(const char* key, const int type,
                                      const bundle_keyval_t* kv,
                                      void* user_data)
{
    void* ptr = NULL;
    char* buff = NULL;
    size_t size = 0;
    ObjectRef* obj = ((IterateToConvertBundleToESObjectData*)user_data)->obj;
    ExecutionStateRef* state =
        ((IterateToConvertBundleToESObjectData*)user_data)->state;

    switch (type) {
    case BUNDLE_TYPE_STR: {
        bundle_keyval_get_basic_val((bundle_keyval_t*)kv, &ptr, &size);
        std::unique_ptr<char> buff(new char[size + 1]);
        snprintf(buff.get(), size + 1, "%s", ((char*)ptr));
        WIDGET_APP_API_LOG_INFO("Found STR (key: %s, val: %s, size: %zu)", key,
                                buff.get(), size);
        obj->set(state, ValueRef::create(StringRef::fromUTF8(key, strlen(key))),
                 ValueRef::create(StringRef::fromUTF8(buff.get(), size)));
        break;
    }
    default: {
        WIDGET_APP_API_LOG_INFO("Illegal type");
        break;
    }
    }
}

WebWidgetAPIInstance::WebWidgetAPIInstance()
    : m_receiveContentListener(nullptr)
{
}

WebWidgetAPIInstance::~WebWidgetAPIInstance()
{
    m_receiveContentListener = nullptr;
}

void WebWidgetAPIInstance::invokeReceiveContentListener(
    Escargot::ContextRef* ctx, const void* data)
{
    WIDGET_APP_API_LOG_INFO("enter");

    if (!m_receiveContentListener ||
        !m_receiveContentListener->isFunctionObject()) {
        WIDGET_APP_API_LOG_INFO(
            "There are no registered ReceiveContentListener");
        return;
    }

    bundle* b = (bundle*)data;
    SandBoxRef* sb = SandBoxRef::create(ctx);
    auto sbresult = sb->run([&](ExecutionStateRef* state) -> ValueRef* {
        ObjectRef* contentData = ObjectRef::create(state);
        IterateToConvertBundleToESObjectData d;
        d.obj = contentData;
        d.state = state;
        bundle_foreach(b, iterateToConvertBundleToESObject, &d);

        ValueRef* arg = ValueRef::create(contentData);
        m_receiveContentListener->call(state, ValueRef::createUndefined(), 1,
                                       &arg);

        return ValueRef::createUndefined();
    });

    sb->destroy();

    if (sbresult.error.hasValue()) {
        STARFISH_LOG_ERROR("Uncaught %s\n",
                           sbresult.msgStr->toStdUTF8String().data());
        for (size_t i = 0; i < sbresult.stackTraceData.size(); i++) {
            STARFISH_LOG_ERROR(
                "at %s(%d:%d)\n",
                sbresult.stackTraceData[i].fileName->toStdUTF8String().data(),
                (int)sbresult.stackTraceData[i].loc.line,
                (int)sbresult.stackTraceData[i].loc.column);
        }
    }
}

ObjectRef* WebWidgetAPIInstance::createWebWidgetAPIObject(
    Escargot::ContextRef* context)
{
    WIDGET_APP_API_LOG_INFO("enter");
    ExecutionStateRef* state = ExecutionStateRef::create(context);

    StringRef* fnString = StringRef::fromASCII("WebWidgetContentManager");
    FunctionObjectRef::NativeFunctionInfo ctorInfo(
        AtomicStringRef::create(context, "WebWidgetContentManager"),
        ::Starfish::errorOnConstructorFunction, 0, nullptr, true, true);
    FunctionObjectRef* webWidgetAPIFunctionObj =
        FunctionObjectRef::createBuiltinFunction(state, ctorInfo);

    ObjectRef* prototypeObj =
        webWidgetAPIFunctionObj->getFunctionPrototype(state)->asObject();
    prototypeObj->removeFromHiddenClassChain(state);

    ObjectRef* targetObject = prototypeObj;

    StringRef* setContentInfoOfContextString =
        StringRef::fromASCII("setContentInfoOfContext");
    FunctionObjectRef* setContentInfoOfContextStringFn =
        FunctionObjectRef::create(
            state,
            FunctionObjectRef::NativeFunctionInfo(
                AtomicStringRef::create(context, "setContentInfoOfContext"),
                [](ExecutionStateRef* state, ValueRef* thisValue, size_t argc,
                   ValueRef** argv, bool isNewExpression) -> ValueRef* {
                    WIDGET_APP_API_LOG_INFO("Call setContentInfoOfContext\n");
                    if (argc != 1) {
                        return ValueRef::create(false);
                    }
                    ::Starfish::Window* window =
                        (::Starfish::Window*)state->context()
                            ->globalObject()
                            ->extraData();
                    const void* widgetContext =
                        window->webView()
                            ->publicLayerUserDataMap()["TizenWebWidgetContext"];
                    ValueRef* arg = argv[0];

                    sfwebWidgetAPISetContentInfoOfContext_cb
                        webWidgetAPISetContentInfoOfContext_cb;
                    webWidgetAPISetContentInfoOfContext_cb =
                        (sfwebWidgetAPISetContentInfoOfContext_cb)window
                            ->webView()
                            ->publicLayerUserDataMap()
                                ["TizenWebWidgetSetContentInfoOfContext"];

                    if (!arg->isObject()) {
                        THROW_WEB_API_EXCEPTION(
                            state, "TYPE_MISMATCH_ERR",
                            "First argument should be object");
                    }

                    if (widgetContext &&
                        webWidgetAPISetContentInfoOfContext_cb != nullptr) {
                        bundle* b = bundle_create();
                        ObjectRef* obj = arg->asObject();
                        obj->enumerateObjectOwnProperies(
                            state, [&](ExecutionStateRef* state,
                                       ValueRef* propertyName, bool isWritable,
                                       bool isEnumerable, bool isConfigurable) {
                                if (isEnumerable) {
                                    ValueRef* data =
                                        obj->get(state, propertyName);
                                    bundle_add(b, propertyName->toString(state)
                                                      ->toStdUTF8String()
                                                      .data(),
                                               data->toString(state)
                                                   ->toStdUTF8String()
                                                   .data());
                                    WIDGET_APP_API_LOG_INFO(
                                        "key : %s, data : %s\n",
                                        propertyName->toString(state)
                                            ->toStdUTF8String()
                                            .data(),
                                        data->toString(state)
                                            ->toStdUTF8String()
                                            .data());
                                }
                                return true;
                            });

                        int ret = webWidgetAPISetContentInfoOfContext_cb(
                            widgetContext, b);
                        bundle_free(b);

                        if (ret != 0) {
                            THROW_WEB_API_EXCEPTION(
                                state, "ABORT_ERR",
                                "The operation cannot be finished properly");
                        }
                        return ValueRef::create(true);
                    } else {
                        THROW_WEB_API_EXCEPTION(
                            state, "ABORT_ERR",
                            "The operation cannot be finished properly");
                    }
                    return ValueRef::create(false);
                },
                1, nullptr, true, false));
    targetObject->defineDataProperty(
        state, ValueRef::create(setContentInfoOfContextString),
        ValueRef::create(setContentInfoOfContextStringFn), false /* writable */,
        false /* enumerable */, false /* configurable */
        );

    StringRef* getContentInfoOfContextString =
        StringRef::fromASCII("getContentInfoOfContext");
    FunctionObjectRef* getContentInfoOfContextStringFn =
        FunctionObjectRef::create(
            state,
            FunctionObjectRef::NativeFunctionInfo(
                AtomicStringRef::create(context, "getContentInfoOfContext"),
                [](ExecutionStateRef* state, ValueRef* thisValue, size_t argc,
                   ValueRef** argv, bool isNewExpression) -> ValueRef* {
                    WIDGET_APP_API_LOG_INFO("Call getContentInfoOfContext\n");

                    ::Starfish::Window* window =
                        (::Starfish::Window*)state->context()
                            ->globalObject()
                            ->extraData();
                    const void* widgetContext =
                        window->webView()
                            ->publicLayerUserDataMap()["TizenWebWidgetContext"];

                    sfwebWidgetAPIGetContentInfoOfContext_cb
                        webWidgetAPIGetContentInfoOfContext_cb;
                    webWidgetAPIGetContentInfoOfContext_cb =
                        (sfwebWidgetAPIGetContentInfoOfContext_cb)window
                            ->webView()
                            ->publicLayerUserDataMap()
                                ["TizenWebWidgetGetContentInfoOfContext"];

                    if (widgetContext &&
                        webWidgetAPIGetContentInfoOfContext_cb != nullptr) {
                        bundle* b;
                        int ret = webWidgetAPIGetContentInfoOfContext_cb(
                            widgetContext, (void**)&b);
                        if (ret != 0) {
                            THROW_WEB_API_EXCEPTION(
                                state, "ABORT_ERR",
                                "The operation cannot be finished properly");
                            return ValueRef::createNull();
                        }
                        ObjectRef* contentData = ObjectRef::create(state);
                        IterateToConvertBundleToESObjectData d;
                        d.obj = contentData;
                        d.state = state;
                        bundle_foreach(b, iterateToConvertBundleToESObject, &d);
                        bundle_free(b);

                        return ValueRef::create(contentData);
                    } else {
                        THROW_WEB_API_EXCEPTION(
                            state, "ABORT_ERR",
                            "The operation cannot be finished properly");
                    }

                    return ValueRef::createNull();
                },
                1, nullptr, true, false));
    targetObject->defineDataProperty(
        state, ValueRef::create(getContentInfoOfContextString),
        ValueRef::create(getContentInfoOfContextStringFn), false /* writable */,
        false /* enumerable */, false /* configurable */
        );

    StringRef* setReceiveContentListenerString =
        StringRef::fromASCII("setReceiveContentListener");
    FunctionObjectRef* setReceiveContentListenerFn = FunctionObjectRef::create(
        state,
        FunctionObjectRef::NativeFunctionInfo(
            AtomicStringRef::create(context, "setReceiveContentListener"),
            [](ExecutionStateRef* state, ValueRef* thisValue, size_t argc,
               ValueRef** argv, bool isNewExpression) -> ValueRef* {
                WIDGET_APP_API_LOG_INFO("Call setReceiveContentListener");
                if (argc != 1 || !argv[0]->isCallable()) {
                    WIDGET_APP_API_LOG_ERROR(
                        "Trying to set receiveContent listener with invalid "
                        "value.");
                    THROW_WEB_API_EXCEPTION(state, "TYPE_MISMATCH_ERR",
                                            "Trying to set receive content "
                                            "listener with invalid value.");
                    return ValueRef::createUndefined();
                }
                ValueRef* listenerValue = argv[0];
                ::Starfish::Window* window =
                    (::Starfish::Window*)state->context()
                        ->globalObject()
                        ->extraData();
                WebWidgetAPIInstance* ww = window->webView()
                                               ->mainBrowsingContext()
                                               ->scriptBindingInstance()
                                               ->deviceAPI()
                                               ->webWidgetAPIInstance();

                if (!listenerValue->isCallable()) {
                    THROW_WEB_API_EXCEPTION(state, "TYPE_MISMATCH_ERR",
                                            "Trying to set receive content "
                                            "listener with invalid value.");
                    WIDGET_APP_API_LOG_ERROR(
                        "Trying to set receiveContent listener with invalid "
                        "value.");
                    return ValueRef::createUndefined();
                }

                WIDGET_APP_API_LOG_INFO("set receiveContent listener");
                ww->setReceiveContentListener(listenerValue->asFunction());
                return ValueRef::createUndefined();
            },
            1, nullptr, true, false));
    targetObject->defineDataProperty(
        state, ValueRef::create(setReceiveContentListenerString),
        ValueRef::create(setReceiveContentListenerFn), false /* writable */,
        false /* enumerable */, false /* configurable */
        );

    StringRef* unsetReceiveContentListenerString =
        StringRef::fromASCII("unsetReceiveContentListener");
    FunctionObjectRef* unsetReceiveContentListenerFn =
        FunctionObjectRef::create(
            state,
            FunctionObjectRef::NativeFunctionInfo(
                AtomicStringRef::create(context, "unsetReceiveContentListener"),
                [](ExecutionStateRef* state, ValueRef* thisValue, size_t argc,
                   ValueRef** argv, bool isNewExpression) -> ValueRef* {
                    ::Starfish::Window* window =
                        (::Starfish::Window*)state->context()
                            ->globalObject()
                            ->extraData();
                    WebWidgetAPIInstance* ww = window->webView()
                                                   ->mainBrowsingContext()
                                                   ->scriptBindingInstance()
                                                   ->deviceAPI()
                                                   ->webWidgetAPIInstance();
                    ww->setReceiveContentListener(nullptr);
                    return ValueRef::createUndefined();
                },
                0, nullptr, true, false));
    targetObject->defineDataProperty(
        state, ValueRef::create(unsetReceiveContentListenerString),
        ValueRef::create(unsetReceiveContentListenerFn), false /* writable */,
        false /* enumerable */, false /* configurable */
        );

    ObjectRef* webWidgetAPIObject = ObjectRef::create(state);
    webWidgetAPIObject->setPrototype(
        state, webWidgetAPIFunctionObj->getFunctionPrototype(state));

    state->destroy();
    return webWidgetAPIObject;
}

extern "C" __attribute__((visibility("default"))) void
starfishWebWidgetAPINotifyReceiveContent(LWE::WebView* instance,
                                         const void* data)
{
    ::Starfish::WebView* w = (::Starfish::WebView*)instance->GetUserData(
        "__internalWebContainerImplementLayerVariable");
    DeviceAPI::ExtensionManagerInstance* em =
        w->mainBrowsingContext()->scriptBindingInstance()->deviceAPI();
    DeviceAPI::WebWidgetAPIInstance* ww = em->webWidgetAPIInstance();
    if (ww) {
        ww->invokeReceiveContentListener(
            w->mainBrowsingContext()->scriptBindingInstance()->scriptContext(),
            data);
    }
}
}

#endif

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"

#include "EscargotPublic.h"

#include <dlfcn.h>
#include "ExtensionAdapter.h"
#include "ExtensionManager.h"

#include "StarFishConfig.h"

using namespace Escargot;

namespace DeviceAPI {

TizenStrings::TizenStrings(Escargot::ContextRef* context)
    : m_context(context)
    , m_initialized(false)
{
    initializeEarlyStrings();
}

#define INIT_TIZEN_STRING(name) \
    name = Escargot::AtomicStringRef::create(m_context, "" #name);
void TizenStrings::initializeEarlyStrings()
{
    DEVICEAPI_LOG_INFO("Enter");

    FOR_EACH_EARLY_TIZEN_STRINGS(INIT_TIZEN_STRING)
}

void TizenStrings::initializeLazyStrings()
{
    DEVICEAPI_LOG_INFO("Enter");

    if (m_initialized)
        return;

    FOR_EACH_LAZY_TIZEN_STRINGS(INIT_TIZEN_STRING)

    // FIXME: this should be automated
    m_entryPoints[ApplicationControl->string()] = application;
    m_entryPoints[ApplicationControlData->string()] = application;

    m_initialized = true;
}
#undef INIT_TIZEN_STRING

void printArguments(Escargot::ContextRef* context, size_t argc,
                    Escargot::ValueRef** argv)
{
    DEVICEAPI_LOG_INFO("printing %u arguments", argc);
    Escargot::ExecutionStateRef* state =
        Escargot::ExecutionStateRef::create(context);
    for (size_t i = 0; i < argc; i++) {
        DEVICEAPI_LOG_INFO("argument %u : %s", i,
                           argv[i]->toString(state)->toStdUTF8String().c_str());
    }
}

wrt::xwalk::Extension* ExtensionManagerInstance::getExtension(
    const char* apiName)
{
    DEVICEAPI_LOG_INFO("Enter");
    wrt::xwalk::ExtensionMap& extensions =
        wrt::xwalk::ExtensionManager::GetInstance()->extensions();

    auto it = extensions.find(apiName);
    if (it == extensions.end()) {
        DEVICEAPI_LOG_INFO("Enter");
        char library_path[512];
        if (!strcmp(apiName, "tizen"))
            snprintf(library_path, 512,
                     "/usr/lib/tizen-extensions-crosswalk/libtizen.so");
        else if (!strcmp(apiName, "sensorservice"))
            snprintf(library_path, 512,
                     "/usr/lib/tizen-extensions-crosswalk/libtizen_sensor.so");
        else
            snprintf(library_path, 512,
                     "/usr/lib/tizen-extensions-crosswalk/libtizen_%s.so",
                     apiName);
        wrt::xwalk::Extension* extension =
            new wrt::xwalk::Extension(library_path, nullptr);
        if (extension->Initialize()) {
            wrt::xwalk::ExtensionManager::GetInstance()->RegisterExtension(
                extension);
            extensions[apiName] = extension;
            return extension;
        } else {
            DEVICEAPI_LOG_INFO("Cannot initialize extension %s", apiName);
            return nullptr;
        }
    } else {
        return it->second;
    }
}

// Caution: this function is called only inside existing js execution context,
// so we don't handle JS exception around ESFunctionObject::call()
Escargot::ObjectRef* ExtensionManagerInstance::initializeExtensionInstance(
    const char* apiName)
{
    DEVICEAPI_LOG_INFO("Enter");

    Escargot::ExecutionStateRef* state =
        Escargot::ExecutionStateRef::create(m_context);
    wrt::xwalk::Extension* extension = getExtension(apiName);
    if (!extension) {
        DEVICEAPI_LOG_INFO("Cannot load extension %s", apiName);
        return Escargot::ObjectRef::create(state);
    }
    std::string str;
    str.append("(function(extension){");
    str.append("extension.internal = {};");
    str.append(
        "extension.internal.sendSyncMessage_ = extension.sendSyncMessage;");
    str.append(
        "extension.internal.sendSyncMessage = function(){ return "
        "extension.internal.sendSyncMessage_.apply(extension, arguments); };");
    str.append("delete extension.sendSyncMessage;");
    str.append("var exports = {};");
    str.append("console.log('Start loading ");
    str.append(apiName);
    str.append("');");
    str.append("(function() {'use strict';");
    str.append(extension->javascript_api().c_str());
    str.append("})();");
    str.append("console.log('Loading ");
    str.append(apiName);
    str.append(" done ');");
    str.append("return exports;})");

    Escargot::StringRef* apiSource =
        Escargot::StringRef::fromASCII(str.c_str());
    Escargot::FunctionObjectRef* initializer = m_context->scriptParser()
                                                   ->parse(apiSource, nullptr)
                                                   .m_script->execute(state)
                                                   ->asFunction();
    Escargot::ObjectRef* extensionObject = createExtensionObject();
    wrt::xwalk::ExtensionInstance* extensionInstance =
        extension->CreateInstance();
    m_extensionInstances[extensionObject] = extensionInstance;
    Escargot::ValueRef* arguments[] = { Escargot::ValueRef::create(
        extensionObject) };
    return initializer
        ->call(state, Escargot::ValueRef::createNull(), 1, arguments)
        ->toObject(state);
}

Escargot::ObjectRef* ExtensionManagerInstance::createExtensionObject()
{
    DEVICEAPI_LOG_INFO("Enter");

    Escargot::ExecutionStateRef* state =
        Escargot::ExecutionStateRef::create(m_context);
    Escargot::ObjectRef* extensionObject = Escargot::ObjectRef::create(state);

    Escargot::FunctionObjectRef* postMessageFn =
        Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->postMessage,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_ERROR("extension.postMessage UNIMPLEMENTED");
                    printArguments(state->context(), argc, argv);
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    return Escargot::ValueRef::createEmpty();
                },
                0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state, Escargot::ValueRef::create(m_strings->postMessage->string()),
        Escargot::ValueRef::create(postMessageFn), true, true, true);

    Escargot::FunctionObjectRef* sendSyncMessageFn =
        Escargot::FunctionObjectRef::create(
            state, Escargot::FunctionObjectRef::NativeFunctionInfo(
                       m_strings->sendSyncMessage,
                       [](Escargot::ExecutionStateRef* state,
                          Escargot::ValueRef* thisValue, size_t argc,
                          Escargot::ValueRef** argv,
                          bool isNewExpression) -> Escargot::ValueRef* {
                           DEVICEAPI_LOG_INFO("extension.sendSyncMessage");
                           printArguments(state->context(), argc, argv);

                           ExtensionManagerInstance* extensionManagerInstance =
                               get(state->context());
                           wrt::xwalk::ExtensionInstance* extensionInstance =
                               extensionManagerInstance
                                   ->getExtensionInstanceFromCallingContext(
                                       state->context(), thisValue);
                           if (!extensionInstance || argc != 1) {
                               return Escargot::ValueRef::create(false);
                           }

                           Escargot::StringRef* message = argv[0]->asString();
                           extensionInstance->HandleSyncMessage(
                               message->toStdUTF8String());

                           std::string reply =
                               extensionInstance->sync_replay_msg();
                           DEVICEAPI_LOG_INFO(
                               "extension.sendSyncMessage Done with reply %s",
                               reply.c_str());

                           if (reply.empty()) {
                               return Escargot::ValueRef::createNull();
                           }
                           return Escargot::ValueRef::create(
                               Escargot::StringRef::fromASCII(reply.c_str()));
                       },
                       0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state, Escargot::ValueRef::create(m_strings->sendSyncMessage->string()),
        Escargot::ValueRef::create(sendSyncMessageFn), true, true, true);

    Escargot::FunctionObjectRef* sendSyncDataFn =
        Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->sendSyncData,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_INFO("extension.sendSyncData");
                    printArguments(state->context(), argc, argv);

                    ExtensionManagerInstance* extensionManagerInstance =
                        get(state->context());
                    wrt::xwalk::ExtensionInstance* extensionInstance =
                        extensionManagerInstance
                            ->getExtensionInstanceFromCallingContext(
                                state->context(), thisValue);
                    if (!extensionInstance || argc < 1) {
                        return Escargot::ValueRef::create(false);
                    }

                    ChunkData chunkData(nullptr, 0);
                    if (argc > 1) {
                        Escargot::ValueRef* dataValue = argv[1];
                        if (dataValue->isObject()) {
                            Escargot::ObjectRef* arrayData =
                                dataValue->toObject(state);
                            size_t length =
                                arrayData
                                    ->get(state,
                                          Escargot::ValueRef::create(
                                              Escargot::StringRef::fromASCII(
                                                  "length")))
                                    ->toLength(state);
                            uint8_t* buffer =
                                (uint8_t*)malloc(sizeof(uint8_t) * length);
                            for (size_t i = 0; i < length; i++) {
                                buffer[i] = static_cast<uint8_t>(
                                    arrayData
                                        ->get(state,
                                              Escargot::ValueRef::create(i))
                                        ->toNumber(state));
                            }
                            chunkData = ChunkData(buffer, length);
                        } else if (dataValue->isString()) {
                            Escargot::StringRef* stringData =
                                dataValue->toString(state);
                            chunkData = ChunkData(
                                (uint8_t*)stringData->toStdUTF8String().c_str(),
                                stringData->length());
                        }
                    }

                    Escargot::StringRef* message = argv[0]->toString(state);
                    extensionInstance->HandleSyncData(
                        message->toStdUTF8String(), chunkData.m_buffer,
                        chunkData.m_length);

                    uint8_t* replyBuffer = nullptr;
                    size_t replyLength = 0;
                    std::string reply = extensionInstance->sync_data_reply_msg(
                        &replyBuffer, &replyLength);

                    DEVICEAPI_LOG_INFO(
                        "extension.sendSyncData Done with reply %s (buffer %s)",
                        reply.c_str(), replyBuffer);

                    if (reply.empty()) {
                        return Escargot::ValueRef::createNull();
                    }

                    Escargot::ObjectRef* returnObject =
                        Escargot::ObjectRef::create(state);
                    returnObject->defineDataProperty(
                        state, Escargot::ValueRef::create(
                                   extensionManagerInstance->strings()
                                       ->reply->string()),
                        Escargot::ValueRef::create(
                            Escargot::StringRef::fromASCII(reply.c_str())),
                        true, true, true);

                    if (replyBuffer || replyLength > 0) {
                        size_t chunkID = extensionManagerInstance->addChunk(
                            replyBuffer, replyLength);
                        returnObject->defineDataProperty(
                            state, Escargot::ValueRef::create(
                                       extensionManagerInstance->strings()
                                           ->chunk_id->string()),
                            Escargot::ValueRef::create(chunkID), true, true,
                            true);
                    }

                    return Escargot::ValueRef::create(returnObject);
                },
                0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state, Escargot::ValueRef::create(m_strings->sendSyncData->string()),
        Escargot::ValueRef::create(sendSyncDataFn), true, true, true);

    Escargot::FunctionObjectRef* sendRuntimeMessageFn =
        Escargot::FunctionObjectRef::create(
            state, Escargot::FunctionObjectRef::NativeFunctionInfo(
                       m_strings->sendRuntimeMessage,
                       [](Escargot::ExecutionStateRef* state,
                          Escargot::ValueRef* thisValue, size_t argc,
                          Escargot::ValueRef** argv,
                          bool isNewExpression) -> Escargot::ValueRef* {
                           DEVICEAPI_LOG_ERROR(
                               "extension.sendRuntimeMessage UNIMPLEMENTED");
                           printArguments(state->context(), argc, argv);
                           STARFISH_RELEASE_ASSERT_NOT_REACHED();
                           return Escargot::ValueRef::createEmpty();
                       },
                       0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state,
        Escargot::ValueRef::create(m_strings->sendRuntimeMessage->string()),
        Escargot::ValueRef::create(sendRuntimeMessageFn), true, true, true);

    Escargot::FunctionObjectRef* sendRuntimeAsyncMessageFn =
        Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->sendRuntimeAsyncMessage,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_ERROR(
                        "extension.sendRuntimeAsyncMessage UNIMPLEMENTED");
                    printArguments(state->context(), argc, argv);
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    return Escargot::ValueRef::createEmpty();
                },
                0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state, Escargot::ValueRef::create(
                   m_strings->sendRuntimeAsyncMessage->string()),
        Escargot::ValueRef::create(sendRuntimeAsyncMessageFn), true, true,
        true);

    Escargot::FunctionObjectRef* sendRuntimeSyncMessageFn =
        Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->sendRuntimeSyncMessage,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_ERROR(
                        "extension.sendRuntimeSyncMessage UNIMPLEMENTED");
                    printArguments(state->context(), argc, argv);
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                    return Escargot::ValueRef::createEmpty();
                },
                0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state,
        Escargot::ValueRef::create(m_strings->sendRuntimeSyncMessage->string()),
        Escargot::ValueRef::create(sendRuntimeSyncMessageFn), true, true, true);

    Escargot::FunctionObjectRef* setMessageListenerFn =
        Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->setMessageListener,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_ERROR("extension.setMessageListener");
                    printArguments(state->context(), argc, argv);

                    ExtensionManagerInstance* extensionManagerInstance =
                        get(state->context());
                    wrt::xwalk::ExtensionInstance* extensionInstance =
                        extensionManagerInstance
                            ->getExtensionInstanceFromCallingContext(
                                state->context(), thisValue);

                    if (!extensionInstance || argc != 1) {
                        return Escargot::ValueRef::create(false);
                    }

                    Escargot::ValueRef* listenerValue = argv[0];
                    if (listenerValue->isUndefined()) {
                        extensionInstance->set_post_message_listener(nullptr);
                        return Escargot::ValueRef::create(true);
                    }

                    if (!listenerValue->isFunction()) {
                        DEVICEAPI_LOG_ERROR(
                            "Trying to set message listener with "
                            "invalid value.");
                        return Escargot::ValueRef::create(false);
                    }

                    Escargot::FunctionObjectRef* listener =
                        listenerValue->asFunction();
                    ESPostMessageListener* postMessageListener =
                        ESPostMessageListener::create(state->context(),
                                                      listener);
                    extensionInstance->set_post_message_listener(
                        postMessageListener);

                    extensionManagerInstance->m_postListeners.push_back(
                        postMessageListener);

                    return Escargot::ValueRef::create(true);
                },
                0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state,
        Escargot::ValueRef::create(m_strings->setMessageListener->string()),
        Escargot::ValueRef::create(setMessageListenerFn), true, true, true);

    Escargot::FunctionObjectRef* receiveChunkDataFn =
        Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->receiveChunkData,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_ERROR("extension.receiveChunkData");
                    printArguments(state->context(), argc, argv);

                    ExtensionManagerInstance* extensionManagerInstance =
                        get(state->context());
                    wrt::xwalk::ExtensionInstance* extensionInstance =
                        extensionManagerInstance
                            ->getExtensionInstanceFromCallingContext(
                                state->context(), thisValue);

                    if (!extensionInstance || argc < 1) {
                        return Escargot::ValueRef::create(false);
                    }

                    TizenStrings* strings = extensionManagerInstance->strings();

                    size_t chunkID = argv[0]->toNumber(state);
                    ExtensionManagerInstance::ChunkData chunkData =
                        extensionManagerInstance->getChunk(chunkID);
                    if (!chunkData.m_buffer) {
                        return Escargot::ValueRef::createNull();
                    }

                    Escargot::StringRef* type = argv[1]->toString(state);
                    bool isStringType =
                        (!type->equals(strings->octet->string()));

                    Escargot::ValueRef* ret;
                    if (isStringType) {
                        ret = Escargot::ValueRef::create(
                            Escargot::StringRef::fromASCII(
                                (const char*)chunkData.m_buffer));
                    } else {
                        Escargot::ArrayObjectRef* octetArray =
                            Escargot::ArrayObjectRef::create(state);
                        for (size_t i = 0; i < chunkData.m_length; i++) {
                            octetArray->set(state,
                                            Escargot::ValueRef::create(i),
                                            Escargot::ValueRef::create(
                                                chunkData.m_buffer[i]));
                        }
                        ret = Escargot::ValueRef::create(octetArray);
                    }
                    free(chunkData.m_buffer);
                    return ret;
                },
                0, nullptr, true, true));

    extensionObject->defineDataProperty(
        state,
        Escargot::ValueRef::create(m_strings->receiveChunkData->string()),
        Escargot::ValueRef::create(receiveChunkDataFn), true, true, true);

    return extensionObject;
}

wrt::xwalk::ExtensionInstance*
ExtensionManagerInstance::getExtensionInstanceFromCallingContext(
    Escargot::ContextRef* context, Escargot::ValueRef* thisValue)
{
    if (thisValue->isUndefinedOrNull()) {
        return nullptr;
    }

    Escargot::ExecutionStateRef* state =
        Escargot::ExecutionStateRef::create(m_context);
    auto it = m_extensionInstances.find(thisValue->toObject(state));
    if (it == m_extensionInstances.end()) {
        return nullptr;
    }

    return it->second;
}

size_t ExtensionManagerInstance::addChunk(uint8_t* buffer, size_t length)
{
    DEVICEAPI_LOG_INFO("Enter");
    size_t chunkID = m_chunkID++;
    m_chunkDataMap[chunkID] = ChunkData(buffer, length);
    return chunkID;
}

ExtensionManagerInstance::ChunkData ExtensionManagerInstance::getChunk(
    size_t chunkID)
{
    DEVICEAPI_LOG_INFO("Enter");
    auto it = m_chunkDataMap.find(chunkID);
    if (it == m_chunkDataMap.end()) {
        return ChunkData(nullptr, 0);
    } else {
        ChunkData chunkData = it->second;
        m_chunkDataMap.erase(it);
        return chunkData;
    }
}

ExtensionManagerInstance::ExtensionManagerInstanceMap
    ExtensionManagerInstance::s_extensionManagerInstances;

ExtensionManagerInstance::ExtensionManagerInstance(
    Escargot::ContextRef* context)
    : m_context(context)
    , m_chunkID(0)
{
    DEVICEAPI_LOG_INFO("new ExtensionManagerInstance %p", this);
    m_strings = new TizenStrings(m_context);
    Escargot::ExecutionStateRef* state =
        Escargot::ExecutionStateRef::create(m_context);

    Escargot::ValueRef* tizenGetter =
        Escargot::ValueRef::create(Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->tizen,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_INFO("Enter");

                    ExtensionManagerInstance* extensionManagerInstance =
                        get(state->context());
                    TizenStrings* strings = extensionManagerInstance->strings();
                    strings->initializeLazyStrings();

                    // initialize tizen object
                    Escargot::ObjectRef* tizenObject =
                        extensionManagerInstance->initializeExtensionInstance(
                            "tizen");

#define DEFINE_SUPPORTED_TIZEN_API(name)                                      \
    tizenObject->defineAccessorProperty(                                      \
        state,                                                                \
        Escargot::ValueRef::create(Escargot::StringRef::fromASCII("" #name)), \
        Escargot::ObjectRef::AccessorPropertyDescriptor(                      \
            Escargot::ValueRef::create(Escargot::FunctionObjectRef::create(   \
                state,                                                        \
                Escargot::FunctionObjectRef::NativeFunctionInfo(              \
                    Escargot::AtomicStringRef::create(state->context(),       \
                                                      "" #name),              \
                    [](Escargot::ExecutionStateRef* state,                    \
                       Escargot::ValueRef* thisValue, size_t argc,            \
                       Escargot::ValueRef** argv,                             \
                       bool isNewExpression) -> Escargot::ValueRef* {         \
                        DEVICEAPI_LOG_INFO("Loading plugin for %s API",       \
                                           "" #name);                         \
                        ExtensionManagerInstance* extensionManagerInstance =  \
                            get(state->context());                            \
                        Escargot::ObjectRef* apiObject =                      \
                            extensionManagerInstance                          \
                                ->initializeExtensionInstance("" #name);      \
                        thisValue->toObject(state)->defineDataProperty(       \
                            state,                                            \
                            Escargot::ValueRef::create(                       \
                                Escargot::StringRef::fromASCII("" #name)),    \
                            Escargot::ValueRef::create(apiObject), false,     \
                            true, false);                                     \
                        return Escargot::ValueRef::create(apiObject);         \
                    },                                                        \
                    0, nullptr, true, true))),                                \
            nullptr,                                                          \
            Escargot::ObjectRef::PresentAttribute::EnumerablePresent));

                    SUPPORTED_TIZEN_PROPERTY(DEFINE_SUPPORTED_TIZEN_API)
#undef DEFINE_SUPPORTED_TIZEN_API

#if 0
#define DEFINE_SUPPORTED_TIZEN_API_ENTRYPOINT(name)                            \
    tizenObject->defineAccessorProperty(                                       \
        state,                                                                 \
        Escargot::ValueRef::create(Escargot::StringRef::fromASCII("" #name)),  \
        Escargot::ObjectRef::AccessorPropertyDescriptor(                       \
            Escargot::ValueRef::create(Escargot::FunctionObjectRef::create(    \
                state,                                                         \
                Escargot::FunctionObjectRef::NativeFunctionInfo(               \
                    Escargot::AtomicStringRef::create(state->context(),        \
                                                      "" #name),               \
                    [](Escargot::ExecutionStateRef* state,                     \
                       Escargot::ValueRef* thisValue, size_t argc,             \
                       Escargot::ValueRef** argv,                              \
                       bool isNewExpression) -> Escargot::ValueRef* {          \
                        ExtensionManagerInstance* extensionManagerInstance =   \
                            get(state->context());                             \
                        TizenStrings* strings =                                \
                            extensionManagerInstance->strings();               \
                        Escargot::StringRef* propertyName =                    \
                            Escargot::StringRef::fromASCII("" #name);          \
                        thisValue->toObject(state)->deleteOwnProperty(         \
                            state, ValueRef::create(propertyName));            \
                        thisValue->toObject(state)->get(                       \
                            state, Escargot::ValueRef::create(                 \
                                       strings->entryPoints()                  \
                                           .find(propertyName)                 \
                                           ->second->string()));               \
                        Escargot::ValueRef* ret =                              \
                            thisValue->toObject(state)->get(                   \
                                state, ValueRef::create(propertyName));        \
                        thisValue->toObject(state)->defineDataProperty(        \
                            state, Escargot::ValueRef::create(propertyName),   \
                            ret, true, true, true);                            \
                        return ret;                                            \
                    },                                                         \
                    0, nullptr, true, true))),                                 \
            Escargot::ValueRef::create(Escargot::FunctionObjectRef::create(    \
                state, Escargot::FunctionObjectRef::NativeFunctionInfo(        \
                           Escargot::AtomicStringRef::create(state->context(), \
                                                             "" #name),        \
                           [](Escargot::ExecutionStateRef* state,              \
                              Escargot::ValueRef* thisValue, size_t argc,      \
                              Escargot::ValueRef** argv,                       \
                              bool isNewExpression) -> Escargot::ValueRef* {   \
                               thisValue->toObject(state)->defineDataProperty( \
                                   state, Escargot::ValueRef::create(          \
                                              Escargot::StringRef::fromASCII(  \
                                                  "" #name)),                  \
                                   thisValue, true, true, true);               \
                               return Escargot::ValueRef::createEmpty();       \
                           },                                                  \
                           0, nullptr, true, true))),                          \
            Escargot::ObjectRef::PresentAttribute::AllPresent));

                    SUPPORTED_TIZEN_ENTRYPOINTS(DEFINE_SUPPORTED_TIZEN_API_ENTRYPOINT)
#undef DEFINE_SUPPORTED_TIZEN_API_ENTRYPOINT
#endif

                    // re-define tizen object
                    thisValue->toObject(state)->defineDataProperty(
                        state,
                        Escargot::ValueRef::create(strings->tizen->string()),
                        Escargot::ValueRef::create(tizenObject), false, true,
                        false);

                    return Escargot::ValueRef::create(tizenObject);
                },
                0, nullptr, true, true)));

    m_context->globalObject()->defineAccessorProperty(
        state, Escargot::ValueRef::create(m_strings->tizen->string()),
        Escargot::ObjectRef::AccessorPropertyDescriptor(
            tizenGetter, nullptr,
            Escargot::ObjectRef::PresentAttribute::EnumerablePresent));

    Escargot::ValueRef* xwalkGetter =
        Escargot::ValueRef::create(Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->xwalk,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_INFO("Enter");

                    ExtensionManagerInstance* extensionManagerInstance =
                        get(state->context());
                    TizenStrings* strings = extensionManagerInstance->strings();
                    strings->initializeLazyStrings();

                    // initialize xwalk object
                    DEVICEAPI_LOG_INFO("Loading plugin for xwalk.utils");
                    Escargot::ObjectRef* xwalkObject =
                        extensionManagerInstance->initializeExtensionInstance(
                            "utils");

                    // re-define xwalk object
                    thisValue->toObject(state)->defineDataProperty(
                        state,
                        Escargot::ValueRef::create(strings->xwalk->string()),
                        Escargot::ValueRef::create(xwalkObject), false, true,
                        false);

                    return Escargot::ValueRef::create(xwalkObject);
                },
                0, nullptr, true, true)));

    m_context->globalObject()->defineAccessorProperty(
        state, Escargot::ValueRef::create(m_strings->xwalk->string()),
        Escargot::ObjectRef::AccessorPropertyDescriptor(
            xwalkGetter, nullptr,
            Escargot::ObjectRef::PresentAttribute::EnumerablePresent));

    Escargot::ValueRef* webapisGetter =
        Escargot::ValueRef::create(Escargot::FunctionObjectRef::create(
            state,
            Escargot::FunctionObjectRef::NativeFunctionInfo(
                m_strings->webapis,
                [](Escargot::ExecutionStateRef* state,
                   Escargot::ValueRef* thisValue, size_t argc,
                   Escargot::ValueRef** argv,
                   bool isNewExpression) -> Escargot::ValueRef* {
                    DEVICEAPI_LOG_INFO("Enter");

                    ExtensionManagerInstance* extensionManagerInstance =
                        get(state->context());
                    TizenStrings* strings = extensionManagerInstance->strings();
                    strings->initializeLazyStrings();

                    // initialize webapis object
                    Escargot::ObjectRef* webapisObject =
                        ObjectRef::create(state);
                    webapisObject->defineAccessorProperty(
                        state,
                        Escargot::ValueRef::create(strings->sa->string()),
                        Escargot::ObjectRef::AccessorPropertyDescriptor(
                            Escargot::ValueRef::create(
                                Escargot::FunctionObjectRef::create(
                                    state,
                                    Escargot::FunctionObjectRef::NativeFunctionInfo(
                                        strings->sa,
                                        [](Escargot::ExecutionStateRef* state,
                                           Escargot::ValueRef* thisValue,
                                           size_t argc,
                                           Escargot::ValueRef** argv,
                                           bool isNewExpression)
                                            -> Escargot::ValueRef* {
                                                DEVICEAPI_LOG_INFO(
                                                    "Loading plugin for "
                                                    "Samsung Accessory "
                                                    "protocol API");
                                                ExtensionManagerInstance*
                                                    extensionManagerInstance =
                                                        get(state->context());
                                                Escargot::ObjectRef* saObject =
                                                    extensionManagerInstance
                                                        ->initializeExtensionInstance(
                                                            "sa");
                                                thisValue->toObject(state)
                                                    ->defineDataProperty(
                                                        state,
                                                        Escargot::ValueRef::
                                                            create("sa"),
                                                        Escargot::ValueRef::
                                                            create(saObject),
                                                        false, true, false);
                                                return Escargot::ValueRef::
                                                    create(saObject);
                                            },
                                        0, nullptr, true, true))),
                            nullptr, Escargot::ObjectRef::PresentAttribute::
                                         EnumerablePresent));

                    // re-define webapis object
                    thisValue->toObject(state)->defineDataProperty(
                        state,
                        Escargot::ValueRef::create(strings->webapis->string()),
                        Escargot::ValueRef::create(webapisObject), false, true,
                        false);

                    return Escargot::ValueRef::create(webapisObject);
                },
                0, nullptr, true, true)));

    m_context->globalObject()->defineAccessorProperty(
        state, Escargot::ValueRef::create(m_strings->webapis->string()),
        Escargot::ObjectRef::AccessorPropertyDescriptor(
            webapisGetter, nullptr,
            Escargot::ObjectRef::PresentAttribute::EnumerablePresent));

    s_extensionManagerInstances[m_context] = this;
    DEVICEAPI_LOG_INFO("%zu => %zu", s_extensionManagerInstances.size() - 1,
                       s_extensionManagerInstances.size());
}

ExtensionManagerInstance::~ExtensionManagerInstance()
{
    DEVICEAPI_LOG_INFO("delete ExtensionManagerInstance %p", this);
    for (auto it : m_extensionInstances)
        delete it.second;
    for (auto it : m_postListeners)
        it->finalize();
    auto it = s_extensionManagerInstances.find(m_context);
    s_extensionManagerInstances.erase(it);
    DEVICEAPI_LOG_INFO("%zu => %zu", s_extensionManagerInstances.size() + 1,
                       s_extensionManagerInstances.size());
}

ExtensionManagerInstance* ExtensionManagerInstance::get(
    Escargot::ContextRef* context)
{
    auto it = s_extensionManagerInstances.find(context);
    if (it == s_extensionManagerInstances.end())
        return nullptr;
    else
        return it->second;
}

void initialize(Escargot::ContextRef* context)
{
    DEVICEAPI_LOG_INFO("Enter with context %p", context);
    new ExtensionManagerInstance(context);
}

void close(Escargot::ContextRef* context)
{
    DEVICEAPI_LOG_INFO("Enter with context %p", context);
    delete ExtensionManagerInstance::get(context);
}
}

#endif

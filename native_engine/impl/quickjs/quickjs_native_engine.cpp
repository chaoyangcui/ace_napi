/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "quickjs_native_engine.h"

#include "native_engine/native_engine.h"
#include "native_engine/native_property.h"
#include "native_value/quickjs_native_array.h"
#include "native_value/quickjs_native_array_buffer.h"
#include "native_value/quickjs_native_boolean.h"
#include "native_value/quickjs_native_data_view.h"
#include "native_value/quickjs_native_external.h"
#include "native_value/quickjs_native_function.h"
#include "native_value/quickjs_native_number.h"
#include "native_value/quickjs_native_object.h"
#include "native_value/quickjs_native_string.h"
#include "native_value/quickjs_native_typed_array.h"
#include "quickjs_native_deferred.h"
#include "quickjs_native_reference.h"
#include "securec.h"

#include "utils/assert.h"
#include "utils/log.h"

const int JS_WRITE_OBJ = (1 << 2) | (1 << 3);
const int JS_ATOM_MESSAGE = 51;

QuickJSNativeEngine::QuickJSNativeEngine(JSRuntime* runtime, JSContext* context)
{
    runtime_ = runtime;
    context_ = context;

    AddIntrinsicBaseClass(context_);
    AddIntrinsicExternal(context_);

    JSValue jsGlobal = JS_GetGlobalObject(context_);
    JSValue jsNativeEngine = (JSValue)JS_MKPTR(JS_TAG_INT, this);
    JSValue jsRequireInternal = JS_NewCFunctionData(
        context_,
        [](JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv, int magic,
           JSValue* funcData) -> JSValue {
            JSValue result = JS_UNDEFINED;

            QuickJSNativeEngine* that = (QuickJSNativeEngine*)JS_VALUE_GET_PTR(funcData[0]);

            const char* moduleName = JS_ToCString(that->GetContext(), argv[0]);

            if (moduleName == nullptr || strlen(moduleName) == 0) {
                HILOG_ERROR("moduleName is nullptr or length is 0");
                return result;
            }

            NativeModuleManager* moduleManager = that->GetModuleManager();
            NativeModule* module = moduleManager->LoadNativeModule(moduleName, nullptr, true);

            if (module != nullptr && module->registerCallback != nullptr) {
                NativeValue* value = new QuickJSNativeObject(that);
                if (value != nullptr) {
                    module->registerCallback(that, value);
                    result = JS_DupValue(that->GetContext(), *value);
                }
            }
            JS_FreeCString(that->GetContext(), moduleName);
            return result;
        },
        0, 0, 1, &jsNativeEngine);

    JSValue jsRequire = JS_NewCFunctionData(
        context_,
        [](JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv, int magic,
           JSValue* funcData) -> JSValue {
            JSValue result = JS_UNDEFINED;

            QuickJSNativeEngine* that = (QuickJSNativeEngine*)JS_VALUE_GET_PTR(funcData[0]);
            const char* moduleName = JS_ToCString(that->GetContext(), argv[0]);
            if (moduleName == nullptr || strlen(moduleName) == 0) {
                HILOG_ERROR("moduleName is nullptr or length is 0");
                return result;
            }

            const char* path = nullptr;
            if (argc == 2) {
                path = JS_ToCString(that->GetContext(), argv[1]);
            }
            NativeModuleManager* moduleManager = that->GetModuleManager();
            NativeModule* module = moduleManager->LoadNativeModule(moduleName, nullptr);

            if (module != nullptr) {
                if (module->jsCode != nullptr) {
                    HILOG_INFO("load js code");
                    NativeValue* script = that->CreateString(module->jsCode, strlen(module->jsCode));
                    NativeValue* exportObject = that->LoadModule(script, "testjsnapi.js");
                    if (exportObject == nullptr) {
                        HILOG_ERROR("load module failed");
                        return result;
                    }
                    result = JS_DupValue(that->GetContext(), *exportObject);
                    HILOG_ERROR("load module succ");
                } else if (module->registerCallback != nullptr) {
                    HILOG_INFO("load napi module");
                    NativeValue* value = new QuickJSNativeObject(that);
                    module->registerCallback(that, value);
                    result = JS_DupValue(that->GetContext(), *value);
                } else {
                    HILOG_ERROR("init module failed");
                }
            }
            JS_FreeCString(that->GetContext(), moduleName);
            return result;
        },
        0, 0, 1, &jsNativeEngine);

    JS_SetPropertyStr(context_, jsGlobal, "requireInternal", jsRequireInternal);
    JS_SetPropertyStr(context_, jsGlobal, "requireNapi", jsRequire);
    JS_FreeValue(context_, jsGlobal);
}

QuickJSNativeEngine::~QuickJSNativeEngine() {}

JSRuntime* QuickJSNativeEngine::GetRuntime()
{
    return runtime_;
}

JSContext* QuickJSNativeEngine::GetContext()
{
    return context_;
}

void QuickJSNativeEngine::Loop(LoopMode mode)
{
    JSContext* context = nullptr;
    NativeEngine::Loop(mode);
    int err = JS_ExecutePendingJob(runtime_, &context);
    if (err < 0) {
        js_std_dump_error(context);
    }
}

NativeValue* QuickJSNativeEngine::GetGlobal()
{
    JSValue value = JS_GetGlobalObject(context_);
    return new QuickJSNativeObject(this, value);
}

NativeValue* QuickJSNativeEngine::CreateNull()
{
    return new QuickJSNativeValue(this, JS_NULL);
}

NativeValue* QuickJSNativeEngine::CreateUndefined()
{
    return new QuickJSNativeValue(this, JS_UNDEFINED);
}

NativeValue* QuickJSNativeEngine::CreateBoolean(bool value)
{
    return new QuickJSNativeBoolean(this, value);
}

NativeValue* QuickJSNativeEngine::CreateNumber(int32_t value)
{
    return new QuickJSNativeNumber(this, value);
}

NativeValue* QuickJSNativeEngine::CreateNumber(uint32_t value)
{
    return new QuickJSNativeNumber(this, value);
}

NativeValue* QuickJSNativeEngine::CreateNumber(int64_t value)
{
    return new QuickJSNativeNumber(this, value);
}

NativeValue* QuickJSNativeEngine::CreateNumber(double value)
{
    return new QuickJSNativeNumber(this, value);
}

NativeValue* QuickJSNativeEngine::CreateString(const char* value, size_t length)
{
    return new QuickJSNativeString(this, value, length);
}

NativeValue* QuickJSNativeEngine::CreateSymbol(NativeValue* value)
{
    JSValue symbol = { 0 };

    JSValue global = JS_GetGlobalObject(context_);
    JSValue symbolCotr = JS_GetPropertyStr(context_, global, "Symbol");

    JSValue jsValue = *value;

    symbol = JS_Call(context_, symbolCotr, global, 1, &jsValue);

    JS_FreeValue(context_, symbolCotr);
    JS_FreeValue(context_, global);

    return new QuickJSNativeValue(this, symbol);
}

NativeValue* QuickJSNativeEngine::CreateFunction(const char* name, size_t length, NativeCallback cb, void* value)
{
    return new QuickJSNativeFunction(this, name, cb, value);
}

NativeValue* QuickJSNativeEngine::CreateExternal(void* value, NativeFinalize callback, void* hint)
{
    return new QuickJSNativeExternal(this, value, callback, hint);
}

NativeValue* QuickJSNativeEngine::CreateObject()
{
    return new QuickJSNativeObject(this);
}

NativeValue* QuickJSNativeEngine::CreateArrayBuffer(void** value, size_t length)
{
    return new QuickJSNativeArrayBuffer(this, (uint8_t**)value, length);
}

NativeValue* QuickJSNativeEngine::CreateArrayBufferExternal(void* value, size_t length, NativeFinalize cb, void* hint)
{
    return new QuickJSNativeArrayBuffer(this, (uint8_t*)value, length, cb, hint);
}

NativeValue* QuickJSNativeEngine::CreateArray(size_t length)
{
    return new QuickJSNativeArray(this, length);
}

NativeValue* QuickJSNativeEngine::CreateDataView(NativeValue* value, size_t length, size_t offset)
{
    return new QuickJSNativeDataView(this, value, length, offset);
}

NativeValue* QuickJSNativeEngine::CreateTypedArray(NativeTypedArrayType type,
                                                   NativeValue* value,
                                                   size_t length,
                                                   size_t offset)
{
    return new QuickJSNativeTypedArray(this, type, value, length, offset);
}

NativeValue* QuickJSNativeEngine::CreatePromise(NativeDeferred** deferred)
{
    JSValue promise = { 0 };
    JSValue resolvingFuncs[2] = { 0 };
    promise = JS_NewPromiseCapability(context_, resolvingFuncs);
    *deferred = new QuickJSNativeDeferred(this, resolvingFuncs);
    return new QuickJSNativeValue(this, promise);
}

NativeValue* QuickJSNativeEngine::CreateError(NativeValue* code, NativeValue* message)
{
    JSValue error = JS_NewError(context_);
    if (code) {
        JS_SetPropertyStr(context_, error, "code", JS_DupValue(context_, *code));
    }
    if (message) {
        JS_SetPropertyStr(context_, error, "message", JS_DupValue(context_, *message));
    }

    return new QuickJSNativeObject(this, error);
}

NativeValue* QuickJSNativeEngine::CreateInstance(NativeValue* constructor, NativeValue* const* argv, size_t argc)
{
    JSValue result = JS_UNDEFINED;
    JSValue* params = nullptr;
    if (argc > 0) {
        params = new JSValue[argc];
        for (size_t i = 0; i < argc; i++) {
            params[i] = *argv[i];
        }
    }
    result = JS_CallConstructor(context_, *constructor, argc, params);
    if (params != nullptr) {
        delete[] params;
    }
    return QuickJSNativeEngine::JSValueToNativeValue(this, result);
}

NativeReference* QuickJSNativeEngine::CreateReference(NativeValue* value, uint32_t initialRefcount)
{
    return new QuickJSNativeReference(this, value, initialRefcount);
}

NativeValue* QuickJSNativeEngine::CallFunction(NativeValue* thisVar,
                                               NativeValue* function,
                                               NativeValue* const* argv,
                                               size_t argc)
{
    JSValue result = JS_UNDEFINED;

    if (function == nullptr) {
        return new QuickJSNativeValue(this, JS_UNDEFINED);
    }

    NativeScope* scope = scopeManager_->Open();
    if (scope == nullptr) {
        HILOG_ERROR("Open scope failed");
        return new QuickJSNativeValue(this, JS_UNDEFINED);
    }

    JSValue* args = nullptr;
    if (argc > 0) {
        args = new JSValue[argc];
        for (size_t i = 0; i < argc; i++) {
            if (argv[i] != nullptr) {
                args[i] = *argv[i];
            } else {
                args[i] = JS_UNDEFINED;
            }
        }
    }

    result = JS_Call(context_, *function, (thisVar != nullptr) ? (JSValue)*thisVar : JS_UNDEFINED, argc, args);
    JS_DupValue(context_, result);

    if (args != nullptr) {
        delete []args;
    }

    scopeManager_->Close(scope);

    if (JS_IsError(context_, result) || JS_IsException(result)) {
        return nullptr;
    }

    return JSValueToNativeValue(this, result);
}

NativeValue* QuickJSNativeEngine::RunScript(NativeValue* script)
{
    JSValue result;
    const char* cScript = JS_ToCString(context_, *script);
    result = JS_Eval(context_, cScript, strlen(cScript), "<input>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeCString(context_, cScript);
    if (JS_IsError(context_, result) || JS_IsException(result)) {
        return nullptr;
    }

    return JSValueToNativeValue(this, result);
}

NativeValue* QuickJSNativeEngine::LoadModule(NativeValue* str, const std::string& fileName)
{
    if (str == nullptr || fileName.empty()) {
        HILOG_ERROR("Module name is nullptr or source code length is 0");
        return nullptr;
    }

    JS_SetModuleLoaderFunc(runtime_, nullptr, js_module_loader, nullptr);
    const char* moduleSource = JS_ToCString(context_, *str);
    size_t len = strlen(moduleSource);
    int flags = JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY;
    JSValue moduleVal = JS_Eval(context_, moduleSource, len, fileName.c_str(), flags);
    if (JS_IsException(moduleVal)) {
        HILOG_ERROR("Eval source code exception");
        JS_FreeCString(context_, moduleSource);
        return nullptr;
    }

    JSValue evalRes = JS_EvalFunction(context_, moduleVal);
    if (JS_IsException(evalRes)) {
        HILOG_ERROR("Eval module exception");
        JS_FreeCString(context_, moduleSource);
        return nullptr;
    }

    JSValue ns = JS_GetNameSpace(context_, moduleVal);
    JSValue result = JS_GetPropertyStr(context_, ns, "default");

    JS_FreeValue(context_, ns);
    JS_FreeValue(context_, evalRes);
    JS_FreeCString(context_, moduleSource);
    return JSValueToNativeValue(this, result);
}

NativeValue* QuickJSNativeEngine::DefineClass(const char* name,
                                              NativeCallback callback,
                                              void* data,
                                              const NativePropertyDescriptor* properties,
                                              size_t length)
{
    NativeFunctionInfo* functionInfo = new NativeFunctionInfo();
    if (functionInfo == nullptr) {
        HILOG_ERROR("new NativeFunctionInfo failed");
        return nullptr;
    }
    functionInfo->engine = this;
    functionInfo->data = data;
    functionInfo->callback = callback;

    JSValue classConstructor = JS_NewCFunction2(
        context_,
        [](JSContext* ctx, JSValueConst newTarget, int argc, JSValueConst* argv) -> JSValue {
            auto callbackInfo = new NativeCallbackInfo();
            JSValue prototype = JS_GetPropertyStr(ctx, newTarget, "prototype");
            JSValue classContext = JS_GetPropertyStr(ctx, newTarget, "_classContext");

            auto functionInfo = (NativeFunctionInfo*)JS_ExternalToNativeObject(ctx, classContext);
            if (functionInfo == nullptr) {
                HILOG_ERROR("functionInfo is nullptr");
                return JS_UNDEFINED;
            }

            QuickJSNativeEngine* engine = (QuickJSNativeEngine*)functionInfo->engine;
            NativeScopeManager* scopeManager = engine->GetScopeManager();
            if (scopeManager == nullptr) {
                HILOG_ERROR("scopeManager is nullptr");
                return JS_UNDEFINED;
            }

            NativeScope* scope = scopeManager->Open();
            if (scope == nullptr) {
                HILOG_ERROR("scope is nullptr");
                return JS_UNDEFINED;
            }

            callbackInfo->argc = argc;
            callbackInfo->argv = nullptr;
            callbackInfo->function = JSValueToNativeValue(engine, JS_DupValue(ctx, newTarget));
            callbackInfo->functionInfo = functionInfo;
            callbackInfo->thisVar =
                JSValueToNativeValue(engine, JS_NewObjectProtoClass(ctx, prototype, GetBaseClassID()));

            if (callbackInfo->argc > 0) {
                callbackInfo->argv = new NativeValue*[argc];
                for (size_t i = 0; i < callbackInfo->argc; i++) {
                    callbackInfo->argv[i] = JSValueToNativeValue(engine, JS_DupValue(ctx, argv[i]));
                }
            }

            NativeValue* value = functionInfo->callback(engine, callbackInfo);

            if (callbackInfo != nullptr) {
                delete callbackInfo->argv;
            }

            JSValue result = JS_UNDEFINED;
            if (value != nullptr) {
                result = JS_DupValue(ctx, *value);
            } else if (engine->IsExceptionPending()) {
                NativeValue* error = engine->GetAndClearLastException();
                if (error != nullptr) {
                    result = JS_DupValue(ctx, *error);
                }
            }

            delete callbackInfo;
            scopeManager->Close(scope);

            return result;
        },
        name, 0, JS_CFUNC_constructor_or_func, 0);
    JSValue proto = JS_NewObject(context_);

    QuickJSNativeObject* nativeClass = new QuickJSNativeObject(this, JS_DupValue(context_, classConstructor));
    QuickJSNativeObject* nativeClassProto = new QuickJSNativeObject(this, proto);

    for (size_t i = 0; i < length; i++) {
        if (properties[i].attributes & NATIVE_STATIC) {
            nativeClass->DefineProperty(properties[i]);
        } else {
            nativeClassProto->DefineProperty(properties[i]);
        }
    }

    JS_DefinePropertyValueStr(context_, *nativeClass, "prototype", JS_DupValue(context_, *nativeClassProto), 0);

    JSValue classContext = JS_NewExternal(context_, functionInfo,
                                          [](JSContext* ctx, void* data, void* hint) {
                                              auto info = (NativeFunctionInfo*)data;
                                              HILOG_INFO("_classContext Destroy");
                                              if (info != nullptr) {
                                                  delete info;
                                              }
                                          }, nullptr);
    JS_DefinePropertyValueStr(context_, *nativeClass, "_classContext", classContext, 0);

    JS_DefinePropertyValueStr(context_, *nativeClassProto, "constructor", JS_DupValue(context_, *nativeClass),
                              JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);

    return nativeClass;
}

bool QuickJSNativeEngine::Throw(NativeValue* error)
{
    JS_Throw(context_, *error);
    this->lastException_ = error;
    return true;
}

bool QuickJSNativeEngine::Throw(NativeErrorType type, const char* code, const char* message)
{
    JSValue error;
    switch (type) {
        case NativeErrorType::NATIVE_TYPE_ERROR:
            error = JS_ThrowTypeError(context_, "code: %s, message: %s\n", code, message);
            break;
        case NativeErrorType::NATIVE_RANGE_ERROR:
            error = JS_ThrowRangeError(context_, "code: %s, message: %s\n", code, message);
            break;
        default:
            error = JS_ThrowInternalError(context_, "code: %s, message: %s\n", code, message);
    }
    this->lastException_ = new QuickJSNativeValue(this, error);
    return true;
}

NativeValue* QuickJSNativeEngine::JSValueToNativeValue(QuickJSNativeEngine* engine, JSValue value)
{
    NativeValue* result = nullptr;
    int tag = JS_VALUE_GET_NORM_TAG(value);
    switch (tag) {
        case JS_TAG_BIG_INT:
        case JS_TAG_BIG_FLOAT:
            result = new QuickJSNativeObject(engine, value);
            break;
        case JS_TAG_SYMBOL:
            result = new QuickJSNativeValue(engine, value);
            break;
        case JS_TAG_STRING:
            result = new QuickJSNativeString(engine, value);
            break;
        case JS_TAG_OBJECT:
            if (JS_IsArray(engine->GetContext(), value)) {
                result = new QuickJSNativeArray(engine, value);
            } else if (JS_IsError(engine->GetContext(), value)) {
                result = new QuickJSNativeValue(engine, value);
            } else if (JS_IsPromise(engine->GetContext(), value)) {
                result = new QuickJSNativeValue(engine, value);
            } else if (JS_IsArrayBuffer(engine->GetContext(), value)) {
                result = new QuickJSNativeArrayBuffer(engine, value);
            } else if (JS_IsDataView(engine->GetContext(), value)) {
                result = new QuickJSNativeDataView(engine, value);
            } else if (JS_IsTypedArray(engine->GetContext(), value)) {
                result = new QuickJSNativeTypedArray(engine, value);
            } else if (JS_IsExternal(engine->GetContext(), value)) {
                result = new QuickJSNativeExternal(engine, value);
            } else if (JS_IsFunction(engine->GetContext(), value)) {
                result = new QuickJSNativeFunction(engine, value);
            } else {
                result = new QuickJSNativeObject(engine, value);
            }
            break;
        case JS_TAG_BOOL:
            result = new QuickJSNativeBoolean(engine, value);
            break;
        case JS_TAG_NULL:
        case JS_TAG_UNDEFINED:
        case JS_TAG_UNINITIALIZED:
        case JS_TAG_CATCH_OFFSET:
        case JS_TAG_EXCEPTION:
            result = new QuickJSNativeValue(engine, value);
            break;
        case JS_TAG_INT:
        case JS_TAG_FLOAT64:
            result = new QuickJSNativeNumber(engine, value);
            break;
        default:
            HILOG_DEBUG("JS_VALUE_GET_NORM_TAG %{public}d", tag);
    }
    return result;
}

 void* QuickJSNativeEngine::CreateRuntime()
 {
    JSRuntime* runtime = JS_NewRuntime();
    if (runtime == nullptr) {
        return nullptr;
    }
    JSContext* context = JS_NewContext(runtime);
    if (context == nullptr) {
        return nullptr;
    }
    auto qjsEngine = new QuickJSNativeEngine(runtime, context);
    if (qjsEngine) {
        return reinterpret_cast<void*>(qjsEngine);
    }
    return nullptr;
}

bool QuickJSNativeEngine::CheckTransferList(JSValue transferList)
{
    if (JS_IsUndefined(transferList)) {
        return true;
    }
    if (!JS_IsArray(context_, transferList)) {
        JS_ThrowTypeError(context_, "postMessage second parameter not a list or undefined");
        return false;
    }
    int64_t len = 0;
    js_get_length64(context_, &len, transferList);
    for (int64_t i = 0; i < len; i++) {
        JSValue tmp = JS_GetPropertyInt64(context_, transferList, i);
        if (!JS_IsException(tmp)) {
            if (!JS_IsArrayBuffer(context_, tmp)) {
                HILOG_ERROR("JS_ISArrayBuffer fail");
                return false;
            }
        } else {
            HILOG_ERROR("JS_GetPropertyInt64 fail");
            return false;
        }
    }
    return true;
}

bool QuickJSNativeEngine::DetachTransferList(JSValue transferList)
{
    if (JS_IsUndefined(transferList)) {
        return true;
    }
    int64_t len = 0;
    js_get_length64(context_, &len, transferList);
    for (int64_t i = 0; i < len; i++) {
        JSValue tmp = JS_GetPropertyInt64(context_, transferList, i);
        if (!JS_IsException(tmp)) {
            JS_DetachArrayBuffer(context_, tmp);
        } else {
            return false;
        }
    }
    return true;

}

NativeValue* QuickJSNativeEngine::Serialize(NativeEngine* context, NativeValue* value, NativeValue* transfer)
{
    if (!CheckTransferList(*transfer)) {
        return nullptr;
    }
    size_t dataLen;
    uint8_t *data = JS_WriteObject(context_, &dataLen, *value, JS_WRITE_OBJ);
    DetachTransferList(*transfer);
    return reinterpret_cast<NativeValue*>(new SerializeData(dataLen, data));
}
 
NativeValue* QuickJSNativeEngine::Deserialize(NativeEngine* context, NativeValue* recorder)
{
    std::unique_ptr<SerializeData> data(reinterpret_cast<SerializeData*>(recorder));
    JSValue result = JS_ReadObject(context_, data->GetData(), data->GetSize(), JS_WRITE_OBJ);
    return JSValueToNativeValue(this, result);
}
 
void QuickJSNativeEngine::DeleteSerializationData(NativeValue* value) const
{
    SerializeData* data = reinterpret_cast<SerializeData*>(value);
    delete data;
}
 
ExceptionInfo* QuickJSNativeEngine::GetExceptionForWorker() const
{
    JSValue exception = JS_GetCurrentException(runtime_);
    ASSERT(JS_IsObject(exception));
    JSValue msg;
    ExceptionInfo* exceptionInfo = new ExceptionInfo();
    msg = JS_GetProperty(context_, exception, JS_ATOM_MESSAGE);
    ASSERT(JS_IsString(msg));
    const char* exceptionStr  = reinterpret_cast<char *>(JS_GetStringFromObject(msg));
    const char* error = "Error: ";
    int len = strlen(exceptionStr) + strlen(error) + 1;
    if (len <= 0) {
        delete exceptionInfo;
        return nullptr;
    }
    char* exceptionMessage = new char[len] { 0 };
    if (memcpy_s(exceptionMessage, len, error, strlen(error)) != EOK) {
        HILOG_INFO("worker:: memcpy_s error");
        delete exceptionInfo;
        delete[] exceptionMessage;
        return nullptr;
    }
    if (memcpy_s(exceptionMessage + strlen(error), len, exceptionStr, strlen(exceptionStr)) != EOK) {
        HILOG_INFO("worker:: memcpy_s error");
        delete exceptionInfo;
        delete[] exceptionMessage;
        return nullptr;
    }
    exceptionInfo->message_ = exceptionMessage;
    return exceptionInfo;
}

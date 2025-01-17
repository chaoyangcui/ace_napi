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

#include "native_engine.h"
#include "utils/log.h"

#include <uv.h>

namespace {
const char* g_errorMessages[] = {
    nullptr,
    "Invalid parameter",
    "Need object",
    "Need string",
    "Need string or symbol",
    "Need function",
    "Need number",
    "Need boolean",
    "Need array",
    "Generic failure",
    "An exception is blocking",
    "Asynchronous work cancelled",
    "Escape called twice",
    "Handle scope mismatch",
    "Callback scope mismatch",
    "Asynchronous work queue is full",
    "Asynchronous work handle is closing",
    "Need bigint",
    "Need date",
    "Need arraybuffer",
    "Need detachable arraybuffer",
};
} // namespace

NativeEngine::NativeEngine()
{
    moduleManager_ = NativeModuleManager::GetInstance();
    scopeManager_ = new NativeScopeManager();
    if (scopeManager_) {
        loop_ = uv_loop_new();
        lastException_ = nullptr;
    } else {
        HILOG_ERROR("contruct NativeEngine error.");
    }
}

NativeEngine::~NativeEngine()
{
    uv_loop_close(loop_);
    delete scopeManager_;
}

NativeScopeManager* NativeEngine::GetScopeManager()
{
    return scopeManager_;
}

NativeModuleManager* NativeEngine::GetModuleManager()
{
    return moduleManager_;
}

uv_loop_t* NativeEngine::GetUVLoop() const
{
    return loop_;
}

void NativeEngine::Loop(LoopMode mode)
{
    bool more = true;
    switch (mode) {
        case LOOP_DEFAULT:
            more = uv_run(loop_, UV_RUN_DEFAULT);
            break;
        case LOOP_ONCE:
            more = uv_run(loop_, UV_RUN_ONCE);
            break;
        case LOOP_NOWAIT:
            more = uv_run(loop_, UV_RUN_NOWAIT);
            break;
        default:
            return;
    }
    if (more == false) {
        more = uv_loop_alive(loop_);
    }
}

NativeAsyncWork* NativeEngine::CreateAsyncWork(NativeValue* asyncResource,
                                               NativeValue* asyncResourceName,
                                               NativeAsyncExecuteCallback execute,
                                               NativeAsyncCompleteCallback complete,
                                               void* data)
{
    (void)asyncResource;
    (void)asyncResourceName;
    return new NativeAsyncWork(this, execute, complete, data);
}

NativeAsyncWork* NativeEngine::CreateAsyncWork(NativeAsyncExecuteCallback execute,
                                               NativeAsyncCompleteCallback complete,
                                               void* data)
{
    return new NativeAsyncWork(this, execute, complete, data);
}

NativeErrorExtendedInfo* NativeEngine::GetLastError()
{
    return &lastError_;
}

void NativeEngine::SetLastError(int errorCode, uint32_t engineErrorCode, void* engineReserved)
{
    lastError_.errorCode = errorCode;
    lastError_.engineErrorCode = engineErrorCode;
    lastError_.message = g_errorMessages[lastError_.errorCode];
    lastError_.reserved = engineReserved;
}

void NativeEngine::ClearLastError()
{
    lastError_.errorCode = 0;
    lastError_.engineErrorCode = 0;
    lastError_.message = nullptr;
    lastError_.reserved = nullptr;
}

bool NativeEngine::IsExceptionPending() const
{
    return !(lastException_ == nullptr);
}

NativeValue* NativeEngine::GetAndClearLastException()
{
    NativeValue* temp = lastException_;
    lastException_ = nullptr;
    return temp;
}

void NativeEngine::EncodeToUtf8(NativeValue* nativeValue,
                                char* buffer,
                                int32_t* written,
                                size_t bufferSize,
                                int32_t* nchars)
{
    if (nativeValue == nullptr || nchars == nullptr || written == nullptr) {
        HILOG_ERROR("NativeEngine EncodeToUtf8 args is nullptr");
        return;
    }

    auto nativeString = reinterpret_cast<NativeString*>(nativeValue->GetInterface(NativeString::INTERFACE_ID));

    if (nativeString) {
        *written = nativeString->EncodeWriteUtf8(buffer, bufferSize, nchars);
    } else {
        HILOG_ERROR("NativeEngine EncodeToUtf8 nativeString is nullptr");
    }
}
void NativeEngine::SetPostTask(PostTask postTask)
{
    HILOG_INFO("SetPostTask in");
    postTask_ = postTask;
}

void NativeEngine::TriggerPostTask()
{
    if (postTask_ == nullptr) {
        HILOG_ERROR("postTask_ is nullptr");
        return;
    }
    postTask_();
}

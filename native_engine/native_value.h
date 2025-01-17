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

#ifndef FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_VALUE_H
#define FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_VALUE_H

#include <stddef.h>
#include <stdint.h>

class NativeValue;
class NativeEngine;

struct NativePropertyDescriptor;
struct NativeCallbackInfo;

typedef NativeValue* (*NativeCallback)(NativeEngine* engine, NativeCallbackInfo*);
typedef void (*NativeFinalize)(NativeEngine* engine, void* data, void* hint);

typedef void (*NativeAsyncExecuteCallback)(NativeEngine* engine, void* data);
typedef void (*NativeAsyncCompleteCallback)(NativeEngine* engine, int status, void* data);

struct NativeObjectInfo {
    NativeEngine* engine = nullptr;
    void* nativeObject = nullptr;
    NativeFinalize callback = nullptr;
    void* hint = nullptr;
};

struct NativeFunctionInfo {
    NativeEngine* engine = nullptr;
    NativeCallback callback = nullptr;
    void* data = nullptr;
};

struct NativeCallbackInfo {
    size_t argc = 0;
    NativeValue** argv = nullptr;
    NativeValue* thisVar = nullptr;
    NativeValue* function = nullptr;
    NativeFunctionInfo* functionInfo = nullptr;
};

typedef void (*NaitveFinalize)(NativeEngine* env, void* data, void* hint);

enum NativeValueType {
    NATIVE_UNDEFINED,
    NATIVE_NULL,
    NATIVE_BOOLEAN,
    NATIVE_NUMBER,
    NATIVE_STRING,
    NATIVE_SYMBOL,
    NATIVE_OBJECT,
    NATIVE_FUNCTION,
    NATIVE_EXTERNAL,
    NATIVE_BIGINT,
};

class NativeValue {
public:
    virtual ~NativeValue() {}

    template<typename T> operator T()
    {
        return value_;
    }

    template<typename T> NativeValue& operator=(T value)
    {
        value_ = value;
        return *this;
    }

    virtual void* GetInterface(int interfaceId) = 0;

    virtual NativeValueType TypeOf() = 0;
    virtual bool InstanceOf(NativeValue* obj) = 0;

    virtual bool IsArray() = 0;
    virtual bool IsArrayBuffer() = 0;
    virtual bool IsDate() = 0;
    virtual bool IsError() = 0;
    virtual bool IsTypedArray() = 0;
    virtual bool IsDataView() = 0;
    virtual bool IsPromise() = 0;
    virtual bool IsCallable() = 0;

    virtual NativeValue* ToBoolean() = 0;
    virtual NativeValue* ToNumber() = 0;
    virtual NativeValue* ToString() = 0;
    virtual NativeValue* ToObject() = 0;

    virtual bool operator==(NativeValue* value) = 0;

protected:
    struct {
        template<typename T> operator T()
        {
            return *(T*)this;
        }
        template<typename T> T operator=(T value)
        {
            *(T*)this = value;
            return *this;
        }
        union {
            int32_t int32;
            double float64;
            void* ptr;
        } u;
        int64_t tag;
    } value_;
};

class NativeBoolean {
public:
    static const int INTERFACE_ID = 0;

    virtual operator bool() = 0;
};

class NativeNumber {
public:
    static const int INTERFACE_ID = 1;

    virtual operator int32_t() = 0;
    virtual operator uint32_t() = 0;
    virtual operator int64_t() = 0;
    virtual operator double() = 0;
};

class NativeString {
public:
    static const int INTERFACE_ID = 2;

    virtual void GetCString(char* buffer, size_t size, size_t* length) = 0;
    virtual size_t GetLength() = 0;
    virtual size_t EncodeWriteUtf8(char* buffer, size_t bufferSize, int32_t* nchars) = 0;
};

class NativeObject {
public:
    static const int INTERFACE_ID = 3;

    virtual void SetNativePointer(void* pointer, NativeFinalize cb, void* hint) = 0;
    virtual void* GetNativePointer() = 0;

    virtual NativeValue* GetPropertyNames() = 0;

    virtual NativeValue* GetPrototype() = 0;

    virtual bool DefineProperty(NativePropertyDescriptor propertyDescriptor) = 0;

    virtual bool SetProperty(NativeValue* key, NativeValue* value) = 0;
    virtual NativeValue* GetProperty(NativeValue* key) = 0;
    virtual bool HasProperty(NativeValue* key) = 0;
    virtual bool DeleteProperty(NativeValue* key) = 0;

    virtual bool SetProperty(const char* name, NativeValue* value) = 0;
    virtual NativeValue* GetProperty(const char* name) = 0;
    virtual bool HasProperty(const char* name) = 0;
    virtual bool DeleteProperty(const char* name) = 0;

    virtual bool SetPrivateProperty(const char* name, NativeValue* value) = 0;
    virtual NativeValue* GetPrivateProperty(const char* name) = 0;
    virtual bool HasPrivateProperty(const char* name) = 0;
    virtual bool DeletePrivateProperty(const char* name) = 0;
};

class NativeArray {
public:
    static const int INTERFACE_ID = 4;

    virtual bool SetElement(uint32_t index, NativeValue* value) = 0;
    virtual NativeValue* GetElement(uint32_t index) = 0;
    virtual bool HasElement(uint32_t index) = 0;
    virtual bool DeleteElement(uint32_t index) = 0;

    virtual uint32_t GetLength() = 0;
};

class NativeArrayBuffer {
public:
    static const int INTERFACE_ID = 5;

    virtual void* GetBuffer() = 0;
    virtual size_t GetLength() = 0;
};

enum NativeTypedArrayType {
    NATIVE_INT8_ARRAY,
    NATIVE_UINT8_ARRAY,
    NATIVE_UINT8_CLAMPED_ARRAY,
    NATIVE_INT16_ARRAY,
    NATIVE_UINT16_ARRAY,
    NATIVE_INT32_ARRAY,
    NATIVE_UINT32_ARRAY,
    NATIVE_FLOAT32_ARRAY,
    NATIVE_FLOAT64_ARRAY,
    NATIVE_BIGINT64_ARRAY,
    NATIVE_BIGUINT64_ARRAY,
};

class NativeTypedArray {
public:
    static const int INTERFACE_ID = 6;

    virtual NativeTypedArrayType GetTypedArrayType() = 0;
    virtual size_t GetLength() = 0;
    virtual NativeValue* GetArrayBuffer() = 0;
    virtual void* GetData() = 0;
    virtual size_t GetOffset() = 0;
};

class NativeDataView {
public:
    static const int INTERFACE_ID = 7;

    virtual void* GetBuffer() = 0;
    virtual size_t GetLength() = 0;
    virtual NativeValue* GetArrayBuffer() = 0;
    virtual size_t GetOffset() = 0;
};

class NativeFunction {
public:
    static const int INTERFACE_ID = 8;
};

class NativeBigint {
public:
    static const int INTERFACE_ID = 9;

    virtual operator int64_t() = 0;
    virtual operator uint64_t() = 0;
};

class NativeDate {
public:
    static const int INTERFACE_ID = 10;

    virtual double GetTime() = 0;
};

class NativeExternal {
public:
    static const int INTERFACE_ID = 11;

    virtual operator void*() = 0;
};

enum NativeErrorType {
    NATIVE_COMMON_ERROR,
    NATIVE_TYPE_ERROR,
    NATIVE_RANGE_ERROR,
};

#endif /* FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_VALUE_H */

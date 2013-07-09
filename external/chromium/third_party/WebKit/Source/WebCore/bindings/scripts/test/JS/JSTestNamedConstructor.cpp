/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestNamedConstructor.h"

#include "ExceptionCode.h"
#include "JSDOMBinding.h"
#include "TestNamedConstructor.h"
#include <runtime/Error.h>
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

/* Hash table for constructor */

static const HashTableValue JSTestNamedConstructorTableValues[] =
{
    { "constructor", DontEnum | ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNamedConstructorConstructor), (intptr_t)0, NoIntrinsic },
    { 0, 0, 0, 0, NoIntrinsic }
};

static const HashTable JSTestNamedConstructorTable = { 2, 1, JSTestNamedConstructorTableValues, 0 };
/* Hash table for constructor */

static const HashTableValue JSTestNamedConstructorConstructorTableValues[] =
{
    { 0, 0, 0, 0, NoIntrinsic }
};

static const HashTable JSTestNamedConstructorConstructorTable = { 1, 0, JSTestNamedConstructorConstructorTableValues, 0 };
const ClassInfo JSTestNamedConstructorConstructor::s_info = { "TestNamedConstructorConstructor", &Base::s_info, &JSTestNamedConstructorConstructorTable, 0, CREATE_METHOD_TABLE(JSTestNamedConstructorConstructor) };

JSTestNamedConstructorConstructor::JSTestNamedConstructorConstructor(Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorObject(structure, globalObject)
{
}

void JSTestNamedConstructorConstructor::finishCreation(ExecState* exec, JSDOMGlobalObject* globalObject)
{
    Base::finishCreation(exec->globalData());
    ASSERT(inherits(&s_info));
    putDirect(exec->globalData(), exec->propertyNames().prototype, JSTestNamedConstructorPrototype::self(exec, globalObject), DontDelete | ReadOnly);
}

bool JSTestNamedConstructorConstructor::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSTestNamedConstructorConstructor, JSDOMWrapper>(exec, &JSTestNamedConstructorConstructorTable, jsCast<JSTestNamedConstructorConstructor*>(cell), propertyName, slot);
}

bool JSTestNamedConstructorConstructor::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<JSTestNamedConstructorConstructor, JSDOMWrapper>(exec, &JSTestNamedConstructorConstructorTable, jsCast<JSTestNamedConstructorConstructor*>(object), propertyName, descriptor);
}

const ClassInfo JSTestNamedConstructorNamedConstructor::s_info = { "AudioConstructor", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSTestNamedConstructorNamedConstructor) };

JSTestNamedConstructorNamedConstructor::JSTestNamedConstructorNamedConstructor(Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorWithDocument(structure, globalObject)
{
}

void JSTestNamedConstructorNamedConstructor::finishCreation(ExecState* exec, JSDOMGlobalObject* globalObject)
{
    Base::finishCreation(globalObject);
    ASSERT(inherits(&s_info));
    putDirect(exec->globalData(), exec->propertyNames().prototype, JSTestNamedConstructorPrototype::self(exec, globalObject), None);
}

EncodedJSValue JSC_HOST_CALL JSTestNamedConstructorNamedConstructor::constructJSTestNamedConstructor(ExecState* exec)
{
    JSTestNamedConstructorNamedConstructor* castedThis = jsCast<JSTestNamedConstructorNamedConstructor*>(exec->callee());
    if (exec->argumentCount() < 1)
        return throwVMError(exec, createNotEnoughArgumentsError(exec));
    ExceptionCode ec = 0;
    const String& str1(MAYBE_MISSING_PARAMETER(exec, 0, DefaultIsUndefined).isEmpty() ? String() : MAYBE_MISSING_PARAMETER(exec, 0, DefaultIsUndefined).toString(exec)->value(exec));
    if (exec->hadException())
        return JSValue::encode(jsUndefined());
    const String& str2(MAYBE_MISSING_PARAMETER(exec, 1, DefaultIsUndefined).isEmpty() ? String() : MAYBE_MISSING_PARAMETER(exec, 1, DefaultIsUndefined).toString(exec)->value(exec));
    if (exec->hadException())
        return JSValue::encode(jsUndefined());
    const String& str3(MAYBE_MISSING_PARAMETER(exec, 2, DefaultIsNullString).isEmpty() ? String() : MAYBE_MISSING_PARAMETER(exec, 2, DefaultIsNullString).toString(exec)->value(exec));
    if (exec->hadException())
        return JSValue::encode(jsUndefined());
    RefPtr<TestNamedConstructor> object = TestNamedConstructor::createForJSConstructor(castedThis->document(), str1, str2, str3, ec);
    if (ec) {
        setDOMException(exec, ec);
        return JSValue::encode(JSValue());
    }
    return JSValue::encode(asObject(toJS(exec, castedThis->globalObject(), object.get())));
}

ConstructType JSTestNamedConstructorNamedConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructJSTestNamedConstructor;
    return ConstructTypeHost;
}

/* Hash table for prototype */

static const HashTableValue JSTestNamedConstructorPrototypeTableValues[] =
{
    { 0, 0, 0, 0, NoIntrinsic }
};

static const HashTable JSTestNamedConstructorPrototypeTable = { 1, 0, JSTestNamedConstructorPrototypeTableValues, 0 };
const ClassInfo JSTestNamedConstructorPrototype::s_info = { "TestNamedConstructorPrototype", &Base::s_info, &JSTestNamedConstructorPrototypeTable, 0, CREATE_METHOD_TABLE(JSTestNamedConstructorPrototype) };

JSObject* JSTestNamedConstructorPrototype::self(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMPrototype<JSTestNamedConstructor>(exec, globalObject);
}

const ClassInfo JSTestNamedConstructor::s_info = { "TestNamedConstructor", &Base::s_info, &JSTestNamedConstructorTable, 0 , CREATE_METHOD_TABLE(JSTestNamedConstructor) };

JSTestNamedConstructor::JSTestNamedConstructor(Structure* structure, JSDOMGlobalObject* globalObject, PassRefPtr<TestNamedConstructor> impl)
    : JSDOMWrapper(structure, globalObject)
    , m_impl(impl.leakRef())
{
}

void JSTestNamedConstructor::finishCreation(JSGlobalData& globalData)
{
    Base::finishCreation(globalData);
    ASSERT(inherits(&s_info));
}

JSObject* JSTestNamedConstructor::createPrototype(ExecState* exec, JSGlobalObject* globalObject)
{
    return JSTestNamedConstructorPrototype::create(exec->globalData(), globalObject, JSTestNamedConstructorPrototype::createStructure(globalObject->globalData(), globalObject, globalObject->objectPrototype()));
}

void JSTestNamedConstructor::destroy(JSC::JSCell* cell)
{
    JSTestNamedConstructor* thisObject = static_cast<JSTestNamedConstructor*>(cell);
    thisObject->JSTestNamedConstructor::~JSTestNamedConstructor();
}

JSTestNamedConstructor::~JSTestNamedConstructor()
{
    releaseImplIfNotNull();
}

bool JSTestNamedConstructor::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSTestNamedConstructor* thisObject = jsCast<JSTestNamedConstructor*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    return getStaticValueSlot<JSTestNamedConstructor, Base>(exec, &JSTestNamedConstructorTable, thisObject, propertyName, slot);
}

bool JSTestNamedConstructor::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    JSTestNamedConstructor* thisObject = jsCast<JSTestNamedConstructor*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    return getStaticValueDescriptor<JSTestNamedConstructor, Base>(exec, &JSTestNamedConstructorTable, thisObject, propertyName, descriptor);
}

JSValue jsTestNamedConstructorConstructor(ExecState* exec, JSValue slotBase, PropertyName)
{
    JSTestNamedConstructor* domObject = jsCast<JSTestNamedConstructor*>(asObject(slotBase));
    return JSTestNamedConstructor::getConstructor(exec, domObject->globalObject());
}

JSValue JSTestNamedConstructor::getConstructor(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestNamedConstructorConstructor>(exec, jsCast<JSDOMGlobalObject*>(globalObject));
}

static inline bool isObservable(JSTestNamedConstructor* jsTestNamedConstructor)
{
    if (jsTestNamedConstructor->hasCustomProperties())
        return true;
    return false;
}

bool JSTestNamedConstructorOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    JSTestNamedConstructor* jsTestNamedConstructor = jsCast<JSTestNamedConstructor*>(handle.get().asCell());
    if (jsTestNamedConstructor->impl()->hasPendingActivity())
        return true;
    if (!isObservable(jsTestNamedConstructor))
        return false;
    UNUSED_PARAM(visitor);
    return false;
}

void JSTestNamedConstructorOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    JSTestNamedConstructor* jsTestNamedConstructor = jsCast<JSTestNamedConstructor*>(handle.get().asCell());
    DOMWrapperWorld* world = static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, jsTestNamedConstructor->impl(), jsTestNamedConstructor);
    jsTestNamedConstructor->releaseImpl();
}

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, TestNamedConstructor* impl)
{
    return wrap<JSTestNamedConstructor>(exec, globalObject, impl);
}

TestNamedConstructor* toTestNamedConstructor(JSC::JSValue value)
{
    return value.inherits(&JSTestNamedConstructor::s_info) ? jsCast<JSTestNamedConstructor*>(asObject(value))->impl() : 0;
}

}
// vcpkg builds jsoncpp as a DLL (jsoncpp.dll) but the installed headers do not
// carry the dllimport/export macros (JSONCPP_API), so consumers reference the
// static data member Json::Value::null directly. The DLL import lib only
// exposes the __imp_ thunk, leaving the plain symbol undefined at link time.
//
// The project originally linked jsoncpp as if it were static (it links the
// bare name "jsoncpp_static"), so provide a local definition here that
// satisfies those direct references.
#include <json/value.h>

#if JSON_USE_NULLREF
namespace Json {
namespace {
const Value& nullValueInstance()
{
    static const Value v(nullValue);
    return v;
}
}  // namespace

const Value& Value::null = nullValueInstance();
const Value& Value::nullRef = nullValueInstance();
}  // namespace Json
#endif

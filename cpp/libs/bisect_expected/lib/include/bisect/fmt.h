/** Wrapper around the fmt headers to deal with an issue caused by libs like CppRestSDK.
 *  CppRestSDK defines a macro "U" and that pollutes everything that uses it directly or indirectly.
 */

#if defined(U)
#undef U
#endif // defined(U)

#include <fmt/format.h>

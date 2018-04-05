#ifndef LYAML_H
#define LYAML_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

struct luaL_serializer;

LUALIB_API int
luaopen_yaml(lua_State *L);

/** @Sa luaL_newserializer(). */
struct luaL_serializer *
lua_yaml_new_serializer(lua_State *L);

/**
 * Encode a Lua object into YAML document onto Lua stack.
 * @param L Lua stack to get an argument and push result.
 * @param serializer Lua YAML serializer.
 * @param tag_handle NULL, or a global tag handle. Handle becomes
 *        a synonym for prefix.
 * @param tag_prefix NULL, or a global tag prefix, to which @a
 *        handle is expanded.
 * @retval nil, error Error.
 * @retval not nil Lua string with dumped object.
 */
int
lua_yaml_encode_tagged(lua_State *L, struct luaL_serializer *serializer,
		       const char *tag_handle, const char *tag_prefix);

/**
 * Same as lua_yaml_encode_tagged, but encode with no tags and
 * with multiple arguments.
 */
int
lua_yaml_encode(lua_State *L, struct luaL_serializer *serializer);

#ifdef __cplusplus
}
#endif
#endif /* #ifndef LYAML_H */

-- fiber.lua (internal file)

local fiber = require('fiber')
local ffi = require('ffi')
ffi.cdef[[
double
fiber_time(void);
uint64_t
fiber_time64(void);
double
fiber_clock(void);
uint64_t
fiber_clock64(void);
struct ev_loop *
fiber_currentloop(void);
]]
local C = ffi.C

local function fiber_time()
    return tonumber(C.fiber_time())
end

local function fiber_time64()
    return C.fiber_time64()
end

local function fiber_clock()
    return tonumber(C.fiber_clock())
end

local function fiber_clock64()
    return C.fiber_clock64()
end

local function fiber_currentloop()
    return C.fiber_currentloop()
end

fiber.time = fiber_time
fiber.time64 = fiber_time64
fiber.clock = fiber_clock
fiber.clock64 = fiber_clock64
fiber.currentloop= fiber_currentloop
return fiber

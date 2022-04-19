#ifndef BINDING_H
#define BINDING_H

#ifdef __cplusplus
#	include "nvmath/nvmath.h"
#	include <cstdint>
using vec2 = nvmath::vec2f;
using vec3 = nvmath::vec3f;
using vec4 = nvmath::vec4f;
using mat4 = nvmath::mat4f;
using uint = unsigned int;
#endif

#ifdef __cplusplus        // Descriptor binding helper for C++ and GLSL
#	define START_BINDING(a) \
		enum a               \
		{
#	define END_BINDING() }
#else
#	define START_BINDING(a) const uint
#	define END_BINDING()
#endif

#ifdef __cplusplus        // glsl reserve enum keyword
#	define START_ENUM(a) \
		enum a            \
		{
#	define END_ENUM() }
#else
#	define START_ENUM(a) const uint
#	define END_ENUM()
#endif

START_BINDING(GPUBindingGraphics)
eGPUBindingGraphicsCamera        = 0,        // Global uniform containing camera matrices
    eGPUBindingGraphicsSceneDesc = 1 END_BINDING();

#define eGPUBindingRaytraceChannelN 4
START_BINDING(GPUBindingRaytrace)
eGPUBindingRaytraceChannel0     = 0,        // Ray tracer channel0
    eGPUBindingRaytraceChannel1 = 1,        // Ray tracer channel1
    eGPUBindingRaytraceChannel2 = 2,        // Ray tracer channel2
    eGPUBindingRaytraceChannel3 = 3,        // Ray tracer channel3
    eGPUBindingRaytraceTlas     = 4         // Top level acceleration structure,
    END_BINDING();

START_BINDING(GPUBindingPost)
eGPUBindingPostImage = 0 END_BINDING();

#endif
/**
 *  @file   JoltSafe.hpp
 *  @brief  This file exist only because of coliding macros.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <yoga/Yoga.h>
#include <SDL3/SDL.h>

#if defined(None)
    #undef None
#endif
#if defined(Status)
    #undef Status
#endif
#if defined(Success)
    #undef Success
#endif
#if defined(Bool)
    #undef Bool
#endif
#if defined(True)
    #undef True
#endif
#if defined(False)
    #undef False
#endif
#if defined(Shape)
    #undef Shape
#endif
#if defined(ConvexShape)
    #undef ConvexShape
#endif
#if defined(Convex)
    #undef Convex
#endif
#if defined(State)
    #undef State
#endif
#if defined(Never)
    #undef Never
#endif
#if defined(Always)
    #undef Always
#endif
#if defined(AlwaysInline)
    #undef AlwaysInline
#endif
#if defined(CurrentTime)
    #undef CurrentTime
#endif
#if defined(KeyPress)
    #undef KeyPress
#endif
#if defined(KeyRelease)
    #undef KeyRelease
#endif
#if defined(Count)
    #undef Count
#endif
#if defined(Round)
    #undef Round
#endif
#if defined(Complex)
    #undef Complex
#endif
#if defined(Min)
    #undef Min
#endif
#if defined(Max)
    #undef Max
#endif



#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>

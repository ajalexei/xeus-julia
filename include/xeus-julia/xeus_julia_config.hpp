/***************************************************************************
* Copyright (c) 2022, Alexei Andreanov                                  
*                                                                          
* Distributed under the terms of the GNU General Public License v3.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#ifndef XEUS_JULIA_CONFIG_HPP
#define XEUS_JULIA_CONFIG_HPP

// Project version
#define XEUS_JULIA_VERSION_MAJOR 0
#define XEUS_JULIA_VERSION_MINOR 1
#define XEUS_JULIA_VERSION_PATCH 0

// Composing the version string from major, minor and patch
#define XEUS_JULIA_CONCATENATE(A, B) XEUS_JULIA_CONCATENATE_IMPL(A, B)
#define XEUS_JULIA_CONCATENATE_IMPL(A, B) A##B
#define XEUS_JULIA_STRINGIFY(a) XEUS_JULIA_STRINGIFY_IMPL(a)
#define XEUS_JULIA_STRINGIFY_IMPL(a) #a

#define XEUS_JULIA_VERSION XEUS_JULIA_STRINGIFY(XEUS_JULIA_CONCATENATE(XEUS_JULIA_VERSION_MAJOR,   \
                 XEUS_JULIA_CONCATENATE(.,XEUS_JULIA_CONCATENATE(XEUS_JULIA_VERSION_MINOR,   \
                                  XEUS_JULIA_CONCATENATE(.,XEUS_JULIA_VERSION_PATCH)))))

#ifdef _WIN32
    #ifdef XEUS_JULIA_EXPORTS
        #define XEUS_JULIA_API __declspec(dllexport)
    #else
        #define XEUS_JULIA_API __declspec(dllimport)
    #endif
#else
    #define XEUS_JULIA_API
#endif

#endif
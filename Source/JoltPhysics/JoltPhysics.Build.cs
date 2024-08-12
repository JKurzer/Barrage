// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class JoltPhysics : ModuleRules
{
	public JoltPhysics(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External; // tells the engine not to look for (or compile) source code. It will use the other settings you define in that file by adding the listed include paths to the compile environment, setting the appropriate macros, and linking against the given static libraries.

        var cmakeOptions = "";

        cmakeOptions += " -DCMAKE_CONFIGURATION_TYPES=Debug;Release;Distribution "; 
        if (Target.Configuration == UnrealTargetConfiguration.Debug) // untested
        {
            cmakeOptions += " -DCMAKE_BUILD_TYPE=Debug ";
            cmakeOptions += " -DUSE_ASSERTS=ON ";
            cmakeOptions += " -DGENERATE_DEBUG_SYMBOLS=ON ";
            cmakeOptions += " -DCMAKE_CXX_FLAGS_RELEASE=\" /GS /Od /Ob0 /RTC1\" "; // probably may be omitted
            cmakeOptions += " -DDEBUG_RENDERER=ON ";
            cmakeOptions += " -DPROFILE_ENABLED=ON ";
            cmakeOptions += " -DENABLE_ASSERTS=ON ";
        }
        else if (Target.Configuration == UnrealTargetConfiguration.DebugGame || Target.Configuration == UnrealTargetConfiguration.Development)
        {
            cmakeOptions += " -DCMAKE_BUILD_TYPE=Release ";
            cmakeOptions += " -DUSE_ASSERTS=ON ";
            cmakeOptions += " -DGENERATE_DEBUG_SYMBOLS=ON ";
            cmakeOptions += " -DCMAKE_CXX_FLAGS_RELEASE=\" /GS /Od /Ob0 /RTC1\" "; // probably may be omitted
            cmakeOptions += " -DDEBUG_RENDERER=ON ";
            cmakeOptions += " -DPROFILE_ENABLED=ON ";
            cmakeOptions += " -DENABLE_ASSERTS=ON ";
        }
        else {
            cmakeOptions += " -DCMAKE_BUILD_TYPE=Distribution ";
            cmakeOptions += " -DDEBUG_RENDERER=OFF ";
            cmakeOptions += " -DPROFILE_ENABLED=OFF ";
            cmakeOptions += " -DENABLE_ASSERTS=OFF ";
        }

        cmakeOptions += " -DFLOATING_POINT_EXCEPTIONS_ENABLED=OFF ";
        cmakeOptions += " -DCROSS_PLATFORM_DETERMINISTIC=ON ";
        cmakeOptions += " -DOBJECT_STREAM=ON ";
        cmakeOptions += " -DDOUBLE_PRECISION=OFF ";
        cmakeOptions += " -DOBJECT_LAYER_BITS=16 ";
        cmakeOptions += " -DINTERPROCEDURAL_OPTIMIZATION=ON ";
        cmakeOptions += " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ";

        cmakeOptions += " -DTARGET_SAMPLES=OFF ";
        cmakeOptions += " -DTARGET_HELLO_WORLD=OFF ";
        cmakeOptions += " -DTARGET_VIEWER=OFF ";
        cmakeOptions += " -DTARGET_UNIT_TESTS=OFF ";
        cmakeOptions += " -DTARGET_PERFORMANCE_TEST=OFF ";
        
        // those are the defines what jolt checks, they need to be identical between this build and your module what includes it
        /*
		check_bit(1, "JPH_DOUBLE_PRECISION");
		check_bit(2, "JPH_CROSS_PLATFORM_DETERMINISTIC");
		check_bit(3, "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED");
		check_bit(4, "JPH_PROFILE_ENABLED");
		check_bit(5, "JPH_EXTERNAL_PROFILE");
		check_bit(6, "JPH_DEBUG_RENDERER");
		check_bit(7, "JPH_DISABLE_TEMP_ALLOCATOR");
		check_bit(8, "JPH_DISABLE_CUSTOM_ALLOCATOR");
		check_bit(9, "JPH_OBJECT_LAYER_BITS");
		check_bit(10, "JPH_ENABLE_ASSERTS");
		check_bit(11, "JPH_OBJECT_STREAM");        
        */

        cmakeOptions += " -DENABLE_ALL_WARNINGS=OFF ";
        cmakeOptions += " -DUSE_STATIC_MSVC_RUNTIME_LIBRARY=OFF ";


        var simdStatus = "ON ";
        cmakeOptions += " -DUSE_AVX512=OFF ";
        cmakeOptions += " -DUSE_AVX2=OFF ";
        cmakeOptions += " -DUSE_AVX=OFF ";
        cmakeOptions += " -DUSE_FMADD=OFF ";
        cmakeOptions += " -DUSE_SSE4_2=" + simdStatus;
        cmakeOptions += " -DUSE_SSE4_1=" + simdStatus;
        cmakeOptions += " -DUSE_LZCNT=" + simdStatus;
        cmakeOptions += " -DUSE_TZCNT=" + simdStatus;
        cmakeOptions += " -DUSE_F16C=" + simdStatus;
        
        //IF YOU USE THE 14.38.33130 zip file, you will need to change these paths AND adjust the VS2022 generator's behavior.
        //cmakeOptions += " -DCMAKE_CXX_COMPILER=\"C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/bin/Hostx64/x64/cl.exe\" ";
        //cmakeOptions += " -DCMAKE_CXX_LINK_EXECUTABLE=\"C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/bin/Hostx64/x64/link.exe\" ";

        // use the UE4CMake plugin to generate project files https://github.com/caseymcc/UE4CMake read the instructions how to use it, it's straight forward.
        CMakeTarget.add(Target, this, "Jolt", Path.Combine(this.ModuleDirectory, "Build"), cmakeOptions, false);

    }
}

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO KNSoft/KNSoft.SlimDetours
    REF 439853a220c217799428944f197309d5cb3e8d17
    SHA512 ebc2f266bb509562fe72df749aeab4375a726594cfba6500e4bcf7b4be1db22745c7baac423cc2aeec60d7a45d61d5dd694c8225ce056317a7d8815bd942afa7
)

set(SLIMDETOURS_VCXPROJ "${SOURCE_PATH}/Source/KNSoft.SlimDetours/KNSoft.SlimDetours.vcxproj")
file(READ "${SLIMDETOURS_VCXPROJ}" SLIMDETOURS_VCXPROJ_CONTENTS)
string(REPLACE "\r\n" "\n" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
string(REPLACE "  <ImportGroup Label=\"ExtensionTargets\">\n    <Import Project=\"..\\packages\\KNSoft.NDK.1.2.48-beta\\build\\KNSoft.NDK.targets\" Condition=\"Exists('..\\packages\\KNSoft.NDK.1.2.48-beta\\build\\KNSoft.NDK.targets')\" />\n  </ImportGroup>\n" "" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
string(REPLACE "  <Target Name=\"EnsureNuGetPackageBuildImports\" BeforeTargets=\"PrepareForBuild\">\n    <PropertyGroup>\n      <ErrorText>This project references NuGet package(s) that are missing on this computer. Use NuGet Package Restore to download them.  For more information, see http://go.microsoft.com/fwlink/?LinkID=322105. The missing file is {0}.</ErrorText>\n    </PropertyGroup>\n    <Error Condition=\"!Exists('..\\packages\\KNSoft.NDK.1.2.48-beta\\build\\KNSoft.NDK.targets')\" Text=\"$([System.String]::Format('$(ErrorText)', '..\\packages\\KNSoft.NDK.1.2.48-beta\\build\\KNSoft.NDK.targets'))\" />\n  </Target>\n" "" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
string(REPLACE "</Project>" "  <ItemDefinitionGroup Condition=\"'$(Configuration)'=='Release'\">\n    <ClCompile>\n      <DebugInformationFormat>OldStyle</DebugInformationFormat>\n    </ClCompile>\n  </ItemDefinitionGroup>\n</Project>" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
string(REPLACE "</Project>" "  <ItemDefinitionGroup>\n    <ClCompile>\n      <AdditionalIncludeDirectories>$(VcpkgIncludePath);%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n    </ClCompile>\n  </ItemDefinitionGroup>\n</Project>" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
file(WRITE "${SLIMDETOURS_VCXPROJ}" "${SLIMDETOURS_VCXPROJ_CONTENTS}")

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(MSBUILD_PLATFORM "Win32")
    set(KNSOFT_ARCH "x86")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(MSBUILD_PLATFORM "x64")
    set(KNSOFT_ARCH "x64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(MSBUILD_PLATFORM "ARM64")
    set(KNSOFT_ARCH "ARM64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64ec")
    set(MSBUILD_PLATFORM "ARM64EC")
    set(KNSOFT_ARCH "ARM64EC")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

set(SLIMDETOURS_MSBUILD_OPTIONS "/p:VcpkgIncludePath=${CURRENT_INSTALLED_DIR}\\include")

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    vcpkg_build_msbuild(
        PROJECT_PATH "${SOURCE_PATH}/Source/KNSoft.SlimDetours/KNSoft.SlimDetours.vcxproj"
        PLATFORM "${MSBUILD_PLATFORM}"
        OPTIONS ${SLIMDETOURS_MSBUILD_OPTIONS} "/p:Configuration=Release"
    )

    file(INSTALL "${SOURCE_PATH}/Source/KNSoft.SlimDetours/OutDir/${KNSOFT_ARCH}/Release/KNSoft.SlimDetours.lib"
        DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
endif()

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    vcpkg_build_msbuild(
        PROJECT_PATH "${SOURCE_PATH}/Source/KNSoft.SlimDetours/KNSoft.SlimDetours.vcxproj"
        PLATFORM "${MSBUILD_PLATFORM}"
        OPTIONS ${SLIMDETOURS_MSBUILD_OPTIONS} "/p:Configuration=Debug"
    )

    file(INSTALL "${SOURCE_PATH}/Source/KNSoft.SlimDetours/OutDir/${KNSOFT_ARCH}/Debug/KNSoft.SlimDetours.lib"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
endif()

file(INSTALL "${SOURCE_PATH}/Source/KNSoft.SlimDetours/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include/KNSoft/SlimDetours"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/include/KNSoft/SlimDetours/IntDir"
    "${CURRENT_PACKAGES_DIR}/include/KNSoft/SlimDetours/OutDir")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/knsoft-slimdetours" RENAME copyright)

#pragma once

#ifndef RESULT_DIAGNOSTICS_LEVEL
#if (defined(RESULT_DEBUG) || defined(RESULT_DEBUG_INFO)) && !defined(RESULT_SUPPRESS_DEBUG_INFO)
#define RESULT_DIAGNOSTICS_LEVEL 5
#else
#define RESULT_DIAGNOSTICS_LEVEL 0
#endif
#endif
#include "wil/wrl.h"
#include "wil/common.h"
#include "wil/cppwinrt.h"
#include "wil/filesystem.h"
#include "wil/win32_helpers.h"
#include "wil/result.h"
#include "wil/registry.h"
#include "wil/resource.h"
#include "wil/stl.h"
#include "wil/safecast.h"
#include "wil/com.h"
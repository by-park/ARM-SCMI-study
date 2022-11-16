/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      ARM-M exception handlers.
 */

#include <cmsis_compiler.h>

#include <fwk_attributes.h>
#include <fwk_noreturn.h>

#include <stdbool.h>

/* 가장 처음으로 arch_exception_reset 이 실행되면 그 내부의 __main 으로 이어진다 */
noreturn void arch_exception_reset(void)
{
    /*
     * When entering the firmware, before the framework is entered the following
     * things happen:
     *  1. The toolchain-specific C runtime is initialized
     *     For Arm Compiler:
     *       1. Zero-initialized data is zeroed
     *       2. Initialized data is decompressed and copied
     *     For GCC/Newlib:
     *       1. Zero-initialized data is zeroed
     *       2. Initialized data is copied by software_init_hook()
     * 2. The main() function is called by the C runtime
     */

#ifdef __ARMCC_VERSION
    extern noreturn void __main(void);

    __main();
#else
    extern noreturn void _start(void);

    _start();
#endif
}

noreturn FWK_WEAK void arch_exception_invalid(void)
{
    while (true) {
        __WFI();
    }
}

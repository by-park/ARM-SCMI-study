# ARM SCMI 코드 스터디

### 1. SCMI Overview

> This document describes the System Control and Management Interface (SCMI), which is a set of operating system-independent software interfaces that are used in system management.

> 이 문서는 운영체제에 의존하지 않고 시스템 관리에 사용할 수 있는 소프트웨어 인터페이스들인 SCMI (System Control and Management Interface) 에 대해 설명한다.

> There is a strong trend in the industry to provide microcontrollers in systems to abstract various power, or other system management tasks, away from APs. These controllers usually have similar interfaces, both in terms of the functions that are provided by them, and in terms of how requests are communicated to them. The Power Control System Architecture (PCSA) describes how systems using this approach can be built.

> 업계에서는 전원 관리나 시스템 관리들을 AP (Application Processor) 로부터 분리해서 마이크로컨트롤러가 하도록 하는 것이 트렌드이다. 이러한 컨트롤러들은 제공하는 함수들이나 주고받는 요청들에 대해 비슷한 인터페이스를 가지고 있다. PCSA (Power Control System Architecture) 는 이런 접근 방법을 사용해서 어떻게 시스템을 구현하는지를 보여준다. \* PCSA 는 ARM 의 비공개 문서이다.

> PCSA defines the concept of the System Control Processor (SCP), a processor that is used to abstract power and system management tasks from the APs. The SCP can take requests from APs and other system agents. It can coordinate these requests and place components in the platform into appropriate power and performance states.

> PCSA 는 AP 로부터 전원 관리와 시스템 관리를 대신하는 SCP (System Control Processor) 의 컨셉을 정의한다. SCP 는 AP 나 다른 시스템들로부터 오는 요청을 받을 수 있다. SCP 는 이러한 요청들을 조정하고 플랫폼의 요소들을 적절한 전원 상태 및 동작 상태로 만든다.

출처: [SCMI 공식 문서](https://developer.arm.com/documentation/den0056/d/?lang=en)

![SCMI Overview](overview.PNG)

SCMI Agent 들이 그림 하단의 SCP Firmware 에 SCMI 프로토콜을 통해 원하는 작업을 요청하는 예시

출처: [SCMI 공식 문서](https://developer.arm.com/documentation/den0056/d/?lang=en)



### 2. SCP Firmware Overview

module/ framework/ architecture



SCP Firmware Initialization

코드 컴파일시 exceptions 가 가장 앞부분에 위치

arch/arm/arm-m/src/arch.scatter.S

```assembly
LR_FIRMWARE FMW_MEM0_BASE {
    ER_EXCEPTIONS ARCH_X_BASE {
        *(.exceptions)
    }

    ER_TEXT +0 {
        *(+CODE)
    }

    ER_RODATA ARCH_R_BASE {
        *(+CONST)
    }

    ER_DATA ARCH_W_BASE {
        *(+DATA)
    }

    ER_BSS +0 {
        *(+BSS)
    }

    ARM_LIB_STACKHEAP +0 EMPTY (ARCH_W_LIMIT - +0) { }
}
```

exceptions 에서 제일 처음 실행되는 코드는 arch_exception_reset

arch/arm/arm-m/src/arch_exceptions.c

```c
const struct {
    uintptr_t stack;
    uintptr_t exceptions[NVIC_USER_IRQ_OFFSET - 1];
} arch_exceptions FWK_SECTION(".exceptions") = {
    .stack = (uintptr_t)(arch_exception_stack),
    .exceptions = {
        [NVIC_USER_IRQ_OFFSET + Reset_IRQn - 1] =
            (uintptr_t)(arch_exception_reset),
        [NonMaskableInt_IRQn +  (NVIC_USER_IRQ_OFFSET - 1)] =
            (uintptr_t)(arch_exception_invalid),
        [NVIC_USER_IRQ_OFFSET + HardFault_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
        [NVIC_USER_IRQ_OFFSET + MemoryManagement_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
        [NVIC_USER_IRQ_OFFSET + BusFault_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
        [NVIC_USER_IRQ_OFFSET + UsageFault_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
#ifdef ARMV8M
        [NVIC_USER_IRQ_OFFSET + SecureFault_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
#endif
        [NVIC_USER_IRQ_OFFSET + DebugMonitor_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),

        [NVIC_USER_IRQ_OFFSET + SVCall_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
        [NVIC_USER_IRQ_OFFSET + PendSV_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
        [NVIC_USER_IRQ_OFFSET + SysTick_IRQn - 1] =
            (uintptr_t)(arch_exception_invalid),
    },
};
```

arch_exception_reset 은 \_\_main 을 호출하게 되고 컴파일러에 의해 최종적으로 main 이 연결된다.

arch/arm/arm-m/src/arch_handlers.c

```c
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
```

main 함수를 보면 CCR 레지스터를 설정해주고, framework architecture 초기화 함수를 실행한다.

arch/arm/arm-m/src/arch_main.c

```c

static const struct fwk_arch_init_driver arch_init_driver = {
    .interrupt = arch_nvic_init,
};

static void arch_init_ccr(void)
{
    /*
     * Set up the Configuration Control Register (CCR) in the System Control
     * Block (1) by setting the following flag bits:
     *
     * DIV_0_TRP   [4]: Enable trapping on division by zero. (1)(2)
     * STKALIGN    [9]: Enable automatic DWORD stack-alignment on exception
     *                  entry (3).
     *
     * All other bits are left in their default state.
     *
     * (1) ARM® v7-M Architecture Reference Manual, section B3.2.8.
     * (2) Arm® v8-M Architecture Reference Manual, section D1.2.9.
     * (3) ARM® v7-M Architecture Reference Manual, section B1.5.7.
     */

    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
#ifdef ARMV7M
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;
#endif
}

int main(void)
{
    arch_init_ccr();

    return fwk_arch_init(&arch_init_driver);
}
```



### 3. 코드

[Linux Kernel mainline 6.1-rc5](https://www.kernel.org/)

[SCP Firmware 2.11](https://github.com/ARM-software/SCP-firmware)



### 참고 문헌

[1] [SCMI 공식 문서](https://developer.arm.com/documentation/den0056/d/?lang=en)

[2] [ARM SCMI 활용을 위한 linux kernel device tree 작성법](https://www.kernel.org/doc/Documentation/devicetree/bindings/arm/arm%2Cscmi.txt)

[3] [SCP Firmware 101 ppt](https://static.linaro.org/connect/san19/presentations/san19-117.pdf) 또는 [대본](http://docplayer.net/167756101-Arm-system-control-processor-scp-firmware-101.html)

[4] [SCP Firmware 101 설명 영상](https://resources.linaro.org/en/resource/PZgSfTGcN3qrz86cbitkMn) 또는 [유튜브 영상](https://www.youtube.com/watch?v=hR5YwfoyVy4)
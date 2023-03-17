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

![SCMI overview diagram](https://community.arm.com/resized-image/__size/1040x0/__key/communityserver-blogs-components-weblogfiles/00-00-00-21-42/SCMI_5F00_Overview_5F00_Diagrams_5F00_Souvik_5F00_1_5F00_ST4.png)

출처: [ARM Community Blog](https://community.arm.com/arm-community-blogs/b/architectures-and-processors-blog/posts/improved-power-management-system-control-through-scmi)



### ARM PCSA

> ARM에서 권장하는 방법은 마이크로컨트롤러와 일부 주변 로직을 사용하여 독립 장치인 시스템 제어 프로세서(시스템 제어 프로세서, 이하 SCP라고 함)를 만드는 것입니다. ARM 계열이므로 이 마이크로컨트롤러는 M 또는 R 시리즈입니다. SoC의 전력 소모량 관리는 SCP에서 완료됩니다(물론 SCP는 이 작업만 완료하는 것은 아닙니다).

![scp](https://aijishu.com/img/bV2rF)

![2.png](https://aijishu.com/img/bV2rH)

https://aijishu.com/a/1060000000208655



### 2. SCP Firmware Overview

module/ framework/ architecture



#### SCP Firmware Initialization

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

arch_exception_reset 은 \_\_main 을 호출하게 되고 라이브러리 함수를 거쳐 최종적으로 main 이 연결된다. [참고](http://recipes.egloos.com/5044366)

![img](http://pds13.egloos.com/pds/200907/15/90/c0098890_4a5dbf701a589.gif)

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



WFE 에서 깨어나 event list 에서 처리할 event 를 꺼내온다.

WFI 와 WFE 의 차이: WFI 는 인터럽트가 발생하면 인터럽트 처리가 시작되지만, WFE 는 깨어나기만 하고 인터럽트를 처리하지는 않는다.

> [WFI 혹은 WFE 명령으로 저전력 모드를 들어갈 수 있는데, 이 둘은 어떤 차이가 있습니까?](https://www.st.com/ko/stm32/stm32/support.html#)
>
> A. WFI는 Wait For Interrupt (인터럽트 대기)의 약자이며, WFE는 Wait For Event (이벤트 대기)의 약자입니다. WFI의 경우 MCU가 저전력 모드에서 복귀 할 때 벡터 인터럽트 컨트롤러 (NVIC)에 의해 인터럽트를 인식하여 인터럽트 처리가 시작됩니다. WFE의 경우 저전력 모드에서에서 복귀 할 때 인터럽트 처리를하지 않고 저전력 모드에서 복귀만 하게됩니다. 따라서 저전력 모드에서 복귀 할 때 WFI를 사용할지 혹은 WFE을 사용할지 저전력 모드로 들어가기 전에 사용자가 컨트롤 해야 합니다.



#### shared memory

arch/arm64/boot/dts/arm/juno-base.dtsi (linux-6.1-rc5)

```c
        sram: sram@2e000000 {
                compatible = "arm,juno-sram-ns", "mmio-sram";
                reg = <0x0 0x2e000000 0x0 0x8000>;

                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0 0x0 0x2e000000 0x8000>;

                cpu_scp_lpri: scp-sram@0 {
                        compatible = "arm,juno-scp-shmem";
                        reg = <0x0 0x200>;
                };

                cpu_scp_hpri: scp-sram@200 {
                        compatible = "arm,juno-scp-shmem";
                        reg = <0x200 0x200>;
                };
        };
```

arch/arm64/boot/dts/arm/juno-scmi.dtsi (linux-6.1-rc5)

```c
        firmware {
                scmi {
                        compatible = "arm,scmi";
                        mbox-names = "tx", "rx";
                        mboxes = <&mailbox 0 0 &mailbox 0 1>;
                        shmem = <&cpu_scp_lpri0 &cpu_scp_lpri1>;
                        #address-cells = <1>;
                        #size-cells = <0>;

                        scmi_devpd: protocol@11 {
                                reg = <0x11>;
                                #power-domain-cells = <1>;
                        };

                        scmi_dvfs: protocol@13 {
                                reg = <0x13>;
                                #clock-cells = <1>;
                                mbox-names = "tx", "rx";
                                mboxes = <&mailbox 1 0 &mailbox 1 1>;
                                shmem = <&cpu_scp_hpri0 &cpu_scp_hpri1>;
                        };

                        scmi_clk: protocol@14 {
                                reg = <0x14>;
                                #clock-cells = <1>;
                        };

                        scmi_sensors0: protocol@15 {
                                reg = <0x15>;
                                #thermal-sensor-cells = <1>;
                        };
                };
        };
```



### MHU (Message Handling Unit)

ARM Juno 보드의 MHU 기준의 코드

Juno 보드 Manual 3.6 절의 MHU Registers 를 보면 SCP_INTR~ (SCP -> AP 방향)과  CPU_INTR~ (AP -> SCP 방향)이 있다.

각 내부는 L, H, S 로 나뉘는데, L은 Low priority Non-secure interrupt 이고, H 는 High priority Non-secure interrupt, S 는 Secure interrupt 이다.

Juno 의 mhu module 의 element 로 3개가 다 등록된 것을 볼 수 있다.

product/juno/scp_ramfw/config_mhu.c

```c
static const struct fwk_element element_table[] = {
    [JUNO_MHU_DEVICE_IDX_S] = {
        .name = "",
        .sub_element_count = 1,
        .data = &(struct mod_mhu_device_config) {
            .irq = (unsigned int) MHU_SECURE_IRQ,
            .in = MHU_CPU_INTR_S_BASE,
            .out = MHU_SCP_INTR_S_BASE,
        },
    },
    [JUNO_MHU_DEVICE_IDX_NS_H] = {
        .name = "",
        .sub_element_count = 2,
        .data = &(struct mod_mhu_device_config) {
            .irq = (unsigned int) MHU_HIGH_PRIO_IRQ,
            .in = MHU_CPU_INTR_H_BASE,
            .out = MHU_SCP_INTR_H_BASE,
        },
    },
    [JUNO_MHU_DEVICE_IDX_NS_L] = {
        .name = "",
        .sub_element_count = 1,
        .data = &(struct mod_mhu_device_config) {
            .irq = (unsigned int) MHU_LOW_PRIO_IRQ,
            .in = MHU_CPU_INTR_L_BASE,
            .out = MHU_SCP_INTR_L_BASE,
        },
    },
    [JUNO_MHU_DEVICE_IDX_COUNT] = { 0 },
};

```

mhu_isr 에서 들어온 인터럽트를 처리한다. 각 인터럽트의 in 을 (바로 위에 정의된 메모리 주소 영역) 접근해서 전달받은 메시지를 열어본다.

module/mhu/src/mod_mhu.c

```c
static void mhu_isr(void)
{
    int status;
    unsigned int interrupt;
    unsigned int device_idx;
    struct mhu_device_ctx *device_ctx;
    struct mhu_reg *reg;
    unsigned int slot;
    struct mhu_smt_channel *smt_channel;

    status = fwk_interrupt_get_current(&interrupt);
    if (status != FWK_SUCCESS) {
        return;
    }

    for (device_idx = 0; device_idx < mhu_ctx.device_count; device_idx++) {
        device_ctx = &mhu_ctx.device_ctx_table[device_idx];
        if (device_ctx->config->irq == interrupt) {
            break;
        }
    }

    if (device_idx >= mhu_ctx.device_count) {
        return;
    }

    reg = (struct mhu_reg *)device_ctx->config->in;

    /* Loop over all the slots */
    while (reg->STAT != 0) {
        slot = (unsigned int)__builtin_ctz(reg->STAT);

```

각각의 주소는 kernel 의 dt 에도 정의된 것을 볼 수 있다. (L, H, S 별 주소)

대부분은 non-secure 이며 low priority 이며, dvfs 만 non-secure 에 high priority 로 되어있다.

arch/arm64/boot/dts/arm/juno-scmi.dtsi

```c
        firmware {
                scmi {
                        compatible = "arm,scmi";
                        mbox-names = "tx", "rx";
                        mboxes = <&mailbox 0 0 &mailbox 0 1>;
                        shmem = <&cpu_scp_lpri0 &cpu_scp_lpri1>;
                        #address-cells = <1>;
                        #size-cells = <0>;

                        scmi_devpd: protocol@11 {
                                reg = <0x11>;
                                #power-domain-cells = <1>;
                        };

                        scmi_dvfs: protocol@13 {
                                reg = <0x13>;
                                #clock-cells = <1>;
                                mbox-names = "tx", "rx";
                                mboxes = <&mailbox 1 0 &mailbox 1 1>;
                                shmem = <&cpu_scp_hpri0 &cpu_scp_hpri1>;
                        };

                        scmi_clk: protocol@14 {
                                reg = <0x14>;
                                #clock-cells = <1>;
                        };

                        scmi_sensors0: protocol@15 {
                                reg = <0x15>;
                                #thermal-sensor-cells = <1>;
                        };
                };
        };
```

mhu 에서 smt api 를 호출하게 되는데, smt api 도 위의 3 종류 인터럽트가 다 정의되어있다.

product/juno/scp_ramfw/config_smt.c

```c
static const struct fwk_element element_table[] = {
    /* JUNO_SCMI_SERVICE_IDX_PSCI_A2P */
    { .name = "",
      .data =
          &(struct mod_smt_channel_config){
              .type = MOD_SMT_CHANNEL_TYPE_COMPLETER,
              .policies = (MOD_SMT_POLICY_INIT_MAILBOX | MOD_SMT_POLICY_SECURE),
              .mailbox_address = (uintptr_t)SCMI_PAYLOAD_S_A2P_BASE,
              .mailbox_size = SCMI_PAYLOAD_SIZE,
              .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                  FWK_MODULE_IDX_MHU,
                  JUNO_MHU_DEVICE_IDX_S,
                  0),
              .driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MHU, 0),
              .pd_source_id = FWK_ID_ELEMENT_INIT(
                  FWK_MODULE_IDX_POWER_DOMAIN,
                  POWER_DOMAIN_IDX_SYSTOP),
          } },
    /* JUNO_SCMI_SERVICE_IDX_OSPM_A2P_0 */
    { .name = "",
      .data =
          &(struct mod_smt_channel_config){
              .type = MOD_SMT_CHANNEL_TYPE_COMPLETER,
              .policies = MOD_SMT_POLICY_INIT_MAILBOX,
              .mailbox_address = (uintptr_t)SCMI_PAYLOAD_LOW_A2P_BASE,
              .mailbox_size = SCMI_PAYLOAD_SIZE,
              .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                  FWK_MODULE_IDX_MHU,
                  JUNO_MHU_DEVICE_IDX_NS_L,
                  0),
              .driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MHU, 0),
              .pd_source_id = FWK_ID_ELEMENT_INIT(
                  FWK_MODULE_IDX_POWER_DOMAIN,
                  POWER_DOMAIN_IDX_SYSTOP),
          } },
    /* JUNO_SCMI_SERVICE_IDX_OSPM_A2P_1 */
    { .name = "",
      .data =
          &(struct mod_smt_channel_config){
              .type = MOD_SMT_CHANNEL_TYPE_COMPLETER,
              .policies = MOD_SMT_POLICY_INIT_MAILBOX,
              .mailbox_address = (uintptr_t)SCMI_PAYLOAD_HIGH_A2P_BASE,
              .mailbox_size = SCMI_PAYLOAD_SIZE,
              .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                  FWK_MODULE_IDX_MHU,
                  JUNO_MHU_DEVICE_IDX_NS_H,
                  0),
              .driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MHU, 0),
              .pd_source_id = FWK_ID_ELEMENT_INIT(
                  FWK_MODULE_IDX_POWER_DOMAIN,
                  POWER_DOMAIN_IDX_SYSTOP),
          } },
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    /* JUNO_SCMI_SERVICE_IDX_OSPM_P2A */
    { .name = "",
      .data =
          &(struct mod_smt_channel_config){
              .type = MOD_SMT_CHANNEL_TYPE_REQUESTER,
              .policies = MOD_SMT_POLICY_INIT_MAILBOX,
              .mailbox_address = (uintptr_t)SCMI_PAYLOAD_HIGH_P2A_BASE,
              .mailbox_size = SCMI_PAYLOAD_SIZE,
              .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                  FWK_MODULE_IDX_MHU,
                  JUNO_MHU_DEVICE_IDX_NS_H,
                  1),
              .driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MHU, 0),
              .pd_source_id = FWK_ID_ELEMENT_INIT(
                  FWK_MODULE_IDX_POWER_DOMAIN,
                  POWER_DOMAIN_IDX_SYSTOP),
          } },
#endif
    [JUNO_SCMI_SERVICE_IDX_COUNT] = { 0 },
};

```



순서

(1) AP (Linux Kernel) 에서 signal_message로 SCP_INTR_L_SET 에 1로 세팅하면 인터럽트가 전달됨

(2) SCP 에서 mhu_isr 핸들러가 호출되면 SCP_INTR_L_STAT 의 1 여부를 확인하고 queue 에 저장함. SCP_INTR_L_CLEAR 를 이용해서 처리가 완료되면 0으로 clear 시켜줌

(3) AP 의 last_tx_done 에서 SCP_INTR_L_STAT 의 0 여부를 확인해서 clear 되어있지 않으면 hr_timer 이용해서 1초 후에 다시 체크함 => 이 로직은 시간 단축을 위해서 polling 적용을 검토해볼 수 있을 것



### 3. ARM JUNO Board

Juno r2 ARM Development Platform SoC Technical Reference Manual

https://developer.arm.com/documentation/ddi0515/f/

ADP (ARM Development Platform) 는 Linux 나 Android 와 같이 Linaro 기반 커널들에 대한 개발 및 테스트가 가능하도록 만들어진 플랫폼이다.

> The ADP SoC is a development chip that is implemented on the TSMC 28HPM process and delivers the following: 
>
> • A platform for ARMv8-A software and tool development to enable robust testing of software deliverables on Linaro-based kernels such as Linux and Android.

![Juno_r2_block_diagram](Juno_r2_block_diagram.PNG)

ADP 의 내부 중 SCP 가 사용하는 메모리 영역은 아래와 같다.

> A Cortex-M3-based SCP designed to function as a trusted subsystem. This subsystem also includes: 
>
> — A local trusted on-chip ROM. 
>
> — Support for a local trusted on-chip SRAM to execute the main SCP firmware.

Trusted RAM 은 256 KB, Trusted ROM 은 64 KB

![Juno_Memory_map1](Juno_r2_memory_map1.PNG)

Non-trusted SRAM 은 32KB, Non-trusted ROM 은 4KB

![Juno_Memory_map2](Juno_r2_memory_map2.PNG))



### ARM Neoverse N2

![RD-N2 software components and features](RD_N2_software_components_and_features.png)

https://developer.arm.com/documentation/102337/0000/Software-stack/About-the-software

![Distributed power management in RD-N2](RD_N2_distributed_power_management.png)

https://developer.arm.com/documentation/102337/0000/Functional-description/System-power-management-and-domains

![img](https://www.electropages.com/storage/app/media/uploaded-files/1620376224124.jpeg)

https://www.electropages.com/blog/2021/05/arm-announces-n2-neoverse-computing-cores-data-centres

RD-N2 의 구조는 위와 같다.

### 4. OP TEE

STM32 에서 개발한 SCMI Overview (위키 문서)

https://wiki.st.com/stm32mpu/wiki/SCMI_overview

Arm Trusted Firmware 코드

```text
commit 제목: drivers/scmi-msg: smt entry points for incoming messages
This change implements SCMI channels for reading a SCMI message from a
shared memory and call the SCMI message drivers to route the message
to the target platform services.

SMT refers to the shared memory management protocol which is used
to get/put message/response in shared memory. SMT is a 28byte header
stating shared memory state and exchanged protocol data.
```

https://github.com/renesas-rcar/arm-trusted-firmware/commit/7d6fa6ecbe6212480ad656731eeddf84291aeba5

OP TEE os 코드

```text
commit 제목: drivers/scmi-msg: smt entry points for incoming messages
This change implements SCMI channels for reading a SCMI message from a
shared memory and call the SCMI message drivers to route the message
to the target platform services.

SMT refers to the shared memory management protocol which is used
to get/put message/response in shared memory. SMT is a 28byte header
stating shared memory state and exchanged protocol data.

The processing entry for a SCMI message can be a secure interrupt
(CFG_SCMI_MSG_SMT_INTERRUPT_ENTRY=y), and fastcall SMC
(CFG_SCMI_MSG_SMT_FASTCALL_ENTRY=y) or a threaded execution
context entry (CFG_SCMI_MSG_SMT_THREAD_ENTRY=y).
```

https://github.com/OP-TEE/optee_os/commit/a58c4d706d2333d2b21a3eba7e2ec0cb257bca1d



### 4. 코드

[Linux Kernel mainline 6.1-rc5](https://www.kernel.org/)

[SCP Firmware 2.11](https://github.com/ARM-software/SCP-firmware)



### 5. 환경 세팅 및 빌드 방법

(1) SCP Firmware git 받기

```shell
$ git clone https://github.com/ARM-software/SCP-firmware.git
```

(2) 연결된 git 들 한번에 sync 하는 방법 (빌드를 위한 오픈소스들이 git 으로 연결되어있음)

```shell
$ git submodule update --init
```

(3) 빌드용 config 설정 (juno 보드용)

```shell
$ ccmake -B tmp/build -DSCP_FIRMWARE_SOURCE_DIR:PATH=juno/scp_ramfw
// 위의 명령어치면 뜨는 창에서 c 누르고 g 누르기 
```

(4) 빌드

```shell
$ make -f Makefile.cmake PRODUCT=juno TOOLCHAIN=ArmClang
```

참고: SCP-firmware-2.11/doc/cmake_readme.md



개선 방향

1. 우선순위는 어떻게 할지: 동일한 우선 순위로 put_event 되고 queue 에서 꺼내 처리되는지?
2. Latency 가 병목이 생기는 건 어떻게 할지. Host CPU 는 빠르고, SCP Firmware 용 Cortex M 은 느리다면? (max frequency 스펙이 어떻게 되지?)
3. 멀티 태스킹 지원



### 참고 문헌

[1] [SCMI 공식 문서](https://developer.arm.com/documentation/den0056/d/?lang=en)

[2] [ARM SCMI 활용을 위한 linux kernel device tree 작성법](https://www.kernel.org/doc/Documentation/devicetree/bindings/arm/arm%2Cscmi.txt)

[3] [SCP Firmware 101 ppt](https://static.linaro.org/connect/san19/presentations/san19-117.pdf) 또는 [대본](http://docplayer.net/167756101-Arm-system-control-processor-scp-firmware-101.html)

[4] [SCP Firmware 101 설명 영상](https://resources.linaro.org/en/resource/PZgSfTGcN3qrz86cbitkMn) 또는 [유튜브 영상](https://www.youtube.com/watch?v=hR5YwfoyVy4)

[5] [Juno ARM Development Platform 공식 문서](https://developer.arm.com/documentation/ddi0515/f/)



### 관련 히스토리

Multi thread 기능 (SCP Firmware 101 ppt 에 설명 있음)은 더이상 지원하지 않는다.

BUILD_HAS_MULTITHREADING 및 SCP_ENABLE_MULTITHREADING 삭제

```text
commit f21ae5220ee44cbc77cedc2443d1a11ebfb2f98d
Author: Tarek El-Sherbiny <tarek.el-sherbiny@arm.com>
Date:   Fri Jan 28 18:12:09 2022 +0000

    remove mt: Remove the deprecated multi-thread feature

    This patch removes the multi-thread feature in the
    code, build, and document files.

    Currently, all SCP Firmware use cases do not require
    multi-threading. The decision has been made to remove
    this feature to reduce the maintenance load, improve
    readability and some code size efficiencies.

    Signed-off-by: Tarek El-Sherbiny <tarek.el-sherbiny@arm.com>
    Change-Id: I45de5822fc65cece997c7ebcb4cb65401018fb5b

```


#include "cmsis_os2.h"
#include "device_cfg.h"
#include "tfm_api.h"
#include "tfm_log.h"
#include "tfm_ns_interface.h"
#include "tfm_ns_svc.h"
#include "tfm_plat_ns.h"
#include "tfm_platform_api.h"
#include "region.h"
#include "target_cfg.h"
#include "uart_stdout.h"
#include "psa/internal_trusted_storage.h"
#include "driver/Driver_Common.h"

#ifndef UNUSED_VARIABLE
#define UNUSED_VARIABLE(x) ((void) x)
#endif

/**
 * \brief Modified table template for user defined SVC functions
 *
 * \details RTX has a weak definition of osRtxUserSVC, which
 *          is overridden here
 */
extern void * const osRtxUserSVC[1+USER_SVC_COUNT];
       void * const osRtxUserSVC[1+USER_SVC_COUNT] = {
  (void *)USER_SVC_COUNT,

#define X(SVC_ENUM, SVC_HANDLER) (void*)SVC_HANDLER,

    /* SVC API for Services */
#ifdef TFM_NS_CLIENT_IDENTIFICATION
    LIST_SVC_NSPM
#endif

#undef X

/*
 * (void *)user_function1,
 *  ...
 */
};

/**
 * \brief Platform peripherals and devices initialization.
 *        Can be overridden for platform specific initialization.
 *
 * \return  ARM_DRIVER_OK if the initialization succeeds
 */
__WEAK int32_t tfm_ns_platform_init(void)
{
    stdio_init();

    return ARM_DRIVER_OK;
}

/**
 * \brief Platform peripherals and devices de-initialization.
 *        Can be overridden for platform specific initialization.
 *
 * \return  ARM_DRIVER_OK if the de-initialization succeeds
 */
__WEAK int32_t tfm_ns_platform_uninit(void)
{
    stdio_uninit();

    return ARM_DRIVER_OK;
}

static uint64_t touch_flash_api_app_stack[(4u * 1024u) / (sizeof(uint64_t))]; /* 4KB */
static const osThreadAttr_t touch_flash_api_thread_attr = {
    .name = "touch_flash_api",
    .stack_mem = touch_flash_api_app_stack,
    .stack_size = sizeof(touch_flash_api_app_stack),
};

__attribute__((noreturn))
void touch_flash_api(void *argument) {
    UNUSED_VARIABLE(argument);

    LOG_MSG("[TOUCH FLASH API] Started\r\n");

    psa_status_t err;
    struct psa_storage_info_t info;
    err = psa_its_get_info(0x1337, &info);
    if (err == PSA_ERROR_DOES_NOT_EXIST) {
        uint32_t init = 0;
        if ((err =  psa_its_set(
            0x1337, sizeof(init), &init, PSA_STORAGE_FLAG_NONE)) != PSA_SUCCESS) {
            LOG_MSG("[TOUCH FLASH API] psa_its_set() returned %d during initial setup\r\n", err);
        }
    } else if (err != PSA_SUCCESS) {
        LOG_MSG("[TOUCH FLASH API] psa_its_get_info() returned %d\r\n", err);
    }

    while (1) {
        osDelay(osKernelGetTickFreq() * 1);

        uint32_t counter = 0;
        size_t counter_length = sizeof(counter);
        if ((err = psa_its_get(
            0x1337, 0, counter_length, &counter, &counter_length)) != PSA_SUCCESS) {
            LOG_MSG("[TOUCH FLASH API] psa_its_get() returned %d\r\n", err);
            continue;
        }

        LOG_MSG("[TOUCH FLASH API] Counter: %d\r\n", counter);

        ++counter;

        if ((err =  psa_its_set(
            0x1337, sizeof(counter), &counter, PSA_STORAGE_FLAG_NONE)) != PSA_SUCCESS) {
            LOG_MSG("[TOUCH FLASH API] psa_its_set() returned %d during update\r\n", err);
        }
    }
}

static uint64_t lpc55_poc_app_stack[(4u * 1024u) / (sizeof(uint64_t))]; /* 4KB */
static const osThreadAttr_t lpc55_poc_thread_attr = {
    .name = "lpc55_poc",
    .stack_mem = lpc55_poc_app_stack,
    .stack_size = sizeof(lpc55_poc_app_stack),
};

__attribute__((noreturn))
void lpc55_poc(void *argument) {
    UNUSED_VARIABLE(argument);

    osDelay(osKernelGetTickFreq() * 5);

    LOG_MSG("[LPC55 PoC] Started\r\n");

    while (1) {
        osDelay(osKernelGetTickFreq() * 5);

        LOG_MSG("[LPC55 PoC] Tick\r\n");

    }
}

int main(void)
{
#if defined(__ARM_ARCH_8_1M_MAIN__) || defined(__ARM_ARCH_8M_MAIN__)
    /* Set Main Stack Pointer limit */
    REGION_DECLARE(Image$$, ARM_LIB_STACK_MSP, $$ZI$$Base);
    __set_MSPLIM((uint32_t)&REGION_NAME(Image$$, ARM_LIB_STACK_MSP,
                                        $$ZI$$Base));
#endif

    if (tfm_ns_platform_init() != ARM_DRIVER_OK) {
        /* Avoid undefined behavior if platform init failed */
        while(1);
    }

    (void) osKernelInitialize();

    /* Initialize the TFM NS interface */
    tfm_ns_interface_init();

    (void)osThreadNew(touch_flash_api, NULL, &touch_flash_api_thread_attr);
    (void)osThreadNew(lpc55_poc, NULL, &lpc55_poc_thread_attr);

    LOG_MSG("LPC55 ROM Patch PoC starting as non-secure app...\r\n");

    (void) osKernelStart();

    // In case of kernel error, hang.
    while (1) {}
}
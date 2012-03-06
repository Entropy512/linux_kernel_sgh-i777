/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.h
 * Platform specific Mali driver functions
 */

#include "mali_osk.h"

#if USING_MALI_PMM
#include "mali_pmm.h"
#endif

#if !USING_MALI_PMM
/* @brief System power up/down cores that can be passed into mali_platform_powerdown/up() */
#define MALI_PLATFORM_SYSTEM  0
#endif

/** @brief Platform specific setup and initialisation of MALI
 * 
 * This is called from the entrypoint of the driver to initialize the platform
 * When using PMM, it is also called from the PMM start up to initialise the 
 * system PMU
 *
 * @param resource This is NULL when called on first driver start up, else it will
 * be a pointer to a PMU resource
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_platform_init(_mali_osk_resource_t *resource);

/** @brief Platform specific deinitialisation of MALI
 * 
 * This is called on the exit of the driver to terminate the platform
 * When using PMM, it is also called from the PMM termination code to clean up the
 * system PMU
 *
 * @param type This is NULL when called on driver exit, else it will
 * be a pointer to a PMU resource type (not the full resource)
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_platform_deinit(_mali_osk_resource_type_t *type);

/** @brief Platform specific powerdown sequence of MALI
 * 
 * Called as part of platform init if there is no PMM support, else the
 * PMM will call it.
 *
 * @param cores This is MALI_PLATFORM_SYSTEM when called without PMM, else it will
 * be a mask of cores to power down based on the mali_pmm_core_id enum
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_platform_powerdown(u32 cores);

/** @brief Platform specific powerup sequence of MALI
 * 
 * Called as part of platform deinit if there is no PMM support, else the
 * PMM will call it.
 *
 * @param cores This is MALI_PLATFORM_SYSTEM when called without PMM, else it will
 * be a mask of cores to power down based on the mali_pmm_core_id enum
 * @return _MALI_OSK_ERR_OK on success otherwise, a suitable _mali_osk_errcode_t error.
 */
_mali_osk_errcode_t mali_platform_powerup(u32 cores);

/** @brief Platform specific handling of GPU utilization data
 *
 * When GPU utilization data is enabled, this function will be
 * periodically called.
 *
 * @param utilization The workload utilization of the Mali GPU. 0 = no utilization, 256 = full utilization.
 */
void mali_gpu_utilization_handler(u32 utilization);

void mali_utilization_suspend(void);

#ifdef CONFIG_REGULATOR
int mali_regulator_get_usecount(void);
void mali_regulator_disable(void);
void mali_regulator_enable(void);
void mali_regulator_set_voltage(int min_uV, int max_uV);
#endif
void mali_clk_set_rate(unsigned int clk, unsigned int mhz);
unsigned long mali_clk_get_rate(void);
void mali_clk_put(void);

#if USING_MALI_PMM
#if MALI_POWER_MGMT_TEST_SUITE
/** @brief function to get status of individual cores
 *
 * This function is used by power management test suite to get the status of powered up/down the number
 * of cores
 * @param utilization The workload utilization of the Mali GPU. 0 = no utilization, 256 = full utilization.
 */
u32 pmu_get_power_up_down_info(void);
#endif
#endif

#if MALI_DVFS_ENABLED
mali_bool init_mali_dvfs_staus(int step);
void deinit_mali_dvfs_staus(void);
mali_bool mali_dvfs_handler(u32 utilization);
int mali_dvfs_is_running(void);
void mali_dvfs_late_resume(void);
#endif
void mali_default_step_set(int step, mali_bool boostup);

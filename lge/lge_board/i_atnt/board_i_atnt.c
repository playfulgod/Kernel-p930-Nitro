/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/mfd/pmic8058.h>

#include <linux/input/pmic8058-keypad.h>
#include <linux/pmic8058-batt-alarm.h>
#include <linux/pmic8058-pwrkey.h>
#include <linux/rtc/rtc-pm8058.h>
#include <linux/pmic8058-vibrator.h>
#include <linux/leds.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/bootmem.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/leds-pmic8058.h>
#include <linux/pmic8058-xoadc.h>
#include <linux/msm_adc.h>
#include <linux/m_adcproc.h>
#include <linux/mfd/marimba.h>
#include <linux/msm-charger.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/smsc911x.h>
#include <linux/spi/spi.h>
#ifdef CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C
#include <touch_synaptics_rmi4_i2c.h>
#else
#include <linux/input/tdisc_shinetsu.h>
#include <linux/input/cy8c_ts.h>
#include <linux/cyttsp.h>
#endif
#include <linux/i2c/isa1200.h>
#include <linux/dma-mapping.h>
#include <linux/i2c/bq27520.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#if defined(CONFIG_SMB137B_CHARGER) || defined(CONFIG_SMB137B_CHARGER_MODULE)
#include <linux/i2c/smb137b.h>
#endif
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/dma.h>
#include <mach/mpp.h>
#include <mach/board.h>
#include <mach/irqs.h>
#include <mach/msm_spi.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_serial_hs_lite.h>
#include <mach/msm_iomap.h>
#include <asm/mach/mmc.h>
#include <mach/msm_battery.h>
#include <mach/msm_hsusb.h>
#ifdef CONFIG_MSM_DSPS
#include <mach/msm_dsps.h>
#endif
#include <mach/msm_xo.h>
#include <mach/msm_bus_board.h>
#include <mach/socinfo.h>
#include <linux/i2c/isl9519.h>
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif
#include <mach/usb_gadget_fserial.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <mach/sdio_al.h>
#include <mach/rpm.h>
#include <mach/rpm-regulator.h>

#include "devices.h"
#include "devices_i_atnt.h"
#include "board_i_atnt.h"
#include "cpuidle.h"
#include "pm.h"
#include "mpm.h"
#include "spm.h"
#include "rpm_log.h"
#include "timer.h"
#include "saw-regulator.h"
#include "gpiomux.h"
#include "gpiomux_i_atnt.h"
#include "board_lge.h"
#include <apds9900.h>
#include "rpm_stats.h"
#include "../../../arch/arm/mach-msm/peripheral-loader.h"
#define MSM_SHARED_RAM_PHYS 0x40000000

#ifdef CONFIG_LGE_FUEL_GAUGE
#include <linux/max17040_battery.h>
#endif

#ifdef CONFIG_LGE_SWITCHING_CHARGER_MAX8971
#include <linux/max8971-charger.h>
#endif

#ifdef CONFIG_LGE_SWITCHING_CHARGER_BQ24160_DOCOMO_ONLY
#include <linux/bq24160-charger.h>
#endif

#ifdef CONFIG_LGE_DIAGTEST
// MOD 0009214: [DIAG] LG Diag feature added in side of android
#include <lg_fw_diag_communication.h>
#endif

#if 0 /* moved following macros into devices_i_atnt.h */
/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_BASE			NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_GPIO_BASE)
#define PM8058_MPP_BASE			(PM8058_GPIO_BASE + PM8058_GPIOS)
#define PM8058_MPP_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_MPP_BASE)
#define PM8058_MPP_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_MPP_BASE)
#define PM8058_IRQ_BASE				(NR_MSM_IRQS + NR_GPIO_IRQS)

#define PM8901_GPIO_BASE			(PM8058_GPIO_BASE + \
						PM8058_GPIOS + PM8058_MPPS)
#define PM8901_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8901_GPIO_BASE)
#define PM8901_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM901_GPIO_BASE)
#define PM8901_IRQ_BASE				(PM8058_IRQ_BASE + \
						NR_PMIC8058_IRQS)
#endif

#define MDM2AP_SYNC 129

//platform.team@lge.com	2011.02.09
//FIXME temp fix to avoid kernel panic. Higher number than GPIO_EXPANDER_GPIO_BASE should not be used.
#define GPIO_MS_SYS_RESET_N 0

enum {
	GPIO_EXPANDER_IRQ_BASE  = PM8901_IRQ_BASE + NR_PMIC8901_IRQS,
	GPIO_EXPANDER_GPIO_BASE = PM8901_GPIO_BASE + PM8901_MPPS,
	/* CORE expander */
	GPIO_CORE_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE,
	GPIO_CLASS_D1_EN        = GPIO_CORE_EXPANDER_BASE,
	GPIO_WLAN_DEEP_SLEEP_N,
	GPIO_LVDS_SHUTDOWN_N,
	GPIO_DISP_RESX_N        = GPIO_LVDS_SHUTDOWN_N,
//platform.team@lge.com 2011.02.09
//FIXME temp fix to avoid kernel panic. Higher number than GPIO_EXPANDER_GPIO_BASE should not be used.
	//GPIO_MS_SYS_RESET_N,
	GPIO_CAP_TS_RESOUT_N,
	GPIO_CAP_GAUGE_BI_TOUT,
	GPIO_ETHERNET_PME,
	GPIO_EXT_GPS_LNA_EN,
	GPIO_MSM_WAKES_BT,
	GPIO_ETHERNET_RESET_N,
	GPIO_HEADSET_DET_N,
	GPIO_USB_UICC_EN,
	GPIO_BACKLIGHT_EN,
	GPIO_EXT_CAMIF_PWR_EN,
	GPIO_BATT_GAUGE_INT_N,
	GPIO_BATT_GAUGE_EN,
	/* DOCKING expander */
	GPIO_DOCKING_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + 16,
	GPIO_MIPI_DSI_RST_N        = GPIO_DOCKING_EXPANDER_BASE,
	GPIO_AUX_JTAG_DET_N,
	GPIO_DONGLE_DET_N,
	GPIO_SVIDEO_LOAD_DET,
	GPIO_SVID_AMP_SHUTDOWN1_N,
	GPIO_SVID_AMP_SHUTDOWN0_N,
	GPIO_SDC_WP,
	GPIO_IRDA_PWDN,
	GPIO_IRDA_RESET_N,
	GPIO_DONGLE_GPIO0,
	GPIO_DONGLE_GPIO1,
	GPIO_DONGLE_GPIO2,
	GPIO_DONGLE_GPIO3,
	GPIO_DONGLE_PWR_EN,
	GPIO_EMMC_RESET_N,
	GPIO_TP_EXP2_IO15,
	/* SURF expander */
	GPIO_SURF_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + (16 * 2),
	GPIO_SD_CARD_DET_1      = GPIO_SURF_EXPANDER_BASE,
	GPIO_SD_CARD_DET_2,
	GPIO_SD_CARD_DET_4,
	GPIO_SD_CARD_DET_5,
	GPIO_UIM3_RST,
	GPIO_SURF_EXPANDER_IO5,
	GPIO_SURF_EXPANDER_IO6,
	GPIO_ADC_I2C_EN,
	GPIO_SURF_EXPANDER_IO8,
	GPIO_SURF_EXPANDER_IO9,
	GPIO_SURF_EXPANDER_IO10,
	GPIO_SURF_EXPANDER_IO11,
	GPIO_SURF_EXPANDER_IO12,
	GPIO_SURF_EXPANDER_IO13,
	GPIO_SURF_EXPANDER_IO14,
	GPIO_SURF_EXPANDER_IO15,
	/* LEFT KB IO expander */
	GPIO_LEFT_KB_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + (16 * 3),
	GPIO_LEFT_LED_1            = GPIO_LEFT_KB_EXPANDER_BASE,
	GPIO_LEFT_LED_2,
	GPIO_LEFT_LED_3,
	GPIO_LEFT_LED_WLAN,
	GPIO_JOYSTICK_EN,
	GPIO_CAP_TS_SLEEP,
	GPIO_LEFT_KB_IO6,
	GPIO_LEFT_LED_5,
	/* RIGHT KB IO expander */
	GPIO_RIGHT_KB_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE + (16 * 3) + 8,
	GPIO_RIGHT_LED_1            = GPIO_RIGHT_KB_EXPANDER_BASE,
	GPIO_RIGHT_LED_2,
	GPIO_RIGHT_LED_3,
	GPIO_RIGHT_LED_BT,
	GPIO_WEB_CAMIF_STANDBY,
	GPIO_COMPASS_RST_N,
	GPIO_WEB_CAMIF_RESET_N,
	GPIO_RIGHT_LED_5,
	GPIO_R_ALTIMETER_RESET_N,
	/* FLUID S IO expander */
	GPIO_SOUTH_EXPANDER_BASE,
	GPIO_MIC2_ANCR_SEL = GPIO_SOUTH_EXPANDER_BASE,
	GPIO_MIC1_ANCL_SEL,
	GPIO_HS_MIC4_SEL,
	GPIO_FML_MIC3_SEL,
	GPIO_FMR_MIC5_SEL,
	GPIO_TS_SLEEP,
	GPIO_HAP_SHIFT_LVL_OE,
	GPIO_HS_SW_DIR,
	/* FLUID N IO expander */
	GPIO_NORTH_EXPANDER_BASE,
	GPIO_EPM_3_3V_EN = GPIO_NORTH_EXPANDER_BASE,
	GPIO_EPM_5V_BOOST_EN,
	GPIO_AUX_CAM_2P7_EN,
	GPIO_LED_FLASH_EN,
	GPIO_LED1_GREEN_N,
	GPIO_LED2_RED_N,
	GPIO_FRONT_CAM_RESET_N,
	GPIO_EPM_LVLSFT_EN,
	GPIO_N_ALTIMETER_RESET_N,
};

/*
 * The UI_INTx_N lines are pmic gpio lines which connect i2c
 * gpio expanders to the pm8058.
 */
#define UI_INT1_N 25
#define UI_INT2_N 34
#define UI_INT3_N 14
/*
FM GPIO is GPIO 18 on PMIC 8058.
As the index starts from 0 in the PMIC driver, and hence 17
corresponds to GPIO 18 on PMIC 8058.
*/
#define FM_GPIO 17

extern unsigned int g_current_boot_step;
extern unsigned int g_sd_power_dircect_ctrl;

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static void (*sdc2_status_notify_cb)(int card_present, void *dev_id);
static void *sdc2_status_notify_cb_devid;
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
static void (*sdc5_status_notify_cb)(int card_present, void *dev_id);
static void *sdc5_status_notify_cb_devid;
#endif

static struct msm_spm_platform_data msm_spm_data_v1[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},
};

static struct msm_acpu_clock_platform_data msm8x60_acpu_clock_data = {
};

static struct regulator_consumer_supply saw_s0_supply =
	REGULATOR_SUPPLY("8901_s0", NULL);
static struct regulator_consumer_supply saw_s1_supply =
	REGULATOR_SUPPLY("8901_s1", NULL);

static struct regulator_init_data saw_s0_init_data = {
		.constraints = {
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 840000,
			.max_uV = 1250000,
		},
		.num_consumer_supplies = 1,
		.consumer_supplies = &saw_s0_supply,
};

static struct regulator_init_data saw_s1_init_data = {
		.constraints = {
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 840000,
			.max_uV = 1250000,
		},
		.num_consumer_supplies = 1,
		.consumer_supplies = &saw_s1_supply,
};

static struct platform_device msm_device_saw_s0 = {
	.name          = "saw-regulator",
	.id            = SAW_VREG_ID_S0,
	.dev           = {
		.platform_data = &saw_s0_init_data,
	},
};

static struct platform_device msm_device_saw_s1 = {
	.name          = "saw-regulator",
	.id            = SAW_VREG_ID_S1,
	.dev           = {
		.platform_data = &saw_s1_init_data,
	},
};


#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000

#define QCE_HW_KEY_SUPPORT	0
#define QCE_SHA_HMAC_SUPPORT	0
#define QCE_SHARE_CE_RESOURCE	2
#define QCE_CE_SHARED		1

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.name = "crypto_crci_hash",
		.start = DMOV_CE_HASH_CRCI,
		.end = DMOV_CE_HASH_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.name = "crypto_crci_hash",
		.start = DMOV_CE_HASH_CRCI,
		.end = DMOV_CE_HASH_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
};

static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_HAPTIC_ISA1200) || \
		defined(CONFIG_HAPTIC_ISA1200_MODULE)

static const char *vregs_isa1200_name[] = {
	"8058_s3",
	"8901_l4",
};

static const int vregs_isa1200_val[] = {
	1800000,/* uV */
	2600000,
};
static struct regulator *vregs_isa1200[ARRAY_SIZE(vregs_isa1200_name)];
static struct msm_xo_voter *xo_handle_a1;

static int isa1200_power(int vreg_on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++) {
		rc = vreg_on ? regulator_enable(vregs_isa1200[i]) :
			regulator_disable(vregs_isa1200[i]);
		if (rc < 0) {
			pr_err("%s: vreg %s %s failed (%d)\n",
				__func__, vregs_isa1200_name[i],
				vreg_on ? "enable" : "disable", rc);
			goto vreg_fail;
		}
	}

	rc = vreg_on ? msm_xo_mode_vote(xo_handle_a1, MSM_XO_MODE_ON) :
			msm_xo_mode_vote(xo_handle_a1, MSM_XO_MODE_OFF);
	if (rc < 0) {
		pr_err("%s: failed to %svote for TCXO A1 buffer%d\n",
				__func__, vreg_on ? "" : "de-", rc);
		goto vreg_fail;
	}
	return 0;

vreg_fail:
	while (i--)
		!vreg_on ? regulator_enable(vregs_isa1200[i]) :
			regulator_disable(vregs_isa1200[i]);
	return rc;
}

static int isa1200_dev_setup(bool enable)
{
	int i, rc;

	if (enable == true) {
		for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++) {
			vregs_isa1200[i] = regulator_get(NULL,
						vregs_isa1200_name[i]);
			if (IS_ERR(vregs_isa1200[i])) {
				pr_err("%s: regulator get of %s failed (%ld)\n",
					__func__, vregs_isa1200_name[i],
					PTR_ERR(vregs_isa1200[i]));
				rc = PTR_ERR(vregs_isa1200[i]);
				goto vreg_get_fail;
			}
			rc = regulator_set_voltage(vregs_isa1200[i],
				vregs_isa1200_val[i], vregs_isa1200_val[i]);
			if (rc) {
				pr_err("%s: regulator_set_voltage(%s) failed\n",
					__func__, vregs_isa1200_name[i]);
				goto vreg_get_fail;
			}
		}

		rc = gpio_request(GPIO_HAP_SHIFT_LVL_OE, "haptics_shft_lvl_oe");
		if (rc) {
			pr_err("%s: unable to request gpio %d (%d)\n",
					__func__, GPIO_HAP_SHIFT_LVL_OE, rc);
			goto vreg_get_fail;
		}

		rc = gpio_direction_output(GPIO_HAP_SHIFT_LVL_OE, 1);
		if (rc) {
			pr_err("%s: Unable to set direction\n", __func__);;
			goto free_gpio;
		}

		xo_handle_a1 = msm_xo_get(MSM_XO_TCXO_A1, "isa1200");
		if (IS_ERR(xo_handle_a1)) {
			rc = PTR_ERR(xo_handle_a1);
			pr_err("%s: failed to get the handle for A1(%d)\n",
							__func__, rc);
			goto gpio_set_dir;
		}
	} else {
		gpio_set_value(GPIO_HAP_SHIFT_LVL_OE, 0);
		gpio_free(GPIO_HAP_SHIFT_LVL_OE);

		for (i = 0; i < ARRAY_SIZE(vregs_isa1200_name); i++)
			regulator_put(vregs_isa1200[i]);

		msm_xo_put(xo_handle_a1);
	}

	return 0;
gpio_set_dir:
	gpio_set_value(GPIO_HAP_SHIFT_LVL_OE, 0);
free_gpio:
	gpio_free(GPIO_HAP_SHIFT_LVL_OE);
vreg_get_fail:
	while (i)
		regulator_put(vregs_isa1200[--i]);
	return rc;
}

#define PMIC_GPIO_HAP_ENABLE   18  /* PMIC GPIO Number 19 */
static struct isa1200_platform_data isa1200_1_pdata = {
	.name = "vibrator",
	.power_on = isa1200_power,
	.dev_setup = isa1200_dev_setup,
	/*gpio to enable haptic*/
	.hap_en_gpio = PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_HAP_ENABLE),
	.max_timeout = 15000,
	.mode_ctrl = PWM_GEN_MODE,
	.pwm_fd = {
		.pwm_div = 256,
	},
	.is_erm = false,
	.smart_en = true,
	.ext_clk_en = true,
	.chip_en = 1,
};

static struct i2c_board_info msm_isa1200_board_info[] = {
	{
		I2C_BOARD_INFO("isa1200_1", 0x90>>1),
		.platform_data = &isa1200_1_pdata,
	},
};
#endif

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 4000,
		.residency = 13000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 500,
		.residency = 6000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.supported = 1,
		.suspend_enabled = 1,
		.idle_enabled = 1,
		.latency = 2,
		.residency = 0,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 600,
		.residency = 7200,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 500,
		.residency = 6000,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.supported = 1,
		.suspend_enabled = 1,
		.idle_enabled = 1,
		.latency = 2,
		.residency = 0,
	},
};

static struct msm_cpuidle_state msm_cstates[] __initdata = {
	{0, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{0, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},

	{0, 2, "C2", "POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE},

	{1, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{1, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},
};
#if defined(CONFIG_USB_PEHCI_HCD) || defined(CONFIG_USB_PEHCI_HCD_MODULE)

#define ISP1763_INT_GPIO		117
#define ISP1763_RST_GPIO		152
static struct resource isp1763_resources[] = {
	[0] = {
		.flags	= IORESOURCE_MEM,
		.start	= 0x1D000000,
		.end	= 0x1D005FFF,		/* 24KB */
	},
	[1] = {
		.flags	= IORESOURCE_IRQ,
	},
};
static void __init msm8x60_cfg_isp1763(void)
{
	isp1763_resources[1].start = gpio_to_irq(ISP1763_INT_GPIO);
	isp1763_resources[1].end = gpio_to_irq(ISP1763_INT_GPIO);
}

static int isp1763_setup_gpio(int enable)
{
	int status = 0;

	if (enable) {
		status = gpio_request(ISP1763_INT_GPIO, "isp1763_usb");
		if (status) {
			pr_err("%s:Failed to request GPIO %d\n",
						__func__, ISP1763_INT_GPIO);
			return status;
		}
		status = gpio_direction_input(ISP1763_INT_GPIO);
		if (status) {
			pr_err("%s:Failed to configure GPIO %d\n",
					__func__, ISP1763_INT_GPIO);
			goto gpio_free_int;
		}
		status = gpio_request(ISP1763_RST_GPIO, "isp1763_usb");
		if (status) {
			pr_err("%s:Failed to request GPIO %d\n",
						__func__, ISP1763_RST_GPIO);
			goto gpio_free_int;
		}
		status = gpio_direction_output(ISP1763_RST_GPIO, 1);
		if (status) {
			pr_err("%s:Failed to configure GPIO %d\n",
					__func__, ISP1763_RST_GPIO);
			goto gpio_free_rst;
		}
		pr_debug("\nISP GPIO configuration done\n");
		return status;
	}

gpio_free_rst:
	gpio_free(ISP1763_RST_GPIO);
gpio_free_int:
	gpio_free(ISP1763_INT_GPIO);

	return status;
}
static struct isp1763_platform_data isp1763_pdata = {
	.reset_gpio	= ISP1763_RST_GPIO,
	.setup_gpio	= isp1763_setup_gpio
};

static struct platform_device isp1763_device = {
	.name          = "isp1763_usb",
	.num_resources = ARRAY_SIZE(isp1763_resources),
	.resource      = isp1763_resources,
	.dev           = {
		.platform_data = &isp1763_pdata
	}
};
#endif

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_MSM_72K)
static struct regulator *ldo6_3p3;
static struct regulator *ldo7_1p8;
static struct regulator *vdd_cx;
#define PMICID_INT		PM8058_GPIO_IRQ(PM8058_IRQ_BASE, 36)
notify_vbus_state notify_vbus_state_func_ptr;
#if 0 /* platform.bsp@lge.com 20110830 */
static int usb_phy_susp_dig_vol = 750000;
#else
static int usb_phy_susp_dig_vol = 500000;
#endif
static int pmic_id_notif_supported;

#ifdef CONFIG_USB_EHCI_MSM_72K
#define USB_PMIC_ID_DET_DELAY	msecs_to_jiffies(100)
struct delayed_work pmic_id_det;

static int __init usb_id_pin_rework_setup(char *support)
{
	if (strncmp(support, "true", 4) == 0)
		pmic_id_notif_supported = 1;

	return 1;
}
__setup("usb_id_pin_rework=", usb_id_pin_rework_setup);

#if ! defined(CONFIG_LGE_PMIC8058_GPIO)
static void pmic_id_detect(struct work_struct *w)
{
	int val = gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(36));
	pr_debug("%s(): gpio_read_value = %d\n", __func__, val);

	if (notify_vbus_state_func_ptr)
		(*notify_vbus_state_func_ptr) (val);
}

static irqreturn_t pmic_id_on_irq(int irq, void *data)
{
	/*
	 * Spurious interrupts are observed on pmic gpio line
	 * even though there is no state change on USB ID. Schedule the
	 * work to to allow debounce on gpio
	 */
	schedule_delayed_work(&pmic_id_det, USB_PMIC_ID_DET_DELAY);

	return IRQ_HANDLED;
}
#endif

static int msm_hsusb_pmic_id_notif_init(void (*callback)(int online), int init)
{
#if defined(CONFIG_LGE_PMIC8058_GPIO)
	return -ENOTSUPP;
#else
	unsigned ret = -ENODEV;

	if (!callback)
		return -EINVAL;

	if (machine_is_msm8x60_fluid())
		return -ENOTSUPP;

	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 2) {
		pr_debug("%s: USB_ID pin is not routed to PMIC"
					"on V1 surf/ffa\n", __func__);
		return -ENOTSUPP;
	}

	if ((machine_is_msm8x60_ffa() || machine_is_msm8x60_charm_ffa()) &&
			!pmic_id_notif_supported) {
		pr_debug("%s: USB_ID is not routed to PMIC"
			"on V2 ffa\n", __func__);
		return -ENOTSUPP;
	}

	usb_phy_susp_dig_vol = 500000;

	if (init) {
		notify_vbus_state_func_ptr = callback;
		ret = pm8901_mpp_config_digital_out(1,
			PM8901_MPP_DIG_LEVEL_L5, 1);
		if (ret) {
			pr_err("%s: MPP2 configuration failed\n", __func__);
			return -ENODEV;
		}
		INIT_DELAYED_WORK(&pmic_id_det, pmic_id_detect);
		ret = request_threaded_irq(PMICID_INT, NULL, pmic_id_on_irq,
			(IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
						"msm_otg_id", NULL);
		if (ret) {
			pm8901_mpp_config_digital_out(1,
					PM8901_MPP_DIG_LEVEL_L5, 0);
			pr_err("%s:pmic_usb_id interrupt registration failed",
					__func__);
			return ret;
		}
		/* Notify the initial Id status */
		pmic_id_detect(&pmic_id_det.work);
	} else {
		free_irq(PMICID_INT, 0);
		cancel_delayed_work_sync(&pmic_id_det);
		notify_vbus_state_func_ptr = NULL;
		ret = pm8901_mpp_config_digital_out(1,
			PM8901_MPP_DIG_LEVEL_L5, 0);
		if (ret) {
			pr_err("%s:MPP2 configuration failed\n", __func__);
			return -ENODEV;
		}
	}
	return 0;
#endif
}
#endif

#define USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL	1000000
#define USB_PHY_MAX_VDD_DIG_VOL			1320000
static int msm_hsusb_init_vddcx(int init)
{
	int ret = 0;

	if (init) {
		vdd_cx = regulator_get(NULL, "8058_s1");
		if (IS_ERR(vdd_cx)) {
			return PTR_ERR(vdd_cx);
		}

		ret = regulator_set_voltage(vdd_cx,
				USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL,
				USB_PHY_MAX_VDD_DIG_VOL);
		if (ret) {
			pr_err("%s: unable to set the voltage for regulator"
				"vdd_cx\n", __func__);
			regulator_put(vdd_cx);
			return ret;
		}

		ret = regulator_enable(vdd_cx);
		if (ret) {
			pr_err("%s: unable to enable regulator"
				"vdd_cx\n", __func__);
			regulator_put(vdd_cx);
		}
	} else {
		ret = regulator_disable(vdd_cx);
		if (ret) {
			pr_err("%s: Unable to disable the regulator:"
				"vdd_cx\n", __func__);
			return ret;
		}

		regulator_put(vdd_cx);
	}

	return ret;
}

static int msm_hsusb_config_vddcx(int high)
{
	int max_vol = USB_PHY_MAX_VDD_DIG_VOL;
	int min_vol;
	int ret;

	if (high)
		min_vol = USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL;
	else
		min_vol = usb_phy_susp_dig_vol;

	ret = regulator_set_voltage(vdd_cx, min_vol, max_vol);
	if (ret) {
		pr_err("%s: unable to set the voltage for regulator"
			"vdd_cx\n", __func__);
		return ret;
	}

	pr_debug("%s: min_vol:%d max_vol:%d\n", __func__, min_vol, max_vol);

	return ret;
}

#define USB_PHY_3P3_VOL_MIN	3050000 /* uV */
#define USB_PHY_3P3_VOL_MAX	3050000 /* uV */
#define USB_PHY_3P3_HPM_LOAD	50000	/* uA */
#define USB_PHY_3P3_LPM_LOAD	4000	/* uA */

#define USB_PHY_1P8_VOL_MIN	1800000 /* uV */
#define USB_PHY_1P8_VOL_MAX	1800000 /* uV */
#define USB_PHY_1P8_HPM_LOAD	50000	/* uA */
#define USB_PHY_1P8_LPM_LOAD	4000	/* uA */
/* [START] hansun.lee@lge.com 2011-06-27, Change 3.075V to 3.5V at USB1.1(56K) */
#ifdef CONFIG_LGE_USB_FACTORY
#include "../../include/lg_power_common.h"
extern int usb_cable_info;
#endif
/* [END] hansun.lee@lge.com 2011-06-27 */

// START sungchae.koo@lge.com 2011/07/25 P1_LAB_BSP {
#ifdef CONFIG_LGE_USB_FACTORY
int usb_ldo_vltg = 0;
int msm_hsusb_ldo_set_3p5( void )
{
    int rc = 0;
    if( ldo6_3p3 )
    {
        if( 3500000 == usb_ldo_vltg )
        {
            pr_info("%s: hsusb_ldo6_3p3 is already set (%d)\n", __func__, usb_ldo_vltg);
            return 0;
        }
        pr_info("%s: regulator_set_voltage(ldo6_3p3, 3500000, 3500000) - Change 3.075V to 3.5V at USB1.1(56K)\n", __func__);
        rc = regulator_disable(ldo6_3p3);
        rc |= regulator_set_voltage(ldo6_3p3, 3500000, 3500000);
        rc |= regulator_enable(ldo6_3p3);
        return rc;
    }
    return -1;
}
#endif
// END sungchae.koo@lge.com 2011/07/25 P1_LAB_BSP }

static int msm_hsusb_ldo_init(int init)
{
	int rc = 0;

	if (init) {
		ldo6_3p3 = regulator_get(NULL, "8058_l6");
		if (IS_ERR(ldo6_3p3))
			return PTR_ERR(ldo6_3p3);

		ldo7_1p8 = regulator_get(NULL, "8058_l7");
		if (IS_ERR(ldo7_1p8)) {
			rc = PTR_ERR(ldo7_1p8);
			goto put_3p3;
		}

        /* [START] hansun.lee@lge.com 2011-06-27, Change 3.075V to 3.5V at USB1.1(56K) */
#ifdef CONFIG_LGE_USB_FACTORY
        if(usb_cable_info==LT_CABLE_56K)
        {
            pr_info("%s: regulator_set_voltage(ldo6_3p3, 3500000, 3500000) - Change 3.075V to 3.5V at USB1.1(56K)\n", __func__);
            rc = regulator_set_voltage(ldo6_3p3, 3500000, 3500000);
            // sungchae.koo@lge.com 2011/07/25 P1_LAB_BSP 
            usb_ldo_vltg = 3500000;
        }
        else
        {
#endif
            rc = regulator_set_voltage(ldo6_3p3, USB_PHY_3P3_VOL_MIN,
                    USB_PHY_3P3_VOL_MAX);
#ifdef CONFIG_LGE_USB_FACTORY
            // sungchae.koo@lge.com 2011/07/25 P1_LAB_BSP 
            usb_ldo_vltg = USB_PHY_3P3_VOL_MIN;
        }
#endif
        /* [END] hansun.lee@lge.com 2011-06-27 */
		if (rc) {
			pr_err("%s: Unable to set voltage level for"
				"ldo6_3p3 regulator\n", __func__);
			goto put_1p8;
		}
		rc = regulator_enable(ldo6_3p3);
		if (rc) {
			pr_err("%s: Unable to enable the regulator:"
				"ldo6_3p3\n", __func__);
			goto put_1p8;
		}
		rc = regulator_set_voltage(ldo7_1p8, USB_PHY_1P8_VOL_MIN,
				USB_PHY_1P8_VOL_MAX);
		if (rc) {
			pr_err("%s: Unable to set voltage level for"
				"ldo7_1p8 regulator\n", __func__);
			goto disable_3p3;
		}
		rc = regulator_enable(ldo7_1p8);
		if (rc) {
			pr_err("%s: Unable to enable the regulator:"
				"ldo7_1p8\n", __func__);
			goto disable_3p3;
		}

		return 0;
	}

	regulator_disable(ldo7_1p8);
disable_3p3:
	regulator_disable(ldo6_3p3);
put_1p8:
	regulator_put(ldo7_1p8);
put_3p3:
	regulator_put(ldo6_3p3);
	return rc;
}

static int msm_hsusb_ldo_enable(int on)
{
	int ret = 0;

	if (!ldo7_1p8 || IS_ERR(ldo7_1p8)) {
		pr_err("%s: ldo7_1p8 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (!ldo6_3p3 || IS_ERR(ldo6_3p3)) {
		pr_err("%s: ldo6_3p3 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (on) {
		ret = regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_HPM_LOAD);
		if (ret < 0) {
			pr_err("%s: Unable to set HPM of the regulator:"
				"ldo7_1p8\n", __func__);
			return ret;
		}
		ret = regulator_set_optimum_mode(ldo6_3p3,
				USB_PHY_3P3_HPM_LOAD);
		if (ret < 0) {
			pr_err("%s: Unable to set HPM of the regulator:"
				"ldo6_3p3\n", __func__);
			regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_LPM_LOAD);
			return ret;
		}
	} else {
		ret = regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_LPM_LOAD);
		if (ret < 0)
			pr_err("%s: Unable to set LPM of the regulator:"
				"ldo7_1p8\n", __func__);
		ret = regulator_set_optimum_mode(ldo6_3p3,
				USB_PHY_3P3_LPM_LOAD);
		if (ret < 0)
			pr_err("%s: Unable to set LPM of the regulator:"
				"ldo6_3p3\n", __func__);
	}

	pr_debug("reg (%s)\n", on ? "HPM" : "LPM");
	return ret < 0 ? ret : 0;
 }
#endif
#ifdef CONFIG_USB_EHCI_MSM_72K
#if defined(CONFIG_SMB137B_CHARGER) || defined(CONFIG_SMB137B_CHARGER_MODULE)
static void msm_hsusb_smb137b_vbus_power(unsigned phy_info, int on)
{
	static int vbus_is_on;

	/* If VBUS is already on (or off), do nothing. */
	if (on == vbus_is_on)
		return;
	smb137b_otg_power(on);
	vbus_is_on = on;
}
#endif
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	static struct regulator *votg_5v_switch;
	static struct regulator *ext_5v_reg;
	static int vbus_is_on;

	/* If VBUS is already on (or off), do nothing. */
	if (on == vbus_is_on)
		return;

	if (!votg_5v_switch) {
		votg_5v_switch = regulator_get(NULL, "8901_usb_otg");
		if (IS_ERR(votg_5v_switch)) {
			pr_err("%s: unable to get votg_5v_switch\n", __func__);
			return;
		}
	}
	if (!ext_5v_reg) {
		ext_5v_reg = regulator_get(NULL, "8901_mpp0");
		if (IS_ERR(ext_5v_reg)) {
			pr_err("%s: unable to get ext_5v_reg\n", __func__);
			return;
		}
	}
	if (on) {
		if (regulator_enable(ext_5v_reg)) {
			pr_err("%s: Unable to enable the regulator:"
					" ext_5v_reg\n", __func__);
			return;
		}
		if (regulator_enable(votg_5v_switch)) {
			pr_err("%s: Unable to enable the regulator:"
					" votg_5v_switch\n", __func__);
			return;
		}
	} else {
		if (regulator_disable(votg_5v_switch))
			pr_err("%s: Unable to enable the regulator:"
				" votg_5v_switch\n", __func__);
		if (regulator_disable(ext_5v_reg))
			pr_err("%s: Unable to enable the regulator:"
				" ext_5v_reg\n", __func__);
	}

	vbus_is_on = on;
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
	.power_budget	= 390,
};
#endif

#ifdef CONFIG_BATTERY_MSM8X60
static int msm_hsusb_pmic_vbus_notif_init(void (*callback)(int online),
								int init)
{
	int ret = -ENOTSUPP;

#if defined(CONFIG_SMB137B_CHARGER) || defined(CONFIG_SMB137B_CHARGER_MODULE)
	if (machine_is_msm8x60_fluid()) {
		if (init)
			msm_charger_register_vbus_sn(callback);
		else
			msm_charger_unregister_vbus_sn(callback);
		return  0;
	}
#endif
	/* ID and VBUS lines are connected to pmic on 8660.V2.SURF,
	 * hence, irrespective of either peripheral only mode or
	 * OTG (host and peripheral) modes, can depend on pmic for
	 * vbus notifications
	 */
	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2)
			&& (machine_is_msm8x60_surf() ||
				pmic_id_notif_supported)) {
		if (init)
			ret = msm_charger_register_vbus_sn(callback);
		else {
			msm_charger_unregister_vbus_sn(callback);
			ret = 0;
		}
	} else {
#if !defined(CONFIG_USB_EHCI_MSM_72K)
	if (init)
		ret = msm_charger_register_vbus_sn(callback);
	else {
		msm_charger_unregister_vbus_sn(callback);
		ret = 0;
	}
#endif
	}
	return ret;
}
#endif

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_MSM_72K)
static struct msm_otg_platform_data msm_otg_pdata = {
	/* if usb link is in sps there is no need for
	 * usb pclk as dayatona fabric clock will be
	 * used instead
	 */
	.pclk_src_name		 = "dfab_usb_hs_clk",
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.se1_gating		 = SE1_GATING_DISABLE,
#ifdef CONFIG_USB_EHCI_MSM_72K
	.pmic_id_notif_init = msm_hsusb_pmic_id_notif_init,
#endif
#ifdef CONFIG_USB_EHCI_MSM_72K
	.vbus_power = msm_hsusb_vbus_power,
#endif
#ifdef CONFIG_BATTERY_MSM8X60
	.pmic_vbus_notif_init	= msm_hsusb_pmic_vbus_notif_init,
#endif
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.config_vddcx            = msm_hsusb_config_vddcx,
	.init_vddcx              = msm_hsusb_init_vddcx,
#ifdef CONFIG_BATTERY_MSM8X60
	.chg_vbus_draw = msm_charger_vbus_draw,
#endif
};
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.is_phy_status_timer_on = 1,
};
#endif

#ifdef CONFIG_LGE_DIAGTEST
// MOD 0009214: [DIAG] LG Diag feature added in side of android
static struct diagcmd_platform_data lg_fw_diagcmd_pdata = {
	.name = "lg_fw_diagcmd",
};

static struct platform_device lg_fw_diagcmd_device = {
	.name = "lg_fw_diagcmd",
	.id = -1,
	.dev = {
		.platform_data = &lg_fw_diagcmd_pdata
	},
};

static struct platform_device lg_diag_cmd_device = {
	.name = "lg_diag_cmd",
	.id = -1,
	.dev = {
		.platform_data = 0, //&lg_diag_cmd_pdata
	},
};
// END: 0009214 sehyuny.kim@lge.com 2010-09-03

#endif

#ifdef CONFIG_USB_ANDROID

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
static char *usb_functions_all[] = {
	"modem",
#ifdef CONFIG_LGE_USB_TWO_MODEM
	"nmea",
#endif
	"diag",
	"diag_mdm",
	"cdc_ethernet",
#ifdef CONFIG_LGE_USB_AUTORUN
	"cdrom",
#endif
	"usb_mass_storage",
	"adb",
};

#ifdef CONFIG_LGE_USB_TWO_MODEM
static char *usb_functions_twomodems[] = {
	"modem",
	"nmea",
	"diag",
	"diag_mdm",
	"cdc_ethernet",
#ifndef CONFIG_LGE_USB_VZW_DRIVER
	"usb_mass_storage",
#endif
	"adb",
};
#endif

static char *usb_functions_ndis[] = {
	"modem",
	"diag",
	"diag_mdm",
	"cdc_ethernet",
#ifndef CONFIG_LGE_USB_VZW_DRIVER
	"usb_mass_storage",
#endif
};

static char *usb_functions_ndis_adb[] = {
	"modem",
	"diag",
	"diag_mdm",
	"cdc_ethernet",
#ifndef CONFIG_LGE_USB_VZW_DRIVER
	"usb_mass_storage",
#endif
	"adb",
};

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};
#ifdef CONFIG_LGE_USB_AUTORUN
static char *usb_functions_cdrom[] = {
	"cdrom",
};
#endif

static char *usb_functions_factory[] = {
	"modem",
	"diag",
#ifdef CONFIG_LGE_USB_FACTORY
	"diag_mdm",
#endif
};

#else	/* below is original */
static char *usb_functions_default[] = {
	"diag",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_default_adb[] = {
	"diag",
	"adb",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *svlte2_usb_functions_default[] = {
	"diag",
	"diag_mdm",
	"modem",
	"nmea",
	"rmnet_smd_sdio",
	"usb_mass_storage",
};

static char *svlte2_usb_functions_default_adb[] = {
	"diag",
	"diag_mdm",
	"adb",
	"modem",
	"nmea",
	"rmnet_smd_sdio",
	"usb_mass_storage",
};

static char *charm_usb_acm_functions[] = {
	"diag",
	"diag_mdm",
	"acm1",
	"acm2",
	"rmnet_sdio",
	"usb_mass_storage",
};

static char *charm_usb_acm_functions_adb[] = {
	"diag",
	"diag_mdm",
	"adb",
	"acm1",
	"acm2",
	"rmnet_sdio",
	"usb_mass_storage",
};

static char *charm_usb_functions_default[] = {
	"diag",
	"diag_mdm",
	"modem",
	"nmea",
	"rmnet_sdio",
	"usb_mass_storage",
};

static char *charm_usb_functions_default_adb[] = {
	"diag",
	"diag_mdm",
	"adb",
	"modem",
	"nmea",
	"rmnet_sdio",
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
	"diag",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"diag",
	"adb",
};

static char *charm_usb_functions_rndis[] = {
	"rndis",
	"diag",
	"diag_mdm",
};

static char *charm_usb_functions_rndis_adb[] = {
	"rndis",
	"diag",
	"diag_mdm",
	"adb",
};

static char *usb_functions_mtp[] = {
	"mtp",
};

static char *usb_functions_mtp_adb[] = {
	"mtp",
	"adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
#ifdef CONFIG_USB_ANDROID_MTP
	"mtp",
#endif
	"adb",
#ifdef CONFIG_USB_F_SERIAL
	"modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	"rmnet",
#endif
	"usb_mass_storage",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};

static char *svlte2_usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
	"diag_mdm",
#endif
#ifdef CONFIG_USB_ANDROID_MTP
	"mtp",
#endif
	"adb",
#ifdef CONFIG_USB_F_SERIAL
	"modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	"acm1",
	"acm2",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET_SMD_SDIO
	"rmnet_smd_sdio",
#endif
	"usb_mass_storage",
};

static char *charm_usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
	"diag_mdm",
#endif
#ifdef CONFIG_USB_ANDROID_MTP
	"mtp",
#endif
	"adb",
#ifdef CONFIG_USB_F_SERIAL
	"modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	"acm1",
	"acm2",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET_SDIO
	"rmnet_sdio",
#endif
	"usb_mass_storage",
};
#endif /* CONFIG_LGE_USB_GADGET_DRIVER */

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
static struct android_usb_product usb_products[] = {
	{ /* modem + diag1 + diag2 + ndis + ums */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
		.product_id = 0x6204,
#else
		.product_id = 0x6315,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions_ndis),
		.functions	= usb_functions_ndis,
	},

	{ /* modem + diag1 + diag2 + ndis + ums +adb */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
		.product_id = 0x6204,
#else
		.product_id = 0x6315,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions_ndis_adb),
		.functions	= usb_functions_ndis_adb,
	},

#ifdef CONFIG_LGE_USB_TWO_MODEM
	{ /* modem + nmea(msm modem) + diag1 + diag2 + ndis + ums +adb */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
		.product_id = 0x6318,
#else
		.product_id = 0x6318,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions_twomodems),
		.functions	= usb_functions_twomodems,
	},
#endif
	{ /* ums only */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
		.product_id = 0x6205,
#else
		.product_id = 0x6320,
#endif
		.num_functions  = ARRAY_SIZE(usb_functions_ums),
		.functions  = usb_functions_ums,
	},
#ifdef CONFIG_LGE_USB_AUTORUN
	{ /* cdrom only */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
		.product_id = 0x6207,
#else
		.product_id = 0x630E,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions_cdrom),
		.functions	= usb_functions_cdrom,
	},
#endif
	{ /* factory mode */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
		.product_id = 0x6000,
#else
		.product_id = 0x6000,
#endif
		.num_functions  = ARRAY_SIZE(usb_functions_factory),
		.functions  = usb_functions_factory,
	},

};

#else	/* below is original */
static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0x9026,
		.num_functions	= ARRAY_SIZE(usb_functions_default),
		.functions	= usb_functions_default,
	},
	{
		.product_id	= 0x9025,
		.num_functions	= ARRAY_SIZE(usb_functions_default_adb),
		.functions	= usb_functions_default_adb,
	},
	{
		.product_id	= 0x902c,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= 0x902d,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
	{
		.product_id	= 0xF003,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
	},
	{
		.product_id	= 0x9039,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb),
		.functions	= usb_functions_mtp_adb,
	},
};

static struct android_usb_product svlte2_usb_products[] = {
	{
		.product_id	= 0x9038,
		.num_functions	= ARRAY_SIZE(svlte2_usb_functions_default),
		.functions	= svlte2_usb_functions_default,
	},
	{
		.product_id	= 0x9037,
		.num_functions	= ARRAY_SIZE(svlte2_usb_functions_default_adb),
		.functions	= svlte2_usb_functions_default_adb,
	},
	{
		.product_id	= 0x9041,
		.num_functions	= ARRAY_SIZE(charm_usb_functions_rndis),
		.functions	= charm_usb_functions_rndis,
	},
	{
		.product_id	= 0x9042,
		.num_functions	= ARRAY_SIZE(charm_usb_functions_rndis_adb),
		.functions	= charm_usb_functions_rndis_adb,
	},
	{
		.product_id	= 0xF003,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
	},
	{
		.product_id	= 0x9039,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb),
		.functions	= usb_functions_mtp_adb,
	},
	{
		.product_id	= 0x903C,
		.num_functions	= ARRAY_SIZE(charm_usb_acm_functions),
		.functions	= charm_usb_acm_functions,
	},
	{
		.product_id	= 0x903B,
		.num_functions	= ARRAY_SIZE(charm_usb_acm_functions_adb),
		.functions	= charm_usb_acm_functions_adb,
	},
};

#endif /* CONFIG_LGE_USB_GADGET_DRIVER */

#ifndef CONFIG_LGE_USB_GADGET_DRIVER
static struct android_usb_product charm_usb_products[] = {
	{
		.product_id	= 0x9032,
		.num_functions	= ARRAY_SIZE(charm_usb_functions_default),
		.functions	= charm_usb_functions_default,
	},
	{
		.product_id	= 0x9031,
		.num_functions	= ARRAY_SIZE(charm_usb_functions_default_adb),
		.functions	= charm_usb_functions_default_adb,
	},
	{
		.product_id	= 0x9041,
		.num_functions	= ARRAY_SIZE(charm_usb_functions_rndis),
		.functions	= charm_usb_functions_rndis,
	},
	{
		.product_id	= 0x9042,
		.num_functions	= ARRAY_SIZE(charm_usb_functions_rndis_adb),
		.functions	= charm_usb_functions_rndis_adb,
	},
	{
		.product_id	= 0xF003,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
	},
	{
		.product_id	= 0x9039,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb),
		.functions	= usb_functions_mtp_adb,
	},
	/* below acm functions are same for charm or svlte2 */
	{
		.product_id	= 0x903C,
		.num_functions	= ARRAY_SIZE(charm_usb_acm_functions),
		.functions	= charm_usb_acm_functions,
	},
	{
		.product_id	= 0x903B,
		.num_functions	= ARRAY_SIZE(charm_usb_acm_functions_adb),
		.functions	= charm_usb_acm_functions_adb,
	},
};
#endif /* CONFIG_LGE_USB_GADGET_DRIVER */

#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "LGE",
	.product        = "Android",
	.can_stall	= 1,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x05C6,
	.vendorDescr	= "Qualcomm Incorporated",
};

static struct platform_device rndis_device = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};
#endif /* CONFIG_USB_ANDROID_RNDIS */

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x1004,
  /* default product ID, make changes in android_probe func. */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
	.product_id	= 0x6204,
#else
	.product_id	= 0x6315,
#endif
	.version	= 0x0100,
	.product_name		= "LG Android USB Device",
	.manufacturer_name	= "LG Electronics Inc.",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.serial_number = "LGOTMS1234567890123456",
};
#else	/* below is original */
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x05C6,
	.product_id	= 0x9026,
	.version	= 0x0100,
	.product_name		= "Qualcomm HSUSB Device",
	.manufacturer_name	= "Qualcomm Incorporated",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.serial_number = "1234567890ABCDEF",
};
#endif /* CONFIG_LGE_USB_GADGET_DRIVER */

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

static int __init board_serialno_setup(char *serialno)
{
#ifdef CONFIG_USB_ANDROID_RNDIS
	int i;
	char *src = serialno;

	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}
	android_usb_pdata.serial_number = serialno;

#else
	if (strlen(serialno) > 15)
		sprintf(android_usb_pdata.serial_number, "LGOTMSOUTOFRANGE");
	else
		sprintf(android_usb_pdata.serial_number, "LGOTMS%s", serialno);
#endif /* CONFIG_USB_ANDROID_RNDIS */
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);
#endif /* CONFIG_USB_ANDROID */


#ifdef CONFIG_I2C_QUP
static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
}
#ifdef CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C
static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
       .clk_freq = 400000,
       .src_clk_rate = 24000000,
       .clk = "gsbi_qup_clk",
       .pclk = "gsbi_pclk",
       .msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

#define GSBI3_SDA 43
#define GSBI3_SCL 44

#define GSBI7_SDA 59
#define GSBI7_SCL 60

 
static uint32_t gsbi3_gpio_table[] = {
	GPIO_CFG(GSBI3_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(GSBI3_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi3_i2c_table[] = {
	GPIO_CFG(GSBI3_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(GSBI3_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi7_gpio_table[] = {
	GPIO_CFG(GSBI7_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(GSBI7_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi7_i2c_table[] = {
	GPIO_CFG(GSBI7_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(GSBI7_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static void gsbi3_qup_i2c_gpio_config(int adap_id, int config_type)
{
	if (config_type == 0) {
		gpio_tlmm_config(gsbi3_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi3_gpio_table[1], GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(gsbi3_i2c_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi3_i2c_table[1], GPIO_CFG_ENABLE);
	}
}
static void gsbi7_qup_i2c_gpio_config(int adap_id, int config_type)
{
	if (config_type == 0) {
		gpio_tlmm_config(gsbi7_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi7_gpio_table[1], GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(gsbi7_i2c_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi7_i2c_table[1], GPIO_CFG_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_gsbi3_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi3_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi4_qup_i2c_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

#if defined(CONFIG_LGE_FUEL_GAUGE) || defined(CONFIG_LGE_SWITCHING_CHARGER_MAX8971) || defined(CONFIG_LGE_SWITCHING_CHARGER_BQ24160_DOCOMO_ONLY)
static struct msm_i2c_platform_data msm_gsbi5_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

static struct msm_i2c_platform_data msm_gsbi7_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.pri_clk = 60,
	.pri_dat = 59,
	.msm_i2c_config_gpio = gsbi7_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi8_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi9_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

#if defined (CONFIG_LGE_SENSOR_DCOMPASS)|| defined (CONFIG_LGE_SENSOR_PROXIMITY)
static struct msm_i2c_platform_data msm_gsbi10_qup_i2c_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif
/* kwangdo.yi@lge.com [jointlab] Thu 14 Apr 2011 S
   add lge sensor config to fix build error
*/
#ifdef CONFIG_LGE_SENSOR
static struct msm_i2c_platform_data msm_gsbi12_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.use_gsbi_shared_mode = 1,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif
/* kwangdo.yi@lge.com [jointlab] Thu 14 Apr 2011 E */
#endif

#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
static struct msm_spi_platform_data msm_gsbi1_qup_spi_pdata = {
	.max_clock_speed = 24000000,
};

static struct msm_spi_platform_data msm_gsbi10_qup_spi_pdata = {
	.max_clock_speed = 24000000,
};
#endif

#ifdef CONFIG_I2C_SSBI
/* PMIC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi1_pdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
};

/* PMIC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi2_pdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
};

/* CODEC/TSSC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi3_pdata = {
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

#ifdef CONFIG_BATTERY_MSM
/* Use basic value for fake MSM battery */
static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.avail_chg_sources = AC_CHG,
};

static struct platform_device msm_batt_device = {
	.name              = "msm-battery",
	.id                = -1,
	.dev.platform_data = &msm_psy_batt_data,
};
#endif

/* Sensors DSPS platform data */
#ifdef CONFIG_MSM_DSPS

static struct dsps_gpio_info dsps_surf_gpios[] = {
	{
		.name = "compass_rst_n",
		.num = GPIO_COMPASS_RST_N,
		.on_val = 1,	/* device not in reset */
		.off_val = 0,	/* device in reset */
	},
	{
		.name = "gpio_r_altimeter_reset_n",
		.num = GPIO_R_ALTIMETER_RESET_N,
		.on_val = 1,	/* device not in reset */
		.off_val = 0,	/* device in reset */
	}
};

static struct dsps_gpio_info dsps_fluid_gpios[] = {
	{
		.name = "gpio_n_altimeter_reset_n",
		.num = GPIO_N_ALTIMETER_RESET_N,
		.on_val = 1,	/* device not in reset */
		.off_val = 0,	/* device in reset */
	}
};

static void __init msm8x60_init_dsps(void)
{
	struct msm_dsps_platform_data *pdata =
		msm_dsps_device.dev.platform_data;
	/*
	 * On Fluid the Compass sensor Chip-Select (CS) is directly connected
	 * to the power supply and not controled via GPIOs. Fluid uses a
	 * different IO-Expender (north) than used on surf/ffa.
	 */
	if (machine_is_msm8x60_fluid()) {
		pdata->gpios = dsps_fluid_gpios;
		pdata->gpios_num = ARRAY_SIZE(dsps_fluid_gpios);
	} else {
		pdata->gpios = dsps_surf_gpios;
		pdata->gpios_num = ARRAY_SIZE(dsps_surf_gpios);
	}
}
#endif /* CONFIG_MSM_DSPS */

#define MSM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */

#define MSM_PMEM_KERNEL_EBI1_SIZE  0x600000
#define MSM_PMEM_ADSP_SIZE         0x2000000
#define MSM_PMEM_AUDIO_SIZE        0x279000

#define MSM_SMI_BASE          0x38000000
/* Kernel SMI PMEM Region for video core, used for Firmware */
/* and encoder,decoder scratch buffers */
/* Kernel SMI PMEM Region Should always precede the user space */
/* SMI PMEM Region, as the video core will use offset address */
/* from the Firmware base */
#define PMEM_KERNEL_SMI_BASE  (MSM_SMI_BASE)
#define PMEM_KERNEL_SMI_SIZE  0x600000
/* User space SMI PMEM Region for video core*/
/* used for encoder, decoder input & output buffers  */
#define MSM_PMEM_SMIPOOL_BASE (PMEM_KERNEL_SMI_BASE + PMEM_KERNEL_SMI_SIZE)
#define MSM_PMEM_SMIPOOL_SIZE 0x3A00000

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
static unsigned pmem_kernel_ebi1_size = MSM_PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static int __init pmem_sf_size_setup(char *p)
{
	pmem_sf_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_sf_size", pmem_sf_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);
#endif

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};
#endif

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
static struct android_pmem_platform_data android_pmem_kernel_smi_pdata = {
	.name = PMEM_KERNEL_SMI_DATA_NAME,
	/* defaults to bitmap don't edit */
	.cached = 0,
};

static struct platform_device android_pmem_kernel_smi_device = {
	.name = "android_pmem",
	.id = 6,
	.dev = { .platform_data = &android_pmem_kernel_smi_pdata },
};
#endif

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

#define PMEM_BUS_WIDTH(_bw) \
	{ \
		.vectors = &(struct msm_bus_vectors){ \
			.src = MSM_BUS_MASTER_AMPSS_M0, \
			.dst = MSM_BUS_SLAVE_SMI, \
			.ib = (_bw), \
			.ab = 0, \
		}, \
	.num_paths = 1, \
	}
static struct msm_bus_paths pmem_smi_table[] = {
	[0] = PMEM_BUS_WIDTH(0), /* Off */
	[1] = PMEM_BUS_WIDTH(1), /* On */
};

static struct msm_bus_scale_pdata smi_client_pdata = {
	.usecase = pmem_smi_table,
	.num_usecases = ARRAY_SIZE(pmem_smi_table),
	.name = "pmem_smi",
};

void pmem_request_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 1);
}

void pmem_release_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 0);
}

void *pmem_setup_smi_region(void)
{
	return (void *)msm_bus_scale_register_client(&smi_client_pdata);
}
static struct android_pmem_platform_data android_pmem_smipool_pdata = {
	.name = "pmem_smipool",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.request_region = pmem_request_smi_region,
	.release_region = pmem_release_smi_region,
	.setup_region = pmem_setup_smi_region,
	.map_on_demand = 1,
};
static struct platform_device android_pmem_smipool_device = {
	.name = "android_pmem",
	.id = 7,
	.dev = { .platform_data = &android_pmem_smipool_pdata },
};

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource[] = {
	{
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resource),
	.resource       = ram_console_resource,
};
#if defined(CONFIG_LGE_HANDLE_PANIC)
static struct resource crash_log_resource[] = {
	{
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device panic_handler_device = {
	.name = "panic-handler",
	.id = -1,
	.num_resources  = ARRAY_SIZE(crash_log_resource),
	.resource       = crash_log_resource,
};
#endif // CONFIG_LGE_HANDLE_PANIC
#endif
#endif

static void __init msm8x60_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}
#endif

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
	size = PMEM_KERNEL_SMI_SIZE;
	if (size) {
		android_pmem_kernel_smi_pdata.start = PMEM_KERNEL_SMI_BASE;
		android_pmem_kernel_smi_pdata.size = size;
		pr_info("allocating %lu bytes at %lx physical for kernel"
			" smi pmem arena\n", size,
			(unsigned long) PMEM_KERNEL_SMI_BASE);
	}
#endif

#ifdef CONFIG_ANDROID_PMEM
	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = MSM_PMEM_SMIPOOL_SIZE;
	if (size) {
		android_pmem_smipool_pdata.start = MSM_PMEM_SMIPOOL_BASE;
		android_pmem_smipool_pdata.size = size;
		pr_info("allocating %lu bytes at %lx physical for user"
			" smi  pmem arena\n", size,
			(unsigned long) MSM_PMEM_SMIPOOL_BASE);
	}

	size = MSM_PMEM_AUDIO_SIZE;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_sf_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for sf "
			"pmem arena\n", size, addr, __pa(addr));
	}
#endif
}

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static void msm8x60_allocate_ram_dump_memory_regions(void)
{
	unsigned long long ram_size = 0x00040000 ;
	unsigned long long ram_base = 0x42DC0000; // lastest 44M refer. lk/target/[model]/atags.c  EBI1_ADDR_1026M+SIZE_44M
	int ret;

	ret = reserve_bootmem(ram_base, ram_size, BOOTMEM_EXCLUSIVE);
	if (ret < 0) {
		printk(KERN_INFO "ram_console reservation failed - "
		       "memory is in use (0x%lx) : 0x%08x \n", (unsigned long)ram_base , ret );
		return;
	}
	ram_console_resource[0].start = ram_base;
	ram_console_resource[0].end = ram_console_resource[0].start + ram_size -1;
}
#if defined(CONFIG_LGE_HANDLE_PANIC)
static void msm8x60_allocate_crash_callstack_regions(void)
{
	unsigned long long ram_size = 0x00040000 ;
	/* 0x42E00000 - ram console size - size for crash call stack store */
	unsigned long long ram_base = 0x42D80000;
	int ret;

	ret = reserve_bootmem(ram_base, ram_size, BOOTMEM_EXCLUSIVE);
	if (ret < 0) {
		printk(KERN_INFO "for crash call stack store reservation failed - "
			"memory is in use (0x%lx) : 0x%08x \n", (unsigned long)ram_base , ret );
		return;
	}
	crash_log_resource[0].start = ram_base;
	crash_log_resource[0].end = crash_log_resource[0].start + ram_size -1;
}
#endif // CONFIG_LGE_HANDLE_PANIC
#endif

#if defined(CONFIG_TOUCHSCREEN_CYTTSP_I2C) || \
		defined(CONFIG_TOUCHSCREEN_CYTTSP_I2C_MODULE)
/*virtual key support */
static ssize_t tma300_vkeys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":60:900:90:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":180:900:90:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":300:900:90:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":420:900:90:120"
	"\n");
}

static struct kobj_attribute tma300_vkeys_attr = {
	.attr = {
		.mode = S_IRUGO,
	},
	.show = &tma300_vkeys_show,
};

static struct attribute *tma300_properties_attrs[] = {
	&tma300_vkeys_attr.attr,
	NULL
};

static struct attribute_group tma300_properties_attr_group = {
	.attrs = tma300_properties_attrs,
};

static struct kobject *properties_kobj;

#define CYTTSP_TS_GPIO_IRQ	61
static int cyttsp_platform_init(struct i2c_client *client)
{
	int rc = -EINVAL;
	struct regulator *pm8058_l5 = NULL, *pm8058_s3;

	if (machine_is_msm8x60_fluid()) {
		pm8058_l5 = regulator_get(NULL, "8058_l5");
		if (IS_ERR(pm8058_l5)) {
			pr_err("%s: regulator get of 8058_l5 failed (%ld)\n",
				__func__, PTR_ERR(pm8058_l5));
			rc = PTR_ERR(pm8058_l5);
			return rc;
		}
		rc = regulator_set_voltage(pm8058_l5, 2850000, 2850000);
		if (rc) {
			pr_err("%s: regulator_set_voltage of 8058_l5 failed(%d)\n",
				__func__, rc);
			goto reg_l5_put;
		}

		rc = regulator_enable(pm8058_l5);
		if (rc) {
			pr_err("%s: regulator_enable of 8058_l5 failed(%d)\n",
				__func__, rc);
			goto reg_l5_put;
		}
	}
	/* vote for s3 to enable i2c communication lines */
	pm8058_s3 = regulator_get(NULL, "8058_s3");
	if (IS_ERR(pm8058_s3)) {
		pr_err("%s: regulator get of 8058_s3 failed (%ld)\n",
			__func__, PTR_ERR(pm8058_s3));
		rc = PTR_ERR(pm8058_s3);
		goto reg_l5_disable;
	}

	rc = regulator_set_voltage(pm8058_s3, 1800000, 1800000);
	if (rc) {
		pr_err("%s: regulator_set_voltage() = %d\n",
			__func__, rc);
		goto reg_s3_put;
	}

	rc = regulator_enable(pm8058_s3);
	if (rc) {
		pr_err("%s: regulator_enable of 8058_l5 failed(%d)\n",
			__func__, rc);
		goto reg_s3_put;
	}

	/* wait for vregs to stabilize */
	usleep_range(10000, 10000);

	/* check this device active by reading first byte/register */
	rc = i2c_smbus_read_byte_data(client, 0x01);
	if (rc < 0) {
		pr_err("%s: i2c sanity check failed\n", __func__);
		goto reg_s3_disable;
	}

	/* configure touchscreen interrupt gpio */
	rc = gpio_request(CYTTSP_TS_GPIO_IRQ, "cyttsp_irq_gpio");
	if (rc) {
		pr_err("%s: unable to request gpio %d\n",
			__func__, CYTTSP_TS_GPIO_IRQ);
		goto reg_s3_disable;
	}

	/* virtual keys */
	if (machine_is_msm8x60_fluid()) {
		tma300_vkeys_attr.attr.name = "virtualkeys.cyttsp-i2c";
		properties_kobj = kobject_create_and_add("board_properties",
					NULL);
		if (properties_kobj)
			rc = sysfs_create_group(properties_kobj,
				&tma300_properties_attr_group);
		if (!properties_kobj || rc)
			pr_err("%s: failed to create board_properties\n",
					__func__);
	}
	return CY_OK;

reg_s3_disable:
	regulator_disable(pm8058_s3);
reg_s3_put:
	regulator_put(pm8058_s3);
reg_l5_disable:
	if (machine_is_msm8x60_fluid())
		regulator_disable(pm8058_l5);
reg_l5_put:
	if (machine_is_msm8x60_fluid())
		regulator_put(pm8058_l5);
	return rc;
}

static int cyttsp_platform_resume(struct i2c_client *client)
{
	/* add any special code to strobe a wakeup pin or chip reset */
	msleep(10);

	return CY_OK;
}

static struct cyttsp_platform_data cyttsp_fluid_pdata = {
	.flags = 0x04,
	.gen = CY_GEN3,	/* or */
	.use_st = CY_USE_ST,
	.use_mt = CY_USE_MT,
	.use_hndshk = CY_SEND_HNDSHK,
	.use_trk_id = CY_USE_TRACKING_ID,
	.use_sleep = CY_USE_DEEP_SLEEP_SEL | CY_USE_LOW_POWER_SEL,
	.use_gestures = CY_USE_GESTURES,
	/* activate up to 4 groups
	 * and set active distance
	 */
	.gest_set = CY_GEST_GRP1 | CY_GEST_GRP2 |
				CY_GEST_GRP3 | CY_GEST_GRP4 |
				CY_ACT_DIST,
	/* change act_intrvl to customize the Active power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.act_intrvl = CY_ACT_INTRVL_DFLT,
	/* change tch_tmout to customize the touch timeout for the
	 * Active power state for Operating mode
	 */
	.tch_tmout = CY_TCH_TMOUT_DFLT,
	/* change lp_intrvl to customize the Low Power power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.lp_intrvl = CY_LP_INTRVL_DFLT,
	.resume = cyttsp_platform_resume,
	.init = cyttsp_platform_init,
};

static struct cyttsp_platform_data cyttsp_tmg240_pdata = {
	.panel_maxx = 1083,
	.panel_maxy = 659,
	.disp_minx = 30,
	.disp_maxx = 1053,
	.disp_miny = 30,
	.disp_maxy = 629,
	.correct_fw_ver = 8,
	.fw_fname = "cyttsp_8660_ffa.hex",
	.flags = 0x00,
	.gen = CY_GEN2,	/* or */
	.use_st = CY_USE_ST,
	.use_mt = CY_USE_MT,
	.use_hndshk = CY_SEND_HNDSHK,
	.use_trk_id = CY_USE_TRACKING_ID,
	.use_sleep = CY_USE_DEEP_SLEEP_SEL | CY_USE_LOW_POWER_SEL,
	.use_gestures = CY_USE_GESTURES,
	/* activate up to 4 groups
	 * and set active distance
	 */
	.gest_set = CY_GEST_GRP1 | CY_GEST_GRP2 |
				CY_GEST_GRP3 | CY_GEST_GRP4 |
				CY_ACT_DIST,
	/* change act_intrvl to customize the Active power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.act_intrvl = CY_ACT_INTRVL_DFLT,
	/* change tch_tmout to customize the touch timeout for the
	 * Active power state for Operating mode
	 */
	.tch_tmout = CY_TCH_TMOUT_DFLT,
	/* change lp_intrvl to customize the Low Power power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.lp_intrvl = CY_LP_INTRVL_DFLT,
	.resume = cyttsp_platform_resume,
	.init = cyttsp_platform_init,
	.disable_ghost_det = true;
};

static struct i2c_board_info cyttsp_fluid_info[] __initdata = {
	{
		I2C_BOARD_INFO(CY_I2C_NAME, 0x24),
		.platform_data = &cyttsp_fluid_pdata,
#ifndef CY_USE_TIMER
		.irq = MSM_GPIO_TO_INT(CYTTSP_TS_GPIO_IRQ),
#endif /* CY_USE_TIMER */
	},
};

static struct i2c_board_info cyttsp_fluid_info[] __initdata = {
	{
		I2C_BOARD_INFO(CY_I2C_NAME, 0x24),
		.platform_data = &cyttsp_fluid_pdata,
#ifndef CY_USE_TIMER
		.irq = MSM_GPIO_TO_INT(FLUID_CYTTSP_TS_GPIO_IRQ),
#endif /* CY_USE_TIMER */
	},
};

static struct i2c_board_info cyttsp_ffa_info[] __initdata = {
	{
		I2C_BOARD_INFO(CY_I2C_NAME, 0x3b),
		.platform_data = &cyttsp_tmg240_pdata,
#ifndef CY_USE_TIMER
		.irq = MSM_GPIO_TO_INT(CYTTSP_TS_GPIO_IRQ),
#endif /* CY_USE_TIMER */
	},
};
#endif

#ifdef CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C

#define SYNAPTICS_T1320_TS_I2C_SDA                 35
#define SYNAPTICS_T1320_TS_I2C_SCL                 36
#define SYNAPTICS_T1320_TS_I2C_INT_GPIO 61

int synaptics_t1320_power_on(int on, bool log_en)
{
	int rc = -EINVAL;
	static struct regulator *vreg_l19;
	static struct regulator *vreg_lvs2;

	if(log_en)
		printk(KERN_INFO "[Touch D] %s: power %s\n", __func__, on ? "On" : "Off");

	if(!vreg_l19)
	{
		vreg_l19 = regulator_get(NULL, "8058_l19");           /* +3V0_TOUCH */
		if (IS_ERR(vreg_l19)) {
			pr_err("%s: regulator get of 8058_l19 failed (%ld)\n", __func__, PTR_ERR(vreg_l19));
			rc = PTR_ERR(vreg_l19);
			return rc;
		}
	}
	rc = regulator_set_voltage(vreg_l19, 3000000, 3000000);

	if(!vreg_lvs2)
	{
		vreg_lvs2 = regulator_get(NULL, "8901_lvs2");         /* +1V8_TOUCH_VIO */
		if (IS_ERR(vreg_lvs2)) {
			pr_err("%s: regulator get of 8901_lvs2 failed (%ld)\n", __func__, PTR_ERR(vreg_lvs2));
			rc = PTR_ERR(vreg_lvs2);
			return rc;
		}
	}
	rc = regulator_set_voltage(vreg_lvs2, 1800000, 1800000);

	if(on)
	{
		rc = regulator_enable(vreg_l19);
		rc = regulator_enable(vreg_lvs2);
	}
	else
	{
		rc = regulator_disable(vreg_lvs2);
		rc = regulator_disable(vreg_l19);
	}

	return rc;
}

static struct synaptics_ts_platform_data synaptics_t1320_ts_platform_data[] = {
	{
		.use_irq        		= 1,
		.irqflags       		= IRQF_TRIGGER_FALLING,
		.i2c_sda_gpio   		= SYNAPTICS_T1320_TS_I2C_SDA,
		.i2c_scl_gpio			= SYNAPTICS_T1320_TS_I2C_SCL,
		.i2c_int_gpio			= SYNAPTICS_T1320_TS_I2C_INT_GPIO,
		.power					= synaptics_t1320_power_on,
		.ic_booting_delay		= 400,		/* ms */
		.report_period			= 12500000, 	/* 12.5 msec */
		.num_of_finger			= 10,
		.num_of_button			= 3,
		.button[0]				= KEY_MENU,
		.button[1]				= KEY_HOME,
		.button[2]				= KEY_BACK,
		.x_max					= 1110,
		.y_max					= 1973,
		.fw_ver					= 8,
		.palm_threshold			= 0,
		.delta_pos_threshold	= 0,
	},
};

static struct i2c_board_info msm_i2c_synaptics_ts_info[] = {
       {
               I2C_BOARD_INFO("synaptics_ts",      0x20),
               .platform_data = &synaptics_t1320_ts_platform_data,
               .irq = MSM_GPIO_TO_INT(SYNAPTICS_T1320_TS_I2C_INT_GPIO),
       }
};
#endif


/* eunmo.yang@lge.com [BSP/Sensor] 20110623 START */
#if defined (CONFIG_LGE_PMIC8058_REGULATOR) && defined (CONFIG_LGE_SENSOR_PROXIMITY)
static int sensor_power_pm8058_l15 = false;
struct regulator *pm8058_l15; //for Proximity, RPM_VREG_ID_PM8058_L15
static int power_set_for_8058_l15(unsigned char onoff)
{
	int rc = -EINVAL;
	printk(KERN_INFO "%s: prox/als power line: %d, onoff(%d)\n", __func__, __LINE__, onoff);

#if 0
	if(sensor_power_pm8058_l15 == onoff){
		printk(KERN_INFO "don't need to handle %s, onoff; %d", __func__, onoff);
		return 0;
	}	
#endif
	
	if(!pm8058_l15) {
		pm8058_l15 = regulator_get(NULL, "8058_l15");
		if (IS_ERR(pm8058_l15)) {
			pr_err("%s: line: %d, vreg_get failed (%ld)\n",
			__func__, __LINE__, PTR_ERR(pm8058_l15));
			rc = PTR_ERR(pm8058_l15);
			return rc;
		}
	}

	if (onoff) {
		rc = regulator_set_voltage(pm8058_l15, 2850000, 2850000);
		if (rc) {
			pr_err("%s: line: %d, unable to set pm8058_l15 voltage to 2.85 V\n",__func__,__LINE__);
			goto vreg_l15_fail;
		}
		rc = regulator_enable(pm8058_l15);
		if (rc) {
			pr_err("%s: line: %d, vreg_enable failed %d\n", __func__, __LINE__, rc);
			goto vreg_l15_fail;
		}
		sensor_power_pm8058_l15 = true;		
	} else {
		rc = regulator_disable(pm8058_l15);
		if (rc) {
			pr_err("%s: line: %d, vreg_disable failed %d\n",__func__, __LINE__, rc);
			goto vreg_l15_fail;
		}
		sensor_power_pm8058_l15 = false;				
	}

	return 0;

vreg_l15_fail:
	regulator_put(pm8058_l15);
	pm8058_l15 = NULL;
	return rc;	
}
#endif 
/* eunmo.yang@lge.com [BSP/Sensor] 20110623 END */

/* eunmo.yang@lge.com [BSP/Sensor] 20110623 START */
#ifdef CONFIG_LGE_SENSOR
#define GPIO_AXIS_I2C_SDA		72	//GPIO I2C, for Accelerometer, Gyroscope
#define GPIO_AXIS_I2C_SCL		73	//GPIO I2C, for Accelerometer, Gyroscope
#define GPIO_SENSOR_I2C_SDA		116 //GSBI12_1, for Digital Compass, Proximity
#define GPIO_SENSOR_I2C_SCL		115 //GSBI12_0, for Digital Compass, Proximity
#define GPIO_3AXIS_INT 			38
#define GPIO_GYRO_INT			37
#define GPIO_COMPASS_INT		39
#define GPIO_COMPASS_INT_1		124 
#define GPIO_PROXIMITY_OUT_INT	41
/* eunmo.yang@lge.com [BSP/Sensor] 20110623 END */


/* eunmo.yang@lge.com [BSP/Sensor] 20110623 START */
#ifdef CONFIG_LGE_SENSOR
static unsigned sensor_int_gpio[] = {GPIO_GYRO_INT, GPIO_3AXIS_INT, GPIO_COMPASS_INT};
static unsigned sensor_config_power_on[] = {
	GPIO_CFG(GPIO_GYRO_INT, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_3AXIS_INT, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_COMPASS_INT, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	
};

#if 0
static unsigned sensor_config_power_off[] = {
	GPIO_CFG(GPIO_GYRO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_3AXIS_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),		
	GPIO_CFG(GPIO_COMPASS_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),			
};
#endif

static void __init sensor_power_init(void)
{
	int rc;
	int i;

	printk(KERN_INFO "%s, line: %d\n", __func__, __LINE__);	
	rc = gpio_request(GPIO_GYRO_INT, "k3g_irq");
	if (rc)
	{
		printk(KERN_ERR "%s: gyro_int  %d request failed\n",__func__,GPIO_GYRO_INT );
		return;
	}

	rc = gpio_request(GPIO_3AXIS_INT, "k3dh_irq");
	if (rc)
	{
		printk(KERN_ERR "%s: 3axis_int  %d request failed\n",__func__,GPIO_3AXIS_INT );
		return;
	}
	
	rc = gpio_request(GPIO_COMPASS_INT, "ami306_irq");
	if (rc)
	{
		printk(KERN_ERR "%s: compass_int  %d request failed\n",__func__,GPIO_COMPASS_INT );
		return;
	} 

	for (i = 0; i < ARRAY_SIZE(sensor_config_power_on); i++)
	{
		rc = gpio_tlmm_config(sensor_config_power_on[i], GPIO_CFG_ENABLE);
		gpio_direction_input(sensor_int_gpio[i]);
		if (rc) 
		{
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=fg%d\n",__func__, sensor_config_power_on[i], rc);
			return;
		}
	}

}
#endif
/* eunmo.yang@lge.com [BSP/Sensor] 20110623 END */

#ifdef CONFIG_LGE_PMIC8058_REGULATOR
struct regulator *pm8058_l11; // for Sensor, +3V0_SENSOR 
static int power_set_for_8058_l11(unsigned char onoff)
{
	int rc = -EINVAL;
	
	if(!pm8058_l11) {
		pm8058_l11 = regulator_get(NULL, "8058_l11");
		if (IS_ERR(pm8058_l11)) {
			pr_err("%s: line: %d, vreg_get failed (%ld)\n",
			__func__, __LINE__, PTR_ERR(pm8058_l11));
			rc = PTR_ERR(pm8058_l11);
			return rc;
		}
	}	
	if (onoff) 
	{
		rc = regulator_set_voltage(pm8058_l11, 3000000, 3000000);
		if (rc) {
			pr_err("%s: line: %d, unable to set pm8058_l11 voltage to 3.0 V\n",__func__,__LINE__);
			goto vreg_l11_fail;
		}
		rc = regulator_enable(pm8058_l11);
		if (rc) {
			pr_err("%s: line: %d, vreg_enable failed %d\n", __func__, __LINE__, rc);
			goto vreg_l11_fail;
		}
	} 
	else 
	{
		rc = regulator_disable(pm8058_l11);
		if (rc) {
			pr_err("%s: line: %d, vreg_disable failed %d\n",__func__, __LINE__, rc);
			goto vreg_l11_fail;
		}
	}
	printk(KERN_INFO "%s: line: %d\n", __func__, __LINE__);
	return 0;
	
vreg_l11_fail:
	regulator_put(pm8058_l11);
	pm8058_l11 = NULL;
	return rc;	

}
#endif

/* eunmo.yang@lge.com [BSP/Sensor] 20110623 START */
#ifdef CONFIG_LGE_SENSOR
uint32_t sensor_pwr_mask = 0;
static int sensor_common_power_set(unsigned char on, int sensor)
{
	int ret = 0;
	printk(KERN_INFO "%s pwr_mask(%d), on(%d), sensor(%d)\n", __func__, sensor_pwr_mask, on, sensor); 

	if(on)
	{
		if(!sensor_pwr_mask)
		{
			ret = power_set_for_8058_l11(on);
			if(ret !=0)
				printk(KERN_ERR "%s, power on, pwr_mask=%d, sensor=%d\n", __func__,sensor_pwr_mask, sensor); 
		}
		sensor_pwr_mask |= sensor;
	}
	else 
	{
		if(sensor_pwr_mask)
		{
			sensor_pwr_mask &= ~sensor;

			if(!sensor_pwr_mask)
			{
				ret = power_set_for_8058_l11(on);
				if(ret !=0)
					printk(KERN_ERR "%s, power off, pwr_mask=%d, sensor=%d\n", __func__,sensor_pwr_mask, sensor); 
			}
		}
	}
	
	return ret;	
}
/* eunmo.yang@lge.com [BSP/Sensor] 20110623 END */

static int sensor_power_on(int sensor)
{
    int ret = 0;
	ret = sensor_common_power_set(1, sensor);
	return ret;
}

static int sensor_power_off(int sensor)
{
    int ret = 0;
	ret = sensor_common_power_set(0, sensor);
	return ret;	
}
#endif

#ifdef CONFIG_LGE_SENSOR_ACCELEROMETER
static int k3dh_init(void){return 0;}
static void k3dh_exit(void){}

struct k3dh_acc_platform_data accelerometer_pdata = {
	.poll_interval = 100,
	.min_interval = 0,
	.g_range = 0x00,
#if 0
	.axis_map_x = 1,
	.axis_map_y = 0,
	.axis_map_z = 2,
	.negate_x = 1,
	.negate_y = 1,
	.negate_z = 1,
#endif
	.init = k3dh_init,
	.exit = k3dh_exit,
	.power_on = sensor_power_on,
	.power_off = sensor_power_off,	
	.gpio_int1 = -1, //GPIO_3AXIS_INT,
	.gpio_int2 = -1,
};
#endif //CONFIG_LGE_SENSOR_ACCELEROMETER

#ifdef CONFIG_LGE_SENSOR_GYROSCOPE
static int k3g_init(void){return 0;}
static void k3g_exit(void){}
struct k3g_platform_data gyroscope_pdata = {
	.fs_range = 0x00 ,
#if 0
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
#endif
	.init = k3g_init,
	.exit = k3g_exit,
	.power_on = sensor_power_on,
	.power_off = sensor_power_off,
};
#endif //CONFIG_LGE_SENSOR_GYROSCOPE

/* eunmo.yang@lge.com [BSP/Sensor] 20110623 START */
#ifdef CONFIG_LGE_SENSOR_DCOMPASS
static int ami306_init(void){return 0;}
static void ami306_exit(void){}
static struct ami306_platform_data dcompss_pdata = {
	.init = ami306_init,
	.exit = ami306_exit,
	.power_on = sensor_power_on,
	.power_off = sensor_power_off,
#if 0
	.fdata_mDir   = 18,
	.fdata_sign_x = 1,
	.fdata_sign_y = 1,
	.fdata_sign_z = -1,
	.fdata_order0 = 1,
	.fdata_order1 = 0,
	.fdata_order2 = 2,
#endif
};
#endif //CONFIG_LGE_SENSOR_DCOMPASS

#ifdef CONFIG_LGE_SENSOR_PROXIMITY
static int sensor_proximity_power_set(unsigned char onoff)
{
	power_set_for_8058_l15(onoff);
	return 0;	
}
/* eunmo.yang@lge.com [BSP/Sensor] 20110623 END */

static struct apds9900_platform_data proximity_pdata = {
	.irq_num= GPIO_PROXIMITY_OUT_INT,
	.power = sensor_proximity_power_set,
	.prox_int_low_threshold = 0,
	.prox_int_high_threshold = 500,
	.als_threshold_hsyteresis = 30,
	.ppcount = 5,
	.B = 1870,
	.C = 0736,
	.D = 1330,
	.alsit = 146880,
	.ga_value = 2175,
	.df_value = 52,
};
#endif //CONFIG_LGE_SENSOR_PROXIMITY

#if defined (CONFIG_LGE_SENSOR_ACCELEROMETER)||defined (CONFIG_LGE_SENSOR_GYROSCOPE)
static struct i2c_board_info msm_i2c_gsbi12_info[] = {
	{
		I2C_BOARD_INFO("k3dh_acc_misc", 0x19),
		.irq =  -1,//MSM_GPIO_TO_INT(GPIO_3AXIS_INT),
		.platform_data = &accelerometer_pdata,
	},
	{
		I2C_BOARD_INFO("k3g", 0x69),
		.irq =  -1,//MSM_GPIO_TO_INT(GPIO_GYRO_INT),
		.platform_data = &gyroscope_pdata,
	},	
};
#endif

#if defined (CONFIG_LGE_SENSOR_DCOMPASS)|| defined (CONFIG_LGE_SENSOR_PROXIMITY) 
#define APDS9900_ADDRESS 0x39

static struct i2c_board_info msm_i2c_gsbi10_info[] = {
	{
		I2C_BOARD_INFO("ami306", 0x0E),
		.irq =  -1,//MSM_GPIO_TO_INT(GPIO_COMPASS_INT),
		.platform_data = &dcompss_pdata,
	},
	{
		I2C_BOARD_INFO("apds9900", APDS9900_ADDRESS),
		.irq =  MSM_GPIO_TO_INT(GPIO_PROXIMITY_OUT_INT),
		.platform_data = &proximity_pdata,
	},	
};
#endif
#endif //CONFIG_LGE_SENSOR

#if defined(CONFIG_TOUCHSCREEN_CY8C_TS)
static struct regulator *vreg_tmg200;

#define TS_PEN_IRQ_GPIO 61
static int tmg200_power(int vreg_on)
{
	int rc = -EINVAL;

	if (!vreg_tmg200) {
		printk(KERN_ERR "%s: regulator 8058_s3 not found (%d)\n",
			__func__, rc);
		return rc;
	}

	rc = vreg_on ? regulator_enable(vreg_tmg200) :
		  regulator_disable(vreg_tmg200);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg 8058_s3 %s failed (%d)\n",
				__func__, vreg_on ? "enable" : "disable", rc);

	/* wait for vregs to stabilize */
	msleep(100);
	return rc;
}

static int tmg200_dev_setup(bool enable)
{
	int rc;

	if (enable) {
		vreg_tmg200 = regulator_get(NULL, "8058_s3");
		if (IS_ERR(vreg_tmg200)) {
			pr_err("%s: regulator get of 8058_s3 failed (%ld)\n",
				__func__, PTR_ERR(vreg_tmg200));
			rc = PTR_ERR(vreg_tmg200);
			return rc;
		}

		rc = regulator_set_voltage(vreg_tmg200, 1800000, 1800000);
		if (rc) {
			pr_err("%s: regulator_set_voltage() = %d\n",
				__func__, rc);
			goto reg_put;
		}
	} else {
		/* put voltage sources */
		regulator_put(vreg_tmg200);
	}
	return 0;
reg_put:
	regulator_put(vreg_tmg200);
	return rc;
}

static struct cy8c_ts_platform_data cy8ctmg200_pdata = {
	.ts_name = "msm_tmg200_ts",
	.dis_min_x = 0,
	.dis_max_x = 1023,
	.dis_min_y = 0,
	.dis_max_y = 599,
	.min_tid = 1,
	.max_tid = 255,
	.min_touch = 0,
	.max_touch = 255,
	.min_width = 0,
	.max_width = 255,
	.power_on = tmg200_power,
	.dev_setup = tmg200_dev_setup,
	.nfingers = 2,
	.irq_gpio = TS_PEN_IRQ_GPIO,
	.resout_gpio = GPIO_CAP_TS_RESOUT_N,
};

static struct i2c_board_info cy8ctmg200_board_info[] = {
	{
		I2C_BOARD_INFO("cy8ctmg200", 0x2),
		.platform_data = &cy8ctmg200_pdata,
	}
};
#endif

//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
#if 0
#ifdef CONFIG_SERIAL_MSM_HS
static int configure_uart_gpios(int on)
{
	int ret = 0, i;
	int uart_gpios[] = {53, 54, 55, 56};
	for (i = 0; i < ARRAY_SIZE(uart_gpios); i++) {
		if (on) {
			ret = msm_gpiomux_get(uart_gpios[i]);
			if (unlikely(ret))
				break;
		} else {
			ret = msm_gpiomux_put(uart_gpios[i]);
			if (unlikely(ret))
				return ret;
		}
	}
	if (ret)
		for (; i >= 0; i--)
			msm_gpiomux_put(uart_gpios[i]);
	return ret;
}
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
       .gpio_config = configure_uart_gpios,
};
#endif
#endif
//[LGE_UPDATE_E] 20110420, sangyeol.lee@lge.com,  [END]


#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)

static struct gpio_led gpio_exp_leds_config[] = {
	{
		.name = "left_led1:green",
		.gpio = GPIO_LEFT_LED_1,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "left_led2:red",
		.gpio = GPIO_LEFT_LED_2,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "left_led3:green",
		.gpio = GPIO_LEFT_LED_3,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "wlan_led:orange",
		.gpio = GPIO_LEFT_LED_WLAN,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "left_led5:green",
		.gpio = GPIO_LEFT_LED_5,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "right_led1:green",
		.gpio = GPIO_RIGHT_LED_1,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "right_led2:red",
		.gpio = GPIO_RIGHT_LED_2,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "right_led3:green",
		.gpio = GPIO_RIGHT_LED_3,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "bt_led:blue",
		.gpio = GPIO_RIGHT_LED_BT,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
	{
		.name = "right_led5:green",
		.gpio = GPIO_RIGHT_LED_5,
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data gpio_leds_pdata = {
	.num_leds = ARRAY_SIZE(gpio_exp_leds_config),
	.leds = gpio_exp_leds_config,
};

static struct platform_device gpio_leds = {
	.name          = "leds-gpio",
	.id            = -1,
	.dev           = {
		.platform_data = &gpio_leds_pdata,
	},
};

static struct gpio_led fluid_gpio_leds[] = {
	{
		.name			= "dual_led:green",
		.gpio			= GPIO_LED1_GREEN_N,
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
		.active_low		= 1,
		.retain_state_suspended = 0,
	},
	{
		.name			= "dual_led:red",
		.gpio			= GPIO_LED2_RED_N,
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
		.active_low		= 1,
		.retain_state_suspended = 0,
	},
};

static struct gpio_led_platform_data gpio_led_pdata = {
	.leds		= fluid_gpio_leds,
	.num_leds	= ARRAY_SIZE(fluid_gpio_leds),
};

static struct platform_device fluid_leds_gpio = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_led_pdata,
	},
};

#endif

#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)

static struct msm_rpm_log_platform_data msm_rpm_log_pdata = {
	.phys_addr_base = 0x00106000,
	.reg_offsets = {
		[MSM_RPM_LOG_PAGE_INDICES] = 0x00000C80,
		[MSM_RPM_LOG_PAGE_BUFFER]  = 0x00000CA0,
	},
	.phys_size = SZ_8K,
	.log_len = 4096,		  /* log's buffer length in bytes */
	.log_len_mask = (4096 >> 2) - 1,  /* length mask in units of u32 */
};

static struct platform_device msm_rpm_log_device = {
	.name	= "msm_rpm_log",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_log_pdata,
	},
};
#endif

#ifdef CONFIG_BATTERY_MSM8X60
#ifdef CONFIG_LGE_CHARGER_VOLTAGE_CURRENT_SCENARIO
static struct msm_charger_platform_data msm_charger_data = {
	.safety_time = 480,
	.update_time = 1,
	.max_voltage = 4350,
	.min_voltage = 3500,
};
#else

static struct msm_charger_platform_data msm_charger_data = {
	.safety_time = 180,
	.update_time = 1,
	.max_voltage = 4200,
	.min_voltage = 3200,
};
#endif

static struct platform_device msm_charger_device = {
	.name = "msm-charger",
	.id = -1,
	.dev = {
		.platform_data = &msm_charger_data,
	}
};
#endif

// START sungchae.koo@lge.com 2011/06/24 P1_LAB_BSP : USB_CABLE_VALUE_TUNNELING {
int usb_cable_info = 0;

static int __init usb_cable_info_setup(char *usb_cable)
{
// START sungchae.koo@lge.com 2011/07/19 P1_LAB_BSP : ADAPTATION_USB_CABLE_INFO {
// cable value ref from "acc_cable_type@android\kernel\lge\include\lg_power_common.h"

    if(!strcmp(usb_cable, "pif_56k"))
        usb_cable_info = 6; // LT_CABLE_56K
    else if(!strcmp(usb_cable, "pif_130k"))
        usb_cable_info = 7; // LT_CABLE_130K
    else if(!strcmp(usb_cable, "pif_910k"))
        usb_cable_info = 11; // LT_CABLE_910K
    else if(!strcmp(usb_cable, "normal"))
        usb_cable_info = 8; // USB_CABLE_400MA
    else
        usb_cable_info = 8; // USB_CABLE_400MA        

	printk(KERN_INFO "usb_cable_info : %s (%d)\n", usb_cable, usb_cable_info);
// END sungchae.koo@lge.com 2011/07/19 P1_LAB_BSP }

	return 1;
}
__setup("lge.usb_cable=", usb_cable_info_setup);
// END sungchae.koo@lge.com 2011/06/24 P1_LAB_BSP }

// START sungchae.koo@lge.com 2011/08/31 P1_LAB_BSP {
#define RESET_MODE_NONE         0xFFFFFFFF
#define MODE_RESET_MODE         0x00000000
#define EMERGENCY_MODE_RESET    0x05011231
unsigned lge_reset_val = RESET_MODE_NONE;

static int __init lge_reset_setup(char *rst_str)
{
    if(!strcmp(rst_str, "emergency_rst"))
        lge_reset_val = EMERGENCY_MODE_RESET;
    else if(!strcmp(rst_str, "mode_reset"))
        lge_reset_val = MODE_RESET_MODE;
    else
        lge_reset_val = RESET_MODE_NONE;

	printk(KERN_INFO "lge_reset_setup : %s (%08X)\n", rst_str, lge_reset_val);

	return 1;
}
__setup("lge.reset=", lge_reset_setup);
// END sungchae.koo@lge.com 2011/08/31 P1_LAB_BSP }

static struct regulator_consumer_supply rpm_vreg_supply[RPM_VREG_ID_MAX] = {
	[RPM_VREG_ID_PM8058_L0]  = REGULATOR_SUPPLY("8058_l0", NULL),
	[RPM_VREG_ID_PM8058_L1]  = REGULATOR_SUPPLY("8058_l1", NULL),
	[RPM_VREG_ID_PM8058_L2]  = REGULATOR_SUPPLY("8058_l2", NULL),
	[RPM_VREG_ID_PM8058_L3]  = REGULATOR_SUPPLY("8058_l3", NULL),
	[RPM_VREG_ID_PM8058_L4]  = REGULATOR_SUPPLY("8058_l4", NULL),
	[RPM_VREG_ID_PM8058_L5]  = REGULATOR_SUPPLY("8058_l5", NULL),
	[RPM_VREG_ID_PM8058_L6]  = REGULATOR_SUPPLY("8058_l6", NULL),
	[RPM_VREG_ID_PM8058_L7]  = REGULATOR_SUPPLY("8058_l7", NULL),
	[RPM_VREG_ID_PM8058_L8]  = REGULATOR_SUPPLY("8058_l8", NULL),
	[RPM_VREG_ID_PM8058_L9]  = REGULATOR_SUPPLY("8058_l9", NULL),
	[RPM_VREG_ID_PM8058_L10] = REGULATOR_SUPPLY("8058_l10", NULL),
	[RPM_VREG_ID_PM8058_L11] = REGULATOR_SUPPLY("8058_l11", NULL),
	[RPM_VREG_ID_PM8058_L12] = REGULATOR_SUPPLY("8058_l12", NULL),
	[RPM_VREG_ID_PM8058_L13] = REGULATOR_SUPPLY("8058_l13", NULL),
	[RPM_VREG_ID_PM8058_L14] = REGULATOR_SUPPLY("8058_l14", NULL),
	[RPM_VREG_ID_PM8058_L15] = REGULATOR_SUPPLY("8058_l15", NULL),
	[RPM_VREG_ID_PM8058_L16] = REGULATOR_SUPPLY("8058_l16", NULL),
	[RPM_VREG_ID_PM8058_L17] = REGULATOR_SUPPLY("8058_l17", NULL),
	[RPM_VREG_ID_PM8058_L18] = REGULATOR_SUPPLY("8058_l18", NULL),
	[RPM_VREG_ID_PM8058_L19] = REGULATOR_SUPPLY("8058_l19", NULL),
	[RPM_VREG_ID_PM8058_L20] = REGULATOR_SUPPLY("8058_l20", NULL),
	[RPM_VREG_ID_PM8058_L21] = REGULATOR_SUPPLY("8058_l21", NULL),
	[RPM_VREG_ID_PM8058_L22] = REGULATOR_SUPPLY("8058_l22", NULL),
	[RPM_VREG_ID_PM8058_L23] = REGULATOR_SUPPLY("8058_l23", NULL),
	[RPM_VREG_ID_PM8058_L24] = REGULATOR_SUPPLY("8058_l24", NULL),
	[RPM_VREG_ID_PM8058_L25] = REGULATOR_SUPPLY("8058_l25", NULL),

	[RPM_VREG_ID_PM8058_S0] = REGULATOR_SUPPLY("8058_s0", NULL),
	[RPM_VREG_ID_PM8058_S1] = REGULATOR_SUPPLY("8058_s1", NULL),
	[RPM_VREG_ID_PM8058_S2] = REGULATOR_SUPPLY("8058_s2", NULL),
	[RPM_VREG_ID_PM8058_S3] = REGULATOR_SUPPLY("8058_s3", NULL),
	[RPM_VREG_ID_PM8058_S4] = REGULATOR_SUPPLY("8058_s4", NULL),

	[RPM_VREG_ID_PM8058_LVS0] = REGULATOR_SUPPLY("8058_lvs0", NULL),
	[RPM_VREG_ID_PM8058_LVS1] = REGULATOR_SUPPLY("8058_lvs1", NULL),

	[RPM_VREG_ID_PM8058_NCP] = REGULATOR_SUPPLY("8058_ncp", NULL),

	[RPM_VREG_ID_PM8901_L0]  = REGULATOR_SUPPLY("8901_l0",  NULL),
	[RPM_VREG_ID_PM8901_L1]  = REGULATOR_SUPPLY("8901_l1",  NULL),
	[RPM_VREG_ID_PM8901_L2]  = REGULATOR_SUPPLY("8901_l2",  NULL),
	[RPM_VREG_ID_PM8901_L3]  = REGULATOR_SUPPLY("8901_l3",  NULL),
	[RPM_VREG_ID_PM8901_L4]  = REGULATOR_SUPPLY("8901_l4",  NULL),
	[RPM_VREG_ID_PM8901_L5]  = REGULATOR_SUPPLY("8901_l5",  NULL),
	[RPM_VREG_ID_PM8901_L6]  = REGULATOR_SUPPLY("8901_l6",  NULL),

	[RPM_VREG_ID_PM8901_S2] = REGULATOR_SUPPLY("8901_s2", NULL),
	[RPM_VREG_ID_PM8901_S3] = REGULATOR_SUPPLY("8901_s3", NULL),
	[RPM_VREG_ID_PM8901_S4] = REGULATOR_SUPPLY("8901_s4", NULL),

	[RPM_VREG_ID_PM8901_LVS0] = REGULATOR_SUPPLY("8901_lvs0", NULL),
	[RPM_VREG_ID_PM8901_LVS1] = REGULATOR_SUPPLY("8901_lvs1", NULL),
	[RPM_VREG_ID_PM8901_LVS2] = REGULATOR_SUPPLY("8901_lvs2", NULL),
	[RPM_VREG_ID_PM8901_LVS3] = REGULATOR_SUPPLY("8901_lvs3", NULL),
	[RPM_VREG_ID_PM8901_MVS0] = REGULATOR_SUPPLY("8901_mvs0", NULL),
};

#define RPM_VREG_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, \
		      _default_uV, _peak_uA, _avg_uA, _pull_down, _pin_ctrl, \
		      _freq, _pin_fn, _rpm_mode, _state, _sleep_selectable, \
		      _always_on) \
	[RPM_VREG_ID_##_id] = { \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask = _modes, \
				.valid_ops_mask = _ops, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
				.input_uV = _min_uV, \
				.apply_uV = _apply_uV, \
				.always_on = _always_on, \
			}, \
			.num_consumer_supplies = 1, \
			.consumer_supplies = \
				&rpm_vreg_supply[RPM_VREG_ID_##_id], \
		}, \
		.default_uV = _default_uV, \
		.peak_uA = _peak_uA, \
		.avg_uA = _avg_uA, \
		.pull_down_enable = _pull_down, \
		.pin_ctrl = _pin_ctrl, \
		.freq = _freq, \
		.pin_fn = _pin_fn, \
		.mode = _rpm_mode, \
		.state = _state, \
		.sleep_selectable = _sleep_selectable, \
	}

/*
 * The default LPM/HPM state of an RPM controlled regulator can be controlled
 * via the peak_uA value specified in the table below.  If the value is less
 * than the high power min threshold for the regulator, then the regulator will
 * be set to LPM.  Otherwise, it will be set to HPM.
 *
 * This value can be further overridden by specifying an initial mode via
 * .init_data.constraints.initial_mode.
 */

#define RPM_VREG_INIT_LDO(_id, _always_on, _pd, _sleep_selectable, _min_uV, \
			  _max_uV, _init_peak_uA, _pin_ctrl) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_FAST | \
		      REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE | \
		      REGULATOR_MODE_STANDBY, REGULATOR_CHANGE_VOLTAGE | \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		      REGULATOR_CHANGE_DRMS, 0, _min_uV, _init_peak_uA, \
		      _init_peak_uA, _pd, _pin_ctrl, RPM_VREG_FREQ_NONE, \
		      RPM_VREG_PIN_FN_ENABLE, RPM_VREG_MODE_NONE, \
		      RPM_VREG_STATE_OFF, _sleep_selectable, _always_on)

#define RPM_VREG_INIT_LDO_PF(_id, _always_on, _pd, _sleep_selectable, _min_uV, \
			  _max_uV, _init_peak_uA, _pin_ctrl, _pin_fn) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_FAST | \
		      REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE | \
		      REGULATOR_MODE_STANDBY, REGULATOR_CHANGE_VOLTAGE | \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		      REGULATOR_CHANGE_DRMS, 0, _min_uV, _init_peak_uA, \
		      _init_peak_uA, _pd, _pin_ctrl, RPM_VREG_FREQ_NONE, \
		      _pin_fn, RPM_VREG_MODE_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define RPM_VREG_INIT_SMPS(_id, _always_on, _pd, _sleep_selectable, _min_uV, \
			   _max_uV, _init_peak_uA, _pin_ctrl, _freq) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_FAST | \
		      REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE | \
		      REGULATOR_MODE_STANDBY, REGULATOR_CHANGE_VOLTAGE | \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		      REGULATOR_CHANGE_DRMS, 0, _min_uV, _init_peak_uA, \
		      _init_peak_uA, _pd, _pin_ctrl, _freq, \
		      RPM_VREG_PIN_FN_ENABLE, RPM_VREG_MODE_NONE, \
		      RPM_VREG_STATE_OFF, _sleep_selectable, _always_on)

#define RPM_VREG_INIT_VS(_id, _always_on, _pd, _sleep_selectable, _pin_ctrl) \
	RPM_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE, \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE, 0, 0, \
		      1000, 1000, _pd, _pin_ctrl, RPM_VREG_FREQ_NONE, \
		      RPM_VREG_PIN_FN_ENABLE, RPM_VREG_MODE_NONE, \
		      RPM_VREG_STATE_OFF, _sleep_selectable, _always_on)

#define RPM_VREG_INIT_NCP(_id, _always_on, _pd, _sleep_selectable, _min_uV, \
			  _max_uV, _pin_ctrl) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_NORMAL, \
		      REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS, 0, \
		      _min_uV, 1000, 1000, _pd, _pin_ctrl, RPM_VREG_FREQ_NONE, \
		      RPM_VREG_PIN_FN_ENABLE, RPM_VREG_MODE_NONE, \
		      RPM_VREG_STATE_OFF, _sleep_selectable, _always_on)

#define LDO50HMIN	RPM_VREG_LDO_50_HPM_MIN_LOAD
#define LDO150HMIN	RPM_VREG_LDO_150_HPM_MIN_LOAD
#define LDO300HMIN	RPM_VREG_LDO_300_HPM_MIN_LOAD
#define SMPS_HMIN	RPM_VREG_SMPS_HPM_MIN_LOAD
#define FTS_HMIN	RPM_VREG_FTSMPS_HPM_MIN_LOAD

static struct rpm_vreg_pdata rpm_vreg_init_pdata[RPM_VREG_ID_MAX] = {
#ifdef CONFIG_LGE_PMIC8058_REGULATOR
	RPM_VREG_INIT_LDO_PF(PM8058_L0,  0, 1, 0, 1200000, 1200000, LDO150HMIN,
			RPM_VREG_PIN_CTRL_NONE, RPM_VREG_PIN_FN_SLEEP_B), /* +1V2_MSM_A2 : not output */
	RPM_VREG_INIT_LDO(PM8058_L1,  0, 1, 0, 1200000, 1200000, LDO300HMIN, 0), /* +1V2_CAM_VDIG */ // jisun.shin@lge.com
	RPM_VREG_INIT_LDO(PM8058_L2,  0, 1, 0, 1800000, 2600000, LDO300HMIN, 0), /* GND */
	RPM_VREG_INIT_LDO(PM8058_L3,  0, 1, 0, 3300000, 3300000, LDO150HMIN, 0), /* +3V3_MHL */
	RPM_VREG_INIT_LDO(PM8058_L4,  0, 1, 0, 2850000, 2850000,  LDO50HMIN, 0), /* +2V85_PM_TCXO */
	RPM_VREG_INIT_LDO(PM8058_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN, 0), /* +2V85_MSM_P2 */
	/* [START] hansun.lee@lge.com 2011-06-27, Change 3.075V to 3.5V at USB1.1(56K) */
#ifdef CONFIG_LGE_USB_FACTORY
	RPM_VREG_INIT_LDO(PM8058_L6,  0, 1, 0, 3050000, 3500000,  LDO50HMIN, 0),	/*+3V075_USB*/
#else
	RPM_VREG_INIT_LDO(PM8058_L6,  0, 1, 0, 3050000, 3050000,  LDO50HMIN, 0), /* +3V05_MSM_USB */
#endif
	/* [START] hansun.lee@lge.com 2011-06-27 */
	RPM_VREG_INIT_LDO(PM8058_L7,  0, 1, 0, 1800000, 1800000,  LDO50HMIN, 0), /* +1V8_USB_VIO */
	RPM_VREG_INIT_LDO(PM8058_L8,  0, 1, 0, 2800000, 2800000, LDO300HMIN, 0), /* +2V8_CAM_VCM */ // jisun.shin@lge.com
	RPM_VREG_INIT_LDO(PM8058_L9,  0, 1, 0, 2800000, 2800000, LDO300HMIN, 0), /* +2V8_CAM_VDD */ //jisun.shin@lge.com
	RPM_VREG_INIT_LDO(PM8058_L10, 0, 1, 0, 2600000, 2600000, LDO300HMIN, 0), /* GND */
	RPM_VREG_INIT_LDO(PM8058_L11, 0, 1, 0, 3000000, 3000000, LDO150HMIN, 0), /* +3V0_SENSORS */
	RPM_VREG_INIT_LDO(PM8058_L12, 0, 1, 0, 1800000, 1800000, LDO150HMIN, 0), /* +1V8_MHL */
	RPM_VREG_INIT_LDO(PM8058_L13, 0, 1, 0, 2050000, 2050000, LDO300HMIN, 0), /* +2V05_MSM_A5 */
	RPM_VREG_INIT_LDO(PM8058_L14, 0, 0, 0, 2850000, 2850000, LDO300HMIN, 0), /* +2V85_MICRO_SD */
	RPM_VREG_INIT_LDO(PM8058_L15, 0, 1, 0, 2850000, 2850000, LDO300HMIN, 0), /* +2V85_PROX_AMBIENT */
	RPM_VREG_INIT_LDO(PM8058_L16, 1, 1, 1, 1800000, 1800000, LDO300HMIN, 0), /* +1V8_MSM_A4 */
        RPM_VREG_INIT_LDO(PM8058_L17, 0, 1, 0, 2850000, 2850000, LDO150HMIN, 0), /* +2V85_GPS */
	RPM_VREG_INIT_LDO(PM8058_L18, 0, 1, 1, 2200000, 2200000, LDO150HMIN, 0), /* +2V2_XOADC_REF */
	RPM_VREG_INIT_LDO(PM8058_L19, 0, 1, 0, 3000000, 3000000, LDO150HMIN, 0), /* +3V0_TOUCH */
	RPM_VREG_INIT_LDO(PM8058_L20, 0, 1, 0, 0, 0, LDO150HMIN, 0), /* NC */
	RPM_VREG_INIT_LDO_PF(PM8058_L21, 1, 1, 0, 1100000, 1100000, LDO150HMIN,
			RPM_VREG_PIN_CTRL_NONE, RPM_VREG_PIN_FN_SLEEP_B), /* +1V1_MSM_PLL */
	RPM_VREG_INIT_LDO(PM8058_L22, 0, 1, 0, 1200000, 1200000, LDO300HMIN, 0), /* +1V2_DDR2_BACKUP */
	RPM_VREG_INIT_LDO(PM8058_L23, 0, 1, 0, 1200000, 1200000, LDO300HMIN, 0), /* +1V2 MHL */
	RPM_VREG_INIT_LDO(PM8058_L24, 0, 1, 0, 0, 0, LDO150HMIN, 0), /* NC */
	RPM_VREG_INIT_LDO(PM8058_L25, 0, 1, 0, 0, 0, LDO150HMIN, 0), /* NC */

	RPM_VREG_INIT_SMPS(PM8058_S0, 0, 1, 1,  500000, 1250000,  SMPS_HMIN, 0,
			RPM_VREG_FREQ_1p60), /* +1V1_MSM_MX */ /* 1100000 not working */
	RPM_VREG_INIT_SMPS(PM8058_S1, 0, 1, 1,  500000, 1250000,  SMPS_HMIN, 0,
			RPM_VREG_FREQ_1p60), /* +1V1_MSM_CX */ /* 1100000 not working */
	RPM_VREG_INIT_SMPS(PM8058_S2, 0, 1, 0, 1200000, 1400000,  SMPS_HMIN,
			RPM_VREG_PIN_CTRL_A0, RPM_VREG_FREQ_1p60), /* +1V3_MSM_A3 */ /* 1300000 not working */
	RPM_VREG_INIT_SMPS(PM8058_S3, 1, 1, 0, 1800000, 1800000,  SMPS_HMIN, 0,
			RPM_VREG_FREQ_1p60), /* +1V8_MSM_VIO */
	RPM_VREG_INIT_SMPS(PM8058_S4, 1, 1, 0, 2200000, 2200000,  SMPS_HMIN, 0,
			RPM_VREG_FREQ_1p60), /* +2V2_QTR */

	RPM_VREG_INIT_VS(PM8058_LVS0, 0, 1, 0,                           0), /* +1.8V_CAM_VDDIO */ //jisun.shin@lge.com
	RPM_VREG_INIT_VS(PM8058_LVS1, 0, 1, 0,                           0), /* +1V8_MSM_QFUSE */

	RPM_VREG_INIT_NCP(PM8058_NCP, 0, 1, 0, 1800000, 1800000,         0), /* -1V8_NCP */
#else
	RPM_VREG_INIT_LDO_PF(PM8058_L0,  0, 1, 0, 1200000, 1200000, LDO150HMIN,
		RPM_VREG_PIN_CTRL_NONE, RPM_VREG_PIN_FN_SLEEP_B),
	RPM_VREG_INIT_LDO(PM8058_L1,  0, 1, 0, 1200000, 1200000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L2,  0, 1, 0, 1800000, 2600000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L3,  0, 1, 0, 1800000, 1800000, LDO150HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L4,  0, 1, 0, 2850000, 2850000,  LDO50HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L6,  0, 1, 0, 3000000, 3600000,  LDO50HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L7,  0, 1, 0, 1800000, 1800000,  LDO50HMIN, 0),
	RPM_VREG_INIT_LDO_PF(PM8058_L8,  0, 1, 0, 2900000, 3050000, LDO300HMIN,
		RPM_VREG_PIN_CTRL_NONE, RPM_VREG_PIN_FN_SLEEP_B),
	RPM_VREG_INIT_LDO(PM8058_L9,  0, 1, 0, 1800000, 1800000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L10, 0, 1, 0, 2600000, 2600000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L11, 0, 1, 0, 1500000, 1500000, LDO150HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L12, 0, 1, 0, 2900000, 2900000, LDO150HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L13, 0, 1, 0, 2050000, 2050000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L14, 0, 0, 0, 2850000, 2850000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L15, 0, 1, 0, 2850000, 2850000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L16, 1, 1, 1, 1800000, 1800000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L17, 0, 1, 0, 3000000, 3000000, LDO150HMIN, 0), /* +3V0_TOUCH */
	RPM_VREG_INIT_LDO(PM8058_L18, 0, 1, 1, 2200000, 2200000, LDO150HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L19, 0, 1, 0, 2500000, 2500000, LDO150HMIN, 0),
	RPM_VREG_INIT_LDO_PF(PM8058_L20, 0, 1, 0, 1800000, 1800000, LDO150HMIN,
		RPM_VREG_PIN_CTRL_NONE, RPM_VREG_PIN_FN_SLEEP_B),
	RPM_VREG_INIT_LDO_PF(PM8058_L21, 1, 1, 0, 1200000, 1200000, LDO150HMIN,
		RPM_VREG_PIN_CTRL_NONE, RPM_VREG_PIN_FN_SLEEP_B),
	RPM_VREG_INIT_LDO(PM8058_L22, 0, 1, 0, 1200000, 1200000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L23, 0, 1, 0, 1200000, 1200000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L24, 0, 1, 0, 1200000, 1200000, LDO150HMIN, 0),
	RPM_VREG_INIT_LDO(PM8058_L25, 0, 1, 0, 1200000, 1200000, LDO150HMIN, 0),

	RPM_VREG_INIT_SMPS(PM8058_S0, 0, 1, 1,  500000, 1250000,  SMPS_HMIN, 0,
		RPM_VREG_FREQ_1p60),
	RPM_VREG_INIT_SMPS(PM8058_S1, 0, 1, 1,  500000, 1250000,  SMPS_HMIN, 0,
		RPM_VREG_FREQ_1p60),
	RPM_VREG_INIT_SMPS(PM8058_S2, 0, 1, 1, 1200000, 1400000,  SMPS_HMIN,
		RPM_VREG_PIN_CTRL_A0, RPM_VREG_FREQ_1p60),
	RPM_VREG_INIT_SMPS(PM8058_S3, 1, 1, 0, 1800000, 1800000,  SMPS_HMIN, 0,
		RPM_VREG_FREQ_1p60),
	RPM_VREG_INIT_SMPS(PM8058_S4, 1, 1, 0, 2200000, 2200000,  SMPS_HMIN, 0,
		RPM_VREG_FREQ_1p60),

	RPM_VREG_INIT_VS(PM8058_LVS0, 0, 1, 0,				 0),
	RPM_VREG_INIT_VS(PM8058_LVS1, 0, 1, 0,				 0),

	RPM_VREG_INIT_NCP(PM8058_NCP, 0, 1, 0, 1800000, 1800000,	 0),
#endif // CONFIG_LGE_PMIC8058_REGULATOR

#ifdef CONFIG_LGE_PMIC8901_REGULATOR
	RPM_VREG_INIT_LDO(PM8901_L0,  0, 1, 0, 0, 0, LDO300HMIN,
			RPM_VREG_PIN_CTRL_A0), /* NC */
	RPM_VREG_INIT_LDO(PM8901_L1,  0, 1, 0, 3000000, 3000000, LDO300HMIN, 0), /* 3V0_MOTOR_VIB */ //sungwoo.cho@lge.com
	RPM_VREG_INIT_LDO(PM8901_L2,  0, 1, 0, 3000000, 3000000, LDO300HMIN, 0), /* +3V0_LCD_VCC */
	RPM_VREG_INIT_LDO(PM8901_L3,  0, 1, 0, 3000000, 3000000, LDO300HMIN, 0), /* +3V0_LCD_VCI */
	RPM_VREG_INIT_LDO(PM8901_L4,  0, 1, 0, 2800000, 2800000, LDO300HMIN, 0), /* 2V8_VTCAM_AVDD */ //jisun.shin@lge.com
	RPM_VREG_INIT_LDO(PM8901_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN, 0), /* +2V85_eMMC */
	RPM_VREG_INIT_LDO(PM8901_L6,  0, 1, 0, 0, 0, LDO300HMIN, 0), /* NC */

	RPM_VREG_INIT_SMPS(PM8901_S2, 0, 1, 0, 0, 0,   FTS_HMIN, 0,
			RPM_VREG_FREQ_1p60), /* NC */
	RPM_VREG_INIT_SMPS(PM8901_S3, 0, 1, 0, 1100000, 1100000,   FTS_HMIN, 0,
			RPM_VREG_FREQ_1p60), /* +1V1_MSM_QDSP */
	RPM_VREG_INIT_SMPS(PM8901_S4, 0, 1, 0, 1200000, 1200000,   FTS_HMIN,
			RPM_VREG_PIN_CTRL_A0, RPM_VREG_FREQ_1p60), /* +1V2_MSM_P1 */

	RPM_VREG_INIT_VS(PM8901_LVS0, 1, 1, 0,                           0), /* +1V8_eMMC_VIO */
	RPM_VREG_INIT_VS(PM8901_LVS1, 0, 1, 0,                           0), /* +1V8_VTCAM_IOVDD */
	RPM_VREG_INIT_VS(PM8901_LVS2, 0, 1, 0,                           0),    /* +1V8_TOUCH_VIO */
	RPM_VREG_INIT_VS(PM8901_LVS3, 0, 1, 0,                           0),    /* 1V8_VTCAM_DVDD */ //jisun.shin@lge.com
	RPM_VREG_INIT_VS(PM8901_MVS0, 0, 1, 0,                           0), /* +1V8_LCD_VIO */
#else
	RPM_VREG_INIT_LDO(PM8901_L0,  0, 1, 0, 1200000, 1200000, LDO300HMIN,
		RPM_VREG_PIN_CTRL_A0),
	RPM_VREG_INIT_LDO(PM8901_L1,  0, 1, 0, 3300000, 3300000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8901_L2,  0, 1, 0, 2850000, 3300000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8901_L3,  0, 1, 0, 3300000, 3300000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8901_L4,  0, 1, 0, 2600000, 2600000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8901_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN, 0),
	RPM_VREG_INIT_LDO(PM8901_L6,  0, 1, 0, 2200000, 2200000, LDO300HMIN, 0),

	RPM_VREG_INIT_SMPS(PM8901_S2, 0, 1, 0, 1300000, 1300000,   FTS_HMIN, 0,
		RPM_VREG_FREQ_1p60),
	RPM_VREG_INIT_SMPS(PM8901_S3, 0, 1, 0, 1100000, 1100000,   FTS_HMIN, 0,
		RPM_VREG_FREQ_1p60),
	RPM_VREG_INIT_SMPS(PM8901_S4, 0, 1, 0, 1225000, 1225000,   FTS_HMIN,
		RPM_VREG_PIN_CTRL_A0, RPM_VREG_FREQ_1p60),

	RPM_VREG_INIT_VS(PM8901_LVS0, 1, 1, 0,				 0),
	RPM_VREG_INIT_VS(PM8901_LVS1, 0, 1, 0,				 0),
	RPM_VREG_INIT_VS(PM8901_LVS2, 0, 1, 0,				 0),
	RPM_VREG_INIT_VS(PM8901_LVS3, 0, 1, 0,				 0),
	RPM_VREG_INIT_VS(PM8901_MVS0, 0, 1, 0,				 0),
#endif // CONFIG_LGE_PMIC8901_REGULATOR
};

#define RPM_VREG(_id) \
	[_id] = { \
		.name = "rpm-regulator", \
		.id = _id, \
		.dev = { \
			.platform_data = &rpm_vreg_init_pdata[_id], \
		}, \
	}

static struct platform_device rpm_vreg_device[RPM_VREG_ID_MAX] = {
	RPM_VREG(RPM_VREG_ID_PM8058_L0),
	RPM_VREG(RPM_VREG_ID_PM8058_L1),
	RPM_VREG(RPM_VREG_ID_PM8058_L2),
	RPM_VREG(RPM_VREG_ID_PM8058_L3),
	RPM_VREG(RPM_VREG_ID_PM8058_L4),
	RPM_VREG(RPM_VREG_ID_PM8058_L5),
	RPM_VREG(RPM_VREG_ID_PM8058_L6),
	RPM_VREG(RPM_VREG_ID_PM8058_L7),
	RPM_VREG(RPM_VREG_ID_PM8058_L8),
	RPM_VREG(RPM_VREG_ID_PM8058_L9),
	RPM_VREG(RPM_VREG_ID_PM8058_L10),
	RPM_VREG(RPM_VREG_ID_PM8058_L11),
	RPM_VREG(RPM_VREG_ID_PM8058_L12),
	RPM_VREG(RPM_VREG_ID_PM8058_L13),
	RPM_VREG(RPM_VREG_ID_PM8058_L14),
	RPM_VREG(RPM_VREG_ID_PM8058_L15),
	RPM_VREG(RPM_VREG_ID_PM8058_L16),
	RPM_VREG(RPM_VREG_ID_PM8058_L17),
	RPM_VREG(RPM_VREG_ID_PM8058_L18),
	RPM_VREG(RPM_VREG_ID_PM8058_L19),
	RPM_VREG(RPM_VREG_ID_PM8058_L20),
	RPM_VREG(RPM_VREG_ID_PM8058_L21),
	RPM_VREG(RPM_VREG_ID_PM8058_L22),
	RPM_VREG(RPM_VREG_ID_PM8058_L23),
	RPM_VREG(RPM_VREG_ID_PM8058_L24),
	RPM_VREG(RPM_VREG_ID_PM8058_L25),
	RPM_VREG(RPM_VREG_ID_PM8058_S0),
	RPM_VREG(RPM_VREG_ID_PM8058_S1),
	RPM_VREG(RPM_VREG_ID_PM8058_S2),
	RPM_VREG(RPM_VREG_ID_PM8058_S3),
	RPM_VREG(RPM_VREG_ID_PM8058_S4),
	RPM_VREG(RPM_VREG_ID_PM8058_LVS0),
	RPM_VREG(RPM_VREG_ID_PM8058_LVS1),
	RPM_VREG(RPM_VREG_ID_PM8058_NCP),
	RPM_VREG(RPM_VREG_ID_PM8901_L0),
	RPM_VREG(RPM_VREG_ID_PM8901_L1),
	RPM_VREG(RPM_VREG_ID_PM8901_L2),
	RPM_VREG(RPM_VREG_ID_PM8901_L3),
	RPM_VREG(RPM_VREG_ID_PM8901_L4),
	RPM_VREG(RPM_VREG_ID_PM8901_L5),
	RPM_VREG(RPM_VREG_ID_PM8901_L6),
	RPM_VREG(RPM_VREG_ID_PM8901_S2),
	RPM_VREG(RPM_VREG_ID_PM8901_S3),
	RPM_VREG(RPM_VREG_ID_PM8901_S4),
	RPM_VREG(RPM_VREG_ID_PM8901_LVS0),
	RPM_VREG(RPM_VREG_ID_PM8901_LVS1),
	RPM_VREG(RPM_VREG_ID_PM8901_LVS2),
	RPM_VREG(RPM_VREG_ID_PM8901_LVS3),
	RPM_VREG(RPM_VREG_ID_PM8901_MVS0),
};

static struct platform_device *early_regulators[] __initdata = {
	&msm_device_saw_s0,
	&msm_device_saw_s1,
#ifdef CONFIG_PMIC8058
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S0],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S1],
#endif
};

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_MSM_BUS_SCALING
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
#endif
	&msm_device_dmov_adm0,
	&msm_device_dmov_adm1,
};

//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
#if 0
#if (defined(CONFIG_MARIMBA_CORE)) && \
	(defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))

static int bluetooth_power(int);
static struct platform_device msm_bt_power_device = {
	.name	 = "bt_power",
	.id	 = -1,
	.dev	 = {
		.platform_data = &bluetooth_power,
	},
};
#endif
#endif
//[LGE_UPDATE_E] 20110420, sangyeol.lee@lge.com,  [END]

static struct platform_device msm_tsens_device = {
	.name   = "tsens-tm",
	.id = -1,
};

#ifdef CONFIG_SENSORS_MSM_ADC
static struct resource resources_adc[] = {
	{
		.start = PM8058_ADC_IRQ(PM8058_IRQ_BASE),
		.end   = PM8058_ADC_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
};

static struct adc_access_fn xoadc_fn = {
	pm8058_xoadc_select_chan_and_start_conv,
	pm8058_xoadc_read_adc_code,
	pm8058_xoadc_get_properties,
	pm8058_xoadc_slot_request,
	pm8058_xoadc_restore_slot,
	pm8058_xoadc_calibrate,
};

static struct msm_adc_channels msm_adc_channels_data[] = {
	{"vbatt", CHANNEL_ADC_VBATT, 0, &xoadc_fn, CHAN_PATH_TYPE2,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"vcoin", CHANNEL_ADC_VCOIN, 0, &xoadc_fn, CHAN_PATH_TYPE1,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"vcharger_channel", CHANNEL_ADC_VCHG, 0, &xoadc_fn, CHAN_PATH_TYPE3,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE4, scale_default},
	{"charger_current_monitor", CHANNEL_ADC_CHG_MONITOR, 0, &xoadc_fn,
		CHAN_PATH_TYPE4,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"vph_pwr", CHANNEL_ADC_VPH_PWR, 0, &xoadc_fn, CHAN_PATH_TYPE5,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"usb_vbus", CHANNEL_ADC_USB_VBUS, 0, &xoadc_fn, CHAN_PATH_TYPE11,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"pmic_therm", CHANNEL_ADC_DIE_TEMP, 0, &xoadc_fn, CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_pmic_therm},
	{"pmic_therm_4K", CHANNEL_ADC_DIE_TEMP_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE7, scale_pmic_therm},
	{"xo_therm", CHANNEL_ADC_XOTHERM, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE5, tdkntcgtherm},
	{"xo_therm_4K", CHANNEL_ADC_XOTHERM_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE6, tdkntcgtherm},
#ifdef CONFIG_LGE_PMIC8058_MPP
	{"pcb_revision", CHANNEL_ADC_PCB_REVISION, 0, &xoadc_fn, CHAN_PATH_TYPE9,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"msm_therm", CHANNEL_ADC_MSM_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_msm_therm},
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE7,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_batt_therm},
#ifndef CONFIG_LGE_MACH_BOARD_REVB
	{"acc_adc", CHANNEL_ADC_ACC, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"wireless_current", CHANNEL_ADC_WIRELESS_CURRENT, 0, &xoadc_fn, CHAN_PATH_TYPE10,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_xtern_chgr_cur},
#endif
#else
	{"hdset_detect", CHANNEL_ADC_HDSET, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"chg_batt_amon", CHANNEL_ADC_BATT_AMON, 0, &xoadc_fn, CHAN_PATH_TYPE10,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1,
		scale_xtern_chgr_cur},
	{"msm_therm", CHANNEL_ADC_MSM_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_msm_therm},
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE7,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_batt_therm},
	{"batt_id", CHANNEL_ADC_BATT_ID, 0, &xoadc_fn, CHAN_PATH_TYPE9,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
#endif
	{"ref_625mv", CHANNEL_ADC_625_REF, 0, &xoadc_fn, CHAN_PATH_TYPE15,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"ref_1250mv", CHANNEL_ADC_1250_REF, 0, &xoadc_fn, CHAN_PATH_TYPE13,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"ref_325mv", CHANNEL_ADC_325_REF, 0, &xoadc_fn, CHAN_PATH_TYPE14,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.channel = msm_adc_channels_data,
	.num_chan_supported = ARRAY_SIZE(msm_adc_channels_data),
};

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

#ifdef CONFIG_LGE_PMIC8058_MPP
static void pmic8058_xoadc_mpp_config(void)
{
	int rc;

#ifdef CONFIG_LGE_MACH_BOARD_REVB
	rc = pm8901_mpp_config_digital_out(XOADC_MPP_4,
			PM8901_MPP_DIG_LEVEL_S4, PM_MPP_DOUT_CTL_LOW);
	if (rc)
		pr_err("%s: Config mpp4 on pmic 8901 failed\n", __func__);
#endif
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_3/*PCB_REVISION_ADC*/,
			PM_MPP_AIN_AMUX_CH8, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp3 on pmic 8058 failed\n", __func__);

#if 0 /* CSFB not used */
	rc = pm8058_mpp_config_analog_output(XOADC_MPP_6/*GSM_VREF*/,
			PM_MPP_AOUT_LVL_1V25, PM_MPP_AOUT_CTL_ENABLE);
	if (rc)
		pr_err("%s: Config mpp6 on pmic 8058 failed\n", __func__);
#endif
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_7/*BATTERY_THERM_ADC_IN*/,
			PM_MPP_AIN_AMUX_CH6, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp7 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_10/*MSM_THERM_ADC*/,
			PM_MPP_AIN_AMUX_CH7, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp10 on pmic 8058 failed\n", __func__);

#ifndef CONFIG_LGE_MACH_BOARD_REVB
#if 0 /* CSFB not used */
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_4/*WIRELESS_CURRENT_ADC*/,
			PM_MPP_AIN_AMUX_CH9, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp4 on pmic 8058 failed\n", __func__);
#endif
	rc = pm8058_mpp_config_analog_input(XOADC_MPP_8/*ACC_ADC*/,
			PM_MPP_AIN_AMUX_CH5, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp8 on pmic 8058 failed\n", __func__);
#endif
}
#else
static void pmic8058_xoadc_mpp_config(void)
{
	int rc;

	rc = pm8901_mpp_config_digital_out(XOADC_MPP_4,
			PM8901_MPP_DIG_LEVEL_S4, PM_MPP_DOUT_CTL_LOW);
	if (rc)
		pr_err("%s: Config mpp4 on pmic 8901 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_3,
			PM_MPP_AIN_AMUX_CH5, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp3 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_5,
			PM_MPP_AIN_AMUX_CH9, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp5 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_7,
			PM_MPP_AIN_AMUX_CH6, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp7 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_8,
			PM_MPP_AIN_AMUX_CH8, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp8 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_10,
			PM_MPP_AIN_AMUX_CH7, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp10 on pmic 8058 failed\n", __func__);
}
#endif

static struct regulator *vreg_ldo18_adc;

static int pmic8058_xoadc_vreg_config(int on)
{
	int rc;

	if (on) {
		rc = regulator_enable(vreg_ldo18_adc);
		if (rc)
			pr_err("%s: Enable of regulator ldo18_adc "
						"failed\n", __func__);
	} else {
#ifdef CONFIG_LGE_PM_CURRENT_CABLE_TYPE
/* kiwone.seo@lge.com,  the adc voltage doesn't have a margine in time, so we always on, 
    but, it must be turned off in sleep. check after.
*/
                rc = 0;
#else
		rc = regulator_disable(vreg_ldo18_adc);
		if (rc)
			pr_err("%s: Disable of regulator ldo18_adc "
						"failed\n", __func__);
#endif
	}

	return rc;
}

static int pmic8058_xoadc_vreg_setup(void)
{
	int rc;

	vreg_ldo18_adc = regulator_get(NULL, "8058_l18");
	if (IS_ERR(vreg_ldo18_adc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_ldo18_adc));
		rc = PTR_ERR(vreg_ldo18_adc);
		goto fail;
	}

	rc = regulator_set_voltage(vreg_ldo18_adc, 2200000, 2200000);
	if (rc) {
		pr_err("%s: unable to set ldo18 voltage to 2.2V\n", __func__);
		goto fail;
	}

	return rc;
fail:
	regulator_put(vreg_ldo18_adc);
	return rc;
}

static void pmic8058_xoadc_vreg_shutdown(void)
{
	regulator_put(vreg_ldo18_adc);
}

/* usec. For this ADC,
 * this time represents clk rate @ txco w/ 1024 decimation ratio.
 * Each channel has different configuration, thus at the time of starting
 * the conversion, xoadc will return actual conversion time
 * */
static struct adc_properties pm8058_xoadc_data = {
	.adc_reference          = 2200, /* milli-voltage for this adc */
	.bitresolution         = 15,
	.bipolar                = 0,
	.conversiontime         = 54,
};

static struct xoadc_platform_data xoadc_pdata = {
	.xoadc_prop = &pm8058_xoadc_data,
	.xoadc_mpp_config = pmic8058_xoadc_mpp_config,
	.xoadc_vreg_set = pmic8058_xoadc_vreg_config,
	.xoadc_num = XOADC_PMIC_0,
	.xoadc_vreg_setup = pmic8058_xoadc_vreg_setup,
	.xoadc_vreg_shutdown = pmic8058_xoadc_vreg_shutdown,
};
#endif


#ifdef CONFIG_MSM_SDIO_AL

static unsigned mdm2ap_status = 140;

static int configure_mdm2ap_status(int on)
{
	int ret = 0;
	if (on)
		ret = msm_gpiomux_get(mdm2ap_status);
	else
		ret = msm_gpiomux_put(mdm2ap_status);

	if (ret)
		pr_err("%s: mdm2ap_status config failed, on = %d\n", __func__,
		       on);

	return ret;
}


static int get_mdm2ap_status(void)
{
	return gpio_get_value(mdm2ap_status);
}

static struct sdio_al_platform_data sdio_al_pdata = {
	.config_mdm2ap_status = configure_mdm2ap_status,
	.get_mdm2ap_status = get_mdm2ap_status,
	.allow_sdioc_version_major_2 = 0,
	.peer_sdioc_version_minor = 0x2002,
	.peer_sdioc_version_major = 0x0004,
	.peer_sdioc_boot_version_minor = 0x0001,
	.peer_sdioc_boot_version_major = 0x0003,
};

struct platform_device msm_device_sdio_al = {
	.name = "msm_sdio_al",
	.id = -1,
	.dev		= {
		.parent = &msm_charm_modem.dev,
		.platform_data	= &sdio_al_pdata,
	},
};

#endif /* CONFIG_MSM_SDIO_AL */

static struct platform_device *charm_devices[] __initdata = {
	&msm_charm_modem,
#ifdef CONFIG_USB_ANDROID
	&usb_diag_mdm_device,
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif
#endif
};

static struct platform_device *surf_devices[] __initdata = {
	&msm_device_smd,
	&msm_device_uart_dm12,
#ifdef CONFIG_I2C_QUP
#ifdef CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C
	&msm_gsbi1_qup_i2c_device,
#endif
	&msm_gsbi3_qup_i2c_device,
#ifdef CONFIG_LGE_CAMERA
	&msm_gsbi4_qup_i2c_device,
#endif
#ifdef CONFIG_LGE_FUEL_GAUGE
	&msm_gsbi5_qup_i2c_device,
#endif
	&msm_gsbi7_qup_i2c_device,
	&msm_gsbi8_qup_i2c_device,
	&msm_gsbi9_qup_i2c_device,

#if defined (CONFIG_LGE_SENSOR_DCOMPASS)|| defined (CONFIG_LGE_SENSOR_PROXIMITY) 
	&msm_gsbi10_qup_i2c_device,
#endif

#ifndef CONFIG_MSM_DSPS
/* kwangdo.yi@lge.com [jointlab] Thu 14 Apr 2011 S
   add lge sensor feature to fix build error
*/
#ifdef CONFIG_LGE_SENSOR
	&msm_gsbi12_qup_i2c_device,
#endif
/* kwangdo.yi@lge.com [jointlab] Thu 14 Apr 2011 E */
#endif
#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	&msm_gsbi1_qup_spi_device,
#endif
//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
#if 0
#ifdef CONFIG_SERIAL_MSM_HS
	&msm_device_uart_dm1,
#endif
#endif
//[LGE_UPDATE_E] 20110420, sangyeol.lee@lge.com,  [END]
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi1,
	&msm_device_ssbi2,
	&msm_device_ssbi3,
#endif
#if defined(CONFIG_USB_PEHCI_HCD) || defined(CONFIG_USB_PEHCI_HCD_MODULE)
	&isp1763_device,
#endif

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_HCD)
	&msm_device_otg,
#endif
#ifdef CONFIG_USB_GADGET_MSM_72K
	&msm_device_gadget_peripheral,
#endif

#ifdef CONFIG_USB_ANDROID
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	&usb_mass_storage_device,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&rndis_device,
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
#ifdef CONFIG_USB_F_SERIAL
	&usb_gadget_fserial_device,
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	&usb_gadget_facm_device,
#endif
	&android_usb_device,
#endif /* CONFIG_USB_ANDROID */

#ifdef CONFIG_BATTERY_MSM
	&msm_batt_device,
#endif
#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
	&android_pmem_kernel_ebi1_device,
#endif
#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
	&android_pmem_kernel_smi_device,
#endif
#ifdef CONFIG_ANDROID_PMEM
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&android_pmem_smipool_device,
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#if defined(CONFIG_LGE_HANDLE_PANIC)
	&panic_handler_device,
#endif
#endif
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&msm_kgsl_3d0,
#ifdef CONFIG_MSM_KGSL_2D
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#endif
#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)
	&msm_rpm_log_device,
#endif
#if defined(CONFIG_MSM_RPM_STATS_LOG)
	&msm_rpm_stat_device,
#endif
	&msm_device_vidc,

//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
#if 0	
#if (defined(CONFIG_MARIMBA_CORE)) && \
	(defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
	&msm_bt_power_device,
#endif
#endif
//[LGE_UPDATE_E] 20110420, sangyeol.lee@lge.com,  [END]

#ifdef CONFIG_SENSORS_MSM_ADC
	&msm_adc_device,
#endif
#ifdef CONFIG_PMIC8058
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L0],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L1],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L2],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L3],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L4],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L5],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L6],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L7],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L8],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L9],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L10],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L11],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L12],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L13],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L14],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L15],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L16],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L17],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L18],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L19],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L20],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L21],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L22],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L23],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L24],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L25],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S2],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S3],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S4],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_LVS0],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_LVS1],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_NCP],
#endif
#ifdef CONFIG_PMIC8901
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L0],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L1],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L2],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L3],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L4],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L5],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L6],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_S2],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_S3],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_S4],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS0],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS1],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS2],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS3],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_MVS0],
#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif


#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
#ifdef CONFIG_MSM_USE_TSIF1
	&msm_device_tsif[1],
#else
	&msm_device_tsif[0],
#endif /* CONFIG_MSM_USE_TSIF1 */
#endif /* CONFIG_TSIF */

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif

#ifdef CONFIG_LGE_DIAGTEST
	&lg_fw_diagcmd_device,
	&lg_diag_cmd_device,
#endif

	&msm_tsens_device,

};

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
enum {
	SX150X_CORE,
	SX150X_DOCKING,
	SX150X_SURF,
	SX150X_LEFT_FHA,
	SX150X_RIGHT_FHA,
	SX150X_SOUTH,
	SX150X_NORTH,
	SX150X_CORE_FLUID,
};

static struct sx150x_platform_data sx150x_data[] __initdata = {
	[SX150X_CORE] = {
		.gpio_base         = GPIO_CORE_EXPANDER_BASE,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0x0c08,
		.io_pulldn_ena     = 0x4060,
		.io_open_drain_ena = 0x000c,
		.io_polarity       = 0,
		.irq_summary       = -1, /* see fixup_i2c_configs() */
		.irq_base          = GPIO_EXPANDER_IRQ_BASE,
	},
	[SX150X_DOCKING] = {
		.gpio_base         = GPIO_DOCKING_EXPANDER_BASE,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0x5e06,
		.io_pulldn_ena     = 0x81b8,
		.io_open_drain_ena = 0,
		.io_polarity       = 0,
		.irq_summary       = PM8058_GPIO_IRQ(PM8058_IRQ_BASE,
						     UI_INT2_N),
		.irq_base          = GPIO_EXPANDER_IRQ_BASE +
				     GPIO_DOCKING_EXPANDER_BASE -
				     GPIO_EXPANDER_GPIO_BASE,
	},
	[SX150X_SURF] = {
		.gpio_base         = GPIO_SURF_EXPANDER_BASE,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0,
		.io_pulldn_ena     = 0,
		.io_open_drain_ena = 0,
		.io_polarity       = 0,
		.irq_summary       = PM8058_GPIO_IRQ(PM8058_IRQ_BASE,
						     UI_INT1_N),
		.irq_base          = GPIO_EXPANDER_IRQ_BASE +
				     GPIO_SURF_EXPANDER_BASE -
				     GPIO_EXPANDER_GPIO_BASE,
	},
	[SX150X_LEFT_FHA] = {
		.gpio_base         = GPIO_LEFT_KB_EXPANDER_BASE,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0,
		.io_pulldn_ena     = 0x40,
		.io_open_drain_ena = 0,
		.io_polarity       = 0,
		.irq_summary       = PM8058_GPIO_IRQ(PM8058_IRQ_BASE,
						     UI_INT3_N),
		.irq_base          = GPIO_EXPANDER_IRQ_BASE +
				     GPIO_LEFT_KB_EXPANDER_BASE -
				     GPIO_EXPANDER_GPIO_BASE,
	},
	[SX150X_RIGHT_FHA] = {
		.gpio_base         = GPIO_RIGHT_KB_EXPANDER_BASE,
		.oscio_is_gpo      = true,
		.io_pullup_ena     = 0,
		.io_pulldn_ena     = 0,
		.io_open_drain_ena = 0,
		.io_polarity       = 0,
		.irq_summary       = PM8058_GPIO_IRQ(PM8058_IRQ_BASE,
						     UI_INT3_N),
		.irq_base          = GPIO_EXPANDER_IRQ_BASE +
				     GPIO_RIGHT_KB_EXPANDER_BASE -
				     GPIO_EXPANDER_GPIO_BASE,
	},
	[SX150X_SOUTH] = {
		.gpio_base    = GPIO_SOUTH_EXPANDER_BASE,
		.irq_base     = GPIO_EXPANDER_IRQ_BASE +
				GPIO_SOUTH_EXPANDER_BASE -
				GPIO_EXPANDER_GPIO_BASE,
		.irq_summary  = PM8058_GPIO_IRQ(PM8058_IRQ_BASE, UI_INT3_N),
	},
	[SX150X_NORTH] = {
		.gpio_base    = GPIO_NORTH_EXPANDER_BASE,
		.irq_base     = GPIO_EXPANDER_IRQ_BASE +
				GPIO_NORTH_EXPANDER_BASE -
				GPIO_EXPANDER_GPIO_BASE,
		.irq_summary  = PM8058_GPIO_IRQ(PM8058_IRQ_BASE, UI_INT3_N),
		.oscio_is_gpo = true,
		.io_open_drain_ena = 0x30,
	},
	[SX150X_CORE_FLUID] = {
		.gpio_base         = GPIO_CORE_EXPANDER_BASE,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0x0408,
		.io_pulldn_ena     = 0x4060,
		.io_open_drain_ena = 0x0008,
		.io_polarity       = 0,
		.irq_summary       = -1, /* see fixup_i2c_configs() */
		.irq_base          = GPIO_EXPANDER_IRQ_BASE,
	},
};

/* sx150x_low_power_cfg
 *
 * This data and init function are used to put unused gpio-expander output
 * lines into their low-power states at boot. The init
 * function must be deferred until a later init stage because the i2c
 * gpio expander drivers do not probe until after they are registered
 * (see register_i2c_devices) and the work-queues for those registrations
 * are processed.  Because these lines are unused, there is no risk of
 * competing with a device driver for the gpio.
 *
 * gpio lines whose low-power states are input are naturally in their low-
 * power configurations once probed, see the platform data structures above.
 */
struct sx150x_low_power_cfg {
	unsigned gpio;
	unsigned val;
};

static struct sx150x_low_power_cfg
common_sx150x_lp_cfgs[] __initdata = {
	{GPIO_WLAN_DEEP_SLEEP_N, 0},
	{GPIO_EXT_GPS_LNA_EN,    0},
	{GPIO_MSM_WAKES_BT,      0},
	{GPIO_USB_UICC_EN,       0},
	{GPIO_BATT_GAUGE_EN,     0},
};

static struct sx150x_low_power_cfg
surf_ffa_sx150x_lp_cfgs[] __initdata = {
	{GPIO_MIPI_DSI_RST_N,      0},
	{GPIO_DONGLE_PWR_EN,       0},
	{GPIO_CAP_TS_SLEEP,        1},
	{GPIO_WEB_CAMIF_RESET_N,   0},
};

static void __init
cfg_gpio_low_power(struct sx150x_low_power_cfg *cfgs, unsigned nelems)
{
	unsigned n;
	int rc;

	for (n = 0; n < nelems; ++n) {
		rc = gpio_request(cfgs[n].gpio, NULL);
		if (!rc) {
			rc = gpio_direction_output(cfgs[n].gpio, cfgs[n].val);
			gpio_free(cfgs[n].gpio);
		}

		if (rc) {
			printk(KERN_NOTICE "%s: failed to sleep gpio %d: %d\n",
			       __func__, cfgs[n].gpio, rc);
		}
	}
}

static int __init cfg_sx150xs_low_power(void)
{
	cfg_gpio_low_power(common_sx150x_lp_cfgs,
		ARRAY_SIZE(common_sx150x_lp_cfgs));
	if (!machine_is_msm8x60_fluid())
		cfg_gpio_low_power(surf_ffa_sx150x_lp_cfgs,
			ARRAY_SIZE(surf_ffa_sx150x_lp_cfgs));
	return 0;
}
module_init(cfg_sx150xs_low_power);

#ifdef CONFIG_I2C
static struct i2c_board_info core_expander_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x3e),
		.platform_data = &sx150x_data[SX150X_CORE]
	},
};

static struct i2c_board_info docking_expander_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x3f),
		.platform_data = &sx150x_data[SX150X_DOCKING]
	},
};

static struct i2c_board_info surf_expanders_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x70),
		.platform_data = &sx150x_data[SX150X_SURF]
	}
};

static struct i2c_board_info fha_expanders_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1508q", 0x21),
		.platform_data = &sx150x_data[SX150X_LEFT_FHA]
	},
	{
		I2C_BOARD_INFO("sx1508q", 0x22),
		.platform_data = &sx150x_data[SX150X_RIGHT_FHA]
	}
};

static struct i2c_board_info fluid_expanders_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1508q", 0x23),
		.platform_data = &sx150x_data[SX150X_SOUTH]
	},
	{
		I2C_BOARD_INFO("sx1508q", 0x20),
		.platform_data = &sx150x_data[SX150X_NORTH]
	}
};

static struct i2c_board_info fluid_core_expander_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x3e),
		.platform_data = &sx150x_data[SX150X_CORE_FLUID]
	},
};
#endif
#endif

#define EXT_CHG_VALID_MPP 10
#define EXT_CHG_VALID_MPP_2 11

#ifdef CONFIG_ISL9519_CHARGER
static int isl_detection_setup(void)
{
	int ret = 0;

	ret = pm8058_mpp_config_digital_in(EXT_CHG_VALID_MPP,
					   PM8058_MPP_DIG_LEVEL_S3,
					   PM_MPP_DIN_TO_INT);
	ret |=  pm8058_mpp_config_bi_dir(EXT_CHG_VALID_MPP_2,
					   PM8058_MPP_DIG_LEVEL_S3,
					   PM_MPP_BI_PULLUP_10KOHM
					   );
	return ret;
}

static struct isl_platform_data isl_data __initdata = {
	.chgcurrent		= 700,
	.valid_n_gpio		= PM8058_MPP_PM_TO_SYS(10),
	.chg_detection_config	= isl_detection_setup,
	.max_system_voltage	= 4200,
	.min_system_voltage	= 3200,
	.term_current		= 120,
	.input_current		= 2048,
};

static struct i2c_board_info isl_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("isl9519q", 0x9),
		.irq = PM8058_CBLPWR_IRQ(PM8058_IRQ_BASE),
		.platform_data = &isl_data,
	},
};
#endif

#if defined(CONFIG_SMB137B_CHARGER) || defined(CONFIG_SMB137B_CHARGER_MODULE)
static int smb137b_detection_setup(void)
{
	int ret = 0;

	ret = pm8058_mpp_config_digital_in(EXT_CHG_VALID_MPP,
					PM8058_MPP_DIG_LEVEL_S3,
					PM_MPP_DIN_TO_INT);
	ret |=  pm8058_mpp_config_bi_dir(EXT_CHG_VALID_MPP_2,
					PM8058_MPP_DIG_LEVEL_S3,
					PM_MPP_BI_PULLUP_10KOHM);
	return ret;
}

static struct smb137b_platform_data smb137b_data __initdata = {
	.chg_detection_config = smb137b_detection_setup,
	.valid_n_gpio = PM8058_MPP_PM_TO_SYS(10),
	.batt_mah_rating = 950,
};

static struct i2c_board_info smb137b_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("smb137b", 0x08),
		.irq = PM8058_CBLPWR_IRQ(PM8058_IRQ_BASE),
		.platform_data = &smb137b_data,
	},
};
#endif


#ifdef CONFIG_LGE_SWITCHING_CHARGER_MAX8971
#define SWITCHING_CHG_IRQ_N	124

static struct max8971_platform_data max8971_data __initdata = {

	.chgcc = 0x0C,			// Fast Charge Current - 600mA
	.fchgtime = 0x02,			// Fast Charge Time - 5hrs
	
	.chgrstrt = 0x0,			// Fast Charge Restart Threshold - 150mV
	.dcilmt = 0x28, 			// Input Current Limit Selection - 1A
	
	.topofftime = 0x03,		// Top Off Timer Setting  - 30min
	.topofftshld = 0x03,		// Done Current Threshold - 200mA
	.chgcv = 0x02,				// Charger Termination Voltage - 4.35V
	
//	.regtemp;			// Die temperature thermal regulation loop setpoint
//	.safetyreg;			// JEITA Safety region selection
	.thm_config = 0x1, 		// Thermal monitor configuration - thermistor disable
	
	.int_mask = 0xF3,			// CHGINT_MASK - mask all

};

static struct i2c_board_info max8971_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("max8971", 0x35/*0x6A*/),
		.irq = MSM_GPIO_TO_INT(SWITCHING_CHG_IRQ_N),
		.platform_data = &max8971_data,
	},
};
#endif

#ifdef CONFIG_LGE_SWITCHING_CHARGER_BQ24160_DOCOMO_ONLY
#define SWITCHING_CHG_IRQ_N	124

static struct bq24160_platform_data bq24160_data __initdata = {
	.tmr_rst = 0b1,
	.supply_sel = 0b1,

	.reset = 0b0,			/* reset all reg to default values : 0 => no effect, 1 => reset all */
	.iusblimit = 0b100,		/* usb current limit : 100 => USB3.0 host 900mA current limit */
	.enstat = 0b1,			/* enable STAT : 0 => disable, 1 => enable */
	.te = 0b1,				/* enable charger termination : 0 => disable, 1 => enable */
	.ce = 0b0,				/* charger enable : 0 => enable, 1 => disable */
	.hz_mode = 0b0,			/* high impedance mode : 0 => no high impedance, 1 => high impedance */

	.vbatt_reg = 0b101011,	/* battery regulation voltage : 101011 => 4.36V*/
	.inlimit_in = 0b0,		/* input limit for IN input : 0 => 1.5A, 1 => 2.5A */
	.dpdm_en = 0b0,			/* D+/D- detention : 0 => normal state, 1 => force D+/D- detection */

	.chgcrnt = 0b00000,		/* charge current : 00000 => 550mA */
	.termcrnt = 0b011,		/* termination current sense : 001 => 100mA*/

	.minsys_stat = 0b0,		/* minimum system voltage mode : 0 => not active, 1 => active */
	.dpm_stat = 0b1,		/* Vin-DPM mode : 0 => not active, 1 => active */
	.vindpm_usb = 0b000,	/* usb input Vin-dpm voltage : 000 => 4.2V */
	.vindpm_in = 0b000,		/* IN input Vin-dpm voltage */

	.tmr2x_en = 0b1,		/* timer slowed by 2x */
	.safety_tmr = 0b01,		/* safety timer : 01 => 6hrs*/
	.ts_en = 0b0,			/* ts function enable */
	.ts_fault = 0b00,		/* ts fault mode */
};

static struct i2c_board_info bq24160_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("bq24160", 0x6B),
		.irq = MSM_GPIO_TO_INT(SWITCHING_CHG_IRQ_N),
		.platform_data = &bq24160_data,
	},
};
#endif


#ifdef CONFIG_PMIC8058
#define PMIC_GPIO_SDC3_DET 22

static int pm8058_gpios_init(void)
{
	int i;
	int rc;
	struct pm8058_gpio_cfg {
		int                gpio;
		struct pm8058_gpio cfg;
	};

	struct pm8058_gpio_cfg gpio_cfgs[] = {
#ifdef CONFIG_LGE_MUIC_TS5USBA33402
		{ /* MUIC_INT_N - GPIO15 */
			14,
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_UP_30,
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
#endif
		{ /* FFA ethernet */
			6,
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_DN,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
		{
			PMIC_GPIO_SDC3_DET - 1,
			{
				.direction		= PM_GPIO_DIR_IN,
#if defined(CONFIG_LGE_PMIC8058_GPIO)//inverted polarity
				.pull			= (lge_bd_rev < LGE_REV_B) ? PM_GPIO_PULL_DN : PM_GPIO_PULL_NO,
#else
				.pull			= PM_GPIO_PULL_UP_30,
#endif
				.vin_sel		= 2,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
#endif
		{ /* core&surf gpio expander */
			UI_INT1_N,
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* docking gpio expander */
			UI_INT2_N,
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
		{ /* FHA/keypad gpio expanders */
			UI_INT3_N,
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
#ifdef CONFIG_LGE_AUDIO
		{ /* RCV AMP RESET */
			PMIC_GPIO_RCV_AMP_RESET - 1,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,

			}
		},
		{ /* CAMERA MIC ENABLE */
			PMIC_GPIO_CAMCORDER_MIC_EN - 1,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,

			}
		},
#endif
		{ /* TouchDisc Interrupt */
			5,
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_UP_1P5,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			}
		},
		{ /* Timpani Reset */
			20,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 1,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},

#ifdef CONFIG_LGE_HEADSET_DETECTION_FSA8008
		{ /* Ear Sense */
			PMIC_GPIO_EAR_SENSE_N - 1,
				{
					.direction	= PM_GPIO_DIR_IN,
					.pull		= PM_GPIO_PULL_NO,
					.function	= PM_GPIO_FUNC_NORMAL,
					.vin_sel	= 2,
					.inv_int_pol	= 0,
				}
		},

		{ /* Ear Key */
			PMIC_GPIO_EAR_KEY_INT - 1,
				{
					.direction	= PM_GPIO_DIR_IN,
					.pull		= PM_GPIO_PULL_NO,
					.function	= PM_GPIO_FUNC_NORMAL,
					.vin_sel	= 2,
					.inv_int_pol	= 0,
				}
		},
#endif

		{ /* PMIC ID interrupt */
			36,
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_NO,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},
	};

#if defined(CONFIG_HAPTIC_ISA1200) || \
		defined(CONFIG_HAPTIC_ISA1200_MODULE)

	struct pm8058_gpio_cfg en_hap_gpio_cfg = {
			PMIC_GPIO_HAP_ENABLE,
			{
				.direction      = PM_GPIO_DIR_OUT,
				.pull           = PM_GPIO_PULL_NO,
				.out_strength   = PM_GPIO_STRENGTH_HIGH,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
				.vin_sel        = 2,
				.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
				.output_value   = 0,
			}

	};

	if (machine_is_msm8x60_fluid()) {
		rc = pm8058_gpio_config(en_hap_gpio_cfg.gpio,
				&en_hap_gpio_cfg.cfg);
		if (rc < 0) {
			pr_err("%s pmic haptics gpio config failed\n",
							__func__);
			return rc;
		}
	}
#endif

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
	struct pm8058_gpio_cfg line_in_gpio_cfg = {
			18,
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_UP_1P5,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			}
	};
		rc = pm8058_gpio_config(line_in_gpio_cfg.gpio,
				&line_in_gpio_cfg.cfg);
		if (rc < 0) {
			pr_err("%s pmic line_in gpio config failed\n",
							__func__);
			return rc;
		}
#endif
#endif
	for (i = 0; i < ARRAY_SIZE(gpio_cfgs); ++i) {
		rc = pm8058_gpio_config(gpio_cfgs[i].gpio,
				&gpio_cfgs[i].cfg);
		if (rc < 0) {
			pr_err("%s pmic gpio config failed\n",
				__func__);
			return rc;
		}
	}

	return 0;
}

static const unsigned int i_atnt_keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
};

static const unsigned int i_atnt_revB_keymap[] = {
	KEY(0, 0, KEY_VOLUMEDOWN),
	KEY(0, 1, KEY_VOLUMEUP),
};

static struct resource resources_keypad[] = {
	{
		.start	= PM8058_KEYPAD_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_KEYPAD_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_KEYSTUCK_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_KEYSTUCK_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct matrix_keymap_data i_atnt_keymap_data = {
	.keymap_size	= ARRAY_SIZE(i_atnt_keymap),
	.keymap		= i_atnt_keymap,
};

static struct matrix_keymap_data i_atnt_revB_keymap_data = {
	.keymap_size	= ARRAY_SIZE(i_atnt_revB_keymap),
	.keymap		= i_atnt_revB_keymap,
};

static struct pmic8058_keypad_data i_atnt_keypad_data = {
	.input_name		= "i_atnt-keypad",
	.input_phys_device	= "i_atnt-keypad/input0",
	.num_rows		= 6,
	.num_cols		= 5,
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.debounce_ms		= {8, 10},
	.scan_delay_ms		= 32,
	.row_hold_ns            = 91500,
	.wakeup			= 1,
	.keymap_data		= &i_atnt_keymap_data,
};

static struct pmic8058_keypad_data i_atnt_revB_keypad_data = {
	.input_name		= "i_atnt-keypad",
	.input_phys_device	= "i_atnt-keypad/input0",
	.num_rows		= 6,
	.num_cols		= 5,
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.debounce_ms		= {8, 10},
	.scan_delay_ms		= 32,
	.row_hold_ns            = 91500,
	.wakeup			= 1,
	.keymap_data		= &i_atnt_revB_keymap_data,
};

static struct resource resources_pwrkey[] = {
	{
		.start	= PM8058_PWRKEY_REL_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_PWRKEY_REL_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_PWRKEY_PRESS_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_PWRKEY_PRESS_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct pmic8058_pwrkey_pdata pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us   = 970,
	.wakeup			= 1,
	.pwrkey_time_ms		= 500,
};

#ifdef CONFIG_PMIC8058_VIBRATOR
static struct pmic8058_vibrator_pdata pmic_vib_pdata = {
	.initial_vibrate_ms  = 500,
	.level_mV = 3000,
	.max_timeout_ms = 15000,
};
#endif

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
#define PM8058_OTHC_CNTR_BASE0	0xA0
#define PM8058_OTHC_CNTR_BASE1	0x134
#define PM8058_OTHC_CNTR_BASE2	0x137
#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
#define PM8058_LINE_IN_DET_GPIO	PM8058_GPIO_PM_TO_SYS(18)
#endif

static struct othc_accessory_info othc_accessories[]  = {
	{
		.accessory = OTHC_SVIDEO_OUT,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT
							| OTHC_ADC_DETECT,
		.key_code = SW_VIDEOOUT_INSERT,
		.enabled = false,
		.adc_thres = {
				.min_threshold = 20,
				.max_threshold = 40,
			},
	},
#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
	{
		.accessory = OTHC_ANC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT |
							OTHC_SWITCH_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_ANC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_MICROPHONE,
		.detect_flags = OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_MICROPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
#endif
};

static struct othc_switch_info switch_info[] = {
	{
		.min_adc_threshold = 0,
		.max_adc_threshold = 100,
		.key_code = KEY_PLAYPAUSE,
	},
	{
		.min_adc_threshold = 100,
		.max_adc_threshold = 200,
		.key_code = KEY_REWIND,
	},
	{
		.min_adc_threshold = 200,
		.max_adc_threshold = 500,
		.key_code = KEY_FASTFORWARD,
	},
};

static struct othc_n_switch_config switch_config = {
	.voltage_settling_time_ms = 0,
	.num_adc_samples = 3,
	.adc_channel = CHANNEL_ADC_HDSET,
	.switch_info = switch_info,
	.num_keys = ARRAY_SIZE(switch_info),
	.default_sw_en = true,
	.default_sw_idx = 0,
};

static struct hsed_bias_config hsed_bias_config = {
	/* HSED mic bias config info */
	.othc_headset = OTHC_HEADSET_NO,
	.othc_lowcurr_thresh_uA = 100,
	.othc_highcurr_thresh_uA = 600,
	.othc_hyst_prediv_us = 7800,
	.othc_period_clkdiv_us = 62500,
	.othc_hyst_clk_us = 121000,
	.othc_period_clk_us = 312500,
	.othc_wakeup = 1,
};

static struct othc_hsed_config hsed_config_1 = {
	.hsed_bias_config = &hsed_bias_config,
	/*
	 * The detection delay and switch reporting delay are
	 * required to encounter a hardware bug (spurious switch
	 * interrupts on slow insertion/removal of the headset).
	 * This will introduce a delay in reporting the accessory
	 * insertion and removal to the userspace.
	 */
	.detection_delay_ms = 1500,
	/* Switch info */
	.switch_debounce_ms = 1500,
	.othc_support_n_switch = false,
	.switch_config = &switch_config,
	.ir_gpio = -1,
	/* Accessory info */
	.accessories_support = true,
	.accessories = othc_accessories,
	.othc_num_accessories = ARRAY_SIZE(othc_accessories),
};

static struct othc_regulator_config othc_reg = {
	.regulator	 = "8058_l5",
	.max_uV		 = 2850000,
	.min_uV		 = 2850000,
};

/* MIC_BIAS0 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_0 = {
	.micbias_select = OTHC_MICBIAS_0,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};

/* MIC_BIAS1 is configured as HSED_BIAS for OTHC */
static struct pmic8058_othc_config_pdata othc_config_pdata_1 = {
	.micbias_select = OTHC_MICBIAS_1,
#if 1
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
#else
	.micbias_capability = OTHC_MICBIAS_HSED,
	.micbias_enable = OTHC_SIGNAL_PWM_TCXO,
#endif
	.micbias_regulator = &othc_reg,
	.hsed_config = &hsed_config_1,
	.hsed_name = "8660_handset",
};

/* MIC_BIAS2 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_2 = {
	.micbias_select = OTHC_MICBIAS_2,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};

static struct resource resources_othc_0[] = {
	{
		.name = "othc_base",
		.start = PM8058_OTHC_CNTR_BASE0,
		.end   = PM8058_OTHC_CNTR_BASE0,
		.flags = IORESOURCE_IO,
	},
};

static struct resource resources_othc_1[] = {
#ifndef CONFIG_LGE_HEADSET_DETECTION_FSA8008
	{
		.start = PM8058_SW_1_IRQ(PM8058_IRQ_BASE),
		.end   = PM8058_SW_1_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = PM8058_IR_1_IRQ(PM8058_IRQ_BASE),
		.end   = PM8058_IR_1_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
#endif
	{
		.name = "othc_base",
		.start = PM8058_OTHC_CNTR_BASE1,
		.end   = PM8058_OTHC_CNTR_BASE1,
		.flags = IORESOURCE_IO,
	},
};

static struct resource resources_othc_2[] = {
	{
		.name = "othc_base",
		.start = PM8058_OTHC_CNTR_BASE2,
		.end   = PM8058_OTHC_CNTR_BASE2,
		.flags = IORESOURCE_IO,
	},
};

static void __init msm8x60_init_pm8058_othc(void)
{
	int i;

	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2 ||
		machine_is_msm8x60_fluid() || machine_is_msm8x60_charm_surf() || machine_is_lge_i_board() ||
		machine_is_msm8x60_charm_ffa()) {
		/* 3-switch headset supported only by V2 FFA and FLUID */
		hsed_config_1.accessories_adc_support = true,
		/* ADC based accessory detection works only on V2 and FLUID */
		hsed_config_1.accessories_adc_channel = CHANNEL_ADC_HDSET,
		hsed_config_1.othc_support_n_switch = true;
	}

	/* IR GPIO is absent on FLUID */
	if (machine_is_msm8x60_fluid())
		hsed_config_1.ir_gpio = -1;

	for (i = 0; i < ARRAY_SIZE(othc_accessories); i++) {
		if (machine_is_msm8x60_fluid()) {
			switch (othc_accessories[i].accessory) {
			case OTHC_ANC_HEADPHONE:
			case OTHC_ANC_HEADSET:
				othc_accessories[i].gpio = GPIO_HEADSET_DET_N;
				break;
			case OTHC_MICROPHONE:
				othc_accessories[i].enabled = false;
				break;
			case OTHC_SVIDEO_OUT:
				othc_accessories[i].enabled = true;
				hsed_config_1.video_out_gpio = GPIO_HS_SW_DIR;
				break;
			}
		}
	}
}
#endif

#ifdef CONFIG_PM8058_CHARGER
static struct resource resources_pm8058_charger[] = {
	{	.name = "CHGVAL",
		.start = PM8058_CHGVAL_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_CHGVAL_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{	.name = "CHGINVAL",
		.start = PM8058_CHGINVAL_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_CHGINVAL_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "CHGILIM",
		.start = PM8058_CHGILIM_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_CHGILIM_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "VCP",
		.start = PM8058_VCP_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_VCP_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
		{
		.name = "ATC_DONE",
		.start = PM8058_ATC_DONE_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_ATC_DONE_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "ATCFAIL",
		.start = PM8058_ATCFAIL_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_ATCFAIL_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "AUTO_CHGDONE",
		 .start = PM8058_AUTO_CHGDONE_IRQ(PM8058_IRQ_BASE),
		 .end = PM8058_AUTO_CHGDONE_IRQ(PM8058_IRQ_BASE),
		 .flags = IORESOURCE_IRQ,
	},
	{
		.name = "AUTO_CHGFAIL",
		.start = PM8058_AUTO_CHGFAIL_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_AUTO_CHGFAIL_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "CHGSTATE",
		.start = PM8058_CHGSTATE_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_CHGSTATE_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "FASTCHG",
		.start = PM8058_FASTCHG_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_FASTCHG_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "CHG_END",
		 .start = PM8058_CHG_END_IRQ(PM8058_IRQ_BASE),
		 .end = PM8058_CHG_END_IRQ(PM8058_IRQ_BASE),
		 .flags = IORESOURCE_IRQ,
	},
	{
		.name = "BATTTEMP",
		.start = PM8058_BATTTEMP_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_BATTTEMP_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "CHGHOT",
		.start = PM8058_CHGHOT_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_CHGHOT_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "CHGTLIMIT",
		.start = PM8058_CHGTLIMIT_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_CHGTLIMIT_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "CHG_GONE",
		 .start = PM8058_CHG_GONE_IRQ(PM8058_IRQ_BASE),
		 .end = PM8058_CHG_GONE_IRQ(PM8058_IRQ_BASE),
		 .flags = IORESOURCE_IRQ,
	},
	{
		.name = "VCPMAJOR",
		 .start = PM8058_VCPMAJOR_IRQ(PM8058_IRQ_BASE),
		 .end = PM8058_VCPMAJOR_IRQ(PM8058_IRQ_BASE),
		 .flags = IORESOURCE_IRQ,
	},
	{
		.name = "VBATDET",
		 .start = PM8058_VBATDET_IRQ(PM8058_IRQ_BASE),
		 .end = PM8058_VBATDET_IRQ(PM8058_IRQ_BASE),
		 .flags = IORESOURCE_IRQ,
	},
	{
		.name = "BATFET",
		 .start = PM8058_BATFET_IRQ(PM8058_IRQ_BASE),
		 .end = PM8058_BATFET_IRQ(PM8058_IRQ_BASE),
		 .flags = IORESOURCE_IRQ,
	},
	{
		.name = "BATT_REPLACE",
		.start = PM8058_BATT_REPLACE_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_BATT_REPLACE_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "BATTCONNECT",
		.start = PM8058_BATTCONNECT_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_BATTCONNECT_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "VBATDET_LOW",
		.start = PM8058_VBATDET_LOW_IRQ(PM8058_IRQ_BASE),
		.end = PM8058_VBATDET_LOW_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
};
#endif

static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm8058_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_VPH,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};

	int rc = -EINVAL;
	int id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8058_gpio_config(id - 1, &pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8058_gpio_config(%d): rc=%d\n",
					__func__, id, rc);
		}
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	case 7:
		id = PM_PWM_LED_FLASH1;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	default:
		break;
	}

	if (ch >= 6 && ch <= 7) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}
	return rc;

}

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
};

#define PM8058_GPIO_INT           88

static struct pm8058_gpio_platform_data pm8058_gpio_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8058_GPIO_IRQ(PM8058_IRQ_BASE, 0),
	.init		= pm8058_gpios_init,
};

static struct pm8058_gpio_platform_data pm8058_mpp_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS),
	.irq_base	= PM8058_MPP_IRQ(PM8058_IRQ_BASE, 0),
};

#ifdef CONFIG_LGE_PM_BATTERY_ALARM
static struct resource resources_battery_alarm[] = {
       {
		.start  = PM8058_BATT_ALARM_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_BATT_ALARM_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
};

#endif


static struct resource resources_rtc[] = {
       {
		.start  = PM8058_RTC_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_RTC_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
       {
		.start  = PM8058_RTC_ALARM_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_RTC_ALARM_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
};

static struct pm8058_rtc_platform_data pm8058_rtc_pdata = {
	.rtc_alarm_powerup	= false,
};

static struct pmic8058_led pmic8058_flash_leds[] = {
	[0] = {
		.name		= "camera:flash0",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_0,
	},
	[1] = {
		.name		= "camera:flash1",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_1,
	},
	[2] = {
		.name		= "button-backlight",//"keypad:drv",
#if 0 /* platform.bsp@lge.com 20110830 */
		.max_brightness = 15, /* keypad led : 30mA */
#else
		.max_brightness = 1, /* keypad led : 40mA */
#endif
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},/* 300 mA keypad drv sink */
};

static struct pmic8058_leds_platform_data pm8058_flash_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_flash_leds),
	.leds	= pmic8058_flash_leds,
};

static struct pmic8058_led pmic8058_fluid_flash_leds[] = {
	[0] = {
		.name		= "led:drv0",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_0,
	},/* 300 mA flash led0 drv sink */
	[1] = {
		.name		= "led:drv1",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_1,
	},/* 300 mA flash led1 sink */
	[2] = {
		.name		= "led:drv2",
		.max_brightness = 20,
		.id		= PMIC8058_ID_LED_0,
	},/* 40 mA led0 sink */
	[3] = {
		.name		= "keypad:drv",
		.max_brightness = 15,
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},/* 300 mA keypad drv sink */
};

static struct pmic8058_leds_platform_data pm8058_fluid_flash_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_fluid_flash_leds),
	.leds	= pmic8058_fluid_flash_leds,
};

static struct resource resources_temp_alarm[] = {
       {
		.start  = PM8058_TEMP_ALARM_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_TEMP_ALARM_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
};

static struct resource resources_pm8058_misc[] = {
       {
		.start  = PM8058_OSCHALT_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_OSCHALT_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
};

#ifdef CONFIG_LGE_PM_BATTERY_ALARM
#else
static struct resource resources_pm8058_batt_alarm[] = {
	{
		.start  = PM8058_BATT_ALARM_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_BATT_ALARM_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
	},
};
#endif

#define PM8058_SUBDEV_KPD 0
#define PM8058_SUBDEV_LED 1

static struct mfd_cell pm8058_subdevs[] = {
	{
		.name = "pm8058-keypad",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(resources_keypad),
		.resources	= resources_keypad,
	},
	{	.name = "pm8058-led",
		.id		= -1,
	},
	{	.name = "pm8058-gpio",
		.id		= -1,
		.platform_data	= &pm8058_gpio_data,
		.data_size	= sizeof(pm8058_gpio_data),
	},
	{	.name = "pm8058-mpp",
		.id		= -1,
		.platform_data	= &pm8058_mpp_data,
		.data_size	= sizeof(pm8058_mpp_data),
	},
	{	.name = "pm8058-pwrkey",
		.id	= -1,
		.resources = resources_pwrkey,
		.num_resources = ARRAY_SIZE(resources_pwrkey),
		.platform_data = &pwrkey_pdata,
		.data_size = sizeof(pwrkey_pdata),
	},
#ifdef CONFIG_PMIC8058_VIBRATOR
	{
		.name = "pm8058-vib",
		.id = -1,
		.platform_data = &pmic_vib_pdata,
		.data_size     = sizeof(pmic_vib_pdata),
	},
#endif
	{
		.name = "pm8058-pwm",
		.id = -1,
		.platform_data = &pm8058_pwm_data,
		.data_size = sizeof(pm8058_pwm_data),
	},
#ifdef CONFIG_SENSORS_MSM_ADC
	{
		.name = "pm8058-xoadc",
		.id = -1,
		.num_resources = ARRAY_SIZE(resources_adc),
		.resources = resources_adc,
		.platform_data = &xoadc_pdata,
		.data_size = sizeof(xoadc_pdata),
	},
#endif
#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	{
		.name = "pm8058-othc",
		.id = 0,
		.platform_data = &othc_config_pdata_0,
		.data_size = sizeof(othc_config_pdata_0),
		.num_resources = ARRAY_SIZE(resources_othc_0),
		.resources = resources_othc_0,
	},
	{
		/* OTHC1 module has headset/switch dection */
		.name = "pm8058-othc",
		.id = 1,
		.num_resources = ARRAY_SIZE(resources_othc_1),
		.resources = resources_othc_1,
		.platform_data = &othc_config_pdata_1,
		.data_size = sizeof(othc_config_pdata_1),
	},
	{
		.name = "pm8058-othc",
		.id = 2,
		.platform_data = &othc_config_pdata_2,
		.data_size = sizeof(othc_config_pdata_2),
		.num_resources = ARRAY_SIZE(resources_othc_2),
		.resources = resources_othc_2,
	},
#endif
	{
		.name = "pm8058-rtc",
		.id = -1,
		.num_resources  = ARRAY_SIZE(resources_rtc),
		.resources      = resources_rtc,
		.platform_data = &pm8058_rtc_pdata,
		.data_size = sizeof(pm8058_rtc_pdata),
	},
	{
		.name = "pm8058-tm",
		.id = -1,
		.num_resources  = ARRAY_SIZE(resources_temp_alarm),
		.resources      = resources_temp_alarm,
	},
	{	.name = "pm8058-upl",
		.id		= -1,
	},
	{
		.name = "pm8058-misc",
		.id = -1,
		.num_resources  = ARRAY_SIZE(resources_pm8058_misc),
		.resources      = resources_pm8058_misc,
	},
#ifdef CONFIG_LGE_PM_BATTERY_ALARM
  {
		.name = "pm8058-batt-alarm",
		.id = -1,
		.num_resources  = ARRAY_SIZE(resources_battery_alarm),
		.resources      = resources_battery_alarm,
	},
#else
	{	.name = "pm8058-batt-alarm",
		.id		= -1,
		.num_resources  = ARRAY_SIZE(resources_pm8058_batt_alarm),
		.resources      = resources_pm8058_batt_alarm,
	},
#endif
};

#ifdef CONFIG_PM8058_CHARGER
static struct mfd_cell pm8058_charger_sub_dev = {
		.name = "pm8058-charger",
		.id = -1,
		.num_resources = ARRAY_SIZE(resources_pm8058_charger),
		.resources = resources_pm8058_charger,
};
#endif

static struct pm8058_platform_data pm8058_platform_data = {
	.irq_base = PM8058_IRQ_BASE,

	.num_subdevs = ARRAY_SIZE(pm8058_subdevs),
	.sub_devices = pm8058_subdevs,
	.irq_trigger_flags = IRQF_TRIGGER_LOW,
};

static struct i2c_board_info pm8058_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8058-core", 0x55),
		.irq = MSM_GPIO_TO_INT(PM8058_GPIO_INT),
		.platform_data = &pm8058_platform_data,
	},
};
#endif /* CONFIG_PMIC8058 */

#if defined(CONFIG_TOUCHDISC_VTD518_SHINETSU) || \
		defined(CONFIG_TOUCHDISC_VTD518_SHINETSU_MODULE)
#define TDISC_I2C_SLAVE_ADDR	0x67
#define PMIC_GPIO_TDISC		PM8058_GPIO_PM_TO_SYS(5)
#define TDISC_INT		PM8058_GPIO_IRQ(PM8058_IRQ_BASE, 5)

static const char *vregs_tdisc_name[] = {
	"8058_l5",
	"8058_s3",
};

static const int vregs_tdisc_val[] = {
	2850000,/* uV */
	1800000,
};
static struct regulator *vregs_tdisc[ARRAY_SIZE(vregs_tdisc_name)];

static int tdisc_shinetsu_setup(void)
{
	int rc, i;

	rc = gpio_request(PMIC_GPIO_TDISC, "tdisc_interrupt");
	if (rc) {
		pr_err("%s: gpio_request failed for PMIC_GPIO_TDISC\n",
								__func__);
		return rc;
	}

	rc = gpio_request(GPIO_JOYSTICK_EN, "tdisc_oe");
	if (rc) {
		pr_err("%s: gpio_request failed for GPIO_JOYSTICK_EN\n",
							__func__);
		goto fail_gpio_oe;
	}

	rc = gpio_direction_output(GPIO_JOYSTICK_EN, 1);
	if (rc) {
		pr_err("%s: gpio_direction_output failed for GPIO_JOYSTICK_EN\n",
								__func__);
		gpio_free(GPIO_JOYSTICK_EN);
		goto fail_gpio_oe;
	}

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++) {
		vregs_tdisc[i] = regulator_get(NULL, vregs_tdisc_name[i]);
		if (IS_ERR(vregs_tdisc[i])) {
			printk(KERN_ERR "%s: regulator get %s failed (%ld)\n",
				__func__, vregs_tdisc_name[i],
				PTR_ERR(vregs_tdisc[i]));
			rc = PTR_ERR(vregs_tdisc[i]);
			goto vreg_get_fail;
		}

		rc = regulator_set_voltage(vregs_tdisc[i],
				vregs_tdisc_val[i], vregs_tdisc_val[i]);
		if (rc) {
			printk(KERN_ERR "%s: regulator_set_voltage() = %d\n",
				__func__, rc);
			goto vreg_set_voltage_fail;
		}
	}

	return rc;
vreg_set_voltage_fail:
	i++;
vreg_get_fail:
	while (i)
		regulator_put(vregs_tdisc[--i]);
fail_gpio_oe:
	gpio_free(PMIC_GPIO_TDISC);
	return rc;
}

static void tdisc_shinetsu_release(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++)
		regulator_put(vregs_tdisc[i]);

	gpio_free(PMIC_GPIO_TDISC);
	gpio_free(GPIO_JOYSTICK_EN);
}

static int tdisc_shinetsu_enable(void)
{
	int i, rc = -EINVAL;

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++) {
		rc = regulator_enable(vregs_tdisc[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s enable failed (%d)\n",
				__func__, vregs_tdisc_name[i], rc);
			goto vreg_fail;
		}
	}

	/* Enable the OE (output enable) gpio */
	gpio_set_value_cansleep(GPIO_JOYSTICK_EN, 1);
	/* voltage and gpio stabilization delay */
	msleep(50);

	return 0;
vreg_fail:
	while (i)
		regulator_disable(vregs_tdisc[--i]);
	return rc;
}

static int tdisc_shinetsu_disable(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vregs_tdisc_name); i++) {
		rc = regulator_disable(vregs_tdisc[i]);
		if (rc < 0) {
			printk(KERN_ERR "%s: vreg %s disable failed (%d)\n",
				__func__, vregs_tdisc_name[i], rc);
			goto tdisc_reg_fail;
		}
	}

	/* Disable the OE (output enable) gpio */
	gpio_set_value_cansleep(GPIO_JOYSTICK_EN, 0);

	return 0;

tdisc_reg_fail:
	while (i)
		regulator_enable(vregs_tdisc[--i]);
	return rc;
}

static struct tdisc_abs_values tdisc_abs = {
	.x_max = 32,
	.y_max = 32,
	.x_min = -32,
	.y_min = -32,
	.pressure_max = 32,
	.pressure_min = 0,
};

static struct tdisc_platform_data tdisc_data = {
	.tdisc_setup = tdisc_shinetsu_setup,
	.tdisc_release = tdisc_shinetsu_release,
	.tdisc_enable = tdisc_shinetsu_enable,
	.tdisc_disable = tdisc_shinetsu_disable,
	.tdisc_wakeup  = 0,
	.tdisc_gpio = PMIC_GPIO_TDISC,
	.tdisc_report_keys = true,
	.tdisc_report_relative = true,
	.tdisc_report_absolute = false,
	.tdisc_report_wheel = false,
	.tdisc_reverse_x = false,
	.tdisc_reverse_y = true,
	.tdisc_abs  = &tdisc_abs,
};

static struct i2c_board_info msm_i2c_gsbi3_tdisc_info[] = {
	{
		I2C_BOARD_INFO("vtd518", TDISC_I2C_SLAVE_ADDR),
		.irq =  TDISC_INT,
		.platform_data = &tdisc_data,
	},
};
#endif

#define PM_GPIO_CDC_RST_N 20
#define GPIO_CDC_RST_N PM8058_GPIO_PM_TO_SYS(PM_GPIO_CDC_RST_N)

static struct regulator *vreg_timpani_1;
static struct regulator *vreg_timpani_2;

static unsigned int msm_timpani_setup_power(void)
{
	int rc;

	vreg_timpani_1 = regulator_get(NULL, "8058_l0");
	if (IS_ERR(vreg_timpani_1)) {
		pr_err("%s: Unable to get 8058_l0\n", __func__);
		return -ENODEV;
	}

	vreg_timpani_2 = regulator_get(NULL, "8058_s3");
	if (IS_ERR(vreg_timpani_2)) {
		pr_err("%s: Unable to get 8058_s3\n", __func__);
		regulator_put(vreg_timpani_1);
		return -ENODEV;
	}

	rc = regulator_set_voltage(vreg_timpani_1, 1200000, 1200000);
	if (rc) {
		pr_err("%s: unable to set L0 voltage to 1.2V\n", __func__);
		goto fail;
	}

	rc = regulator_set_voltage(vreg_timpani_2, 1800000, 1800000);
	if (rc) {
		pr_err("%s: unable to set S3 voltage to 1.8V\n", __func__);
		goto fail;
	}

	rc = regulator_enable(vreg_timpani_1);
	if (rc) {
		pr_err("%s: Enable regulator 8058_l0 failed\n", __func__);
		goto fail;
	}

	/* The settings for LDO0 should be set such that
	*  it doesn't require to reset the timpani. */
	rc = regulator_set_optimum_mode(vreg_timpani_1, 5000);
	if (rc < 0) {
		pr_err("Timpani regulator optimum mode setting failed\n");
		goto fail;
	}

	rc = regulator_enable(vreg_timpani_2);
	if (rc) {
		pr_err("%s: Enable regulator 8058_s3 failed\n", __func__);
		regulator_disable(vreg_timpani_1);
		goto fail;
	}

	rc = gpio_request(GPIO_CDC_RST_N, "CDC_RST_N");
	if (rc) {
		pr_err("%s: GPIO Request %d failed\n", __func__,
			GPIO_CDC_RST_N);
		regulator_disable(vreg_timpani_1);
		regulator_disable(vreg_timpani_2);
		goto fail;
	} else {
		gpio_direction_output(GPIO_CDC_RST_N, 1);
		usleep_range(1000, 1050);
		gpio_direction_output(GPIO_CDC_RST_N, 0);
		usleep_range(1000, 1050);
		gpio_direction_output(GPIO_CDC_RST_N, 1);
		gpio_free(GPIO_CDC_RST_N);
	}
	return rc;

fail:
	regulator_put(vreg_timpani_1);
	regulator_put(vreg_timpani_2);
	return rc;
}

static void msm_timpani_shutdown_power(void)
{
	int rc;

	rc = regulator_disable(vreg_timpani_1);
	if (rc)
		pr_err("%s: Disable regulator 8058_l0 failed\n", __func__);

	regulator_put(vreg_timpani_1);

	rc = regulator_disable(vreg_timpani_2);
	if (rc)
		pr_err("%s: Disable regulator 8058_s3 failed\n", __func__);

	regulator_put(vreg_timpani_2);
}

/* Power analog function of codec */
static struct regulator *vreg_timpani_cdc_apwr;
static int msm_timpani_codec_power(int vreg_on)
{
	int rc = 0;

	if (!vreg_timpani_cdc_apwr) {

		vreg_timpani_cdc_apwr = regulator_get(NULL, "8058_s4");

		if (IS_ERR(vreg_timpani_cdc_apwr)) {
			pr_err("%s: vreg_get failed (%ld)\n",
			__func__, PTR_ERR(vreg_timpani_cdc_apwr));
			rc = PTR_ERR(vreg_timpani_cdc_apwr);
			return rc;
		}
	}

	if (vreg_on) {

		rc = regulator_set_voltage(vreg_timpani_cdc_apwr,
				2200000, 2200000);
		if (rc) {
			pr_err("%s: unable to set 8058_s4 voltage to 2.2 V\n",
					__func__);
			goto vreg_fail;
		}

		rc = regulator_enable(vreg_timpani_cdc_apwr);
		if (rc) {
			pr_err("%s: vreg_enable failed %d\n", __func__, rc);
			goto vreg_fail;
		}
	} else {
		rc = regulator_disable(vreg_timpani_cdc_apwr);
		if (rc) {
			pr_err("%s: vreg_disable failed %d\n",
			__func__, rc);
			goto vreg_fail;
		}
	}

	return 0;

vreg_fail:
	regulator_put(vreg_timpani_cdc_apwr);
	vreg_timpani_cdc_apwr = NULL;
	return rc;
}

static struct marimba_codec_platform_data timpani_codec_pdata = {
	.marimba_codec_power =  msm_timpani_codec_power,
};

#define TIMPANI_SLAVE_ID_CDC_ADDR		0X77
#define TIMPANI_SLAVE_ID_QMEMBIST_ADDR		0X66

static struct marimba_platform_data timpani_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_CDC]	= TIMPANI_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = TIMPANI_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_timpani_setup_power,
	.marimba_shutdown = msm_timpani_shutdown_power,
	.codec = &timpani_codec_pdata,
};

#define TIMPANI_I2C_SLAVE_ADDR	0xD

static struct i2c_board_info msm_i2c_gsbi7_timpani_info[] = {
	{
		I2C_BOARD_INFO("timpani", TIMPANI_I2C_SLAVE_ADDR),
		.platform_data = &timpani_pdata,
	},
};

#ifdef CONFIG_PMIC8901

#define PM8901_GPIO_INT           91

static struct pm8901_gpio_platform_data pm8901_mpp_data = {
	.gpio_base	= PM8901_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8901_MPP_IRQ(PM8901_IRQ_BASE, 0),
};

static struct resource pm8901_temp_alarm[] = {
	{
		.start = PM8901_TEMP_ALARM_IRQ(PM8901_IRQ_BASE),
		.end = PM8901_TEMP_ALARM_IRQ(PM8901_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = PM8901_TEMP_HI_ALARM_IRQ(PM8901_IRQ_BASE),
		.end = PM8901_TEMP_HI_ALARM_IRQ(PM8901_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
};

static struct regulator_consumer_supply pm8901_vreg_supply[PM8901_VREG_MAX] = {
	[PM8901_VREG_ID_MPP0] =     REGULATOR_SUPPLY("8901_mpp0",     NULL),
	[PM8901_VREG_ID_USB_OTG]  = REGULATOR_SUPPLY("8901_usb_otg",  NULL),
	[PM8901_VREG_ID_HDMI_MVS] = REGULATOR_SUPPLY("8901_hdmi_mvs", NULL),
};

#define PM8901_VREG_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, \
			 _always_on, _active_high) \
	[_id] = { \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask = _modes, \
				.valid_ops_mask = _ops, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
				.input_uV = _min_uV, \
				.apply_uV = _apply_uV, \
				.always_on = _always_on, \
			}, \
			.num_consumer_supplies = 1, \
			.consumer_supplies = &pm8901_vreg_supply[_id], \
		}, \
		.active_high = _active_high, \
	}

#define PM8901_VREG_INIT_MPP(_id, _active_high) \
	PM8901_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL, \
			REGULATOR_CHANGE_STATUS, 0, 0, _active_high)

#define PM8901_VREG_INIT_VS(_id) \
	PM8901_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL, \
			REGULATOR_CHANGE_STATUS, 0, 0, 0)

static struct pm8901_vreg_pdata pm8901_vreg_init_pdata[PM8901_VREG_MAX] = {
	PM8901_VREG_INIT_MPP(PM8901_VREG_ID_MPP0, 1),

	PM8901_VREG_INIT_VS(PM8901_VREG_ID_USB_OTG),
	PM8901_VREG_INIT_VS(PM8901_VREG_ID_HDMI_MVS),
};

#define PM8901_VREG(_id) { \
	.name = "pm8901-regulator", \
	.id = _id, \
	.platform_data = &pm8901_vreg_init_pdata[_id], \
	.data_size = sizeof(pm8901_vreg_init_pdata[_id]), \
}

static struct mfd_cell pm8901_subdevs[] = {
	{	.name = "pm8901-mpp",
		.id		= -1,
		.platform_data	= &pm8901_mpp_data,
		.data_size	= sizeof(pm8901_mpp_data),
	},
	{	.name = "pm8901-tm",
		.id		= -1,
		.num_resources  = ARRAY_SIZE(pm8901_temp_alarm),
		.resources      = pm8901_temp_alarm,
	},
	PM8901_VREG(PM8901_VREG_ID_MPP0),
	PM8901_VREG(PM8901_VREG_ID_USB_OTG),
	PM8901_VREG(PM8901_VREG_ID_HDMI_MVS),
};

static struct pm8901_platform_data pm8901_platform_data = {
	.irq_base = PM8901_IRQ_BASE,
	.num_subdevs = ARRAY_SIZE(pm8901_subdevs),
	.sub_devices = pm8901_subdevs,
	.irq_trigger_flags = IRQF_TRIGGER_LOW,
};

static struct i2c_board_info pm8901_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8901-core", 0x55),
		.irq = MSM_GPIO_TO_INT(PM8901_GPIO_INT),
		.platform_data = &pm8901_platform_data,
	},
};

#endif /* CONFIG_PMIC8901 */

#if defined(CONFIG_MARIMBA_CORE)

static struct regulator *vreg_bahama;

struct bahama_config_register{
	u8 reg;
	u8 value;
	u8 mask;
};

enum version{
	VER_1_0,
	VER_2_0,
	VER_UNSUPPORTED = 0xFF
};

static u8 read_bahama_ver(void)
{
	int rc;
	struct marimba config = { .mod_id = SLAVE_ID_BAHAMA };
	u8 bahama_version;

	rc = marimba_read_bit_mask(&config, 0x00,  &bahama_version, 1, 0x1F);
	if (rc < 0) {
		printk(KERN_ERR
			 "%s: version read failed: %d\n",
			__func__, rc);
			return VER_UNSUPPORTED;
	} else {
		printk(KERN_INFO
		"%s: version read got: 0x%x\n",
		__func__, bahama_version);
	}

	switch (bahama_version) {
	case 0x08: /* varient of bahama v1 */
	case 0x10:
	case 0x00:
		return VER_1_0;
	case 0x09: /* variant of bahama v2 */
		return VER_2_0;
	default:
		return VER_UNSUPPORTED;
	}
}

static unsigned int msm_bahama_setup_power(void)
{
	int rc = 0;
	const char *msm_bahama_regulator = "8058_s3";
	vreg_bahama = regulator_get(NULL, msm_bahama_regulator);

	if (IS_ERR(vreg_bahama)) {
		rc = PTR_ERR(vreg_bahama);
		pr_err("%s: regulator_get %s = %d\n", __func__,
			msm_bahama_regulator, rc);
	}

	if (!rc)
		rc = regulator_set_voltage(vreg_bahama, 1800000, 1800000);
	else {
		pr_err("%s: regulator_set_voltage %s = %d\n", __func__,
			msm_bahama_regulator, rc);
		goto unget;
	}

	if (!rc)
		rc = regulator_enable(vreg_bahama);
	else {
		pr_err("%s: regulator_enable %s = %d\n", __func__,
			msm_bahama_regulator, rc);
		goto unget;
	}

	if (!rc)
		rc = gpio_request(GPIO_MS_SYS_RESET_N, "bahama sys_rst_n");
	else {
		pr_err("%s: gpio_request %d = %d\n", __func__,
			GPIO_MS_SYS_RESET_N, rc);
		goto unenable;
	}

	if (!rc) {
		gpio_direction_output(GPIO_MS_SYS_RESET_N, 0);
		usleep_range(1000, 1050);
		gpio_direction_output(GPIO_MS_SYS_RESET_N, 1);
		usleep_range(1000, 1050);
	} else {
		pr_err("%s: gpio_direction_output %d = %d\n", __func__,
			GPIO_MS_SYS_RESET_N, rc);
		goto unrequest;
	}

	return rc;

unrequest:
	gpio_free(GPIO_MS_SYS_RESET_N);
unenable:
	regulator_disable(vreg_bahama);
unget:
	regulator_put(vreg_bahama);
	return rc;
};
static unsigned int msm_bahama_shutdown_power(int value)


{
	gpio_set_value_cansleep(GPIO_MS_SYS_RESET_N, 0);

	gpio_free(GPIO_MS_SYS_RESET_N);

	regulator_disable(vreg_bahama);

	regulator_put(vreg_bahama);

	return 0;
};

static unsigned int msm_bahama_core_config(int type)
{
	int rc = 0;

	if (type == BAHAMA_ID) {

		int i;
		struct marimba config = { .mod_id = SLAVE_ID_BAHAMA };

		const struct bahama_config_register v20_init[] = {
			/* reg, value, mask */
			{ 0xF4, 0x84, 0xFF }, /* AREG */
			{ 0xF0, 0x04, 0xFF } /* DREG */
		};

		if (read_bahama_ver() == VER_2_0) {
			for (i = 0; i < ARRAY_SIZE(v20_init); i++) {
				u8 value = v20_init[i].value;
				rc = marimba_write_bit_mask(&config,
					v20_init[i].reg,
					&value,
					sizeof(v20_init[i].value),
					v20_init[i].mask);
				if (rc < 0) {
					printk(KERN_ERR
						"%s: reg %d write failed: %d\n",
						__func__, v20_init[i].reg, rc);
					return rc;
				}
				printk(KERN_INFO "%s: reg 0x%02x value 0x%02x"
					" mask 0x%02x\n",
					__func__, v20_init[i].reg,
					v20_init[i].value, v20_init[i].mask);
			}
		}
	}
	printk(KERN_INFO "core type: %d\n", type);

	return rc;
}

static struct regulator *fm_regulator_s3;
static struct msm_xo_voter *fm_clock;

static int fm_radio_setup(struct marimba_fm_platform_data *pdata)
{
	int rc = 0;
	struct pm8058_gpio cfg = {
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_S3,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
				};

	if (!fm_regulator_s3) {
		fm_regulator_s3 = regulator_get(NULL, "8058_s3");
		if (IS_ERR(fm_regulator_s3)) {
			rc = PTR_ERR(fm_regulator_s3);
			printk(KERN_ERR "%s: regulator get s3 (%d)\n",
				__func__, rc);
			goto out;
			}
	}


	rc = regulator_set_voltage(fm_regulator_s3, 1800000, 1800000);
	if (rc < 0) {
		printk(KERN_ERR "%s: regulator set voltage failed (%d)\n",
				__func__, rc);
		goto fm_fail_put;
	}

	rc = regulator_enable(fm_regulator_s3);
	if (rc < 0) {
		printk(KERN_ERR "%s: regulator s3 enable failed (%d)\n",
					__func__, rc);
		goto fm_fail_put;
	}

	/*Vote for XO clock*/
	fm_clock = msm_xo_get(MSM_XO_TCXO_D0, "fm_power");

	if (IS_ERR(fm_clock)) {
		rc = PTR_ERR(fm_clock);
		printk(KERN_ERR "%s: Couldn't get TCXO_D0 vote for FM (%d)\n",
					__func__, rc);
		goto fm_fail_switch;
	}

	rc = msm_xo_mode_vote(fm_clock, MSM_XO_MODE_ON);
	if (rc < 0) {
		printk(KERN_ERR "%s:  Failed to vote for TCX0_D0 ON (%d)\n",
					__func__, rc);
		goto fm_fail_vote;
	}

	/*GPIO 18 on PMIC is FM_IRQ*/
	rc = pm8058_gpio_config(FM_GPIO, &cfg);
	if (rc) {
		printk(KERN_ERR "%s: return val of pm8058_gpio_config: %d\n",
						__func__,  rc);
		goto fm_fail_clock;
	}
	goto out;

fm_fail_clock:
		msm_xo_mode_vote(fm_clock, MSM_XO_MODE_OFF);
fm_fail_vote:
		msm_xo_put(fm_clock);
fm_fail_switch:
		regulator_disable(fm_regulator_s3);
fm_fail_put:
		regulator_put(fm_regulator_s3);
out:
	return rc;
};

static void fm_radio_shutdown(struct marimba_fm_platform_data *pdata)
{
	int rc = 0;
	if (fm_regulator_s3 != NULL) {
		rc = regulator_disable(fm_regulator_s3);
		if (rc < 0) {
			printk(KERN_ERR "%s: regulator s3 disable (%d)\n",
							__func__, rc);
		}
		regulator_put(fm_regulator_s3);
		fm_regulator_s3 = NULL;
	}
	printk(KERN_ERR "%s: Voting off for XO", __func__);

	if (fm_clock != NULL) {
		rc = msm_xo_mode_vote(fm_clock, MSM_XO_MODE_OFF);
		if (rc < 0) {
			printk(KERN_ERR "%s: Voting off XO clock (%d)\n",
					__func__, rc);
		}
		msm_xo_put(fm_clock);
	}
	printk(KERN_ERR "%s: coming out of fm_radio_shutdown", __func__);
}

/* Slave id address for FM/CDC/QMEMBIST
 * Values can be programmed using Marimba slave id 0
 * should there be a conflict with other I2C devices
 * */
#define BAHAMA_SLAVE_ID_FM_ADDR         0x2A
#define BAHAMA_SLAVE_ID_QMEMBIST_ADDR   0x7B

static struct marimba_fm_platform_data marimba_fm_pdata = {
	.fm_setup =  fm_radio_setup,
	.fm_shutdown = fm_radio_shutdown,
	.irq = PM8058_GPIO_IRQ(PM8058_IRQ_BASE, FM_GPIO),
	.is_fm_soc_i2s_master = false,
};

/*
Just initializing the BAHAMA related slave
*/
static struct marimba_platform_data marimba_pdata = {
	.slave_id[SLAVE_ID_BAHAMA_FM]        = BAHAMA_SLAVE_ID_FM_ADDR,
	.slave_id[SLAVE_ID_BAHAMA_QMEMBIST]  = BAHAMA_SLAVE_ID_QMEMBIST_ADDR,
	.bahama_setup = msm_bahama_setup_power,
	.bahama_shutdown = msm_bahama_shutdown_power,
	.bahama_core_config = msm_bahama_core_config,
	.fm = &marimba_fm_pdata,
};


static struct i2c_board_info msm_marimba_board_info[] = {
	{
		I2C_BOARD_INFO("marimba", 0xc),
		.platform_data = &marimba_pdata,
	}
};
#endif /* CONFIG_MAIMBA_CORE */

#ifdef CONFIG_LGE_MHL_SII9244

static struct regulator *vreg_l23_mhl; // VREG_L23 - 1.2 V	
static struct regulator *vreg_l3_mhl;  // VREG_L3 - 3.3 V
static struct regulator *vreg_l12_mhl; // VREG_L12 - 1.8V		

static int mhl_power_onoff(int on)
{
	static bool power_state = 0;
	int rc =0;
	
	if (power_state == on) 
	{
		printk("sii_power_state is already %s ", power_state?"on":"off");
		return rc;
	}

	power_state = on;

	if(on)
	{
		printk("sii9234_cfg_power on\n");
	
		rc = regulator_enable(vreg_l23_mhl);	 // VREG_L23 - 1.2 V		
		if (rc) {
			pr_err("%s: l25 vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}

		rc = regulator_enable(vreg_l3_mhl);		// VREG_L3 - 3.3 V 
		if (rc) {
			pr_err("%s: l2 vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}


		rc = regulator_enable(vreg_l12_mhl);	// VREG_L12 - 1.8V			
		if (rc) {
			pr_err("%s: l2 vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}
	}
	else
	{
		printk("sii9234_cfg_power off\n");

		rc = regulator_disable(vreg_l23_mhl);		
		if (rc) {
			pr_err("%s: vreg_l23_mhl vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}

		rc = regulator_disable(vreg_l3_mhl);		
		if (rc) {
			pr_err("%s: vreg_l3_mhl vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}

		rc = regulator_disable(vreg_l12_mhl);		
		if (rc) {
			pr_err("%s: vreg_l12_mhl vreg enable failed (%d)\n", __func__, rc);
			return rc;
		}
	}	

	return rc;
	
}

static int mhl_power_config(void)
{
	int rc = 0;

	printk("%s\n",__func__);
		
	vreg_l23_mhl = regulator_get(NULL, "8058_l23");
	if (IS_ERR(vreg_l23_mhl)) {
		rc = PTR_ERR(vreg_l23_mhl);
		pr_err("%s: vreg_l23_mhl get failed (%d)\n", __func__, rc);
		return rc;
	}
	
	rc = regulator_set_voltage(vreg_l23_mhl, 1200000, 1200000);
	if (rc) {
		pr_err("%s: vreg_l23_mhl set level failed (%d)\n", __func__, rc);
		return rc;
	}

	vreg_l3_mhl = regulator_get(NULL, "8058_l3");
	if (IS_ERR(vreg_l3_mhl)) {
		rc = PTR_ERR(vreg_l3_mhl);
		pr_err("%s: vreg_l3_mhl get failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = regulator_set_voltage(vreg_l3_mhl, 3300000, 3300000);
	if (rc) {
		pr_err("%s: vreg_l3_mhl set level failed (%d)\n", __func__, rc);
		return rc;
	}

	vreg_l12_mhl = regulator_get(NULL, "8058_l12");
	if (IS_ERR(vreg_l12_mhl)) {
		rc = PTR_ERR(vreg_l12_mhl);
		pr_err("%s: mvs0 get failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = regulator_set_voltage(vreg_l12_mhl, 1800000, 1800000);
	if (rc) {
		pr_err("%s: vreg_l12_mhl set level failed (%d)\n", __func__, rc);
		return rc;
	}

	return rc;

}

const int mhl_support_hwrevision_map[LGE_I_BOARD_MAX] = 
{ 
  LGE_REV_D,  /* ATNT */
  LGE_REV_C,  /* DCM  */
  LGE_REV_B,  /* SKT  */
  LGE_REV_D,  /* VZW  */
  LGE_REV_B   /* LGU */
};

#define GPIO_MHL_INT          		30
#define GPIO_MHL_WAKEUP       	153
#define GPIO_MHL_RESET_N      	142
#define GPIO_MHL_SELECT_CSFB  	139
#define GPIO_MHL_SELECT_SVLTE 	  33
#define GPIO_MHL_SELECT       GPIO_MHL_SELECT_CSFB

static struct mhl_platform_data mhl_pdata = {
	.is_support = 0,
	.interrupt_pin = GPIO_MHL_INT,
	.reset_pin = GPIO_MHL_RESET_N,
	.select_pin = GPIO_MHL_SELECT,
	.wakeup_pin = GPIO_MHL_WAKEUP,
	.ldo_id_1v2 = "8058_l23",
	.ldo_id_1v8 = "8058_l12",
	.ldo_id_3v3 = "8058_l3",
	.power = mhl_power_onoff,
	.power_config = mhl_power_config,
};

static struct platform_device mhl_sii9244_device = {
	.name = "sii9244_driver",
	.id = 0,
	.dev.platform_data = &mhl_pdata,
};

/*
  CI2CA    LOW          HIGH
    HDMI : 0x92(0x49)   0x96(0x4B)
    MHL  : 0x72(0x39)   0x76(0x3B)
    TPI  : 0x7A(0x3D)   0x7E(0x3F)
    CBUS : 0xC8(0x64)   0xCC(0x66)
*/

static struct i2c_board_info sii9244_p0_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page0", 0x39),
	}
};

static struct i2c_board_info sii9244_p1_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page1", 0x3D),
	}
};

static struct i2c_board_info sii9244_p2_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page2", 0x49),
	}
};

static struct i2c_board_info sii9244_p3_mhl_info[] = {
	{
		I2C_BOARD_INFO("sii9244_i2c_page3", 0x64),
	}
};

#endif /*CONFIG_LGE_MHL_SII9244 */

#ifdef CONFIG_LGE_FUEL_GAUGE
static int max17040_battery_online(void)
{
	return 0;
};

static int max17040_charger_online(void)
{
	return 0;
};

static int max17040_charger_enable(void)
{
	return 0;
};

static struct max17040_platform_data max17040_pdata = {
	.battery_online = max17040_battery_online,
	.charger_online = max17040_charger_online,
	.charger_enable = max17040_charger_enable,
};

static struct i2c_board_info max17040_i2c_info[] = {
	{
		I2C_BOARD_INFO("max17040",0x36),
		.platform_data = &max17040_pdata,
	}
};
#endif

#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

static struct i2c_registry msm8x60_i2c_devices[] __initdata = {
#ifdef CONFIG_PMIC8058
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_SSBI1_I2C_BUS_ID,
		pm8058_boardinfo,
		ARRAY_SIZE(pm8058_boardinfo),
	},
#endif
#ifdef CONFIG_PMIC8901
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_SSBI2_I2C_BUS_ID,
		pm8901_boardinfo,
		ARRAY_SIZE(pm8901_boardinfo),
	},
#endif
#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
	{
		I2C_SURF | I2C_FFA,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		core_expander_i2c_info,
		ARRAY_SIZE(core_expander_i2c_info),
	},
	{
		I2C_SURF | I2C_FFA,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		docking_expander_i2c_info,
		ARRAY_SIZE(docking_expander_i2c_info),
	},
	{
		I2C_SURF,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		surf_expanders_i2c_info,
		ARRAY_SIZE(surf_expanders_i2c_info),
	},
	{
		I2C_SURF | I2C_FFA,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		fha_expanders_i2c_info,
		ARRAY_SIZE(fha_expanders_i2c_info),
	},
	{
		I2C_FLUID,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		fluid_expanders_i2c_info,
		ARRAY_SIZE(fluid_expanders_i2c_info),
	},
	{
		I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		fluid_core_expander_i2c_info,
		ARRAY_SIZE(fluid_core_expander_i2c_info),
	},
#endif
#if defined(CONFIG_TOUCHDISC_VTD518_SHINETSU) || \
		defined(CONFIG_TOUCHDISC_VTD518_SHINETSU_MODULE)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		msm_i2c_gsbi3_tdisc_info,
		ARRAY_SIZE(msm_i2c_gsbi3_tdisc_info),
	},
#endif
#if defined(CONFIG_TOUCHSCREEN_CY8C_TS)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		cy8ctmg200_board_info,
		ARRAY_SIZE(cy8ctmg200_board_info),
	},
#endif
#if defined(CONFIG_TOUCHSCREEN_CYTTSP_I2C) || \
		defined(CONFIG_TOUCHSCREEN_CYTTSP_I2C_MODULE)
	{
		I2C_FLUID,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		cyttsp_fluid_info,
		ARRAY_SIZE(cyttsp_fluid_info),
	},
	{
		I2C_FFA | I2C_SURF,
		MSM_GSBI3_QUP_I2C_BUS_ID,
		cyttsp_ffa_info,
		ARRAY_SIZE(cyttsp_ffa_info),
	},
#endif
#ifdef CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C
       {
               I2C_SURF | I2C_FFA | I2C_FLUID,
               MSM_GSBI1_QUP_I2C_BUS_ID,
               msm_i2c_synaptics_ts_info,
               ARRAY_SIZE(msm_i2c_synaptics_ts_info),
       },
#endif
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI7_QUP_I2C_BUS_ID,
		msm_i2c_gsbi7_timpani_info,
		ARRAY_SIZE(msm_i2c_gsbi7_timpani_info),
	},
#if defined(CONFIG_MARIMBA_CORE)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI7_QUP_I2C_BUS_ID,
		msm_marimba_board_info,
		ARRAY_SIZE(msm_marimba_board_info),
	},
#endif /* CONFIG_MARIMBA_CORE */
#ifdef CONFIG_ISL9519_CHARGER
	{
		I2C_SURF | I2C_FFA,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		isl_charger_i2c_info,
		ARRAY_SIZE(isl_charger_i2c_info),
	},
#endif
#if defined(CONFIG_HAPTIC_ISA1200) || \
		defined(CONFIG_HAPTIC_ISA1200_MODULE)
	{
		I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		msm_isa1200_board_info,
		ARRAY_SIZE(msm_isa1200_board_info),
	},
#endif
#if defined(CONFIG_SMB137B_CHARGER) || defined(CONFIG_SMB137B_CHARGER_MODULE)
	{
		I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		smb137b_charger_i2c_info,
		ARRAY_SIZE(smb137b_charger_i2c_info),
	},
#endif
#ifdef CONFIG_LGE_MHL_SII9244
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p0_mhl_info,
		ARRAY_SIZE(sii9244_p0_mhl_info),
	},
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p1_mhl_info,
		ARRAY_SIZE(sii9244_p1_mhl_info),
	},	
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p2_mhl_info,
		ARRAY_SIZE(sii9244_p2_mhl_info),
	},
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		sii9244_p3_mhl_info,
		ARRAY_SIZE(sii9244_p3_mhl_info),
	},	
#endif
#ifdef CONFIG_LGE_FUEL_GAUGE
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI5_QUP_I2C_BUS_ID,
		max17040_i2c_info,
		ARRAY_SIZE(max17040_i2c_info),
	},
#endif

#ifdef CONFIG_LGE_SWITCHING_CHARGER_MAX8971
    {
      I2C_SURF | I2C_FFA | I2C_FLUID,
      MSM_GSBI5_QUP_I2C_BUS_ID,
      max8971_charger_i2c_info,
      ARRAY_SIZE(max8971_charger_i2c_info),
    },
#endif

#ifdef CONFIG_LGE_SWITCHING_CHARGER_BQ24160_DOCOMO_ONLY
    {
		  I2C_SURF | I2C_FFA | I2C_FLUID,
	 	  MSM_GSBI5_QUP_I2C_BUS_ID,
		  bq24160_charger_i2c_info,
		  ARRAY_SIZE(bq24160_charger_i2c_info),
	  },
#endif

#if defined (CONFIG_LGE_SENSOR_DCOMPASS)|| defined (CONFIG_LGE_SENSOR_PROXIMITY)
    {
        I2C_SURF | I2C_FFA | I2C_FLUID,
	    MSM_GSBI10_QUP_I2C_BUS_ID,
	    msm_i2c_gsbi10_info,
	    ARRAY_SIZE(msm_i2c_gsbi10_info),
    },
#endif

#if defined (CONFIG_LGE_SENSOR_ACCELEROMETER) || defined (CONFIG_LGE_SENSOR_GYROSCOPE)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_GSBI12_QUP_I2C_BUS_ID,
		msm_i2c_gsbi12_info,
		ARRAY_SIZE(msm_i2c_gsbi12_info),
	},
#endif

};
#endif /* CONFIG_I2C */

#ifdef CONFIG_LGE_MUIC_TS5USBA33402
#define TS5USBA33402_MUIC_ADDRESS 0x44
static struct i2c_board_info msm_i2c_muic_info[] = {
	{
		I2C_BOARD_INFO("ts5usba_i2c_muic", TS5USBA33402_MUIC_ADDRESS),
	}
};

static struct i2c_registry muic_device __initdata = {
		0,
		MSM_GSBI8_QUP_I2C_BUS_ID,
		msm_i2c_muic_info,
		ARRAY_SIZE(msm_i2c_muic_info),
};

static void i2c_register_muic_info(void)
{
	i2c_register_board_info(muic_device.bus,
							muic_device.info,
							muic_device.len);
}
#endif


static void fixup_i2c_configs(void)
{
#ifdef CONFIG_I2C
#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
	if (machine_is_lge_i_board())
		sx150x_data[SX150X_CORE].irq_summary =
			PM8058_GPIO_IRQ(PM8058_IRQ_BASE, UI_INT2_N);
	else if (machine_is_msm8x60_ffa() || machine_is_msm8x60_charm_ffa())
		sx150x_data[SX150X_CORE].irq_summary =
			PM8058_GPIO_IRQ(PM8058_IRQ_BASE, UI_INT1_N);
	else if (machine_is_msm8x60_fluid())
		sx150x_data[SX150X_CORE_FLUID].irq_summary =
			PM8058_GPIO_IRQ(PM8058_IRQ_BASE, UI_INT1_N);
#endif
	/*
	 * Set PMIC 8901 MPP0 active_high to 0 for surf and charm_surf. This
	 * implies that the regulator connected to MPP0 is enabled when
	 * MPP0 is low.
	 */
	if (machine_is_lge_i_board())
		pm8901_vreg_init_pdata[PM8901_VREG_ID_MPP0].active_high = 0;
	else
		pm8901_vreg_init_pdata[PM8901_VREG_ID_MPP0].active_high = 1;
#endif
}

static void register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	u8 mach_mask = 0;
	int i;

	/* Build the matching 'supported_machs' bitmask */
	if (machine_is_lge_i_board())
		mach_mask = I2C_SURF;
	else if (machine_is_msm8x60_ffa() || machine_is_msm8x60_charm_ffa())
		mach_mask = I2C_FFA;
	else if (machine_is_msm8x60_rumi3())
		mach_mask = I2C_RUMI;
	else if (machine_is_msm8x60_sim())
		mach_mask = I2C_SIM;
	else if (machine_is_msm8x60_fluid())
		mach_mask = I2C_FLUID;
	else
		pr_err("unmatched machine ID in register_i2c_devices\n");

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8x60_i2c_devices); ++i) {
		if (msm8x60_i2c_devices[i].machs & mach_mask)
			i2c_register_board_info(msm8x60_i2c_devices[i].bus,
						msm8x60_i2c_devices[i].info,
						msm8x60_i2c_devices[i].len);
	}
#endif

	/* platform.team@lge.com separate backlight devices init */
	i2c_register_backlight_info();

	/* jisun.shin@lge.com Camera Platform*/
#ifdef CONFIG_LGE_CAMERA
	i2c_register_camera_info();
#endif

#ifdef CONFIG_LGE_MUIC_TS5USBA33402
	if(lge_bd_rev < LGE_REV_C) 
		i2c_register_muic_info();
#endif

}

static void __init msm8x60_init_uart12dm(void)
{
#if !defined(CONFIG_USB_PEHCI_HCD) && !defined(CONFIG_USB_PEHCI_HCD_MODULE)
	/* 0x1D000000 now belongs to EBI2:CS3 i.e. USB ISP Controller */
	void *fpga_mem = ioremap_nocache(0x1D000000, SZ_4K);
	/* Advanced mode */
	writew(0xFFFF, fpga_mem + 0x15C);
	/* FPGA_UART_SEL */
	writew(0, fpga_mem + 0x172);
	/* FPGA_GPIO_CONFIG_117 */
	writew(1, fpga_mem + 0xEA);
	/* FPGA_GPIO_CONFIG_118 */
	writew(1, fpga_mem + 0xEC);
	dsb();
	iounmap(fpga_mem);
#endif
}

#define MSM_GSBI9_PHYS		0x19900000
#define GSBI_DUAL_MODE_CODE	0x60

static void __init msm8x60_init_buses(void)
{
#ifdef CONFIG_I2C_QUP
	void *gsbi_mem = ioremap_nocache(0x19C00000, 4);
	/* Setting protocol code to 0x60 for dual UART/I2C in GSBI12 */
	writel_relaxed(0x6 << 4, gsbi_mem);
	/* Ensure protocol code is written before proceeding further */
	dsb();
	iounmap(gsbi_mem);

#ifdef CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C
       msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
#endif
	msm_gsbi3_qup_i2c_device.dev.platform_data = &msm_gsbi3_qup_i2c_pdata;
	msm_gsbi4_qup_i2c_device.dev.platform_data = &msm_gsbi4_qup_i2c_pdata;
#if defined(CONFIG_LGE_FUEL_GAUGE) || defined(CONFIG_LGE_SWITCHING_CHARGER_MAX8971) || defined(CONFIG_LGE_SWITCHING_CHARGER_BQ24160_DOCOMO_ONLY)
  msm_gsbi5_qup_i2c_device.dev.platform_data = &msm_gsbi5_qup_i2c_pdata;
#endif
	msm_gsbi7_qup_i2c_device.dev.platform_data = &msm_gsbi7_qup_i2c_pdata;
	msm_gsbi8_qup_i2c_device.dev.platform_data = &msm_gsbi8_qup_i2c_pdata;

#ifdef CONFIG_MSM_GSBI9_UART
		/* Setting protocol code to 0x60 for dual UART/I2C in GSBI9 */
		gsbi_mem = ioremap_nocache(MSM_GSBI9_PHYS, 4);
		writel(GSBI_DUAL_MODE_CODE, gsbi_mem);
		iounmap(gsbi_mem);
		msm_gsbi9_qup_i2c_pdata.use_gsbi_shared_mode = 1;
#endif
	msm_gsbi9_qup_i2c_device.dev.platform_data = &msm_gsbi9_qup_i2c_pdata;
#if defined (CONFIG_LGE_SENSOR_DCOMPASS)|| defined (CONFIG_LGE_SENSOR_PROXIMITY)
    msm_gsbi10_qup_i2c_device.dev.platform_data = &msm_gsbi10_qup_i2c_pdata;
#endif
    /* kwangdo.yi@lge.com [jointlab] Thu 14 Apr 2011 S
       add lge sensor config to fix build error
*/
#ifdef CONFIG_LGE_SENSOR
	msm_gsbi12_qup_i2c_device.dev.platform_data = &msm_gsbi12_qup_i2c_pdata;
#endif
	/* kwangdo.yi@lge.com [jointlab] Thu 14 Apr 2011 E */
#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	msm_gsbi1_qup_spi_device.dev.platform_data = &msm_gsbi1_qup_spi_pdata;
#endif
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi1.dev.platform_data = &msm_ssbi1_pdata;
	msm_device_ssbi2.dev.platform_data = &msm_ssbi2_pdata;
	msm_device_ssbi3.dev.platform_data = &msm_ssbi3_pdata;
#endif

	if (machine_is_msm8x60_fluid()) {
#if defined(CONFIG_SMB137B_CHARGER) || defined(CONFIG_SMB137B_CHARGER_MODULE) //CONFIG_USB_EHCI_MSM_72K
		msm_otg_pdata.vbus_power = msm_hsusb_smb137b_vbus_power;
#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
		msm_gsbi10_qup_spi_device.dev.platform_data =
					&msm_gsbi10_qup_spi_pdata;
#endif
	}

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_HCD)
	/*
	 * We can not put USB regulators (8058_l6 and 8058_l7) in LPM
	 * when we depend on USB PHY for VBUS/ID notifications. VBUS
	 * and ID notifications are available only on V2 surf and FFA
	 * with a hardware workaround.
	 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2 &&
			(machine_is_msm8x60_surf() ||
			(machine_is_msm8x60_ffa() &&
			pmic_id_notif_supported)))
		msm_otg_pdata.phy_can_powercollapse = 1;
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif

#if defined(CONFIG_USB_ANDROID_ACM_SMD) || defined(CONFIG_USB_ANDROID_ACM_SDIO)
	if (machine_is_lge_i_board()) {
		struct usb_gadget_facm_pdata *facm_pdata =
			usb_gadget_facm_device.dev.platform_data;

		/* for charm: Port1: SDIO - DUN,  Port2: TTY - NMEA
		 * for svlte-2: Port1: SDIO - DUN1,  Port2: SMD - DUN2
		 */
		if (socinfo_get_platform_subtype() == 3) {
			facm_pdata->transport[0] =
				USB_GADGET_FSERIAL_TRANSPORT_SDIO;
			facm_pdata->transport[1] =
				USB_GADGET_FSERIAL_TRANSPORT_SMD;
		} else {
			facm_pdata->transport[0] =
				USB_GADGET_FSERIAL_TRANSPORT_SDIO;
			facm_pdata->transport[1] =
				USB_GADGET_FSERIAL_TRANSPORT_TTY;
		}
	}
#endif
#if defined(CONFIG_USB_F_SERIAL_SDIO) || defined(CONFIG_USB_F_SERIAL_SMD)
	if (machine_is_lge_i_board()) {
		struct usb_gadget_fserial_platform_data *fserial_pdata =
			usb_gadget_fserial_device.dev.platform_data;

		//fserial_pdata->transport = USB_GADGET_FSERIAL_TRANSPORT_SDIO;
		/* for charm: Port1: SDIO - DUN,  Port2: TTY - NMEA
		 * for svlte-2: Port1: SDIO - DUN1,  Port2: SDIO - DUN2
		 */
		if (socinfo_get_platform_subtype() == 3) {
			fserial_pdata->transport[0] =
				USB_GADGET_FSERIAL_TRANSPORT_SDIO;
			fserial_pdata->transport[1] =
				USB_GADGET_FSERIAL_TRANSPORT_SMD;
		} else {
			fserial_pdata->transport[0] =
				USB_GADGET_FSERIAL_TRANSPORT_SDIO;
			fserial_pdata->transport[1] =
				USB_GADGET_FSERIAL_TRANSPORT_TTY;
		}

#ifdef CONFIG_LGE_USB_TWO_MODEM
		fserial_pdata->transport[1] = USB_GADGET_FSERIAL_TRANSPORT_SMD;
#endif
	}
#endif


//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
#if 0	
#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(54); /* GSBI6(2) */
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif
#endif
//[LGE_UPDATE_E] 20110420, sangyeol.lee@lge.com,  [END]

#ifdef CONFIG_MSM_GSBI9_UART
	if (machine_is_lge_i_board()) {
		msm_device_uart_gsbi9 = msm_add_gsbi9_uart();
		if (IS_ERR(msm_device_uart_gsbi9))
			pr_err("%s(): Failed to create uart gsbi9 device\n",
								__func__);
	}
#endif

#ifdef CONFIG_MSM_BUS_SCALING

	/* RPM calls are only enabled on V2 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2) {
		msm_bus_apps_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fabric_pdata.rpm_enabled = 1;
		msm_bus_mm_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fpb_pdata.rpm_enabled = 1;
		msm_bus_cpss_fpb_pdata.rpm_enabled = 1;
	}

	msm_bus_apps_fabric.dev.platform_data = &msm_bus_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_cpss_fpb_pdata;
#endif
}

static void __init msm8x60_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8x60_io();
#if defined(CONFIG_LGE_HIDDEN_RESET_PATCH)
	lge_make_fb_pmem();
#endif
	msm8x60_allocate_msm_fb_memory_regions();
	msm8x60_allocate_memory_regions();
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	msm8x60_allocate_ram_dump_memory_regions();
#if defined(CONFIG_LGE_HANDLE_PANIC)
	msm8x60_allocate_crash_callstack_regions();
#endif
#endif
}

static void __init msm8x60_init_tlmm(void)
{
	if (machine_is_msm8x60_rumi3())
		msm_gpio_install_direct_irq(0, 0, 1);
}

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC5_SUPPORT))

/* 8x60 is having 5 SDCC controllers */
#define MAX_SDCC_CONTROLLER	5

struct msm_sdcc_gpio {
	/* maximum 10 GPIOs per SDCC controller */
	s16 no;
	/* name of this GPIO */
	const char *name;
	bool always_on;
	bool is_enabled;
};

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct msm_sdcc_gpio sdc1_gpio_cfg[] = {
	{159, "sdc1_dat_0"},
	{160, "sdc1_dat_1"},
	{161, "sdc1_dat_2"},
	{162, "sdc1_dat_3"},
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	{163, "sdc1_dat_4"},
	{164, "sdc1_dat_5"},
	{165, "sdc1_dat_6"},
	{166, "sdc1_dat_7"},
#endif
	{167, "sdc1_clk"},
	{168, "sdc1_cmd"}
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct msm_sdcc_gpio sdc2_gpio_cfg[] = {
	{143, "sdc2_dat_0"},
	{144, "sdc2_dat_1", 1},
	{145, "sdc2_dat_2"},
	{146, "sdc2_dat_3"},
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	{147, "sdc2_dat_4"},
	{148, "sdc2_dat_5"},
	{149, "sdc2_dat_6"},
	{150, "sdc2_dat_7"},
#endif
	{151, "sdc2_cmd"},
	{152, "sdc2_clk", 1}
};
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
static struct msm_sdcc_gpio sdc5_gpio_cfg[] = {
	{95, "sdc5_cmd"},
	{96, "sdc5_dat_3"},
	{97, "sdc5_clk", 1},
	{98, "sdc5_dat_2"},
	{99, "sdc5_dat_1", 1},
	{100, "sdc5_dat_0"}
};
#endif

struct msm_sdcc_pad_pull_cfg {
	enum msm_tlmm_pull_tgt pull;
	u32 pull_val;
};

struct msm_sdcc_pad_drv_cfg {
	enum msm_tlmm_hdrive_tgt drv;
	u32 drv_val;
};

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct msm_sdcc_pad_drv_cfg sdc3_pad_on_drv_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_8MA}
};

static struct msm_sdcc_pad_pull_cfg sdc3_pad_on_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_sdcc_pad_drv_cfg sdc3_pad_off_drv_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_2MA}
};

#ifdef CONFIG_LGE_PM_CURRENT_CONSUMPTION_FIX  //hyogook.lee@lge.com, 2011.08.24, SDC3 DATA Line PU for reduce sleep (200uA)
static struct msm_sdcc_pad_pull_cfg sdc3_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};
#else
static struct msm_sdcc_pad_pull_cfg sdc3_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_DOWN}
};
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct msm_sdcc_pad_drv_cfg sdc4_pad_on_drv_cfg[] = {
	{TLMM_HDRV_SDC4_CLK, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC4_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC4_DATA, GPIO_CFG_8MA}
};

static struct msm_sdcc_pad_pull_cfg sdc4_pad_on_pull_cfg[] = {
	{TLMM_PULL_SDC4_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC4_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_sdcc_pad_drv_cfg sdc4_pad_off_drv_cfg[] = {
	{TLMM_HDRV_SDC4_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC4_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC4_DATA, GPIO_CFG_2MA}
};

static struct msm_sdcc_pad_pull_cfg sdc4_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC4_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC4_DATA, GPIO_CFG_PULL_DOWN}
};
#endif

struct msm_sdcc_pin_cfg {
	/*
	 * = 1 if controller pins are using gpios
	 * = 0 if controller has dedicated MSM pins
	 */
	u8 is_gpio;
	u8 cfg_sts;
	u8 gpio_data_size;
	struct msm_sdcc_gpio *gpio_data;
	struct msm_sdcc_pad_drv_cfg *pad_drv_on_data;
	struct msm_sdcc_pad_drv_cfg *pad_drv_off_data;
	struct msm_sdcc_pad_pull_cfg *pad_pull_on_data;
	struct msm_sdcc_pad_pull_cfg *pad_pull_off_data;
	u8 pad_drv_data_size;
	u8 pad_pull_data_size;
	u8 sdio_lpm_gpio_cfg;
};


static struct msm_sdcc_pin_cfg sdcc_pin_cfg_data[MAX_SDCC_CONTROLLER] = {
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	[0] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc1_gpio_cfg),
		.gpio_data = sdc1_gpio_cfg
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	[1] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc2_gpio_cfg),
		.gpio_data = sdc2_gpio_cfg
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	[2] = {
		.is_gpio = 0,
		.pad_drv_on_data = sdc3_pad_on_drv_cfg,
		.pad_drv_off_data = sdc3_pad_off_drv_cfg,
		.pad_pull_on_data = sdc3_pad_on_pull_cfg,
		.pad_pull_off_data = sdc3_pad_off_pull_cfg,
		.pad_drv_data_size = ARRAY_SIZE(sdc3_pad_on_drv_cfg),
		.pad_pull_data_size = ARRAY_SIZE(sdc3_pad_on_pull_cfg)
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	[3] = {
		.is_gpio = 0,
		.pad_drv_on_data = sdc4_pad_on_drv_cfg,
		.pad_drv_off_data = sdc4_pad_off_drv_cfg,
		.pad_pull_on_data = sdc4_pad_on_pull_cfg,
		.pad_pull_off_data = sdc4_pad_off_pull_cfg,
		.pad_drv_data_size = ARRAY_SIZE(sdc4_pad_on_drv_cfg),
		.pad_pull_data_size = ARRAY_SIZE(sdc4_pad_on_pull_cfg)
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
	[4] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc5_gpio_cfg),
		.gpio_data = sdc5_gpio_cfg
	}
#endif
};

static int msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct msm_sdcc_pin_cfg *curr;
	int n;

	curr = &sdcc_pin_cfg_data[dev_id - 1];
	if (!curr->gpio_data)
		goto out;

	for (n = 0; n < curr->gpio_data_size; n++) {
		if (enable) {

			if (curr->gpio_data[n].always_on &&
				curr->gpio_data[n].is_enabled)
				continue;
			pr_debug("%s: enable: %s\n", __func__,
					curr->gpio_data[n].name);
			rc = gpio_request(curr->gpio_data[n].no,
				curr->gpio_data[n].name);
			if (rc) {
				pr_err("%s: gpio_request(%d, %s)"
					"failed", __func__,
					curr->gpio_data[n].no,
					curr->gpio_data[n].name);
				goto free_gpios;
			}
			/* set direction as output for all GPIOs */
			rc = gpio_direction_output(
				curr->gpio_data[n].no, 1);
			if (rc) {
				pr_err("%s: gpio_direction_output"
					"(%d, 1) failed\n", __func__,
					curr->gpio_data[n].no);
				goto free_gpios;
			}
			curr->gpio_data[n].is_enabled = 1;
		} else {
			/*
			 * now free this GPIO which will put GPIO
			 * in low power mode and will also put GPIO
			 * in input mode
			 */
			if (curr->gpio_data[n].always_on)
				continue;
			pr_debug("%s: disable: %s\n", __func__,
					curr->gpio_data[n].name);
			gpio_free(curr->gpio_data[n].no);
			curr->gpio_data[n].is_enabled = 0;
		}
	}
	curr->cfg_sts = enable;
	goto out;

free_gpios:
	for (; n >= 0; n--)
		gpio_free(curr->gpio_data[n].no);
out:
	return rc;
}

static int msm_sdcc_setup_pad(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct msm_sdcc_pin_cfg *curr;
	int n;

	curr = &sdcc_pin_cfg_data[dev_id - 1];
	if (!curr->pad_drv_on_data || !curr->pad_pull_on_data)
		goto out;

	if (enable) {
		/*
		 * set up the normal driver strength and
		 * pull config for pads
		 */
		for (n = 0; n < curr->pad_drv_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_drv_on_data[n].drv ==
						TLMM_HDRV_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_hdrive(curr->pad_drv_on_data[n].drv,
				curr->pad_drv_on_data[n].drv_val);
		}
		for (n = 0; n < curr->pad_pull_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_pull_on_data[n].pull ==
						TLMM_PULL_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_pull(curr->pad_pull_on_data[n].pull,
				curr->pad_pull_on_data[n].pull_val);
		}
	} else {
		/* set the low power config for pads */
		for (n = 0; n < curr->pad_drv_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_drv_off_data[n].drv ==
						TLMM_HDRV_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_hdrive(
				curr->pad_drv_off_data[n].drv,
				curr->pad_drv_off_data[n].drv_val);
		}
		for (n = 0; n < curr->pad_pull_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_pull_off_data[n].pull ==
						TLMM_PULL_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_pull(
				curr->pad_pull_off_data[n].pull,
				curr->pad_pull_off_data[n].pull_val);
		}
	}
	curr->cfg_sts = enable;
out:
	return rc;
}

struct sdcc_reg {
	/* VDD/VCC/VCCQ regulator name on PMIC8058/PMIC8089*/
	const char *reg_name;
	/*
	 * is set voltage supported for this regulator?
	 * 0 = not supported, 1 = supported
	 */
	unsigned char set_voltage_sup;
	/* voltage level to be set */
	unsigned int level;
	/* VDD/VCC/VCCQ voltage regulator handle */
	struct regulator *reg;
	/* is this regulator enabled? */
	bool enabled;
	/* is this regulator needs to be always on? */
	bool always_on;
	/* is operating power mode setting required for this regulator? */
	bool op_pwr_mode_sup;
	/* Load values for low power and high power mode */
	unsigned int lpm_uA;
	unsigned int hpm_uA;
};
/* all SDCC controllers requires VDD/VCC voltage */
static struct sdcc_reg sdcc_vdd_reg_data[MAX_SDCC_CONTROLLER];
/* only SDCC1 requires VCCQ voltage */
static struct sdcc_reg sdcc_vccq_reg_data[1];
/* all SDCC controllers may require voting for VDD PAD voltage */
static struct sdcc_reg sdcc_vddp_reg_data[MAX_SDCC_CONTROLLER];

struct sdcc_reg_data {
	struct sdcc_reg *vdd_data; /* keeps VDD/VCC regulator info */
	struct sdcc_reg *vccq_data; /* keeps VCCQ regulator info */
	struct sdcc_reg *vddp_data; /* keeps VDD Pad regulator info */
	unsigned char sts; /* regulator enable/disable status */
};
/* msm8x60 have 5 SDCC controllers */
static struct sdcc_reg_data sdcc_vreg_data[MAX_SDCC_CONTROLLER];

static int msm_sdcc_vreg_init_reg(struct sdcc_reg *vreg)
{
	int rc = 0;

	/* Get the regulator handle */
	vreg->reg = regulator_get(NULL, vreg->reg_name);
	if (IS_ERR(vreg->reg)) {
		rc = PTR_ERR(vreg->reg);
		pr_err("%s: regulator_get(%s) failed. rc=%d\n",
			__func__, vreg->reg_name, rc);
		goto out;
	}

	/* Set the voltage level if required */
	if (vreg->set_voltage_sup) {
		rc = regulator_set_voltage(vreg->reg, vreg->level,
					vreg->level);
		if (rc) {
			pr_err("%s: regulator_set_voltage(%s) failed rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto vreg_put;
		}
	}
	goto out;

vreg_put:
	regulator_put(vreg->reg);
out:
	return rc;
}

static inline void msm_sdcc_vreg_deinit_reg(struct sdcc_reg *vreg)
{
	regulator_put(vreg->reg);
}

/* this init function should be called only once for each SDCC */
static int msm_sdcc_vreg_init(int dev_id, unsigned char init)
{
	int rc = 0;
	struct sdcc_reg *curr_vdd_reg, *curr_vccq_reg, *curr_vddp_reg;
	struct sdcc_reg_data *curr;

	curr = &sdcc_vreg_data[dev_id - 1];
	curr_vdd_reg = curr->vdd_data;
	curr_vccq_reg = curr->vccq_data;
	curr_vddp_reg = curr->vddp_data;

	if (init) {
		/*
		 * get the regulator handle from voltage regulator framework
		 * and then try to set the voltage level for the regulator
		 */
		if (curr_vdd_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vdd_reg);
			if (rc)
				goto out;
		}
		if (curr_vccq_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vccq_reg);
			if (rc)
				goto vdd_reg_deinit;
		}
		if (curr_vddp_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vddp_reg);
			if (rc)
				goto vccq_reg_deinit;
		}
		goto out;
	} else
		/* deregister with all regulators from regulator framework */
		goto vddp_reg_deinit;

vddp_reg_deinit:
	if (curr_vddp_reg)
		msm_sdcc_vreg_deinit_reg(curr_vddp_reg);
vccq_reg_deinit:
	if (curr_vccq_reg)
		msm_sdcc_vreg_deinit_reg(curr_vccq_reg);
vdd_reg_deinit:
	if (curr_vdd_reg)
		msm_sdcc_vreg_deinit_reg(curr_vdd_reg);
out:
	return rc;
}

static int msm_sdcc_vreg_enable(struct sdcc_reg *vreg)
{
	int rc;

	if (!vreg->enabled) {
		rc = regulator_enable(vreg->reg);
		if (rc) {
			pr_err("%s: regulator_enable(%s) failed. rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto out;
		}
		vreg->enabled = 1;
	}

	/* Put always_on regulator in HPM (high power mode) */
	if (vreg->always_on && vreg->op_pwr_mode_sup) {
		rc = regulator_set_optimum_mode(vreg->reg, vreg->hpm_uA);
		if (rc < 0) {
			pr_err("%s: reg=%s: HPM setting failed"
				" hpm_uA=%d, rc=%d\n",
				__func__, vreg->reg_name,
				vreg->hpm_uA, rc);
			goto vreg_disable;
		}
		rc = 0;
	}
	goto out;

vreg_disable:
	regulator_disable(vreg->reg);
	vreg->enabled = 0;
out:
	return rc;
}

static int msm_sdcc_vreg_disable(struct sdcc_reg *vreg)
{
	int rc;

	/* Never disable always_on regulator */
	if (!vreg->always_on) {
		rc = regulator_disable(vreg->reg);
		if (rc) {
			pr_err("%s: regulator_disable(%s) failed. rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto out;
		}
		vreg->enabled = 0;
	}

	/* Put always_on regulator in LPM (low power mode) */
	if (vreg->always_on && vreg->op_pwr_mode_sup) {
		rc = regulator_set_optimum_mode(vreg->reg, vreg->lpm_uA);
		if (rc < 0) {
			pr_err("%s: reg=%s: LPM setting failed"
				" lpm_uA=%d, rc=%d\n",
				__func__,
				vreg->reg_name,
				vreg->lpm_uA, rc);
			goto out;
		}
		rc = 0;
	}

out:
	return rc;
}

static int msm_sdcc_setup_vreg(int dev_id, unsigned char enable)
{
	int rc = 0;
	int l_changed = 0;
	struct sdcc_reg *curr_vdd_reg, *curr_vccq_reg, *curr_vddp_reg;
	struct sdcc_reg_data *curr;

	curr = &sdcc_vreg_data[dev_id - 1];
	curr_vdd_reg = curr->vdd_data;
	curr_vccq_reg = curr->vccq_data;
	curr_vddp_reg = curr->vddp_data;

	/* check if regulators are initialized or not? */
	if ((curr_vdd_reg && !curr_vdd_reg->reg) ||
		(curr_vccq_reg && !curr_vccq_reg->reg) ||
		(curr_vddp_reg && !curr_vddp_reg->reg)) {
		/* initialize voltage regulators required for this SDCC */
		rc = msm_sdcc_vreg_init(dev_id, 1);
		if (rc) {
			pr_err("%s: regulator init failed = %d\n",
				__func__, rc);
			goto out;
		}
	}

	if((dev_id == 3)&&(g_sd_power_dircect_ctrl))
	{
		printk("\n(SD power will be OFFed!! (ctrl_flag:%d)\n",g_sd_power_dircect_ctrl);
		curr_vdd_reg->always_on = 0;
		l_changed = 1;
	}

	if (curr->sts == enable)
		goto out;

	if (curr_vdd_reg) {
		if (enable)
		{
			rc = msm_sdcc_vreg_enable(curr_vdd_reg);
		}
		else
		{
			rc = msm_sdcc_vreg_disable(curr_vdd_reg);
		}
		if (rc)
			goto out;
	}

	if (curr_vccq_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vccq_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vccq_reg);
		if (rc)
			goto out;
	}

	if (curr_vddp_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vddp_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vddp_reg);
		if (rc)
			goto out;
	}
	curr->sts = enable;

out:
	if(l_changed)
	{
		curr_vdd_reg->always_on = 1;
		g_sd_power_dircect_ctrl = 0;
		printk("\n(SD power direct ctrl flag restored! (ctrl_flag:%d)\n",g_sd_power_dircect_ctrl);
	}
	return rc;
}

static u32 msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	u32 rc_pin_cfg = 0;
	u32 rc_vreg_cfg = 0;
	u32 rc = 0;
	struct platform_device *pdev;
	struct msm_sdcc_pin_cfg *curr_pin_cfg;

	pdev = container_of(dv, struct platform_device, dev);

	/* setup gpio/pad */
	curr_pin_cfg = &sdcc_pin_cfg_data[pdev->id - 1];
	if (curr_pin_cfg->cfg_sts == !!vdd)
		goto setup_vreg;

	if (curr_pin_cfg->is_gpio)
		rc_pin_cfg = msm_sdcc_setup_gpio(pdev->id, !!vdd);
	else
		rc_pin_cfg = msm_sdcc_setup_pad(pdev->id, !!vdd);

setup_vreg:
	/* setup voltage regulators */
	rc_vreg_cfg = msm_sdcc_setup_vreg(pdev->id, !!vdd);

	if (rc_pin_cfg || rc_vreg_cfg)
		rc = rc_pin_cfg ? rc_pin_cfg : rc_vreg_cfg;

	return rc;
}

#if (defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC5_SUPPORT))
static void msm_sdcc_sdio_lpm_gpio(struct device *dv, unsigned int active)
{
	struct msm_sdcc_pin_cfg *curr_pin_cfg;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	/* setup gpio/pad */
	curr_pin_cfg = &sdcc_pin_cfg_data[pdev->id - 1];

	if (curr_pin_cfg->cfg_sts == active)
		return;

	curr_pin_cfg->sdio_lpm_gpio_cfg = 1;
	if (curr_pin_cfg->is_gpio)
		msm_sdcc_setup_gpio(pdev->id, active);
	else
		msm_sdcc_setup_pad(pdev->id, active);
	curr_pin_cfg->sdio_lpm_gpio_cfg = 0;
}
#endif

static int msm_sdc3_get_wpswitch(struct device *dev)
{
	struct platform_device *pdev;
	int status;
	pdev = container_of(dev, struct platform_device, dev);

	status = gpio_request(GPIO_SDC_WP, "SD_WP_Switch");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n",
					__func__, GPIO_SDC_WP);
	} else {
		status = gpio_direction_input(GPIO_SDC_WP);
		if (!status) {
			status = gpio_get_value_cansleep(GPIO_SDC_WP);
			pr_info("%s: WP Status for Slot %d = %d\n",
				 __func__, pdev->id, status);
		}
		gpio_free(GPIO_SDC_WP);
	}
	return status;
}

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
int sdc5_register_status_notify(void (*callback)(int, void *),
	void *dev_id)
{
	sdc5_status_notify_cb = callback;
	sdc5_status_notify_cb_devid = dev_id;
	return 0;
}
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
int sdc2_register_status_notify(void (*callback)(int, void *),
	void *dev_id)
{
	sdc2_status_notify_cb = callback;
	sdc2_status_notify_cb_devid = dev_id;
	return 0;
}
#endif

/* Interrupt handler for SDC2 and SDC5 detection
 * This function uses dual-edge interrputs settings in order
 * to get SDIO detection when the GPIO is rising and SDIO removal
 * when the GPIO is falling */

char mdm_error_fatal_for_did =0;
static irqreturn_t msm8x60_multi_sdio_slot_status_irq(int irq, void *dev_id)
{
	int status;

//	if (!machine_is_msm8x60_charm_surf() &&
//	    !machine_is_msm8x60_charm_ffa())
//		return IRQ_NONE;

	status = gpio_get_value(MDM2AP_SYNC);
	pr_info("%s: MDM2AP_SYNC Status = %d\n",
		 __func__, status);

    //[START]2011.08.09 younghoon.jeung temporary flag for MDM error dump
    if(!status)
    {
        mdm_error_fatal_for_did = 1;
        pr_info("mdm_error_fatal_for_did = TRUE\n");
    }
    else
        mdm_error_fatal_for_did = 0;
    //[END]2011.08.09 younghoon.jeung temporary flag for MDM error dump

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	if (sdc2_status_notify_cb) {
		pr_info("%s: calling sdc2_status_notify_cb\n", __func__);
		sdc2_status_notify_cb(status,
			sdc2_status_notify_cb_devid);
	}
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
	if (sdc5_status_notify_cb) {
		pr_info("%s: calling sdc5_status_notify_cb\n", __func__);
		sdc5_status_notify_cb(status,
			sdc5_status_notify_cb_devid);
	}
#endif
	return IRQ_HANDLED;
}

static int msm8x60_multi_sdio_init(void)
{
	int ret, irq_num;

//	if (!machine_is_msm8x60_charm_surf() &&
//	    !machine_is_msm8x60_charm_ffa())
//		return 0;

	ret = msm_gpiomux_get(MDM2AP_SYNC);
	if (ret) {
		pr_err("%s:Failed to request GPIO %d, ret=%d\n",
					__func__, MDM2AP_SYNC, ret);
		return ret;
	}

	irq_num = gpio_to_irq(MDM2AP_SYNC);

	ret = request_irq(irq_num,
		msm8x60_multi_sdio_slot_status_irq,
		IRQ_TYPE_EDGE_BOTH,
		"sdio_multidetection", NULL);

	if (ret) {
		pr_err("%s:Failed to request irq, ret=%d\n",
					__func__, ret);
		return ret;
	}

	return ret;
}

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int msm8x60_sdcc_slot_status(struct device *dev)
{
	int status;

	status = gpio_request(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1)
				, "SD_HW_Detect");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n", __func__,
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
	} else {
		status = gpio_direction_input(
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
		if (!status){
			status = !(gpio_get_value_cansleep(
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1)));
#if defined(CONFIG_LGE_PMIC8058_GPIO)
			if(lge_bd_rev < LGE_REV_B) status = !status;
#endif
		}
		gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
	}
	return (unsigned int) status;
}
#endif
#endif

#ifdef	CONFIG_MMC_MSM_SDC4_SUPPORT
#if defined(CONFIG_LGE_BCM432X_PATCH)
#else
static int msm_sdcc_cfg_mpm_sdiowakeup(struct device *dev, unsigned mode)
{
	struct platform_device *pdev;
	enum msm_mpm_pin pin;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	/* Only SDCC4 slot connected to WLAN chip has wakeup capability */
	if (pdev->id == 4)
		pin = MSM_MPM_PIN_SDC4_DAT1;
	else
		return -EINVAL;

	switch (mode) {
	case SDC_DAT1_DISABLE:
		ret = msm_mpm_enable_pin(pin, 0);
		break;
	case SDC_DAT1_ENABLE:
		ret = msm_mpm_set_pin_type(pin, IRQ_TYPE_LEVEL_LOW);
		ret = msm_mpm_enable_pin(pin, 1);
		break;
	case SDC_DAT1_ENWAKE:
		ret = msm_mpm_set_pin_wake(pin, 1);
		break;
	case SDC_DAT1_DISWAKE:
		ret = msm_mpm_set_pin_wake(pin, 0);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}
#endif
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct mmc_platform_data msm8x60_sdc1_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 1,
	.pclk_src_dfab	= 1,
#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm8x60_sdc2_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_165_195,
	.translate_vdd  = msm_sdcc_setup_power,
	.sdio_lpm_gpio_setup = msm_sdcc_sdio_lpm_gpio,
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
	.register_status_notify = sdc2_register_status_notify,
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	.is_sdio_al_client = 1,
#endif
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm8x60_sdc3_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.wpswitch  	= msm_sdc3_get_wpswitch,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm8x60_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PM8058_IRQ_BASE,
				       PMIC_GPIO_SDC3_DET - 1),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
#ifdef CONFIG_MMC_MSM_SDC3_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#if defined(CONFIG_LGE_BCM432X_PATCH)
/* LGE_CHANGE_S, [yhcha @ 110303], <Add BCM4330> */
static unsigned int bcm432x_sdcc_wlan_slot_status(struct device *dev)
{
	printk(KERN_ERR "%s: %d %d\n", __func__, CONFIG_BCM4330_GPIO_WL_RESET, gpio_get_value(CONFIG_BCM4330_GPIO_WL_RESET));
    return gpio_get_value(CONFIG_BCM4330_GPIO_WL_RESET);
}

static struct mmc_platform_data bcm432x_sdcc_wlan_data = {
    .ocr_mask   	= MMC_VDD_30_31,
    .translate_vdd	= msm_sdcc_setup_power,
    .mmc_bus_width	= MMC_CAP_4_BIT_DATA,
    .status     	= bcm432x_sdcc_wlan_slot_status,
    .status_irq		= MSM_GPIO_TO_INT(CONFIG_BCM4330_GPIO_WL_RESET),
    .irq_flags		= IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
    .dummy52_required   = 1,
    .msmsdcc_fmin	= 400000,//144000,
    .msmsdcc_fmid	= 24000000,//24576000,
    .msmsdcc_fmax	= 48000000,//24576000, // 49152000,
    .nonremovable	= 0,
    .pclk_src_dfab  = 1,	// bluetooth.kang
};
/* LGE_CHANGE_E, [yhcha @ 110303] <Add BCM4330> */
#else /* qualcomm or google */
static struct mmc_platform_data msm8x60_sdc4_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
	.cfg_mpm_sdiowakeup = msm_sdcc_cfg_mpm_sdiowakeup,
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
};
#endif /* CONFIG_LGE_BCM432X_PATCH */
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
static struct mmc_platform_data msm8x60_sdc5_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_165_195,
	.translate_vdd  = msm_sdcc_setup_power,
	.sdio_lpm_gpio_setup = msm_sdcc_sdio_lpm_gpio,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
	.register_status_notify = sdc5_register_status_notify,
#ifdef CONFIG_MMC_MSM_SDC5_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	.is_sdio_al_client = 1,
#endif
};
#endif

static void __init msm8x60_init_mmc(void)
{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	/* SDCC1 : eMMC card connected */
	sdcc_vreg_data[0].vdd_data = &sdcc_vdd_reg_data[0];
	sdcc_vreg_data[0].vdd_data->reg_name = "8901_l5";
	sdcc_vreg_data[0].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[0].vdd_data->level = 2850000;
	sdcc_vreg_data[0].vdd_data->always_on = 1;
	sdcc_vreg_data[0].vdd_data->op_pwr_mode_sup = 1;
	sdcc_vreg_data[0].vdd_data->lpm_uA = 9000;
	sdcc_vreg_data[0].vdd_data->hpm_uA = 200000;

	sdcc_vreg_data[0].vccq_data = &sdcc_vccq_reg_data[0];
	sdcc_vreg_data[0].vccq_data->reg_name = "8901_lvs0";
	sdcc_vreg_data[0].vccq_data->set_voltage_sup = 0;
	sdcc_vreg_data[0].vccq_data->always_on = 1;

	msm_add_sdcc(1, &msm8x60_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	/*
	 * MDM SDIO client is connected to SDC2 on charm SURF/FFA
	 * and no card is connected on 8660 SURF/FFA/FLUID.
	 */
	sdcc_vreg_data[1].vdd_data = &sdcc_vdd_reg_data[1];
	sdcc_vreg_data[1].vdd_data->reg_name = "8058_s3";
	sdcc_vreg_data[1].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[1].vdd_data->level = 1800000;

	sdcc_vreg_data[1].vccq_data = NULL;

//	if (machine_is_msm8x60_charm_surf())
		msm8x60_sdc2_data.msmsdcc_fmax = 24000000;
//	if (machine_is_msm8x60_charm_surf() || machine_is_msm8x60_charm_ffa())
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
		msm8x60_sdc2_data.sdiowakeup_irq = gpio_to_irq(144);
		msm_sdcc_setup_gpio(2, 1);
#endif
		msm_add_sdcc(2, &msm8x60_sdc2_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	/* SDCC3 : External card slot connected */
	sdcc_vreg_data[2].vdd_data = &sdcc_vdd_reg_data[2];
	sdcc_vreg_data[2].vdd_data->reg_name = "8058_l14";
	sdcc_vreg_data[2].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[2].vdd_data->level = 2850000;
#if 0//def CONFIG_LGE_PMIC8058_REGULATOR //platform-bsp@lge.com : SD-card VDD shutdown@sleep
	sdcc_vreg_data[2].vdd_data->always_on = 0;
#else
	sdcc_vreg_data[2].vdd_data->always_on = 1;
#endif
	sdcc_vreg_data[2].vdd_data->op_pwr_mode_sup = 1;
	sdcc_vreg_data[2].vdd_data->lpm_uA = 9000;
	sdcc_vreg_data[2].vdd_data->hpm_uA = 200000;

	sdcc_vreg_data[2].vccq_data = NULL;

	sdcc_vreg_data[2].vddp_data = &sdcc_vddp_reg_data[2];
	sdcc_vreg_data[2].vddp_data->reg_name = "8058_l5";
	sdcc_vreg_data[2].vddp_data->set_voltage_sup = 1;
	sdcc_vreg_data[2].vddp_data->level = 2850000;
	sdcc_vreg_data[2].vddp_data->always_on = 1;
	sdcc_vreg_data[2].vddp_data->op_pwr_mode_sup = 1;
	/* Sleep current required is ~300 uA. But min. RPM
	 * vote can be in terms of mA (min. 1 mA).
	 * So let's vote for 2 mA during sleep.
	 */
	sdcc_vreg_data[2].vddp_data->lpm_uA = 2000;
	/* Max. Active current required is 16 mA */
	sdcc_vreg_data[2].vddp_data->hpm_uA = 16000;

	if (machine_is_lge_i_board())
		msm8x60_sdc3_data.wpswitch = NULL;
	msm_add_sdcc(3, &msm8x60_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	/* SDCC4 : WLAN WCN1314 chip is connected */
	sdcc_vreg_data[3].vdd_data = &sdcc_vdd_reg_data[3];
	sdcc_vreg_data[3].vdd_data->reg_name = "8058_s3";
	sdcc_vreg_data[3].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[3].vdd_data->level = 1800000;

	sdcc_vreg_data[3].vccq_data = NULL;

/* LGE_CHANGE_S, [gsm-wifi@lge.com], 2011-02-16, <Add BCM4330> */
#if defined(CONFIG_LGE_BCM432X_PATCH)
	/* GPIO config */
	gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4330_GPIO_WL_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(CONFIG_BCM4330_GPIO_WL_RESET, 0);
	
	gpio_tlmm_config(GPIO_CFG(CONFIG_BCM4330_GPIO_WL_HOSTWAKEUP, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	/* Register platform device */
	msm_add_sdcc(4, &bcm432x_sdcc_wlan_data);

	/* Enable RESET IRQ for wlan card detect */
	enable_irq(gpio_to_irq(CONFIG_BCM4330_GPIO_WL_RESET));
#else /* qualcomm or google */
	msm_add_sdcc(4, &msm8x60_sdc4_data);
#endif /* CONFIG_LGE_BCM432X_PATCH */
/* LGE_CHANGE_E, [gsm-wifi@lge.com], 2011-02-16, <Add BCM4330> */	
#endif

#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
	/*
	 * MDM SDIO client is connected to SDC5 on charm SURF/FFA
	 * and no card is connected on 8660 SURF/FFA/FLUID.
	 */
	sdcc_vreg_data[4].vdd_data = &sdcc_vdd_reg_data[4];
	sdcc_vreg_data[4].vdd_data->reg_name = "8058_s3";
	sdcc_vreg_data[4].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[4].vdd_data->level = 1800000;

	sdcc_vreg_data[4].vccq_data = NULL;

//	if (machine_is_msm8x60_charm_surf())
		msm8x60_sdc5_data.msmsdcc_fmax = 24000000;
//	if (machine_is_msm8x60_charm_surf() || machine_is_msm8x60_charm_ffa())
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
		msm8x60_sdc5_data.sdiowakeup_irq = gpio_to_irq(99);
		msm_sdcc_setup_gpio(5, 1);
#endif
		msm_add_sdcc(5, &msm8x60_sdc5_data);
#endif
}

#if 0                           /* byongdoo.oh@lge.com temporary block  */
#if (defined(CONFIG_MARIMBA_CORE)) && \
	(defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))

static const struct {
	char *name;
	int vmin;
	int vmax;
} bt_regs_info[] = {
	{ "8058_s3", 1800000, 1800000 },
	{ "8058_s2", 1300000, 1300000 },
	{ "8058_l8", 2900000, 3050000 },
};

static struct {
	bool enabled;
} bt_regs_status[] = {
	{ false },
	{ false },
	{ false },
};
static struct regulator *bt_regs[ARRAY_SIZE(bt_regs_info)];

static int bahama_bt(int on)
{
	int rc;
	int i;
	struct marimba config = { .mod_id =  SLAVE_ID_BAHAMA};

	struct bahama_variant_register {
		const size_t size;
		const struct bahama_config_register *set;
	};

	const struct bahama_config_register *p;

	u8 version;

	const struct bahama_config_register v10_bt_on[] = {
		{ 0xE9, 0x00, 0xFF },
		{ 0xF4, 0x80, 0xFF },
		{ 0xE4, 0x00, 0xFF },
		{ 0xE5, 0x00, 0x0F },
#ifdef CONFIG_WLAN
		{ 0xE6, 0x38, 0x7F },
		{ 0xE7, 0x06, 0xFF },
#endif
		{ 0xE9, 0x21, 0xFF },
		{ 0x01, 0x0C, 0x1F },
		{ 0x01, 0x08, 0x1F },
	};

	const struct bahama_config_register v20_bt_on_fm_off[] = {
		{ 0x11, 0x0C, 0xFF },
		{ 0x13, 0x01, 0xFF },
		{ 0xF4, 0x80, 0xFF },
		{ 0xF0, 0x00, 0xFF },
		{ 0xE9, 0x00, 0xFF },
#ifdef CONFIG_WLAN
		{ 0x81, 0x00, 0x7F },
		{ 0x82, 0x00, 0xFF },
		{ 0xE6, 0x38, 0x7F },
		{ 0xE7, 0x06, 0xFF },
#endif
		{ 0xE9, 0x21, 0xFF },
	};

	const struct bahama_config_register v20_bt_on_fm_on[] = {
		{ 0x11, 0x0C, 0xFF },
		{ 0x13, 0x01, 0xFF },
		{ 0xF4, 0x86, 0xFF },
		{ 0xF0, 0x06, 0xFF },
		{ 0xE9, 0x00, 0xFF },
#ifdef CONFIG_WLAN
		{ 0x81, 0x00, 0x7F },
		{ 0x82, 0x00, 0xFF },
		{ 0xE6, 0x38, 0x7F },
		{ 0xE7, 0x06, 0xFF },
#endif
		{ 0xE9, 0x21, 0xFF },
	};

	const struct bahama_config_register v10_bt_off[] = {
		{ 0xE9, 0x00, 0xFF },
	};

	const struct bahama_config_register v20_bt_off_fm_off[] = {
		{ 0xF4, 0x84, 0xFF },
		{ 0xF0, 0x04, 0xFF },
		{ 0xE9, 0x00, 0xFF }
	};

	const struct bahama_config_register v20_bt_off_fm_on[] = {
		{ 0xF4, 0x86, 0xFF },
		{ 0xF0, 0x06, 0xFF },
		{ 0xE9, 0x00, 0xFF }
	};
	const struct bahama_variant_register bt_bahama[2][3] = {
		{
			{ ARRAY_SIZE(v10_bt_off), v10_bt_off },
			{ ARRAY_SIZE(v20_bt_off_fm_off), v20_bt_off_fm_off },
			{ ARRAY_SIZE(v20_bt_off_fm_on), v20_bt_off_fm_on }
		},
		{
			{ ARRAY_SIZE(v10_bt_on), v10_bt_on },
			{ ARRAY_SIZE(v20_bt_on_fm_off), v20_bt_on_fm_off },
			{ ARRAY_SIZE(v20_bt_on_fm_on), v20_bt_on_fm_on }
		}
	};

	u8 offset = 0; /* index into bahama configs */

	on = on ? 1 : 0;
	version = read_bahama_ver();

	if (version ==  VER_UNSUPPORTED) {
		dev_err(&msm_bt_power_device.dev,
			"%s: unsupported version\n",
			__func__);
		return -EIO;
	}

	if (version == VER_2_0) {
		if (marimba_get_fm_status(&config))
			offset = 0x01;
	}

	/* Voting off 1.3V S2 Regulator,BahamaV2 used in Normal mode */
	if (on && (version == VER_2_0)) {
		for (i = 0; i < ARRAY_SIZE(bt_regs_info); i++) {
			if ((!strcmp(bt_regs_info[i].name, "8058_s2"))
				&& (bt_regs_status[i].enabled == true)) {
				if (regulator_disable(bt_regs[i])) {
					dev_err(&msm_bt_power_device.dev,
						"%s: regulator disable failed",
						__func__);
				}
				bt_regs_status[i].enabled = false;
				break;
			}
		}
	}

	p = bt_bahama[on][version + offset].set;

	dev_info(&msm_bt_power_device.dev,
		"%s: found version %d\n", __func__, version);

	for (i = 0; i < bt_bahama[on][version + offset].size; i++) {
		u8 value = (p+i)->value;
		rc = marimba_write_bit_mask(&config,
			(p+i)->reg,
			&value,
			sizeof((p+i)->value),
			(p+i)->mask);
		if (rc < 0) {
			dev_err(&msm_bt_power_device.dev,
				"%s: reg %d write failed: %d\n",
				__func__, (p+i)->reg, rc);
			return rc;
		}
		dev_dbg(&msm_bt_power_device.dev,
			"%s: reg 0x%02x write value 0x%02x mask 0x%02x\n",
				__func__, (p+i)->reg,
				value, (p+i)->mask);
	}
	/* Update BT Status */
	if (on)
		marimba_set_bt_status(&config, true);
	else
		marimba_set_bt_status(&config, false);

	return 0;
}

static int bluetooth_use_regulators(int on)
{
	int i, recover = -1, rc = 0;

	for (i = 0; i < ARRAY_SIZE(bt_regs_info); i++) {
		bt_regs[i] = on ? regulator_get(&msm_bt_power_device.dev,
						bt_regs_info[i].name) :
				(regulator_put(bt_regs[i]), NULL);
		if (IS_ERR(bt_regs[i])) {
			rc = PTR_ERR(bt_regs[i]);
			dev_err(&msm_bt_power_device.dev,
				"regulator %s get failed (%d)\n",
				bt_regs_info[i].name, rc);
			recover = i - 1;
			bt_regs[i] = NULL;
			break;
		}

		if (!on)
			continue;

		rc = regulator_set_voltage(bt_regs[i],
					  bt_regs_info[i].vmin,
					  bt_regs_info[i].vmax);
		if (rc < 0) {
			dev_err(&msm_bt_power_device.dev,
				"regulator %s voltage set (%d)\n",
				bt_regs_info[i].name, rc);
			recover = i;
			break;
		}
	}

	if (on && (recover > -1))
		for (i = recover; i >= 0; i--) {
			regulator_put(bt_regs[i]);
			bt_regs[i] = NULL;
		}

	return rc;
}

static int bluetooth_switch_regulators(int on)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(bt_regs_info); i++) {
		if (on && (bt_regs_status[i].enabled == false)) {
			rc = regulator_enable(bt_regs[i]);
			if (rc < 0) {
				dev_err(&msm_bt_power_device.dev,
					"regulator %s %s failed (%d)\n",
					bt_regs_info[i].name,
					"enable", rc);
				if (i > 0) {
					while (--i) {
						regulator_disable(bt_regs[i]);
						bt_regs_status[i].enabled
								 = false;
					}
					break;
				}
			}
			bt_regs_status[i].enabled = true;
		} else if (!on && (bt_regs_status[i].enabled == true)) {
			rc = regulator_disable(bt_regs[i]);
			if (rc < 0) {
				dev_err(&msm_bt_power_device.dev,
					"regulator %s %s failed (%d)\n",
					bt_regs_info[i].name,
					"disable", rc);
				break;
			}
			bt_regs_status[i].enabled = false;
		}
	}
	return rc;
}

static struct msm_xo_voter *bt_clock;

static int bluetooth_power(int on)
{
	int rc = 0;
	int id;

	/* In case probe function fails, cur_connv_type would be -1 */
	id = adie_get_detected_connectivity_type();
	if (id != BAHAMA_ID) {
		pr_err("%s: unexpected adie connectivity type: %d\n",
			__func__, id);
		return -ENODEV;
	}

	if (on) {

		rc = bluetooth_use_regulators(1);
		if (rc < 0)
			goto out;

		rc = bluetooth_switch_regulators(1);

		if (rc < 0)
			goto fail_put;

		bt_clock = msm_xo_get(MSM_XO_TCXO_D0, "bt_power");

		if (IS_ERR(bt_clock)) {
			pr_err("Couldn't get TCXO_D0 voter\n");
			goto fail_switch;
		}

		rc = msm_xo_mode_vote(bt_clock, MSM_XO_MODE_ON);

		if (rc < 0) {
			pr_err("Failed to vote for TCXO_DO ON\n");
			goto fail_vote;
		}

		rc = bahama_bt(1);

		if (rc < 0)
			goto fail_clock;

		msleep(10);

		rc = msm_xo_mode_vote(bt_clock, MSM_XO_MODE_PIN_CTRL);

		if (rc < 0) {
			pr_err("Failed to vote for TCXO_DO pin control\n");
			goto fail_vote;
		}
	} else {
		/* check for initial RFKILL block (power off) */
		/* some RFKILL versions/configurations rfkill_register */
		/* calls here for an initial set_block */
		/* avoid calling i2c and regulator before unblock (on) */
		if (platform_get_drvdata(&msm_bt_power_device) == NULL) {
			dev_info(&msm_bt_power_device.dev,
				"%s: initialized OFF/blocked\n", __func__);
			goto out;
		}

		bahama_bt(0);

fail_clock:
		msm_xo_mode_vote(bt_clock, MSM_XO_MODE_OFF);
fail_vote:
		msm_xo_put(bt_clock);
fail_switch:
		bluetooth_switch_regulators(0);
fail_put:
		bluetooth_use_regulators(0);
	}

out:
	if (rc < 0)
		on = 0;
	dev_info(&msm_bt_power_device.dev,
		"Bluetooth power switch: state %d result %d\n", on, rc);

	return rc;
}

#endif /*CONFIG_MARIMBA_CORE, CONFIG_MSM_BT_POWER, CONFIG_MSM_BT_POWER_MODULE*/
#endif


//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
#if 0	
/* byongdoo.oh@lge.com Broadcomm BT */
#define BT_RESET_N 138
enum {
	BT_WAKE,
	BT_HOST_WAKE,
};


static unsigned bt_config_power_on[] = {
	GPIO_CFG(137, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(127, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(BT_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* BT_RESET */
};
static unsigned bt_config_power_off[] = {
	GPIO_CFG(137, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* WAKE */
	GPIO_CFG(127, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* HOST_WAKE */
	GPIO_CFG(BT_RESET_N, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* BT_RESET */
};

static int configure_pcm_gpios(int on)
{
	int ret = 0, i;
	int pcm_gpios[] = {111, 112, 113, 114};
	for (i = 0; i < ARRAY_SIZE(pcm_gpios); i++) {
		if (on) {
			ret = msm_gpiomux_get(pcm_gpios[i]);
			if (unlikely(ret))
				break;
		} else {
			ret = msm_gpiomux_put(pcm_gpios[i]);
			if (unlikely(ret))
				return ret;
		}
	}
	if (ret)
		for (; i >= 0; i--)
			msm_gpiomux_put(pcm_gpios[i]);
	return ret;
}

static int bluetooth_power(int on)
{
  int ret, pin;

  if(on)
    {
 /*   	
      gpio_direction_output(BT_RESET_N,1);
      mdelay(100);
*/
      if(configure_uart_gpios(1))
        {
          printk(KERN_ERR"bluetooth_power on fail");
          return -EIO;
        }

      if(configure_pcm_gpios(1))
        {
          printk(KERN_ERR "bluetooth_power on fail");
          return -EIO;
        }

      for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++)
        {
          ret = gpio_tlmm_config(bt_config_power_on[pin],GPIO_CFG_ENABLE);
          if (ret) 
            {
              printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=fg%d\n",__func__, bt_config_power_on[pin], ret);
              return -EIO;
            }

		gpio_direction_output(BT_RESET_N, 0);
		mdelay(100);
		gpio_direction_output(BT_RESET_N, 1);
		mdelay(100);
        }
    }
  else
    {
      gpio_direction_output(BT_RESET_N,0);

      if(configure_uart_gpios(0))
        {
          printk(KERN_ERR"bluetooth_power on fail");
          return -EIO;
        }

      if(configure_pcm_gpios(0))
        {
          printk(KERN_ERR "bluetooth_power on fail");
          return -EIO;
        }

      for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++)
        {
          ret = gpio_tlmm_config(bt_config_power_off[pin],GPIO_CFG_ENABLE);
          if (ret) 
            {
              printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",__func__, bt_config_power_off[pin], ret);
              return -EIO;
            }
        }
    }
  return 0;
}

static void __init bt_power_init(void)
{
  int rc;
	
  printk(KERN_ERR"----------------mwKim bt Power Init ");  	 	
  rc = gpio_request(BT_RESET_N, "bt_reset_n");
  if (rc)
   {
     printk(KERN_ERR "%s: bt reset  %d request failed\n",__func__,BT_RESET_N );
     return;
   }
}

#endif
//[LGE_UPDATE_E] 20110420, sangyeol.lee@lge.com,  [END]



#ifdef CONFIG_MSM_RPM
static struct msm_rpm_platform_data msm_rpm_data = {
	.reg_base_addrs = {
		[MSM_RPM_PAGE_STATUS] = MSM_RPM_BASE,
		[MSM_RPM_PAGE_CTRL] = MSM_RPM_BASE + 0x400,
		[MSM_RPM_PAGE_REQ] = MSM_RPM_BASE + 0x600,
		[MSM_RPM_PAGE_ACK] = MSM_RPM_BASE + 0xa00,
	},

	.irq_ack = RPM_SCSS_CPU0_GP_HIGH_IRQ,
	.irq_err = RPM_SCSS_CPU0_GP_LOW_IRQ,
	.irq_vmpm = RPM_SCSS_CPU0_GP_MEDIUM_IRQ,
	.msm_apps_ipc_rpm_reg = MSM_GCC_BASE + 0x008,
	.msm_apps_ipc_rpm_val = 4,
};
#endif

void msm_fusion_setup_pinctrl(void)
{
	struct msm_xo_voter *a1;

	if (socinfo_get_platform_subtype() == 0x3) {
		/*
		 * Vote for the A1 clock to be in pin control mode before
		* the external images are loaded.
		*/
		a1 = msm_xo_get(MSM_XO_TCXO_A1, "mdm");
		BUG_ON(!a1);
		msm_xo_mode_vote(a1, MSM_XO_MODE_PIN_CTRL);
	}
}

struct msm_board_data {
	struct msm_gpiomux_configs *gpiomux_cfgs;
};
/* kwangdo.yi@lge.com [jointlab] Mon 13 Jun 2011 S
   i_atnt gpio suspend/active config added
*/
#ifdef CONFIG_LGE_BOARD_GPIO
static struct msm_board_data msm8x60_i_atnt_rev_board_data __initdata = {
	.gpiomux_cfgs = msm8x60_i_atnt_rev_gpiomux_cfgs,
};
#endif
/* kwangdo.yi@lge.com [jointlab] Mon 13 Jun 2011 E */
#ifdef CONFIG_LGE_I_POWER_REVB_GPIO
static struct msm_board_data msm8x60_p930_board_data __initdata = {
	.gpiomux_cfgs = msm8x60_p930_gpiomux_cfgs,
};
#else

static struct msm_board_data msm8x60_i_atnt_board_data __initdata = {
	.gpiomux_cfgs = msm8x60_i_atnt_gpiomux_cfgs,
};

//mipi shutdown
void *mipi_base;
void *mdp_video_base;
#endif
/* kwangdo.yi@lge.com [jointlab] Wed 30 Mar 2011 E */

static void __init msm8x60_board_init(struct msm_board_data *board_data)
{
	uint32_t soc_platform_version;

	/*
	 * Initialize RPM first as other drivers and devices may need
	 * it for their initialization.
	 */
#ifdef CONFIG_MSM_RPM
	BUG_ON(msm_rpm_init(&msm_rpm_data));
#endif
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");

	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);
#ifdef CONFIG_MSM_KGSL_2D
	msm8x60_check_2d_hardware();
#endif

	/* Change SPM handling of core 1 if PMM 8160 is present. */
	soc_platform_version = socinfo_get_platform_version();
	if (SOCINFO_VERSION_MAJOR(soc_platform_version) == 1 &&
			SOCINFO_VERSION_MINOR(soc_platform_version) >= 2) {
		struct msm_spm_platform_data *spm_data;

		spm_data = &msm_spm_data_v1[1];
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] &= ~0x0F00UL;
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] |= 0x0100UL;

		spm_data = &msm_spm_data[1];
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] &= ~0x0F00UL;
		spm_data->reg_init_values[MSM_SPM_REG_SAW_CFG] |= 0x0100UL;
	}

	/*
	 * Initialize SPM before acpuclock as the latter calls into SPM
	 * driver to set ACPU voltages.
	 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1)
		msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	else
		msm_spm_init(msm_spm_data_v1, ARRAY_SIZE(msm_spm_data_v1));

	/*
	 * Set regulators 8901_l4 and 8901_l6 to be always on in HPM for SURF
	 * devices so that the RPM doesn't drop into a low power mode that an
	 * un-reworked SURF cannot resume from.
	 */
	if (machine_is_msm8x60_surf()) {
		rpm_vreg_init_pdata[RPM_VREG_ID_PM8901_L4]
			.init_data.constraints.always_on = 1;
		rpm_vreg_init_pdata[RPM_VREG_ID_PM8901_L6]
			.init_data.constraints.always_on = 1;
	}

	/*
	 * Disable regulator info printing so that regulator registration
	 * messages do not enter the kmsg log.
	 */
	regulator_suppress_info_printing();

	/* Initialize regulators needed for clock_init. */
	platform_add_devices(early_regulators, ARRAY_SIZE(early_regulators));

	//backlight off
	gpio_tlmm_config(GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_DISABLE);
	gpio_set_value(49, 0);
	//mipi shutdown
	mipi_base = ioremap(0x4700000, 0x1000);
	mdp_video_base = ioremap(0x51e0000, 0x1000);
	
	writel(0x00000000, mdp_video_base);
	mdelay(60);
	writel(0x01010101, (mipi_base+0x10C)); 
	writel(0x13FF3BFF, (mipi_base+0x108));	
	writel(0, (mipi_base+0x200));
	writel(0, (mipi_base+0x118));
	writel(0, (mipi_base+0x000));
	iounmap(mipi_base);
	iounmap(mdp_video_base);
	msm_clock_init(msm_clocks_8x60, msm_num_clocks_8x60);

	/* Buses need to be initialized before early-device registration
	 * to get the platform data for fabrics.
	 */
	msm8x60_init_buses();
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));
	/* CPU frequency control is not supported on simulated targets. */
	msm_acpu_clock_init(&msm8x60_acpu_clock_data);

	/* No EBI2 on 8660 charm targets */
	msm8x60_init_tlmm();
	msm8x60_init_gpiomux(board_data->gpiomux_cfgs);
	msm8x60_init_uart12dm();
	msm8x60_init_mmc();
        //byongdoo.oh
//[LGE_UPDATE_S] 20110420, sangyeol.lee@lge.com, [START]
#if 0
        bt_power_init();
#else
	lge_add_btpower_devices();
#endif
//[LGE_UPDATE_E] 20110420, sangyeol.lee@lge.com,  [END]



/* eunmo.yang@lge.com [BSP/Sensor] 20110623 START */
#ifdef CONFIG_LGE_SENSOR
	sensor_power_init();
#endif
/* eunmo.yang@lge.com [BSP/Sensor] 20110623 END */


#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	msm8x60_init_pm8058_othc();
#endif

#ifdef CONFIG_LGE_PMIC8058_KEYPAD
	if(lge_bd_rev == LGE_REV_B) {
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_KPD].
			platform_data = &i_atnt_revB_keypad_data;
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_KPD].data_size
			= sizeof(i_atnt_revB_keypad_data);
	}else{
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_KPD].
			platform_data = &i_atnt_keypad_data;
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_KPD].data_size
			= sizeof(i_atnt_keypad_data);
	}
#endif

#ifdef CONFIG_ATCMD_VIRTUAL_KBD
	lge_add_atcmd_virtual_kbd_device();
#endif

#ifdef CONFIG_ETA_EVENT_LOG
	lge_add_eta_event_log_device();
#endif

#ifdef CONFIG_USB_ANDROID
	if (machine_is_lge_i_board()) {
#ifdef CONFIG_LGE_USB_GADGET_DRIVER
        /* default product ID, make changes in android_probe func. */
#ifdef CONFIG_LGE_USB_VZW_DRIVER
		android_usb_pdata.product_id = 0x6204;
#else
		android_usb_pdata.product_id = 0x6315;
#endif

		android_usb_pdata.functions = usb_functions_all,
		android_usb_pdata.num_functions =
				ARRAY_SIZE(usb_functions_all),
		android_usb_pdata.products = usb_products;
		android_usb_pdata.num_products =
				ARRAY_SIZE(usb_products);
#else /* below is original */
		android_usb_pdata.product_id = 0x9032;
		android_usb_pdata.functions = charm_usb_functions_all,
		android_usb_pdata.num_functions =
				ARRAY_SIZE(charm_usb_functions_all);
		android_usb_pdata.products = charm_usb_products;
		android_usb_pdata.num_products =
				ARRAY_SIZE(charm_usb_products);
#endif /* CONFIG_LGE_USB_GADGET_DRIVER */
	}
#endif /* CONFIG_USB_ANDROID */

	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1)
		platform_add_devices(msm_footswitch_devices, msm_num_footswitch_devices);

	platform_add_devices(surf_devices, ARRAY_SIZE(surf_devices));

	/* platform.team@lge.com separate panel devices init */
	msm_panel_init();

	/* jisun.shin@lge.com separate camera devices */
#ifdef CONFIG_LGE_CAMERA
	msm_camera_init();
#endif

#ifdef CONFIG_MSM_DSPS
		if (machine_is_msm8x60_fluid()) {
			platform_device_unregister(&msm_gsbi12_qup_i2c_device);
			msm8x60_init_dsps();
		}
#endif

#ifdef CONFIG_USB_EHCI_MSM_72K
	msm_add_host(0, &msm_usb_host_pdata);
#endif

#ifdef CONFIG_LGE_SENSOR 
    /*
        actually, sensor datas used in register_i2c_devices()
        but we modified datas hear according to "hardware revision"
    */
#ifdef CONFIG_LGE_SENSOR_ACCELEROMETER
    if(lge_bd_rev < LGE_REV_C)
    {
        accelerometer_pdata.axis_map_x = 1;
    	accelerometer_pdata.axis_map_y = 0;
    	accelerometer_pdata.axis_map_z = 2;
    	accelerometer_pdata.negate_x = 1;
    	accelerometer_pdata.negate_y = 1;
    	accelerometer_pdata.negate_z = 1;
    }
    else
    {
        accelerometer_pdata.axis_map_x = 0;
    	accelerometer_pdata.axis_map_y = 1;
    	accelerometer_pdata.axis_map_z = 2;
    	accelerometer_pdata.negate_x = 0;
    	accelerometer_pdata.negate_y = 1;
    	accelerometer_pdata.negate_z = 1;
    }
#endif //CONFIG_LGE_SENSOR_ACCELEROMETER

#ifdef CONFIG_LGE_SENSOR_GYROSCOPE
    if(lge_bd_rev < LGE_REV_C)
    {
    	gyroscope_pdata.axis_map_x = 1;
		gyroscope_pdata.axis_map_y = 0;
		gyroscope_pdata.axis_map_z = 2;
		gyroscope_pdata.negate_x = 0;
		gyroscope_pdata.negate_y = 0;
		gyroscope_pdata.negate_z = 1;
    }
    else
    {
    	gyroscope_pdata.axis_map_x = 1;
		gyroscope_pdata.axis_map_y = 0;
		gyroscope_pdata.axis_map_z = 2;
		gyroscope_pdata.negate_x = 0;
		gyroscope_pdata.negate_y = 0;
		gyroscope_pdata.negate_z = 1;
    }
#endif //CONFIG_LGE_SENSOR_GYROSCOPE

#ifdef CONFIG_LGE_SENSOR_DCOMPASS
	dcompss_pdata.fdata_mDir   = 18;
    dcompss_pdata.fdata_sign_x = 1;
	dcompss_pdata.fdata_sign_y = -1;
	dcompss_pdata.fdata_sign_z = -1;
	dcompss_pdata.fdata_order0 = 0;
	dcompss_pdata.fdata_order1 = 1;
	dcompss_pdata.fdata_order2 = 2;
#endif
#endif

#ifdef CONFIG_LGE_MHL_SII9244
  if(lge_bd_rev >= mhl_support_hwrevision_map[lge_bd_target])
    mhl_pdata.is_support = 1;
  else
    mhl_pdata.is_support = 0;

  if((lge_bd_target == LGE_I_BOARD_VZW) || (lge_bd_target == LGE_I_BOARD_LGU)) 
    mhl_pdata.select_pin = GPIO_MHL_SELECT_SVLTE;

   platform_device_register(&mhl_sii9244_device);
#endif


#if defined(CONFIG_USB_PEHCI_HCD) || defined(CONFIG_USB_PEHCI_HCD_MODULE)
	if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa())
		msm8x60_cfg_isp1763();
#endif
#ifdef CONFIG_BATTERY_MSM8X60
	platform_device_register(&msm_charger_device);
#endif

	platform_add_devices(charm_devices, ARRAY_SIZE(charm_devices));

#ifdef CONFIG_PM8058_CHARGER
	pm8058_platform_data.charger_sub_device = &pm8058_charger_sub_dev;
#endif

	if (!machine_is_msm8x60_sim())
		msm_fb_add_devices();
	fixup_i2c_configs();
	register_i2c_devices();

	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);

#ifdef CONFIG_SENSORS_MSM_ADC
	msm_adc_pdata.target_hw = MSM_8x60;
#endif
#ifdef CONFIG_MSM8X60_AUDIO
	msm_snddev_init();
#endif

	lge_add_misc_devices();

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
	if (machine_is_msm8x60_fluid())
		platform_device_register(&fluid_leds_gpio);
	else
		platform_device_register(&gpio_leds);
#endif

	/* configure pmic leds */
	if (machine_is_msm8x60_fluid()) {
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_LED].
			platform_data = &pm8058_fluid_flash_leds_data;
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_LED].data_size
			= sizeof(pm8058_fluid_flash_leds_data);
	} else {
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_LED].
			platform_data = &pm8058_flash_leds_data;
		pm8058_platform_data.sub_devices[PM8058_SUBDEV_LED].data_size
			= sizeof(pm8058_flash_leds_data);
	}

	msm8x60_multi_sdio_init();

	if (machine_is_msm8x60_charm_surf() || machine_is_msm8x60_charm_ffa()|| machine_is_lge_i_board())
		msm_fusion_setup_pinctrl();

#if defined(CONFIG_MACH_LGE_I_BOARD)
	/* retreive boot reason */
	{
		extern uint16_t power_on_status_info_get(void);
		boot_reason = (unsigned int)power_on_status_info_get();
		printk(KERN_NOTICE "Boot Reason = 0x%02x\n", boot_reason);
	}
#endif/*CONFIG_MACH_LGE_I_BOARD*/
}
/* kwangdo.yi@lge.com [jointlab] Sat 11 Jun 2011 S
   i_atnt board init data added
*/
static void __init msm8x60_i_atnt_init(void)
{
	switch(lge_bd_rev)
	{
		case LGE_REV_C:
		case LGE_REV_D:
#ifdef CONFIG_LGE_BOARD_GPIO
			msm8x60_board_init(&msm8x60_i_atnt_rev_board_data); 
			printk("\n### lge revc bd config !!\n");
#else
			msm8x60_board_init(&msm8x60_i_atnt_board_data);
#endif
			break;
		default:
			msm8x60_board_init(&msm8x60_i_atnt_board_data);
			break;
	}
}
/* kwangdo.yi@lge.com [jointlab] Sat 11 Jun 2011 E */
MACHINE_START(LGE_I_BOARD, "LGE I BOARD MSM8X60")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.map_io = msm8x60_map_io,
	.init_irq = msm8x60_init_irq,
	.init_machine = msm8x60_i_atnt_init,
	.timer = &msm_timer,
MACHINE_END


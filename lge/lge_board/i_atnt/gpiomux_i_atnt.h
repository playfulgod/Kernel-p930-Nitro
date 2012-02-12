/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
 */
#ifndef __ARCH_ARM_MACH_MSM_GPIOMUX_8X60_H
#define __ARCH_ARM_MACH_MSM_GPIOMUX_8X60_H

void __init msm8x60_init_gpiomux(struct msm_gpiomux_configs *cfgs);

extern struct msm_gpiomux_configs msm8x60_qrdc_gpiomux_cfgs[] __initdata;
extern struct msm_gpiomux_configs msm8x60_surf_ffa_gpiomux_cfgs[] __initdata;
extern struct msm_gpiomux_configs msm8x60_fluid_gpiomux_cfgs[] __initdata;
extern struct msm_gpiomux_configs msm8x60_charm_gpiomux_cfgs[] __initdata;
extern struct msm_gpiomux_configs msm8x60_qt_gpiomux_cfgs[] __initdata;
extern struct msm_gpiomux_configs msm8x60_i_atnt_gpiomux_cfgs[] __initdata;
/* kwangdo.yi@lge.com [jointlab] Sat 11 Jun 2011 S
   add revc gpio configs
*/
extern struct msm_gpiomux_configs msm8x60_i_atnt_rev_gpiomux_cfgs[] __initdata;
/* kwangdo.yi@lge.com [jointlab] Sat 11 Jun 2011 E */

#endif

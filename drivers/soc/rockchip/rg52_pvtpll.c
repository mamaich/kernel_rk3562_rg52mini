// SPDX-License-Identifier: GPL-2.0
/*
 * RG52 Mini: real CPU and GPU clock of the RK3562 from the PVTPLL counters.
 *
 * scaling_cur_freq and devfreq cur_freq report the rate the kernel asked
 * BL31 for over SCMI. At 816 MHz and above the CPU (and the GPU at 500 MHz
 * and above) runs from a PVTPLL ring oscillator whose real rate is a few
 * percent lower and drifts with temperature: about 1960 MHz instead of 2016
 * on a cool chip, about 1910 on a hot one. The SoC measures it itself; the
 * result, already in MHz and averaged by hardware, sits in sys_grf:
 *
 *   0x0634  CPU (one for all four cores)
 *   0x0654  GPU (0 while the GPU is idle or powered down)
 *
 * Checked against a dependent-add loop on every core: within 0.3 %, noise
 * about +-2 MHz. The values are only meaningful while the ring is in use;
 * below those rates the clock comes from a PLL divider and the counters are
 * stale - the reader has to know that.
 *
 *   /sys/kernel/rg52/cpu_mhz, /sys/kernel/rg52/gpu_mhz  (0444)
 *
 * Each read is one regmap read of one register; nothing polls in the
 * background. Used by the power menu of the Android port.
 */

#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/sysfs.h>

#define RG52_PVTPLL_CPU	0x0634
#define RG52_PVTPLL_GPU	0x0654

static struct regmap *rg52_grf;

static ssize_t rg52_pvtpll_show(char *buf, unsigned int reg)
{
	unsigned int val;

	if (regmap_read(rg52_grf, reg, &val))
		return -EIO;
	return sprintf(buf, "%u\n", val);
}

static ssize_t cpu_mhz_show(struct kobject *kobj, struct kobj_attribute *attr,
			    char *buf)
{
	return rg52_pvtpll_show(buf, RG52_PVTPLL_CPU);
}

static ssize_t gpu_mhz_show(struct kobject *kobj, struct kobj_attribute *attr,
			    char *buf)
{
	return rg52_pvtpll_show(buf, RG52_PVTPLL_GPU);
}

static struct kobj_attribute rg52_cpu_mhz = __ATTR_RO(cpu_mhz);
static struct kobj_attribute rg52_gpu_mhz = __ATTR_RO(gpu_mhz);

static struct attribute *rg52_pvtpll_attrs[] = {
	&rg52_cpu_mhz.attr,
	&rg52_gpu_mhz.attr,
	NULL,
};

static const struct attribute_group rg52_pvtpll_group = {
	.attrs = rg52_pvtpll_attrs,
};

static int __init rg52_pvtpll_init(void)
{
	struct kobject *kobj;
	int ret;

	if (!of_machine_is_compatible("rockchip,rk3562"))
		return 0;

	rg52_grf = syscon_regmap_lookup_by_compatible("rockchip,rk3562-sys-grf");
	if (IS_ERR(rg52_grf))
		return PTR_ERR(rg52_grf);

	kobj = kobject_create_and_add("rg52", kernel_kobj);
	if (!kobj)
		return -ENOMEM;

	ret = sysfs_create_group(kobj, &rg52_pvtpll_group);
	if (ret)
		kobject_put(kobj);
	return ret;
}
late_initcall(rg52_pvtpll_init);

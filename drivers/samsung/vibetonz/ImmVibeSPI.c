/*
** =========================================================================
** File:
**     ImmVibeSPI.c
**
** Description:
**     Device-dependent functions called by Immersion TSP API
**     to control PWM duty cycle, amp enable/disable, save IVT file, etc...
**
** Portions Copyright (c) 2008-2010 Immersion Corporation. All Rights Reserved.
**
** This file contains Original Code and/or Modifications of Original Code
** as defined in and that are subject to the GNU Public License v2 -
** (the 'License'). You may not use this file except in compliance with the
** License. You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or contact
** TouchSenseSales@immersion.com.
**
** The Original Code and all software distributed under the License are
** distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESS OR IMPLIED, AND IMMERSION HEREBY DISCLAIMS ALL SUCH WARRANTIES,
** INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
** FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see
** the License for the specific language governing rights and limitations
** under the License.
** =========================================================================
*/
#include <linux/pwm.h>
#include <plat/gpio-cfg.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/mfd/max8997.h>
#include <linux/mfd/max8997-private.h>

#include "tspdrv.h"

#ifdef IMMVIBESPIAPI
#undef IMMVIBESPIAPI
#endif
#define IMMVIBESPIAPI static

/*
** This SPI supports only one actuator.
*/
#define NUM_ACTUATORS	1

#define PWM_DUTY_MAX    579 /* 13MHz / (579 + 1) = 22.4kHz */
#if defined (CONFIG_TARGET_LOCALE_KOR) || defined (CONFIG_TARGET_LOCALE_NA)
#define FREQ_COUNT	44643
#elif defined (CONFIG_TARGET_LOCALE_NTT)
#define FREQ_COUNT      44138
#else
#define FREQ_COUNT	38022
#endif

#define PWM_DEVICE	1


/* Haptic configuration 2 */
#define MOTOR_LRA		(1<<7)
#define MOTOR_EN		(1<<6)
#define EXT_PWM			(0<<5)


struct pwm_device	*Immvib_pwm;

static bool g_bAmpEnabled;
long int freq_count = FREQ_COUNT;

static int prev_duty_ns;
static int prev_period_ns;

static struct i2c_client *haptic_i2c;


static void _pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	if ((prev_duty_ns != duty_ns) || (prev_period_ns != period_ns)) {
		prev_duty_ns = duty_ns;
		prev_period_ns = period_ns;

		pwm_config(pwm, duty_ns, period_ns);
		DbgOut(KERN_DEBUG "_pwm_config called in [if]\n");
	} else
		DbgOut(KERN_DEBUG "_pwm_config called in [else]\n");
}

static void vibe_control_max8997(struct pwm_device *pwm, bool on)
{
	int ret;
	u8 value;

	if (on == 1) {
		value = (MOTOR_LRA | MOTOR_EN | EXT_PWM | 0x02);

		ret = max8997_write_reg(haptic_i2c, MAX8997_MUIC_REG_INT2, value);
		if (ret < 0)
			DbgOut(KERN_ERR "i2c write err in on\n");

		pwm_enable(pwm);
	} else {
		pwm_disable(pwm);

		value = 0x02;

		ret = max8997_write_reg(haptic_i2c, MAX8997_MUIC_REG_INT2, value);
		if (ret < 0)
			DbgOut(KERN_ERR "i2c write err in off\n");
	}
}



/*
** Called to disable amp (disable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpDisable(VibeUInt8 nActuatorIndex)
{
#if 0
	if (g_bAmpEnabled) {
		DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpDisable.\n"));

		g_bAmpEnabled = false;

		/* Disable amp */


		/* Disable PWM CLK */

	}
#endif

	if (g_bAmpEnabled) {
		struct regulator *regulator;

		g_bAmpEnabled = false;
		_pwm_config(Immvib_pwm, freq_count/2, freq_count);

		if (regulator_hapticmotor_enabled == 1) {
			regulator = regulator_get(NULL, "vmotor");

			if (IS_ERR(regulator)) {
				DbgOut((KERN_ERR "Failed to get vmoter regulator.\n"));
				return 0;
			}

			regulator_force_disable(regulator);
			regulator_put(regulator);

			regulator_hapticmotor_enabled = 0;

			printk(KERN_DEBUG "tspdrv: %s (%d)\n", __func__, regulator_hapticmotor_enabled);
		}

		vibe_control_max8997(Immvib_pwm, 0);
	}

	return VIBE_S_SUCCESS;
}

/*
** Called to enable amp (enable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpEnable(VibeUInt8 nActuatorIndex)
{
#if 0
	if (!g_bAmpEnabled)	{
		DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpEnable.\n"));

		g_bAmpEnabled = true;

		/* Generate PWM CLK with proper frequency(ex. 22400Hz) and 50% duty cycle.*/


		/* Enable amp */

	}
#endif

	if (!g_bAmpEnabled) {
		struct regulator *regulator;

		g_bAmpEnabled = true;

		_pwm_config(Immvib_pwm, freq_count/2, freq_count);
		vibe_control_max8997(Immvib_pwm, 1);

		regulator = regulator_get(NULL, "vmotor");

		if (IS_ERR(regulator)) {
			DbgOut((KERN_ERR "Failed to get vmoter regulator.\n"));
			return 0;
		}

		regulator_enable(regulator);
		regulator_put(regulator);

		regulator_hapticmotor_enabled = 1;

		printk(KERN_DEBUG "tspdrv: %s (%d)\n", __func__, regulator_hapticmotor_enabled);
	}

	return VIBE_S_SUCCESS;
}

/*
** Called at initialization time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Initialize(void)
{
	DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Initialize.\n"));

	g_bAmpEnabled = true;   /* to force ImmVibeSPI_ForceOut_AmpDisable disabling the amp */

	/*
	** Disable amp.
	** If multiple actuators are supported, please make sure to call
	** ImmVibeSPI_ForceOut_AmpDisable for each actuator (provide the actuator index as
	** input argument).
	*/

	Immvib_pwm = pwm_request(PWM_DEVICE, "Immvibtonz");
	_pwm_config(Immvib_pwm, freq_count/2, freq_count);

	ImmVibeSPI_ForceOut_AmpDisable(0);

	regulator_hapticmotor_enabled = 0;

	return VIBE_S_SUCCESS;
}

/*
** Called at termination time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Terminate(void)
{
	DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Terminate.\n"));

	/*
	** Disable amp.
	** If multiple actuators are supported, please make sure to call
	** ImmVibeSPI_ForceOut_AmpDisable for each actuator (provide the actuator index as
	** input argument).
	*/
	ImmVibeSPI_ForceOut_AmpDisable(0);

	return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set PWM duty cycle
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetSamples(VibeUInt8 nActuatorIndex,
							VibeUInt16 nOutputSignalBitDepth,
							VibeUInt16 nBufferSizeInBytes,
							VibeInt8 * pForceOutputBuffer)
{
#if 0
	VibeInt8 nForce;

	switch (nOutputSignalBitDepth) {
	case 8:
		/* pForceOutputBuffer is expected to contain 1 byte */
		if (nBufferSizeInBytes != 1) {
			DbgOut((KERN_ERR "[ImmVibeSPI] ImmVibeSPI_ForceOut_SetSamples nBufferSizeInBytes =  %d\n", nBufferSizeInBytes));
			return VIBE_E_FAIL;
		}
		nForce = pForceOutputBuffer[0];
		break;
	case 16:
		/* pForceOutputBuffer is expected to contain 2 byte */
		if (nBufferSizeInBytes != 2)
			return VIBE_E_FAIL;

		/* Map 16-bit value to 8-bit */
		nForce = ((VibeInt16 *)pForceOutputBuffer)[0] >> 8;
		break;
	default:
		/* Unexpected bit depth */
		return VIBE_E_FAIL;
	}

	if (nForce == 0)
		/* Set 50% duty cycle or disable amp */
	else
		/* Map force from [-127, 127] to [0, PWM_DUTY_MAX] */


#endif

	VibeInt8 nForce;
	int pwm_duty;

	switch (nOutputSignalBitDepth) {
	case 8:
		/* pForceOutputBuffer is expected to contain 1 byte */
		if (nBufferSizeInBytes != 1) {
			DbgOut((KERN_ERR "[ImmVibeSPI] ImmVibeSPI_ForceOut_SetSamples nBufferSizeInBytes =  %d\n", nBufferSizeInBytes));
			return VIBE_E_FAIL;
		}
		nForce = pForceOutputBuffer[0];
		break;
	case 16:
		/* pForceOutputBuffer is expected to contain 2 byte */
		if (nBufferSizeInBytes != 2)
			return VIBE_E_FAIL;

		/* Map 16-bit value to 8-bit */
		nForce = ((VibeInt16 *)pForceOutputBuffer)[0] >> 8;
		break;
	default:
		/* Unexpected bit depth */
		return VIBE_E_FAIL;
	}

	pwm_duty = freq_count/2 + ((freq_count/2 - 2) * nForce)/127;

	if (nForce == 0) {
		/* Set 50% duty cycle or disable amp */
		ImmVibeSPI_ForceOut_AmpDisable(0);
	} else {
		/* Map force from [-127, 127] to [0, PWM_DUTY_MAX] */
		ImmVibeSPI_ForceOut_AmpEnable(0);
		_pwm_config(Immvib_pwm, pwm_duty, freq_count);
	}

	return VIBE_S_SUCCESS;
}

#if 0	/* Unused */
/*
** Called to set force output frequency parameters
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetFrequency(VibeUInt8 nActuatorIndex, VibeUInt16 nFrequencyParameterID, VibeUInt32 nFrequencyParameterValue)
{
	return VIBE_S_SUCCESS;
}
#endif

/*
** Called to get the device name (device name must be returned as ANSI char)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_Device_GetName(VibeUInt8 nActuatorIndex, char *szDevName, int nSize)
{
	return VIBE_S_SUCCESS;
}


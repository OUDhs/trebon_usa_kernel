/*
** =========================================================================
** File:
**     ImmVibeSPI.c
**
** Description: 
**     Device-dependent functions called by Immersion TSP API
**     to control PWM duty cycle, amp enable/disable, save IVT file, etc...
**
** Portions Copyright (c) 2008-2009 Immersion Corporation. All Rights Reserved. 
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
#include <mach/gpio.h>
#include "tspdrv.h"

#if defined(CONFIG_MACH_CHIEF)
#include <linux/regulator/consumer.h>
#endif

#ifdef IMMVIBESPIAPI
#undef IMMVIBESPIAPI
#endif
#define IMMVIBESPIAPI static

/*
** This SPI supports only one actuator.
*/
#define NUM_ACTUATORS 1

#define PWM_DUTY_MAX    579 /* 13MHz / (579 + 1) = 22.4kHz */

#if defined (CONFIG_TIKAL_MPCS)
#define FREQ_COUNT	37026	// 1 / (128 * 211 Hz) in nanoseconds
#elif defined(CONFIG_MACH_CHIEF) || defined(CONFIG_MACH_AEGIS)
#define FREQ_COUNT 	87084/2
#elif defined(CONFIG_TIKAL_USCC)
#define FREQ_COUNT	37473		/*89284*/
#else
#define FREQ_COUNT	38022		/*89284*/
#endif

#define PWM_DEVICE	1

#define GPD0_TOUT_1		2 << 4

struct pwm_device	*Immvib_pwm;

VibeInt8 nForce_last = 0; 

static bool g_bAmpEnabled = false;

long int freq_count = FREQ_COUNT;

#if defined(CONFIG_MACH_VIPER) || defined(CONFIG_MACH_CHIEF)
extern void motor_control(bool onOff);
#endif

#if defined (CONFIG_MACH_TREBON)
#define FREQ_COUNT_TREBON_HWREV_04      36838   // 1 / (128 * 205 Hz) in nanoseconds
extern unsigned int HWREV;
#endif

/*
** Called to disable amp (disable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpDisable(VibeUInt8 nActuatorIndex)
{
	if (g_bAmpEnabled)
	{
		g_bAmpEnabled = false;

#if defined (CONFIG_MACH_TREBON)
                if (HWREV >= 0x04)
                    freq_count = FREQ_COUNT_TREBON_HWREV_04;
#endif

#if defined(CONFIG_MACH_VIPER) 	
		motor_control(0);
#elif defined(CONFIG_MACH_CHIEF)

		pwm_config(Immvib_pwm, 0, freq_count/2);
		pwm_disable(Immvib_pwm);
    	gpio_direction_output(GPIO_VIBTONE_EN1,0);
		gpio_direction_input(GPIO_VIBTONE_EN1);
		s3c_gpio_setpull(GPIO_VIBTONE_EN1,S3C_GPIO_PULL_DOWN);
		
		motor_control(0);
#else 
		pwm_config(Immvib_pwm, 0, freq_count/2);
		pwm_disable(Immvib_pwm);

		//gpio_request(GPIO_VIBTONE_EN1, "GPIO_VIBTONE_EN1");
		gpio_direction_output(GPIO_VIBTONE_EN1, GPIO_LEVEL_LOW);
		gpio_direction_output(GPIO_VIBTONE_PWM, GPIO_LEVEL_LOW);
		//gpio_free(GPIO_VIBTONE_EN1);
#endif		
	}

	return VIBE_S_SUCCESS;
}

/*
** Called to enable amp (enable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpEnable(VibeUInt8 nActuatorIndex)
{
	if (!g_bAmpEnabled)
	{
		g_bAmpEnabled = true;
#if defined(CONFIG_MACH_VIPER) 
		motor_control(1);
#elif defined(CONFIG_MACH_CHIEF)
		pwm_enable(Immvib_pwm);
		motor_control(1);
		gpio_direction_output(GPIO_VIBTONE_EN1, GPIO_LEVEL_LOW);
		s3c_gpio_cfgpin(GPIO_VIBTONE_PWM, GPD0_TOUT_1);
		mdelay(1);
		gpio_set_value(GPIO_VIBTONE_EN1, GPIO_LEVEL_HIGH);
		//gpio_free(GPIO_VIBTONE_EN1);
#else
		pwm_enable(Immvib_pwm);

		//gpio_request(GPIO_VIBTONE_EN1, "GPIO_VIBTONE_EN1");
		gpio_direction_output(GPIO_VIBTONE_EN1, GPIO_LEVEL_LOW);
		s3c_gpio_cfgpin(GPIO_VIBTONE_PWM, GPD0_TOUT_1);
		mdelay(1);
		gpio_set_value(GPIO_VIBTONE_EN1, GPIO_LEVEL_HIGH);
		//gpio_free(GPIO_VIBTONE_EN1);
#endif
	}

return VIBE_S_SUCCESS;
}

/*
** Called at initialization time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Initialize(void)
{
	g_bAmpEnabled = true; 

#if defined (CONFIG_MACH_TREBON)
        if (HWREV >= 0x04)
            freq_count = FREQ_COUNT_TREBON_HWREV_04;
#endif

	Immvib_pwm = pwm_request(PWM_DEVICE, "Immvibtonz");
	pwm_config(Immvib_pwm, freq_count/2, freq_count);
	ImmVibeSPI_ForceOut_AmpDisable(0);

    return VIBE_S_SUCCESS;
}

/*
** Called at termination time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Terminate(void)
{
//#error Please review the code between the #if and #endif

 //   DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Terminate.\n"));

    /* 
    ** Disable amp.
    ** If multiple actuators are supported, please make sure to call ImmVibeSPI_ForceOut_AmpDisable
    ** for each actuator (provide the actuator index as input argument).
    */
    ImmVibeSPI_ForceOut_AmpDisable(0);

#if 0
    /* Set PWM frequency */
    PWM_CTRL  = 0;                  /* 13Mhz / (0 + 1) = 13MHz */
    PWM_PERIOD = PWM_DUTY_MAX;      /* 13Mhz / (PWM_DUTY_MAX + 1) = 22.4kHz */

    /* Set duty cycle to 50% */
    PWM_DUTY = (PWM_DUTY_MAX+1)>>1; /* Duty cycle range = [0, PWM_DUTY_MAX] */
#endif

    return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set PWM duty cycle, and enable amp if required
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Set(VibeUInt8 nActuatorIndex, VibeInt8 nForce)
{
#if 0
#error Please review the code between the #if and #endif

#if 0
    VibeUInt32 nTmp;
#endif

    if (nForce == 0)
    {
#if 0
		/* Set 50% duty cycle */
        PWM_DUTY = (PWM_DUTY_MAX+1)>>1;
#endif
    }
    else
    {
#if 0
        /* Map force from [-127, 127] to [0, PWM_DUTY_MAX] */
        nTmp = ((nForce + 127) * 9) >> 2;
        
        /* 
        ** The above mapping will output PWM duty cycle in [0, PWM_DUTY_MAX] range.
        ** For PWM_DUTY_MAX == 579, max duty cycle is 98.6%.
        ** As long as we don't hit the 100% mark, we're OK
        */
        PWM_DUTY = nTmp;
#endif
    }
#endif

        int pwm_duty = 0;

#if defined (CONFIG_MACH_TREBON)
        if (HWREV >= 0x04)
            freq_count = FREQ_COUNT_TREBON_HWREV_04;
#endif

	pwm_duty = freq_count/2 + ((freq_count/2 - 2) * nForce)/127;

	if (nForce == 0)
	{
		ImmVibeSPI_ForceOut_AmpDisable(0);

	}
	else
	{
		if(nForce_last!= nForce) 
			pwm_config(Immvib_pwm, pwm_duty, freq_count);

		ImmVibeSPI_ForceOut_AmpEnable(0);
	}

	printk("[VIBETONZ] %s nForce: %d\n",__func__, nForce);
	nForce_last = nForce; 

	return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set force output, and enable amp if required
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetSamples(VibeUInt8 nActuatorIndex, VibeUInt16 nOutputSignalBitDepth, VibeUInt16 nBufferSizeInBytes, VibeInt8* pForceOutputBuffer)
{
    /* This function is not called for LRA device */
    return VIBE_S_SUCCESS;
}

/*
** Called to set force output frequency parameters
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetFrequency(VibeUInt8 nActuatorIndex, VibeUInt16 nFrequencyParameterID, VibeUInt32 nFrequencyParameterValue)
{
//#error Please review the code between the #if and #endif

#if 0 
    #error  "The OEM must handle different frequency parameters here"
#endif

    return VIBE_S_SUCCESS;
}

/*
** Called to get the device name (device name must be returned as ANSI char)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_Device_GetName(VibeUInt8 nActuatorIndex, char *szDevName, int nSize)
{
//#error Please review the code between the #if and #endif

#if 0   /* The following code is provided as a sample. Please modify as required. */
    if ((!szDevName) || (nSize < 1)) return VIBE_E_FAIL;

    DbgOut((KERN_DEBUG "ImmVibeSPI_Device_GetName.\n"));

    strncpy(szDevName, "Generic Linux Device", nSize-1);
    szDevName[nSize - 1] = '\0';    /* make sure the string is NULL terminated */
#endif

    return VIBE_S_SUCCESS;
}

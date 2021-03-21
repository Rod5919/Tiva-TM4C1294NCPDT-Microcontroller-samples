#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/pwm.h"




#define freq_m1 1000
uint32_t g_ui32SysClock;
uint32_t g_ui32PWMIncrement;//!
uint32_t vel_pwm = 0;
uint32_t g_ui32Flags;

#ifdef DEBUG
void
_error_(char *pcFilename, uint32_t ui32Line)
{
}
#endif

char data[30] = "";

void
Timer1IntHandler(void)
{
    char cOne, cTwo;

    //
    // Clear the timer interrupt.
    //
    MAP_TimerIntClear(TIMER1_BASE, TIMER_TIMB_TIMEOUT); //?TIMEOUT: Tiempo de espera cumplido

    //
    // Toggle the flag for the first timer.
    //
    HWREGBITW(&g_ui32Flags, 0) ^= 1; //? ^=: XOR

    //
    // Update the interrupt status.
    //
    MAP_IntMasterDisable(); //? Ya no admite en ese instante las interrupciones
    
    //? lógica adicional
    cOne = HWREGBITW(&g_ui32Flags, 0) ? '1' : '0'; //? Obtener status
    cTwo = HWREGBITW(&g_ui32Flags, 1) ? '1' : '0'; //? Obtener status
    //?

    MAP_IntMasterEnable();
}


void
UARTIntHandler(void)
{
    uint32_t ui32Status;

    ui32Status = MAP_UARTIntStatus(UART0_BASE, true);

    MAP_UARTIntClear(UART0_BASE, ui32Status);
    uint8_t ind = 0;
    uint8_t dig1,dig2,dig3,dig4;
    while(MAP_UARTCharsAvail(UART0_BASE))
    {
        MAP_UARTCharPutNonBlocking(UART0_BASE,
                                   MAP_UARTCharGetNonBlocking(UART0_BASE)); //enviar
	
	data[ind] = MAP_UARTCharGetNonBlocking(UART0_BASE);
        MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
	ind++;
    }
    if (data[0] == 'P' && data[1] == 'W' && data[2] == 'M' && data[3] == '_')
    {
        dig1 = data[4] - 48; 
        dig2 = data[5] - 48; 
        dig3 = data[6] - 48; 
        dig4 = data[7] - 48; 
        vel_pwm = dig1 * 1000 + dig2 *100 + dig3 *10 + dig4 * 1;
        MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, vel_pwm); ////Third argument  {0 - (clock_frequency/prescaler)/PWM_frequency -1} ---> (120000000/8)/250 -1
    }
    UARTCharPut(UART0_BASE, data[0]);
    UARTCharPut(UART0_BASE, data[1]);
    UARTCharPut(UART0_BASE, data[2]);
    UARTCharPut(UART0_BASE, data[3]);
    UARTCharPut(UART0_BASE, data[4]);
    if (data[0] == 'L' && data[1] == 'D' && data[2] == '_' && data[3] == 'O' && data[4] == 'N')
    {
        MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0xFF);
       	

    }
    
    if (data[0] == 'L' && data[1] == 'D' && data[2] == '_' && data[3] == 'O' && data[4] == 'F' && data[5] == 'F')
    {
        MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0x00);    
    }
    

}


void
UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while(ui32Count--)
    {
/* code */
        MAP_UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}


int
main(void)
{
    
    uint32_t ui32PWMClockRate;////!!!
    
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);
                                             
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4);
////////////////////////////////////////////////////////////////////////////////////////
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); //Enable timer
    //
    // Enable the GPIO port that is used for the PWM output.
    //

    MAP_GPIOPinConfigure(GPIO_PK5_M0PWM7);//**
    MAP_GPIOPinConfigure(GPIO_PF0_M0PWM0);//**
    MAP_GPIOPinConfigure(GPIO_PF4_M0FAULT0);//**
    
    MAP_GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_5);
    MAP_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
    
    MAP_PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_64);
    ui32PWMClockRate = g_ui32SysClock / 64;

    MAP_PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    MAP_PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
    MAP_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, (ui32PWMClockRate / freq_m1)); //250 = freq
    MAP_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, (ui32PWMClockRate / freq_m1)); //250 = freq

    g_ui32PWMIncrement = ((ui32PWMClockRate / freq_m1) / 1000);
    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, g_ui32PWMIncrement); ////Third argument  {0 - (clock_frequency/prescaler)/PWM_frequency -1} ---> (120000000/8)/250 -1
    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, g_ui32PWMIncrement); ////Third argument  {0 - (clock_frequency/prescaler)/PWM_frequency -1} ---> (120000000/8)/250 -1
//////////////////////////////////////////////////////////////////////////////////////////
    MAP_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    MAP_TimerLoadSet(TIMER1_BASE, TIMER_B, g_ui32SysClock);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    MAP_IntMasterEnable();

    MAP_IntEnable(INT_TIMER1B);
    MAP_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntRegister(TIMER1_BASE, TIMER_B, Timer1IntHandler); //? Registrar interrupción (op 2)
    MAP_TimerEnable(TIMER1_BASE, TIMER_B);

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    MAP_UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Enable the UART interrupt.
    //
    MAP_IntEnable(INT_UART0);
    MAP_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    //UARTSend((uint8_t *)"\033[2JEnter text: ", 16);

    while(1)
    {
    }
}
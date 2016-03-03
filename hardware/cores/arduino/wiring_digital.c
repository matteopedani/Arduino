/*
  Copyright (c) 2011 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio_api.h"
#include "gpio_irq_api.h"
#include "pwmout_api.h"

void *gpio_pin_struct[TOTAL_GPIO_PIN_NUM] = {NULL};
void *gpio_irq_handler_list[TOTAL_GPIO_PIN_NUM] = {NULL};

void gpioIrqHandler(uint32_t id, gpio_irq_event event) {
    if ( gpio_irq_handler_list[id] != NULL ) {
        ( (void (*)(uint32_t,uint32_t) )gpio_irq_handler_list[id] ) ( id, (uint32_t)event );
    }
}

void pinMode( uint32_t ulPin, uint32_t ulMode)
{
    void *pGpio_t;

    if ( ulPin < 0 || ulPin > TOTAL_GPIO_PIN_NUM )
    {
        return;
    }

    if ( (g_APinDescription[ulPin].ulPinType == PIO_GPIO || g_APinDescription[ulPin].ulPinType == PIO_GPIO_IRQ) 
            && g_APinDescription[ulPin].ulPinMode == ulMode)
    {
        return;
    }

    if ( g_APinDescription[ulPin].ulPinType == PIO_PWM ) {
        free ( (pwmout_t *)gpio_pin_struct[ulPin] );
        gpio_pin_struct[ulPin] = NULL;
        g_APinDescription[ulPin].ulPinType = NOT_INITIAL;
        g_APinDescription[ulPin].ulPinMode = NOT_INITIAL;
    }

    if ( g_APinDescription[ulPin].ulPinType == PIO_GPIO && 
            (ulMode == INPUT_IRQ_FALL || ulMode == INPUT_IRQ_RISE) ) {
        // requst type from gpio_t to gpio_irq_t
        free ( (gpio_t *)gpio_pin_struct[ulPin] );
        gpio_pin_struct[ulPin] = NULL;
        g_APinDescription[ulPin].ulPinType = NOT_INITIAL;
        g_APinDescription[ulPin].ulPinMode = NOT_INITIAL;
    }

    if ( g_APinDescription[ulPin].ulPinType == PIO_GPIO_IRQ && 
            (ulMode == INPUT || ulMode == OUTPUT || ulMode == INPUT_PULLUP || ulMode == INPUT_PULLNONE || ulMode == OUTPUT_OPENDRAIN) ) {
        // requst type from gpio_irq_t to gpio_t
        free ( (gpio_irq_t *)gpio_pin_struct[ulPin] );
        gpio_pin_struct[ulPin] = NULL;
        g_APinDescription[ulPin].ulPinType = NOT_INITIAL;
        g_APinDescription[ulPin].ulPinMode = NOT_INITIAL;
    }

    if ( g_APinDescription[ulPin].ulPinType == NOT_INITIAL ) {
        if (ulMode == INPUT_IRQ_FALL || ulMode == INPUT_IRQ_RISE) {
            gpio_pin_struct[ulPin] = malloc ( sizeof(gpio_irq_t) );
            pGpio_t = gpio_pin_struct[ulPin];
            gpio_irq_init( pGpio_t, g_APinDescription[ulPin].pinname, gpioIrqHandler, ulPin);
            g_APinDescription[ulPin].ulPinType = PIO_GPIO_IRQ;
        } else {
            gpio_pin_struct[ulPin] = malloc ( sizeof(gpio_t) );
            pGpio_t = gpio_pin_struct[ulPin];
            gpio_init( pGpio_t, g_APinDescription[ulPin].pinname );
            g_APinDescription[ulPin].ulPinType = PIO_GPIO;
        }
        g_APinDescription[ulPin].ulPinMode = ulMode;
    }

    switch ( ulMode )
    {
        case INPUT:
            gpio_dir( (gpio_t *)pGpio_t, PIN_INPUT );
            gpio_mode( (gpio_t *)pGpio_t, PullDown );
            break;

        case INPUT_PULLNONE:
            gpio_dir( (gpio_t *)pGpio_t, PIN_INPUT );
            gpio_mode( (gpio_t *)pGpio_t, PullNone );
            break;

        case INPUT_PULLUP:
            gpio_dir( (gpio_t *)pGpio_t, PIN_INPUT );
            gpio_mode( (gpio_t *)pGpio_t, PullUp );
            break;

        case OUTPUT:
            gpio_dir( (gpio_t *)pGpio_t, PIN_OUTPUT );
            gpio_mode( (gpio_t *)pGpio_t, PullNone );
            break;

        case OUTPUT_OPENDRAIN:
            gpio_dir( (gpio_t *)pGpio_t, PIN_OUTPUT );
            gpio_mode( (gpio_t *)pGpio_t, OpenDrain );
            break;

        case INPUT_IRQ_FALL:
            gpio_irq_set( (gpio_irq_t *)pGpio_t, IRQ_FALL, 1 );
            gpio_irq_enable( (gpio_irq_t *)pGpio_t );
            break;

        case INPUT_IRQ_RISE:
            gpio_irq_set( (gpio_irq_t *)pGpio_t, IRQ_RISE, 1 );
            gpio_irq_enable( (gpio_irq_t *)pGpio_t );
            break;

        default:
            break ;
    }
}

void digitalWrite( uint32_t ulPin, uint32_t ulVal )
{
    gpio_t *pGpio_t;

    if ( ulPin < 0 || ulPin > TOTAL_GPIO_PIN_NUM )
    {
        return;
    }

    if ( g_APinDescription[ulPin].ulPinType != PIO_GPIO )
    {
        return;
    }

    pGpio_t = (gpio_t *)gpio_pin_struct[ulPin];

    gpio_write( pGpio_t, ulVal );
}

int digitalRead( uint32_t ulPin )
{
    gpio_t *pGpio_t;
    int pin_status;
    
    if ( ulPin < 0 || ulPin > TOTAL_GPIO_PIN_NUM )
    {
        return -1;
    }

    if ( g_APinDescription[ulPin].ulPinType != PIO_GPIO )
    {
        return -1;
    }

    pGpio_t = (gpio_t *)gpio_pin_struct[ulPin];

    pin_status = gpio_read( pGpio_t );

    return pin_status;
}

void digitalChangeDir( uint32_t ulPin, uint8_t direction)
{
    gpio_t *pGpio_t;
    u32 RegValue;

    if ( ulPin < 0 || ulPin > TOTAL_GPIO_PIN_NUM )
    {
        return;
    }

    if ( g_APinDescription[ulPin].ulPinType != PIO_GPIO )
    {
        return;
    }

    pGpio_t = (gpio_t *)gpio_pin_struct[ulPin];

    gpio_dir( pGpio_t, direction );
}

/**************************** Extend API by RTK ***********************************/

uint32_t digitalPinToPort( uint32_t ulPin )
{
    uint32_t pin_name;

    if ( ulPin < 0 || ulPin > TOTAL_GPIO_PIN_NUM )
    {
        return 0xFFFFFFFF;
    }

    pin_name = HAL_GPIO_GetPinName(g_APinDescription[ulPin].pinname);
    return HAL_GPIO_GET_PORT_BY_NAME(pin_name);
}

uint32_t digitalPinToBitMask( uint32_t ulPin )
{
    uint32_t pin_name;

    if ( ulPin < 0 || ulPin > TOTAL_GPIO_PIN_NUM )
    {
        return 0xFFFFFFFF;
    }

    pin_name = HAL_GPIO_GetPinName(g_APinDescription[ulPin].pinname);

    return 1 << (HAL_GPIO_GET_PIN_BY_NAME(pin_name));
}

uint32_t digitalSetIrqHandler( uint32_t ulPin, void (*handler)(uint32_t id, uint32_t event) ) {
    gpio_irq_handler_list[ulPin] = (void *) handler;
}

uint32_t digitalClearIrqHandler( uint32_t ulPin ) {
    gpio_irq_handler_list[ulPin] = NULL;
}

#ifdef __cplusplus
}
#endif


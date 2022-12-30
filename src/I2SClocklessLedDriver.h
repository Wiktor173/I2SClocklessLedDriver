/* library options
 *  ENABLE_HARDWARE_SCROLL : to enable the HARDWARE SCROLL. Attention wjhen enabled you can use the offset  but it could mean slow when using all the pins
 *  NUMSTRIPS add this before the #include of the library this will help with the speed of the buffer calculation
 *  USE_PIXELSLIB : to use tthe pixel lib library automatic functions
 */

#ifndef __I2S_CLOCKLESS_DRIVER_H
#define __I2S_CLOCKLESS_DRIVER_H

 
#pragma once


#include "esp_heap_caps.h"
#include "soc/soc.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "rom/lldesc.h"
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <rom/ets_sys.h>
#include "esp32-hal-log.h"
#include "Math.h"



#ifndef NUMSTRIPS
#define NUMSTRIPS 16
#endif

#ifndef SNAKEPATTERN
#define SNAKEPATTERN 1
#endif

#ifndef ALTERNATEPATTERN
#define ALTERNATEPATTERN 1
#endif

#define I2S_DEVICE 0

#define AA (0x00AA00AAL)
#define CC (0x0000CCCCL)
#define FF (0xF0F0F0F0L)
#define FF2 (0x0F0F0F0FL)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifndef HARDWARESPRITES
#define HARDWARESPRITES 0
#endif

#if HARDWARESPRITES == 1
#include "hardwareSprite.h"
#endif

#ifdef COLOR_ORDER_GRBW
#define _p_r 1
#define _p_g 0
#define _p_b 2
#define _nb_components 4
#else
#ifdef COLOR_ORDER_RGBW
#define _p_r 0
#define _p_g 1
#define _p_b 2
#define _nb_components 3
#else
#ifdef  COLOR_ORDER_RGBW
#define _p_r 0
#define _p_g 2
#define _p_b 1
#define _nb_components 3
#else
#ifdef COLOR_ORDER_RGBW
#define _p_r 2
#define _p_g 0
#define _p_b 1
#define _nb_components 3
#else
#ifdef COLOR_ORDER_RGBW
#define _p_r 2
#define _p_g 1
#define _p_b 0
#define _nb_components 3
#else
#ifdef COLOR_ORDER_RGBW
#define _p_r 1
#define _p_g 2
#define _p_b 0
#define _nb_components 3
#else
#ifdef COLOR_ORDER_RGBW
#define _p_r 1
#define _p_g 0
#define _p_b 2
#define _nb_components 3
#else

#define _p_r 1
#define _p_g 0
#define _p_b 2
#define _nb_components 3
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#ifdef USE_PIXELSLIB
#include "pixelslib.h"
#else
#include "___pixeltypes.h"
#endif

#define FULL_DMA_BUFFER

typedef union
{
    uint8_t bytes[16];
    uint32_t shorts[8];
    uint32_t raw[2];
} Lines;

struct OffsetDisplay
{
    int offsetx;
    int offsety;
    int panel_height;
    int panel_width;
};
static const char *TAG = "I2SClocklessLedDriver";
static void IRAM_ATTR _I2SClocklessLedDriverinterruptHandler(void *arg);
static void IRAM_ATTR transpose16x1_noinline2(unsigned char *A, uint16_t *B);
#ifdef ENABLE_HARDWARE_SCROLL
static void IRAM_ATTR loadAndTranspose(uint8_t *ledt, int led_per_strip, int num_stripst, OffsetDisplay offdisp, uint16_t *buffer, int ledtodisp, uint8_t *mapg, uint8_t *mapr, uint8_t *mapb, uint8_t *mapw, int nbcomponents, int pg, int pr, int pb);
#else
static void IRAM_ATTR loadAndTranspose(uint8_t *ledt, int *sizes, int num_stripst, uint16_t *buffer, int ledtodisp, uint8_t *mapg, uint8_t *mapr, uint8_t *mapb, uint8_t *mapw, int nbcomponents, int pg, int pr, int pb);
#endif

enum colorarrangment
{
    ORDER_GRBW,
    ORDER_RGB,
    ORDER_RBG,
    ORDER_GRB,
    ORDER_GBR,
    ORDER_BRG,
    ORDER_BGR,
};

enum displayMode
{
    NO_WAIT,
    WAIT,
    LOOP,
    LOOP_INTERUPT,
};

int MOD(int a, int b)
{
    /* if (b == 1)
    {
        if (a < 0)
            return -a;
        else
            return a;
    }*/
    if (a < 0)
    {
        if (-a % b == 0)
            return 0;
        else
            return b - (-a) % b;
    }
    else
        return a % b;
}

struct LedTiming
{

    //led timing
    uint32_t T0;
    uint32_t T1;
    uint32_t T2;

    //compileled
    uint8_t f1;
    uint8_t f2;
    uint8_t f3;
};

class I2SClocklessLedDriver
{

    struct I2SClocklessLedDriverDMABuffer
    {
        lldesc_t descriptor;
        uint8_t *buffer;
    };

    const int deviceBaseIndex[2] = {I2S0O_DATA_OUT0_IDX, I2S1O_DATA_OUT0_IDX};
    const int deviceClockIndex[2] = {I2S0O_BCK_OUT_IDX, I2S1O_BCK_OUT_IDX};
    const int deviceWordSelectIndex[2] = {I2S0O_WS_OUT_IDX, I2S1O_WS_OUT_IDX};
    const periph_module_t deviceModule[2] = {PERIPH_I2S0_MODULE, PERIPH_I2S1_MODULE};

public:
    i2s_dev_t *i2s;
    uint8_t __green_map[256];
    uint8_t __blue_map[256];
    uint8_t __red_map[256];
    uint8_t __white_map[256];
    uint8_t _brightness;
    float _gammar, _gammab, _gammag, _gammaw;
    intr_handle_t _gI2SClocklessDriver_intr_handle;
    volatile xSemaphoreHandle I2SClocklessLedDriver_sem = NULL;
    volatile xSemaphoreHandle I2SClocklessLedDriver_semSync = NULL;
    volatile xSemaphoreHandle I2SClocklessLedDriver_semDisp = NULL;
    volatile int dmaBufferActive = 0;
    volatile bool wait;
    displayMode __displayMode;
    volatile int ledToDisplay;
    OffsetDisplay _offsetDisplay, _defaultOffsetDisplay;
    // volatile int oo=0;
    uint8_t *leds,*saveleds;
    int startleds;
    int linewidth;
    int dmaBufferCount = 2; //we use two buffers
    volatile bool transpose = false;

    volatile int num_strips;
    volatile int num_led_per_strip;
    volatile uint16_t total_leds;
    //int clock_pin;
    int p_r, p_g, p_b;
    int i2s_base_pin_index;
    int nb_components;
    int stripSize[16];
    /*
     This flag is used when using the NO_WAIT modeÒÒ
     */
    volatile bool isDisplaying = false;
    volatile bool isWaiting = true;
    volatile bool framesync = false;
    volatile bool wasWaitingtofinish = false;
    volatile int counti;

    I2SClocklessLedDriver(){};
    void setPins(int *Pins)
    {

        for (int i = 0; i < num_strips; i++)
        {

            PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[Pins[i]], PIN_FUNC_GPIO);
            gpio_set_direction((gpio_num_t)Pins[i], (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
            gpio_matrix_out(Pins[i], deviceBaseIndex[I2S_DEVICE] + i + 8, false, false);
        }
    }

    //Corrected = 255 * (Image/255)^(1/2.2).

    void setBrightness(int brightness)
    {
        _brightness = brightness;
        float tmp;
        for (int i = 0; i < 256; i++)
        {
            tmp = powf((float)i / 255, 1 / _gammag);
            __green_map[i] = (uint8_t)(tmp * brightness);
            tmp = powf((float)i / 255, 1 / _gammag);
            __blue_map[i] = (uint8_t)(tmp * brightness);
            tmp = powf((float)i / 255, 1 / _gammag);
            __red_map[i] = (uint8_t)(tmp * brightness);
            tmp = powf((float)i / 255, 1 / _gammag);
            __white_map[i] = (uint8_t)(tmp * brightness);
        }
    }

    void setGamma(float gammar, float gammab, float gammag, float gammaw)
    {
        _gammag = gammag;
        _gammar = gammar;
        _gammaw = gammaw;
        _gammab = gammab;
        setBrightness(_brightness);
    }

    void setGamma(float gammar, float gammab, float gammag)
    {
        _gammag = gammag;
        _gammar = gammar;
        _gammab = gammab;
        setBrightness(_brightness);
    }

    void i2sInit()
    {
        int interruptSource;
        if (I2S_DEVICE == 0)
        {
            i2s = &I2S0;
            periph_module_enable(PERIPH_I2S0_MODULE);
            interruptSource = ETS_I2S0_INTR_SOURCE;
            i2s_base_pin_index = I2S0O_DATA_OUT0_IDX;
        }
        else
        {
            i2s = &I2S1;
            periph_module_enable(PERIPH_I2S1_MODULE);
            interruptSource = ETS_I2S1_INTR_SOURCE;
            i2s_base_pin_index = I2S1O_DATA_OUT0_IDX;
        }

        i2sReset();
        i2sReset_DMA();
        i2sReset_FIFO();
        i2s->conf.tx_right_first = 0;

        // -- Set parallel mode
        i2s->conf2.val = 0;
        i2s->conf2.lcd_en = 1;
        i2s->conf2.lcd_tx_wrx2_en = 1; // 0 for 16 or 32 parallel output
        i2s->conf2.lcd_tx_sdx2_en = 0; // HN

        // -- Set up the clock rate and sampling
        i2s->sample_rate_conf.val = 0;
        i2s->sample_rate_conf.tx_bits_mod = 16; // Number of parallel bits/pins
        i2s->clkm_conf.val = 0;

        i2s->clkm_conf.clka_en = 0;

//add the capability of going a bit faster
        i2s->clkm_conf.clkm_div_a = 3;    // CLOCK_DIVIDER_A;
        i2s->clkm_conf.clkm_div_b = 1;    //CLOCK_DIVIDER_B;
        i2s->clkm_conf.clkm_div_num = 33; //CLOCK_DIVIDER_N;


        i2s->fifo_conf.val = 0;
        i2s->fifo_conf.tx_fifo_mod_force_en = 1;
        i2s->fifo_conf.tx_fifo_mod = 1;  // 16-bit single channel data
        i2s->fifo_conf.tx_data_num = 32; //32; // fifo length
        i2s->fifo_conf.dscr_en = 1;      // fifo will use dma
        i2s->sample_rate_conf.tx_bck_div_num = 1;
        i2s->conf1.val = 0;
        i2s->conf1.tx_stop_en = 0;
        i2s->conf1.tx_pcm_bypass = 1;

        i2s->conf_chan.val = 0;
        i2s->conf_chan.tx_chan_mod = 1; // Mono mode, with tx_msb_right = 1, everything goes to right-channel

        i2s->timing.val = 0;
        i2s->int_ena.val = 0;
        /*
        // -- Allocate i2s interrupt
        SET_PERI_REG_BITS(I2S_INT_ENA_REG(I2S_DEVICE), I2S_OUT_EOF_INT_ENA_V,1, I2S_OUT_EOF_INT_ENA_S);
        SET_PERI_REG_BITS(I2S_INT_ENA_REG(I2S_DEVICE), I2S_OUT_TOTAL_EOF_INT_ENA_V, 1, I2S_OUT_TOTAL_EOF_INT_ENA_S);
        SET_PERI_REG_BITS(I2S_INT_ENA_REG(I2S_DEVICE), I2S_OUT_TOTAL_EOF_INT_ENA_V, 1, I2S_OUT_TOTAL_EOF_INT_ENA_S);
        */
        esp_err_t e = esp_intr_alloc(interruptSource, ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM, &_I2SClocklessLedDriverinterruptHandler, this, &_gI2SClocklessDriver_intr_handle);

        // -- Create a semaphore to block execution until all the controllers are done

        if (I2SClocklessLedDriver_sem == NULL)
        {
            I2SClocklessLedDriver_sem = xSemaphoreCreateBinary();
        }

        if (I2SClocklessLedDriver_semSync == NULL)
        {
            I2SClocklessLedDriver_semSync = xSemaphoreCreateBinary();
        }
        if (I2SClocklessLedDriver_semDisp == NULL)
        {
            I2SClocklessLedDriver_semDisp = xSemaphoreCreateBinary();
        }
    }

    void initDMABuffers()
    {
        DMABuffersTampon[0] = allocateDMABuffer(nb_components * 8 * 2 * 3); //the buffers for the
        DMABuffersTampon[1] = allocateDMABuffer(nb_components * 8 * 2 * 3);
        DMABuffersTampon[2] = allocateDMABuffer(nb_components * 8 * 2 * 3);
        DMABuffersTampon[3] = allocateDMABuffer(nb_components * 8 * 2 * 3 * 4);

        putdefaultones((uint16_t *)DMABuffersTampon[0]->buffer);
        putdefaultones((uint16_t *)DMABuffersTampon[1]->buffer);

#ifdef FULL_DMA_BUFFER
        /*
         We do create n+2 buffers
         the first buffer is to be sure that everything is 0
         the last one is to put back the I2S at 0 the last bufffer is longer because when using the loop display mode the time between two frames needs to be longh enough.
         */
        DMABuffersTransposed = (I2SClocklessLedDriverDMABuffer **)malloc(sizeof(I2SClocklessLedDriverDMABuffer *) * (num_led_per_strip + 2));
        for (int i = 0; i < num_led_per_strip + 2; i++)
        {
            if (i < num_led_per_strip + 1)
                DMABuffersTransposed[i] = allocateDMABuffer(nb_components * 8 * 2 * 3);
            else
                DMABuffersTransposed[i] = allocateDMABuffer(nb_components * 8 * 2 * 3 * 4);
            if (i < num_led_per_strip)
                DMABuffersTransposed[i]->descriptor.eof = 0;
            if (i)
            {
                DMABuffersTransposed[i - 1]->descriptor.qe.stqe_next = &(DMABuffersTransposed[i]->descriptor);
                if (i < num_led_per_strip + 1)
                {
                    putdefaultones((uint16_t *)DMABuffersTransposed[i]->buffer);
                }
            }
        }
#endif
    }

#ifdef FULL_DMA_BUFFER

    void stopDisplayLoop()
    {
        DMABuffersTransposed[num_led_per_strip + 1]->descriptor.qe.stqe_next = 0;
    }

    void showPixelsFromBuffer()
    {
        showPixelsFromBuffer(NO_WAIT);
    }

    void showPixelsFromBuffer(displayMode dispmode)
    {
        /*
         We cannot launch twice when in loopmode
         */
        if (__displayMode == LOOP && isDisplaying)
        {
            ESP_LOGE(TAG, "The loop mode is activated execute stopDisplayLoop() first");
            return;
        }
        /*
         We wait for the display to be stopped before launching a new one
         */
        
         __displayMode = dispmode;
        isWaiting = false;
        if (dispmode == LOOP or dispmode == LOOP_INTERUPT)
        {
            DMABuffersTransposed[num_led_per_strip + 1]->descriptor.qe.stqe_next = &(DMABuffersTransposed[0]->descriptor);
        }
        transpose = false;
        i2sStart(DMABuffersTransposed[0]);

        if (dispmode == WAIT)
        {
            isWaiting = true;
            I2SClocklessLedDriver_sem=xSemaphoreCreateBinary();
            xSemaphoreTake(I2SClocklessLedDriver_sem, portMAX_DELAY);
        }
    }


    void showPixelsFirstTranspose(OffsetDisplay offdisp)
    {
        _offsetDisplay = offdisp;
        showPixelsFirstTranspose();
        _offsetDisplay = _defaultOffsetDisplay;
    }
    
    void showPixelsFirstTranspose(OffsetDisplay offdisp,uint8_t * temp_leds)
    {
        _offsetDisplay = offdisp;
        showPixelsFirstTranspose(temp_leds);
        _offsetDisplay = _defaultOffsetDisplay;
    }

    void showPixelsFirstTranspose(uint8_t *new_leds)
    {
        uint8_t *tmp_leds;
        tmp_leds = leds;
        leds = new_leds;
        showPixelsFirstTranspose();
        leds = tmp_leds;
    }

    void showPixelsFirstTranspose()
    {
        showPixelsFirstTranspose(NO_WAIT);
    }
    void showPixelsFirstTranspose(displayMode dispmode)
    {
        
        if (leds == NULL)
        {
           ESP_LOGE(TAG,"no led");
            return;
        }
        if (__displayMode == NO_WAIT && isDisplaying == true)
        {
            if(I2SClocklessLedDriver_semDisp==NULL)
         I2SClocklessLedDriver_semDisp= xSemaphoreCreateBinary();
          xSemaphoreTake(I2SClocklessLedDriver_semDisp, portMAX_DELAY);

        }
        transposeAll();
        showPixelsFromBuffer(dispmode);
       
    }

    void transposeAll()
    {
        ledToDisplay = 0;
        /*Lines secondPixel[nb_components];
        for (int j = 0; j < num_led_per_strip; j++)
        {
            uint8_t *poli = leds + ledToDisplay * nb_components;
            for (int i = 0; i < num_strips; i++)
            {

                secondPixel[p_g].bytes[i] = __green_map[*(poli + 1)];
                secondPixel[p_r].bytes[i] = __red_map[*(poli + 0)];
                secondPixel[p_b].bytes[i] = __blue_map[*(poli + 2)];
                if (nb_components > 3)
                    secondPixel[3].bytes[i] = __white_map[*(poli + 3)];
                //#endif
                poli += num_led_per_strip * nb_components;
            }
            ledToDisplay++;
            transpose16x1_noinline2(secondPixel[0].bytes, (uint16_t *)DMABuffersTransposed[j + 1]->buffer);
            transpose16x1_noinline2(secondPixel[1].bytes, (uint16_t *)DMABuffersTransposed[j + 1]->buffer + 3 * 8);
            transpose16x1_noinline2(secondPixel[2].bytes, (uint16_t *)DMABuffersTransposed[j + 1]->buffer + 2 * 3 * 8);
            if (nb_components > 3)
                transpose16x1_noinline2(secondPixel[3].bytes, (uint16_t *)DMABuffersTransposed[j + 1]->buffer + 3 * 3 * 8);
        }*/
        for (int j = 0; j < num_led_per_strip; j++)
        {
            #ifdef ENABLE_HARDWARE_SCROLL
            loadAndTranspose(leds, num_led_per_strip, num_strips, _offsetDisplay, (uint16_t *)DMABuffersTransposed[j + 1]->buffer, j, __green_map, __red_map, __blue_map, __white_map, nb_components, p_g, p_r, p_b);
            #else
            loadAndTranspose(leds, stripSize, num_strips, (uint16_t *)DMABuffersTransposed[j+1]->buffer, j, __green_map, __red_map, __blue_map, __white_map, nb_components, p_g, p_r, p_b);
            #endif
        }
    }

    void setPixelinBufferByStrip(int stripNumber,int posOnStrip,uint8_t red, uint8_t green, uint8_t blue)
    {
        uint8_t W = 0;
        if (nb_components > 3)
        {
            W = MIN(red, green);
            W = MIN(W, blue);
            red = red - W;
            green = green - W;
            blue = blue - W;
        }
        setPixelinBufferByStrip(stripNumber,posOnStrip, red, green, blue, W);
    }

    void setPixelinBufferByStrip(int stripNumber,int posOnStrip,uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
    {
        uint16_t mask = ~(1 << stripNumber);
        uint8_t colors[3];
        colors[p_g] = __green_map[green];
        colors[p_r] = __red_map[red];
        colors[p_b] = __blue_map[blue];
        uint16_t *B = (uint16_t *)DMABuffersTransposed[posOnStrip + 1]->buffer;
        // printf("nb c:%d\n",nb_components);
        uint8_t y = colors[0];
        *((uint16_t *)(B)) = (*((uint16_t *)(B)) & mask) | ((uint16_t)((y & 128) >> 7) << stripNumber);
        *((uint16_t *)(B + 5)) = (*((uint16_t *)(B + 5)) & mask) | ((uint16_t)((y & 64) >> 6) << stripNumber);
        *((uint16_t *)(B + 6)) = (*((uint16_t *)(B + 6)) & mask) | ((uint16_t)((y & 32) >> 5) << stripNumber);
        *((uint16_t *)(B + 11)) = (*((uint16_t *)(B + 11)) & mask) | ((uint16_t)((y & 16) >> 4) << stripNumber);
        *((uint16_t *)(B + 12)) = (*((uint16_t *)(B + 12)) & mask) | ((uint16_t)((y & 8) >> 3) << stripNumber);
        *((uint16_t *)(B + 17)) = (*((uint16_t *)(B + 17)) & mask) | ((uint16_t)((y & 4) >> 2) << stripNumber);
        *((uint16_t *)(B + 18)) = (*((uint16_t *)(B + 18)) & mask) | ((uint16_t)((y & 2) >> 1) << stripNumber);
        *((uint16_t *)(B + 23)) = (*((uint16_t *)(B + 23)) & mask) | ((uint16_t)(y & 1) << stripNumber);

        B += 3 * 8;
        y = colors[1];
        *((uint16_t *)(B)) = (*((uint16_t *)(B)) & mask) | ((uint16_t)((y & 128) >> 7) << stripNumber);
        *((uint16_t *)(B + 5)) = (*((uint16_t *)(B + 5)) & mask) | ((uint16_t)((y & 64) >> 6) << stripNumber);
        *((uint16_t *)(B + 6)) = (*((uint16_t *)(B + 6)) & mask) | ((uint16_t)((y & 32) >> 5) << stripNumber);
        *((uint16_t *)(B + 11)) = (*((uint16_t *)(B + 11)) & mask) | ((uint16_t)((y & 16) >> 4) << stripNumber);
        *((uint16_t *)(B + 12)) = (*((uint16_t *)(B + 12)) & mask) | ((uint16_t)((y & 8) >> 3) << stripNumber);
        *((uint16_t *)(B + 17)) = (*((uint16_t *)(B + 17)) & mask) | ((uint16_t)((y & 4) >> 2) << stripNumber);
        *((uint16_t *)(B + 18)) = (*((uint16_t *)(B + 18)) & mask) | ((uint16_t)((y & 2) >> 1) << stripNumber);
        *((uint16_t *)(B + 23)) = (*((uint16_t *)(B + 23)) & mask) | ((uint16_t)(y & 1) << stripNumber);

        B += 3 * 8;
        y = colors[2];
        *((uint16_t *)(B)) = (*((uint16_t *)(B)) & mask) | ((uint16_t)((y & 128) >> 7) << stripNumber);
        *((uint16_t *)(B + 5)) = (*((uint16_t *)(B + 5)) & mask) | ((uint16_t)((y & 64) >> 6) << stripNumber);
        *((uint16_t *)(B + 6)) = (*((uint16_t *)(B + 6)) & mask) | ((uint16_t)((y & 32) >> 5) << stripNumber);
        *((uint16_t *)(B + 11)) = (*((uint16_t *)(B + 11)) & mask) | ((uint16_t)((y & 16) >> 4) << stripNumber);
        *((uint16_t *)(B + 12)) = (*((uint16_t *)(B + 12)) & mask) | ((uint16_t)((y & 8) >> 3) << stripNumber);
        *((uint16_t *)(B + 17)) = (*((uint16_t *)(B + 17)) & mask) | ((uint16_t)((y & 4) >> 2) << stripNumber);
        *((uint16_t *)(B + 18)) = (*((uint16_t *)(B + 18)) & mask) | ((uint16_t)((y & 2) >> 1) << stripNumber);
        *((uint16_t *)(B + 23)) = (*((uint16_t *)(B + 23)) & mask) | ((uint16_t)(y & 1) << stripNumber);
        if (nb_components > 3)
        {
            B += 3 * 8;
            y = __white_map[white];
            *((uint16_t *)(B)) = (*((uint16_t *)(B)) & mask) | ((uint16_t)((y & 128) >> 7) << stripNumber);
            *((uint16_t *)(B + 5)) = (*((uint16_t *)(B + 5)) & mask) | ((uint16_t)((y & 64) >> 6) << stripNumber);
            *((uint16_t *)(B + 6)) = (*((uint16_t *)(B + 6)) & mask) | ((uint16_t)((y & 32) >> 5) << stripNumber);
            *((uint16_t *)(B + 11)) = (*((uint16_t *)(B + 11)) & mask) | ((uint16_t)((y & 16) >> 4) << stripNumber);
            *((uint16_t *)(B + 12)) = (*((uint16_t *)(B + 12)) & mask) | ((uint16_t)((y & 8) >> 3) << stripNumber);
            *((uint16_t *)(B + 17)) = (*((uint16_t *)(B + 17)) & mask) | ((uint16_t)((y & 4) >> 2) << stripNumber);
            *((uint16_t *)(B + 18)) = (*((uint16_t *)(B + 18)) & mask) | ((uint16_t)((y & 2) >> 1) << stripNumber);
            *((uint16_t *)(B + 23)) = (*((uint16_t *)(B + 23)) & mask) | ((uint16_t)(y & 1) << stripNumber);
        }
    }

    void setPixelinBuffer(uint32_t pos, uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
    {

        int stripNumber=-1;
        int total=0;
        int posOnStrip =pos;
        if (pos>total_leds-1)
        {
            printf("Position out of bound %d > %d\n",pos,total_leds-1);
            return;
        }
        while(total<=pos)
        {
            stripNumber++;
            total+=stripSize[stripNumber];
        }
        if(stripNumber>0)
            {
                posOnStrip=-total+pos+stripSize[stripNumber];
            }
        else
        {
            posOnStrip=pos;
        }

       setPixelinBufferByStrip(stripNumber,posOnStrip, red, green, blue, white);
    }


    void setPixelinBuffer(uint32_t pos, uint8_t red, uint8_t green, uint8_t blue)
    {
        uint8_t W = 0;
        if (nb_components > 3)
        {
            W = MIN(red, green);
            W = MIN(W, blue);
            red = red - W;
            green = green - W;
            blue = blue - W;
        }

        setPixelinBuffer(pos, red, green, blue, W);
    }

    void initled(int *Pinsq, int num_strips, int num_led_per_strip)
    {
        initled(NULL, Pinsq, num_strips, num_led_per_strip);
    }
    void waitSync()
    {
        I2SClocklessLedDriver_semSync=xSemaphoreCreateBinary();
        xSemaphoreTake(I2SClocklessLedDriver_semSync, portMAX_DELAY);
    }
#endif
    void setPixel(uint32_t pos, uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
    {
        uint8_t *offset = leds + (pos << 2); //faster than doing * 4
        *(offset) = red;
        *(++offset) = green;
        *(++offset) = blue;
        *(++offset) = white;
    }

    void setPixel(uint32_t pos, uint8_t red, uint8_t green, uint8_t blue)
    {

        if (nb_components == 3)
        {
            uint8_t *offset = leds + (pos << 1) + pos;
            *(offset) = red;
            *(++offset) = green;
            *(++offset) = blue;
        }
        else
        {
            /*
                Code to transform RBG into RGBW thanks to @Jonathanese https://github.com/Jonathanese/NodeMCUPoleDriver/blob/master/LED_Framework.cpp
            */
            uint8_t W = MIN(red, green);
            W = MIN(W, blue);
            red = red - W;
            green = green - W;
            blue = blue - W;
            setPixel(pos, red, green, blue, W);
        }
    }

    /* hardware scroll

*/

    OffsetDisplay getDefaultOffset()
    {
        return _defaultOffsetDisplay;
    }

    void showPixels(uint8_t *new_leds, OffsetDisplay offdisp)
    {
        _offsetDisplay = offdisp;
        showPixels(new_leds);
        _offsetDisplay = _defaultOffsetDisplay;
    }

    void showPixels(OffsetDisplay offdisp)
    {
        _offsetDisplay = offdisp;
        showPixels();
        _offsetDisplay = _defaultOffsetDisplay;
    }

    /*
Show pixel circular
*/
    /*
    void showPixels(uint8_t *new_leds)
    {
         _offsetDisplay=offdisp
        showPixels(new_leds);
        startleds=0;
    }*/

    /*
    void showPixels(int offsett)
    {
        startleds=offsett;
        showPixels();
        startleds=0;
    }
    */

    /*
Show pixels classiques
*/
    void showPixels(uint8_t *newleds)
    {
       // uint8_t *tmp_leds;
        //tmp_leds = leds;
       // leds = newleds;
        showPixels(WAIT,newleds);
     //   leds = tmp_leds;
    }

    void showPixels()
    {
        showPixels(WAIT);
    }

    
        void showPixels(displayMode dispmode,uint8_t *newleds)
    {
        //uint8_t *tmp_leds;
        //tmp_leds = leds;
         if(isDisplaying == true and __displayMode==NO_WAIT)
         {
            //printf("dejà en cours on attend\n");
            //long t1=ESP.getCycleCount();
            wasWaitingtofinish = true;
            if(I2SClocklessLedDriver_semDisp==NULL)
                I2SClocklessLedDriver_semDisp = xSemaphoreCreateBinary();
                const TickType_t xDelay = 50 ; //to avoid full blocking
            xSemaphoreTake(I2SClocklessLedDriver_semDisp, xDelay);
            //printf("on retourne %ld\n",(ESP.getCycleCount()-t1)/240000);
         }
        leds = newleds;
        showPixels(dispmode);
        //leds = tmp_leds;
    }

    void showPixels(displayMode dispmode)
    {

 if (dispmode == NO_WAIT && isDisplaying == true)
            {
                //printf("deja display\n");
                //return;
                wasWaitingtofinish = true;
                if(I2SClocklessLedDriver_semDisp==NULL)
                    I2SClocklessLedDriver_semDisp = xSemaphoreCreateBinary();
                    const TickType_t xDelay = 50 ; //to avoid full blocking
                xSemaphoreTake(I2SClocklessLedDriver_semDisp, xDelay);
                //printf("one re\n");
            }

        #if HARDWARESPRITES == 1
        memset(target, 0, num_led_per_strip * num_strips  * 2);
        for (int i = 0; i < 8; i++)
        {
            sprites[i].reorder(_offsetDisplay.panel_width, _offsetDisplay.panel_height);
        }
        #endif
        if (leds == NULL)
        {
            ESP_LOGE(TAG, "no leds buffer defined");
            return;
        }
        ledToDisplay = 0;
        transpose = true;
        DMABuffersTampon[0]->descriptor.qe.stqe_next = &(DMABuffersTampon[1]->descriptor);
        DMABuffersTampon[1]->descriptor.qe.stqe_next = &(DMABuffersTampon[0]->descriptor);
        DMABuffersTampon[2]->descriptor.qe.stqe_next = &(DMABuffersTampon[0]->descriptor);
        DMABuffersTampon[3]->descriptor.qe.stqe_next = 0;
        dmaBufferActive = 0;
        #ifdef ENABLE_HARDWARE_SCROLL
        loadAndTranspose(leds, num_led_per_strip, num_strips, _offsetDisplay, (uint16_t *)DMABuffersTampon[0]->buffer, ledToDisplay, __green_map, __red_map, __blue_map, __white_map, nb_components, p_g, p_r, p_b);
        #else
        loadAndTranspose(leds, stripSize, num_strips, (uint16_t *)DMABuffersTampon[0]->buffer, ledToDisplay, __green_map, __red_map, __blue_map, __white_map, nb_components, p_g, p_r, p_b);
        #endif
        dmaBufferActive = 1;
        i2sStart(DMABuffersTampon[2]);
        isDisplaying=true;
        if (dispmode == WAIT)
        {
            isWaiting = true;
            if (I2SClocklessLedDriver_sem==NULL)
            I2SClocklessLedDriver_sem=xSemaphoreCreateBinary();
            xSemaphoreTake(I2SClocklessLedDriver_sem, portMAX_DELAY);
        }
        else
        {
            isWaiting = false;
        }
        __displayMode=dispmode;
        //isWaiting = true;
        //I2SClocklessLedDriver_sem=xSemaphoreCreateBinary();
        //xSemaphoreTake(I2SClocklessLedDriver_sem, portMAX_DELAY);
    }


    Pixel * strip(int stripNum)
    {
        Pixel * l =(Pixel *)leds;
        //Serial.printf(" strip %d\n",stripNum);

        for(int i=0;i< (stripNum % num_strips);i++)
        {
             //Serial.printf("     strip %d\n",stripSize[i]);
            l=l+stripSize[i];
        }
        return l;
    }

    int maxLength(int *sizes,int num_strips)
    {
            int max=0;
            for(int i=0;i<num_strips;i++)
            {
                if(max<sizes[i])
                {
                    max=sizes[i];
                }
            }
            return max;
    }

#ifdef USE_PIXELSLIB
    void initled(Pixels pix,int *Pinsq)
    { 
        initled((uint8_t *)pix.getPixels(),Pinsq,pix.getLengths(), pix.getNumStrip());
    }
#endif
    void initled(uint8_t *leds, int *Pinsq, int *sizes,  int num_strips)
    {
        total_leds=0;
        for(int i=0;i<num_strips;i++)
        {
            this->stripSize[i]=sizes[i];
            total_leds+=sizes[i];
        }
        int maximum= maxLength( sizes,num_strips);
        //Serial.printf("maximum %d\n",maximum);
         ESP_LOGE(TAG, "maximum %d\n",maximum);
        nb_components = _nb_components;
        p_r = _p_r;
        p_g = _p_g;
        p_b = _p_b;
          __initled(leds, Pinsq, num_strips, maximum);
    }

    void initled(uint8_t *leds, int *Pinsq, int num_strips, int num_led_per_strip)
    {
         for(int i=0;i<num_strips;i++)
        {
            this->stripSize[i]=num_led_per_strip;
        }
        initled(leds, Pinsq,this->stripSize, num_strips);
    }


    void initled(uint8_t *leds, int *Pinsq, int *sizes,  int num_strips,colorarrangment cArr)
    {
        total_leds=0;
        for(int i=0;i<num_strips;i++)
        {
            this->stripSize[i]=sizes[i];
            total_leds+=sizes[i];
        }
        int maximum= maxLength( sizes,num_strips);

           
        switch (cArr)
        {
        case ORDER_RGB:
            nb_components = 3;
            p_r = 0;
            p_g = 1;
            p_b = 2;
            break;
        case ORDER_RBG:
            nb_components = 3;
            p_r = 0;
            p_g = 2;
            p_b = 1;
            break;
        case ORDER_GRB:
            nb_components = 3;
            p_r = 1;
            p_g = 0;
            p_b = 2;
            break;
        case ORDER_GBR:
            nb_components = 3;
            p_r = 2;
            p_g = 0;
            p_b = 1;
            break;
        case ORDER_BRG:
            nb_components = 3;
            p_r = 1;
            p_g = 2;
            p_b = 0;
            break;
        case ORDER_BGR:
            nb_components = 3;
            p_r = 2;
            p_g = 1;
            p_b = 0;
            break;
        case ORDER_GRBW:
            nb_components = 4;
            p_r = 1;
            p_g = 0;
            p_b = 2;
            break;
        }
          __initled(leds, Pinsq, num_strips, maximum);
    }

    void initled(uint8_t *leds, int *Pinsq, int num_strips, int num_led_per_strip,colorarrangment cArr)
    {
         for(int i=0;i<num_strips;i++)
        {
            this->stripSize[i]=num_led_per_strip;
        }
        initled(leds, Pinsq,this->stripSize, num_strips,cArr);
    }

/*
*
*
*
*
*/

    void __initled(uint8_t *leds, int *Pinsq, int num_strips, int num_led_per_strip)
    {
        _gammab = 1;
        _gammar = 1;
        _gammag = 1;
        _gammaw = 1;
        startleds = 0;
#if HARDWARESPRITES == 1
        //Serial.println(NUM_LEDS_PER_STRIP * NBIS2SERIALPINS * 8);
        target = (uint16_t *)malloc(num_led_per_strip * num_strips * 2 + 2);
#endif
        setBrightness(255);
        dmaBufferCount = 2;
        this->leds = leds;
        this->saveleds = leds;
        this->num_led_per_strip = num_led_per_strip;
        _offsetDisplay.offsetx = 0;
        _offsetDisplay.offsety = 0;
        _offsetDisplay.panel_width = num_led_per_strip;
        _offsetDisplay.panel_height = 9999;
        _defaultOffsetDisplay = _offsetDisplay;
        linewidth = num_led_per_strip;
        this->num_strips = num_strips;
        this->dmaBufferCount = dmaBufferCount;
        setPins(Pinsq);
        i2sInit();
        initDMABuffers();
    }

    //private:

    // intr_handle_t I2SClocklessLedDriver_intr_handle;// = NULL;
    //    xSemaphoreHandle I2SClocklessLedDriver_sem = NULL;
    //   xSemaphoreHandle I2SClocklessLedDriver_semSync = NULL;
    //   xSemaphoreHandle I2SClocklessLedDriver_semDisp= NULL;
    //buffer array for the transposed leds
    I2SClocklessLedDriverDMABuffer **DMABuffersTransposed = NULL;
    //buffer array for the regular way
    I2SClocklessLedDriverDMABuffer *DMABuffersTampon[4];

    I2SClocklessLedDriverDMABuffer *allocateDMABuffer(int bytes)
    {
        I2SClocklessLedDriverDMABuffer *b = (I2SClocklessLedDriverDMABuffer *)heap_caps_malloc(sizeof(I2SClocklessLedDriverDMABuffer), MALLOC_CAP_DMA);
        if (!b)
        {
            ESP_LOGE(TAG, "No more memory\n");
            return NULL;
        }

        b->buffer = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
        if (!b->buffer)
        {
            ESP_LOGE(TAG, "No more memory\n");
            return NULL;
        }
        memset(b->buffer, 0, bytes);

        b->descriptor.length = bytes;
        b->descriptor.size = bytes;
        b->descriptor.owner = 1;
        b->descriptor.sosf = 1;
        b->descriptor.buf = b->buffer;
        b->descriptor.offset = 0;
        b->descriptor.empty = 0;
        b->descriptor.eof = 1;
        b->descriptor.qe.stqe_next = 0;

        return b;
    }

    void i2sReset_DMA()
    {

        (&I2S0)->lc_conf.out_rst = 1;
        (&I2S0)->lc_conf.out_rst = 0;
    }

    void i2sReset_FIFO()
    {

        (&I2S0)->conf.tx_fifo_reset = 1;
        (&I2S0)->conf.tx_fifo_reset = 0;
    }

    void IRAM_ATTR i2sStop()
    {

       

       // xSemaphoreGive(I2SClocklessLedDriver_semDisp);
        esp_intr_disable(_gI2SClocklessDriver_intr_handle);
        i2sReset();

        (&I2S0)->conf.tx_start = 0;
        while( (&I2S0)->conf.tx_start ==1){}
         ets_delay_us(30);
       // isDisplaying = false;
        /*
         We have finished to display the strips
         */

        //xSemaphoreGive(I2SClocklessLedDriver_semDisp);
        if( __displayMode == NO_WAIT && wasWaitingtofinish == true)
        {
            // REG_WRITE(I2S_INT_CLR_REG(0), (REG_READ(I2S_INT_RAW_REG(0)) & 0xffffffc0) | 0x3f);
         //portBASE_TYPE HPTaskAwoken = 0;
               
               wasWaitingtofinish = false;
                  xSemaphoreGive(I2SClocklessLedDriver_semDisp);
                 
        }
    isDisplaying =false;
        leds=saveleds;
    }

    void putdefaultones(uint16_t *buffer)
    {
        /*order to push the data to the pins
         0:D7
         1:1
         2:1
         3:0
         4:0
         5:D6
         6:D5
         7:1
         8:1
         9:0
         10:0
         11:D4
         12:D3
         13:1
         14:1
         15:0
         16:0
         17:D2
         18:D1
         19:1
         20:1
         21:0
         22:0
         23:D0
         */
        for (int i = 0; i < nb_components * 8 / 2; i++)
        {
            buffer[i * 6 + 1] = 0xffff;
            buffer[i * 6 + 2] = 0xffff;
        }
    }

    /*
     Transpose the pixel, as the function is static and all the variables are not static or global, we need to provide all of them.
     */

    //    void transpose16x1_noinline2(uint8_t y,uint16_t *B,uint16_t mask,uint16_t mask2,int stripNumber) {
    //
    //        *((uint16_t*)(B)) =   (*((uint16_t*)(B))& mask) | ((uint16_t)((y &   128)>>7) <<stripNumber);
    //        *((uint16_t*)(B+5)) =   (*((uint16_t*)(B+5))& mask) | ((uint16_t)((y & 64)>>6) <<stripNumber);
    //        *((uint16_t*)(B+6)) =   (*((uint16_t*)(B+6))& mask) | ((uint16_t)((y & 32)>>5) <<stripNumber);
    //        *((uint16_t*)(B+11)) =   (*((uint16_t*)(B+11))& mask) | ((uint16_t)((y& 16)>>4)<<stripNumber);
    //        *((uint16_t*)(B+12)) =   (*((uint16_t*)(B+12))& mask) | ((uint16_t)((y& 8)>>3) <<stripNumber);
    //        *((uint16_t*)(B+17)) =   (*((uint16_t*)(B+17))& mask) | ((uint16_t)((y& 4)>>2) <<stripNumber);
    //        *((uint16_t*)(B+18)) =   (*((uint16_t*)(B+18))& mask) | ((uint16_t)((y& 2)>>1) <<stripNumber);
    //        *((uint16_t*)(B+23)) =   (*((uint16_t*)(B+23))& mask) | ((uint16_t)(y & 1) <<stripNumber);
    //
    //    }

    void i2sStart(I2SClocklessLedDriverDMABuffer *startBuffer)
    {

        i2sReset();
        framesync = false;
        counti = 0;

        (&I2S0)->lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN | I2S_OUT_DATA_BURST_EN;

        (&I2S0)->out_link.addr = (uint32_t) & (startBuffer->descriptor);

        (&I2S0)->out_link.start = 1;

        (&I2S0)->int_clr.val = (&I2S0)->int_raw.val;

        (&I2S0)->int_clr.val = (&I2S0)->int_raw.val;
        (&I2S0)->int_ena.val = 0;

        /*
         If we do not use the regular showpixels, then no need to activate the interupt at the end of each pixels
         */
        //if(transpose)
        (&I2S0)->int_ena.out_eof = 1;

        (&I2S0)->int_ena.out_total_eof = 1;
        esp_intr_enable(_gI2SClocklessDriver_intr_handle);

        //We start the I2S
        (&I2S0)->conf.tx_start = 1;

        //Set the mode to indicate that we've started
        isDisplaying = true;
    }

    void IRAM_ATTR i2sReset()
    {
        const unsigned long lc_conf_reset_flags = I2S_IN_RST_M | I2S_OUT_RST_M | I2S_AHBM_RST_M | I2S_AHBM_FIFO_RST_M;
        (&I2S0)->lc_conf.val |= lc_conf_reset_flags;
        (&I2S0)->lc_conf.val &= ~lc_conf_reset_flags;
        const uint32_t conf_reset_flags = I2S_RX_RESET_M | I2S_RX_FIFO_RESET_M | I2S_TX_RESET_M | I2S_TX_FIFO_RESET_M;
        (&I2S0)->conf.val |= conf_reset_flags;
        (&I2S0)->conf.val &= ~conf_reset_flags;
    }

    // static void IRAM_ATTR interruptHandler(void *arg);
};
static void IRAM_ATTR _I2SClocklessLedDriverinterruptHandler(void *arg)
{
#ifdef DO_NOT_USE_INTERUPT
    REG_WRITE(I2S_INT_CLR_REG(0), (REG_READ(I2S_INT_RAW_REG(0)) & 0xffffffc0) | 0x3f);
    return;
#else
    I2SClocklessLedDriver *cont = (I2SClocklessLedDriver *)arg;

    if (GET_PERI_REG_BITS(I2S_INT_ST_REG(I2S_DEVICE), I2S_OUT_EOF_INT_ST_V, I2S_OUT_EOF_INT_ST_S))
    {
        cont->framesync = !cont->framesync;

        if (((I2SClocklessLedDriver *)arg)->transpose)
        {
            cont->ledToDisplay++;
            if (cont->ledToDisplay < cont->num_led_per_strip)
            {
                #ifdef ENABLE_HARDWARE_SCROLL
                loadAndTranspose(cont->leds, cont->num_led_per_strip, cont->num_strips, cont->_offsetDisplay, (uint16_t *)cont->DMABuffersTampon[cont->dmaBufferActive]->buffer, cont->ledToDisplay, cont->__green_map, cont->__red_map, cont->__blue_map, cont->__white_map, cont->nb_components, cont->p_g, cont->p_r, cont->p_b);
               #else
               loadAndTranspose(cont->leds, cont->stripSize, cont->num_strips, (uint16_t *)cont->DMABuffersTampon[cont->dmaBufferActive]->buffer, cont->ledToDisplay, cont->__green_map, cont->__red_map, cont->__blue_map, cont->__white_map, cont->nb_components, cont->p_g, cont->p_r, cont->p_b);
               #endif

               if (cont->ledToDisplay == cont->num_led_per_strip - 3) //here it's not -1 because it takes time top have the change into account and it reread the buufer
                {
                    cont->DMABuffersTampon[cont->dmaBufferActive]->descriptor.qe.stqe_next = &(cont->DMABuffersTampon[3]->descriptor);
                }
                cont->dmaBufferActive = (cont->dmaBufferActive + 1) % 2;
            }
        }
        else
        {
            if (cont->framesync)
            {
                portBASE_TYPE HPTaskAwoken = 0;
                xSemaphoreGiveFromISR(cont->I2SClocklessLedDriver_semSync, &HPTaskAwoken);
                if (HPTaskAwoken == pdTRUE)
                    portYIELD_FROM_ISR();
            }
        }
    }

    if (GET_PERI_REG_BITS(I2S_INT_ST_REG(I2S_DEVICE), I2S_OUT_TOTAL_EOF_INT_ST_V, I2S_OUT_TOTAL_EOF_INT_ST_S))
    {

             
                 
        ((I2SClocklessLedDriver *)arg)->i2sStop();
        /*
        if( cont->__displayMode == NO_WAIT && cont->wasWaitingtofinish == true)
        {
            // REG_WRITE(I2S_INT_CLR_REG(0), (REG_READ(I2S_INT_RAW_REG(0)) & 0xffffffc0) | 0x3f);
         portBASE_TYPE HPTaskAwoken = 0;
               cont->isDisplaying =false;
               cont->wasWaitingtofinish = false;
                  xSemaphoreGiveFromISR(((I2SClocklessLedDriver *)arg)->I2SClocklessLedDriver_semDisp, &HPTaskAwoken);
                  if(HPTaskAwoken == pdTRUE) portYIELD_FROM_ISR(HPTaskAwoken);
        }*/
        if (cont->isWaiting)
        {
            portBASE_TYPE HPTaskAwoken = 0;
            xSemaphoreGiveFromISR(cont->I2SClocklessLedDriver_sem, &HPTaskAwoken);
            if (HPTaskAwoken == pdTRUE)
                portYIELD_FROM_ISR(HPTaskAwoken);
        }
    }
    REG_WRITE(I2S_INT_CLR_REG(0), (REG_READ(I2S_INT_RAW_REG(0)) & 0xffffffc0) | 0x3f);
#endif
}

static void IRAM_ATTR transpose16x1_noinline2(unsigned char *A, uint16_t *B)
{

    uint32_t x, y, x1, y1, t;

    y = *(unsigned int *)(A);
#if NUMSTRIPS > 4
    x = *(unsigned int *)(A + 4);
#else
    x = 0;
#endif

#if NUMSTRIPS > 8
    y1 = *(unsigned int *)(A + 8);
#else
    y1 = 0;
#endif
#if NUMSTRIPS > 12
    x1 = *(unsigned int *)(A + 12);
#else
    x1 = 0;
#endif

    // pre-transform x
#if NUMSTRIPS > 4
    t = (x ^ (x >> 7)) & AA;
    x = x ^ t ^ (t << 7);
    t = (x ^ (x >> 14)) & CC;
    x = x ^ t ^ (t << 14);
#endif
#if NUMSTRIPS > 12
    t = (x1 ^ (x1 >> 7)) & AA;
    x1 = x1 ^ t ^ (t << 7);
    t = (x1 ^ (x1 >> 14)) & CC;
    x1 = x1 ^ t ^ (t << 14);
#endif
    // pre-transform y
    t = (y ^ (y >> 7)) & AA;
    y = y ^ t ^ (t << 7);
    t = (y ^ (y >> 14)) & CC;
    y = y ^ t ^ (t << 14);
#if NUMSTRIPS > 8
    t = (y1 ^ (y1 >> 7)) & AA;
    y1 = y1 ^ t ^ (t << 7);
    t = (y1 ^ (y1 >> 14)) & CC;
    y1 = y1 ^ t ^ (t << 14);
#endif
    // final transform
    t = (x & FF) | ((y >> 4) & FF2);
    y = ((x << 4) & FF) | (y & FF2);
    x = t;

    t = (x1 & FF) | ((y1 >> 4) & FF2);
    y1 = ((x1 << 4) & FF) | (y1 & FF2);
    x1 = t;

    *((uint16_t *)(B)) = (uint16_t)(((x & 0xff000000) >> 8 | ((x1 & 0xff000000))) >> 16);
    *((uint16_t *)(B + 5)) = (uint16_t)(((x & 0xff0000) >> 16 | ((x1 & 0xff0000) >> 8)));
    *((uint16_t *)(B + 6)) = (uint16_t)(((x & 0xff00) | ((x1 & 0xff00) << 8)) >> 8);
    *((uint16_t *)(B + 11)) = (uint16_t)((x & 0xff) | ((x1 & 0xff) << 8));
    *((uint16_t *)(B + 12)) = (uint16_t)(((y & 0xff000000) >> 8 | ((y1 & 0xff000000))) >> 16);
    *((uint16_t *)(B + 17)) = (uint16_t)(((y & 0xff0000) | ((y1 & 0xff0000) << 8)) >> 16);
    *((uint16_t *)(B + 18)) = (uint16_t)(((y & 0xff00) | ((y1 & 0xff00) << 8)) >> 8);
    *((uint16_t *)(B + 23)) = (uint16_t)((y & 0xff) | ((y1 & 0xff) << 8));
}

#ifdef ENABLE_HARDWARE_SCROLL
static void IRAM_ATTR loadAndTranspose(uint8_t *ledt, int led_per_strip, int num_stripst, OffsetDisplay offdisp, uint16_t *buffer, int ledtodisp, uint8_t *mapg, uint8_t *mapr, uint8_t *mapb, uint8_t *mapw, int nbcomponents, int pg, int pr, int pb)
{
    Lines secondPixel[nbcomponents];

    uint32_t offp, offi, offsetled;
    uint8_t _g, _r, _b;
    int x, y, X, Y, deltaY;
    deltaY = led_per_strip / offdisp.panel_width; //calculation of height per strip
    //printf("li%d:\n",line_width);
    y = ledtodisp / offdisp.panel_width;
    x = ledtodisp % offdisp.panel_width;
    Y = y;
    X = x;
#if SNAKEPATTERN == 1

    //move in y
    if (MOD(offdisp.offsety + 2 * y, offdisp.panel_height) % 2 == 0)
    {
        y = MOD(offdisp.offsety + y, offdisp.panel_height);
    }
    else
    {
        y = MOD(offdisp.offsety + y, offdisp.panel_height);
        x = offdisp.panel_width - x - 1;
    }

    //move in x
    if (y % 2 == 0)
    {
        //x=ledtodisp%offdisp.panel_width;
        // off=((x+startleds)%line_width)+y*line_width;
        offi = MOD(x + offdisp.offsetx, offdisp.panel_width) + y * offdisp.panel_width;
#if ALTERNATEPATTERN == 1
        offp = MOD(x - offdisp.offsetx, offdisp.panel_width) + y * offdisp.panel_width;
#else
        offp = offi;
#endif
    }
    else
    {
        //x=ledtodisp%offdisp.panel_width;
        //off=(line_width-x-startleds+1)%line_width+y*line_width;
        offi = MOD(x - offdisp.offsetx, offdisp.panel_width) + y * offdisp.panel_width;
#if ALTERNATEPATTERN == 1
        offp = MOD(x + offdisp.offsetx, offdisp.panel_width) + y * offdisp.panel_width;
#else
        offp = offi;
#endif
    }

#else
    //x = ledtodisp % offdisp.panel_width;
    offi = ((x + offdisp.offsetx) % offdisp.panel_width) + y * offdisp.panel_width;
#if ALTERNATEPATTERN == 1
    offp = MOD(x - offdisp.offsetx, offdisp.panel_width) + y * offdisp.panel_width;
#else
    offp = offi;
#endif
#endif

    // uint8_t *poli = ledt + ((ledtodisp+startleds)%led_per_strip) * nbcomponents;
    offi = offi * nbcomponents;
    offp = offp * nbcomponents;
    uint8_t *poli = ledt + offi;
    offsetled = ledtodisp;
    for (int i = 0; i < num_stripst; i++)
    {

        if (poli >= ledt + (uint32_t)offdisp.panel_width * offdisp.panel_height * nbcomponents)
        {
            //Serial.printf("%d %d %d\n",i,ledtodisp,((uint32_t)poli - (uint32_t)offdisp.panel_width * offdisp.panel_height * nbcomponents-(uint32_t)ledt))/3;
            poli = poli - (uint32_t)offdisp.panel_width * offdisp.panel_height * nbcomponents;
        }
        else
        {
            if (poli < ledt)
            {
                //Serial.printf("neg %d %d %d\n",i,ledtodisp,(ledt-poli)/3);
                poli += (uint32_t)offdisp.panel_width * offdisp.panel_height * nbcomponents;
            }
        }
        /*
            
        */
#if HARDWARESPRITES == 1
        //res re=RE(offsetled);
        if (target[offsetled] == 0)
        {
            _g = *(poli + 1);
            _r = *(poli);
            _b = *(poli + 2);
        }
        else
        {
            _g = _spritesleds[target[offsetled] - 1 + 1];
            _r = _spritesleds[target[offsetled] - 1];
            _b = _spritesleds[target[offsetled] - 1 + 2];
        }
#else
        _g = *(poli + 1);
        _r = *(poli);
        _b = *(poli + 2);
#endif

        secondPixel[pg].bytes[i] = mapg[_g];
        secondPixel[pr].bytes[i] = mapr[_r];
        secondPixel[pb].bytes[i] = mapb[_b];
        if (nbcomponents > 3)
            secondPixel[3].bytes[i] = mapw[*(poli + 3)];

        poli += led_per_strip * nbcomponents;
#if HARDWARESPRITES == 1
        offsetled += led_per_strip;
#endif
        Y += deltaY;

        if (i % 2 == 0)
        {
            poli = poli - offi + offp;
        }
        else
        {
            poli = poli - offp + offi;
        }
    }

    transpose16x1_noinline2(secondPixel[0].bytes, (uint16_t *)buffer);
    transpose16x1_noinline2(secondPixel[1].bytes, (uint16_t *)buffer + 3 * 8);
    transpose16x1_noinline2(secondPixel[2].bytes, (uint16_t *)buffer + 2 * 3 * 8);
    if (nbcomponents > 3)
        transpose16x1_noinline2(secondPixel[3].bytes, (uint16_t *)buffer + 3 * 3 * 8);
}

#else
static void IRAM_ATTR loadAndTranspose(uint8_t *ledt, int *sizes, int num_stripst, uint16_t *buffer, int ledtodisp, uint8_t *mapg, uint8_t *mapr, uint8_t *mapb, uint8_t *mapw, int nbcomponents, int pg, int pr, int pb)
{
    Lines secondPixel[nbcomponents];

    memset(secondPixel,0,sizeof(secondPixel));
    uint8_t *poli = ledt + ledtodisp * nbcomponents;
    for (int i = 0; i < num_stripst; i++)
    {
        if(ledtodisp < sizes[i])
        {
        secondPixel[pg].bytes[i] = mapg[*(poli + 1)];
        secondPixel[pr].bytes[i] = mapr[*(poli + 0)];
        secondPixel[pb].bytes[i] = mapb[*(poli + 2)];
        if (nbcomponents > 3)
            secondPixel[3].bytes[i] = mapw[*(poli + 3)];
        }
        

        poli += sizes[i]* nbcomponents;
    }

    transpose16x1_noinline2(secondPixel[0].bytes, (uint16_t *)buffer);
    transpose16x1_noinline2(secondPixel[1].bytes, (uint16_t *)buffer + 3 * 8);
    transpose16x1_noinline2(secondPixel[2].bytes, (uint16_t *)buffer + 2 * 3 * 8);
    if (nbcomponents > 3)
        transpose16x1_noinline2(secondPixel[3].bytes, (uint16_t *)buffer + 3 * 3 * 8);
}
#endif

#endif
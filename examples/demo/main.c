/*
 * MIT License
 *
 * Copyright(c) 2023-present All contributors of SGL
 * Document reference link: docs directory
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include "stm32f7xx_hal.h"
#include "sgl.h"
#include "sgl_anim.h"
#include "sgl_font.h"

#define TFT_RST_PORT GPIOD
#define TFT_RST_PIN GPIO_PIN_12
#define TFT_DC_PORT GPIOD
#define TFT_DC_PIN GPIO_PIN_13

#define TFT_WIDTH 128
#define TFT_HEIGHT 160

static UART_HandleTypeDef
huart3;

static SPI_HandleTypeDef
hspi2;

static DMA_HandleTypeDef
hdma1ch4;

static sgl_color_t
panel_buffer[TFT_WIDTH * 16];

static unsigned int
frames, fps;

static bool
info_update;

int
__io_putchar(int ch)
{
    if (ch == '\n')
        HAL_UART_Transmit(&huart3, (void *)"\r", 1, -1);
    HAL_UART_Transmit(&huart3, (void *)&ch, 1, -1);
    return ch;
}

static void
logger(const char *str)
{
    printf("%s\n", str);
}

static void
tft_write_cmd(uint16_t cmd)
{
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, 0);
    HAL_SPI_Transmit(&hspi2, (void *)&cmd, 1, -1);
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, 1);
}

static void
tft_write_data(uint16_t data)
{
    HAL_SPI_Transmit(&hspi2, (void *)&data, 1, -1);
}

static void
tft_set_win(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /* Set column address */
    tft_write_cmd(0x2a);
	tft_write_data(x1);
	tft_write_data(x2);

    /* Set row address */
    tft_write_cmd(0x2b);
	tft_write_data(y1);
	tft_write_data(y2);

    /* Write frame buffer */
    tft_write_cmd(0x2c);
}

static void
tft_init_sequence(void)
{
	tft_write_cmd(0x11);
	HAL_Delay(120);

    /* Rotation */
	tft_write_cmd(0x36);
	tft_write_data(0xc0);

	tft_write_cmd(0x3a);
	tft_write_data(0x00);

    /* Frame rate setting */
	tft_write_cmd(0xb1);
	tft_write_data(0x05);
	tft_write_data(0x3c);
	tft_write_data(0x3c);

	tft_write_cmd(0xb2);
	tft_write_data(0x05);
	tft_write_data(0x3c);
	tft_write_data(0x3c);

    tft_write_cmd(0xb3);
	tft_write_data(0x05);
	tft_write_data(0x3c);
	tft_write_data(0x3c);
	tft_write_data(0x05);
	tft_write_data(0x3c);
	tft_write_data(0x3c);

	tft_write_cmd(0xb4);
	tft_write_data(0x03);

    /* Power setting */
    tft_write_cmd(0xc0);
	tft_write_data(0x28);
	tft_write_data(0x08);
	tft_write_data(0x04);

    tft_write_cmd(0xc1);
	tft_write_data(0xc0);

    tft_write_cmd(0xc2);
	tft_write_data(0x0d);
	tft_write_data(0x00);

    tft_write_cmd(0xc3);
	tft_write_data(0x8d);
	tft_write_data(0x2a);

    tft_write_cmd(0xc4);
	tft_write_data(0x8d);
	tft_write_data(0xee);

    tft_write_cmd(0xc5);
	tft_write_data(0x1a);

    tft_write_cmd(0xe0);
	tft_write_data(0x04);
	tft_write_data(0x22);
	tft_write_data(0x07);
	tft_write_data(0x0a);
	tft_write_data(0x2e);
	tft_write_data(0x30);
	tft_write_data(0x25);
	tft_write_data(0x2a);
	tft_write_data(0x28);
	tft_write_data(0x26);
	tft_write_data(0x2e);
	tft_write_data(0x3a);
	tft_write_data(0x00);
	tft_write_data(0x01);
	tft_write_data(0x03);
	tft_write_data(0x13);

	tft_write_cmd(0xe1);
	tft_write_data(0x04);
	tft_write_data(0x16);
	tft_write_data(0x06);
	tft_write_data(0x0d);
	tft_write_data(0x2d);
	tft_write_data(0x26);
	tft_write_data(0x23);
	tft_write_data(0x27);
	tft_write_data(0x27);
	tft_write_data(0x25);
	tft_write_data(0x2d);
	tft_write_data(0x3b);
	tft_write_data(0x00);
	tft_write_data(0x01);
	tft_write_data(0x04);
	tft_write_data(0x13);

    tft_write_cmd(0x3a);
	tft_write_data(0x05);
	tft_write_cmd(0x29);
}

static void
tft_init(void)
{
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, 0);
    HAL_Delay(100);
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, 1);
    HAL_Delay(100);

    tft_init_sequence();
    hspi2.Init.DataSize = SPI_DATASIZE_16BIT;
    HAL_SPI_Init(&hspi2);
}

static void
tft_flush_area(int16_t x, int16_t y, int16_t w, int16_t h, sgl_color_t *src)
{
    while (hspi2.State != HAL_SPI_STATE_READY)
        __NOP();

    tft_set_win(x, y, x + w - 1, y + h - 1);
    SCB_CleanDCache_by_Addr((void *)src, w * h * sizeof(sgl_color_t));
	HAL_SPI_Transmit_DMA(&hspi2, (void *)src, w * h);
}

static void
demo_anim_path(struct sgl_anim *anim, int32_t value)
{
    sgl_obj_set_pos(anim->obj, value, value);
}

static void
demo_anim_finish(struct sgl_anim *anim)
{
    uint16_t tmp;

    tmp = anim->start_value;
    anim->start_value = anim->end_value;
    anim->end_value = tmp;
}

void
DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(hspi2.hdmatx);
}

void
SysTick_Handler(void)
{
    static unsigned int timer, last;

    if (++timer >= 1000) {
        fps = frames - last;
        printf("Render: %ufps\n", fps);
        info_update = true;
        last = frames;
        timer = 0;
    }

    sgl_anim_tick_inc(1);
    HAL_IncTick();
}

int
main(void)
{
    RCC_OscInitTypeDef OscInitType = {};
    RCC_ClkInitTypeDef ClkInitType = {};
    GPIO_InitTypeDef GPIOInitType = {};
    char buffer1[64];

    HAL_Init();
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    OscInitType.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    OscInitType.HSEState = RCC_HSE_ON;
    OscInitType.PLL.PLLState = RCC_PLL_ON;
    OscInitType.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    OscInitType.PLL.PLLM = 6;
    OscInitType.PLL.PLLN = 216;
    OscInitType.PLL.PLLP = RCC_PLLP_DIV2;
    OscInitType.PLL.PLLQ = 2;
    OscInitType.PLL.PLLR = 2;
    HAL_RCC_OscConfig(&OscInitType);
    HAL_PWREx_EnableOverDrive();

    ClkInitType.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                            RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    ClkInitType.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    ClkInitType.AHBCLKDivider = RCC_SYSCLK_DIV1;
    ClkInitType.APB1CLKDivider = RCC_HCLK_DIV4;
    ClkInitType.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&ClkInitType, FLASH_LATENCY_4);

    SCB_EnableICache();
    SCB_EnableDCache();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* UART: TX */
    GPIOInitType.Pin = GPIO_PIN_10;
    GPIOInitType.Mode = GPIO_MODE_AF_PP;
    GPIOInitType.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIOInitType.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIOInitType);

    /* TFT: CS SCK SDA */
    GPIOInitType.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;
    GPIOInitType.Mode = GPIO_MODE_AF_PP;
    GPIOInitType.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIOInitType.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIOInitType);

    /* TFT: DC */
    GPIOInitType.Pin = TFT_DC_PIN;
    GPIOInitType.Mode = GPIO_MODE_OUTPUT_PP;
    GPIOInitType.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(TFT_DC_PORT, &GPIOInitType);

    /* TFT: RST */
    GPIOInitType.Pin = TFT_RST_PIN;
    GPIOInitType.Mode = GPIO_MODE_OUTPUT_PP;
    GPIOInitType.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(TFT_RST_PORT, &GPIOInitType);

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.Mode = UART_MODE_TX;
    HAL_UART_Init(&huart3);

    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 7;
    hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    HAL_SPI_Init(&hspi2);
    tft_init();

    hdma1ch4.Instance = DMA1_Stream4;
    hdma1ch4.Init.Channel = DMA_CHANNEL_0;
    hdma1ch4.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma1ch4.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma1ch4.Init.MemInc = DMA_MINC_ENABLE;
    hdma1ch4.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma1ch4.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma1ch4.Init.Mode = DMA_NORMAL;
    hdma1ch4.Init.Priority = DMA_PRIORITY_LOW;
    hdma1ch4.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma1ch4);

    __HAL_LINKDMA(&hspi2, hdmatx, hdma1ch4);
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

    sgl_device_fb_t fbdev = {
        .xres = TFT_WIDTH,
        .yres = TFT_HEIGHT,
        .xres_virtual = TFT_WIDTH,
        .yres_virtual = TFT_HEIGHT,
        .flush_area = tft_flush_area,
        .framebuffer = panel_buffer,
        .framebuffer_size = SGL_ARRAY_SIZE(panel_buffer),
    };

    sgl_device_fb_register(&fbdev);
    sgl_device_log_register(logger);
    sgl_init();

    sgl_obj_t *textline1 = sgl_textline_create(NULL);
    sgl_obj_set_pos(textline1, 0, TFT_HEIGHT - 25);
    sgl_obj_set_size(textline1, TFT_WIDTH, 25);
    sgl_obj_set_alpha(textline1, 180);
    sgl_obj_set_bg_color(textline1, SGL_GRAY);
    sgl_obj_set_style(textline1, SGL_STYLE_TEXT_COLOR, SGL_COLOR(SGL_WHITE));
    sgl_obj_set_style(textline1, SGL_STYLE_FONT, SGL_FONT(song23));
    sgl_obj_set_style(textline1, SGL_STYLE_TEXT_MARGIN, 10);
    sgl_obj_set_text(textline1, "FPS: 0");

    sgl_obj_t *rect1 = sgl_rect_create(NULL);
    sgl_obj_set_pos(rect1, 0, 0);
    sgl_obj_set_size(rect1, 30, 30);
    sgl_obj_set_color(rect1, SGL_GRAY);
    sgl_obj_set_border_color(rect1, SGL_GREEN);
    sgl_obj_set_border_width(rect1, 3);
    sgl_obj_set_radius(rect1, 4);
    sgl_obj_set_alpha(rect1, 100);

    sgl_obj_t *rect2 = sgl_rect_create(NULL);
    sgl_obj_set_pos(rect2, 0, 0);
    sgl_obj_set_size(rect2, 30, 30);
    sgl_obj_set_color(rect2, SGL_BRIGHT_PURPLE);
    sgl_obj_set_border_color(rect2, SGL_GREEN);
    sgl_obj_set_border_width(rect2, 3);
    sgl_obj_set_radius(rect2, 5);
    sgl_obj_set_alpha(rect2, 100);

    sgl_anim_t *anim1 = sgl_anim_create();
    sgl_anim_set_obj(anim1, rect1);
    sgl_anim_set_act_duration(anim1, 1200);
    sgl_anim_set_start_value(anim1, 10);
    sgl_anim_set_end_value(anim1, TFT_WIDTH - 30);
    sgl_anim_set_path(anim1, demo_anim_path);
    sgl_anim_set_path_algo(anim1, SGL_ANIM_PATH_LINEAR);
    sgl_anim_set_repeat_cnt(anim1, -1);
    anim1->finish_cb = demo_anim_finish;

    sgl_anim_t *anim2 = sgl_anim_create();
    sgl_anim_set_obj(anim2, rect2);
    sgl_anim_set_act_duration(anim2, 1200);
    sgl_anim_set_start_value(anim2, 30);
    sgl_anim_set_end_value(anim2, TFT_WIDTH - 30);
    sgl_anim_set_path(anim2, demo_anim_path);
    sgl_anim_set_path_algo(anim2, SGL_ANIM_PATH_LINEAR);
    sgl_anim_set_repeat_cnt(anim2, -1);
    anim2->finish_cb = demo_anim_finish;

    sgl_anim_start(anim1);
    sgl_anim_start(anim2);

    for (;;) {
        if (info_update) {
            snprintf(buffer1, sizeof(buffer1) - 1, "FPS: %d", fps);
            sgl_obj_set_text(textline1, buffer1);
            info_update = false;
        }

        sgl_task_handle();
        frames++;
    }

    return 0;
}

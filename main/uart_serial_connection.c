/* UART Request Handling Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

/**
 * This example handles UART communication, responding to requests from the PC
 * with the first number in an array. If the array is empty, it throws an error message.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (GPIO_NUM_15)   // GPIO number for TX (UART1)
#define ECHO_TEST_RXD (GPIO_NUM_16)   // GPIO number for RX (UART1)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE) // RTS not used
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE) // CTS not used

#define ECHO_UART_PORT_NUM      (UART_NUM_1) // Use UART1
#define ECHO_UART_BAUD_RATE     (115200)     // Baud rate
#define ECHO_TASK_STACK_SIZE    (4096)       // Stack size for the echo task

static const char *TAG = "UART TEST";

#define BUF_SIZE (1024)
#define ARRAY_SIZE 10
#define RESPONSE "Hello, it is me, ESP32"
#define ERROR_MESSAGE "Error: Array is empty"

// Array of 10 numbers
int number_array[ARRAY_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int array_length = ARRAY_SIZE;

static void handle_request()
{
    char response[BUF_SIZE];
    
    if (array_length > 0) {
        // Prepare the response with the first number in the array
        snprintf(response, BUF_SIZE, "Number: %d", number_array[0]);
        
        // Shift the rest of the numbers
        for (int i = 1; i < array_length; ++i) {
            number_array[i - 1] = number_array[i];
        }
        array_length--;
    } else {
        // Array is empty, prepare error message
        strncpy(response, ERROR_MESSAGE, BUF_SIZE);
    }
    
    // Send the response or error message
    uart_write_bytes(ECHO_UART_PORT_NUM, response, strlen(response));
    ESP_LOGI(TAG, "%s", response);
}

static void echo_task(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            // Handle the request and respond
            handle_request();
        }
    }
}

void app_main(void)
{
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}
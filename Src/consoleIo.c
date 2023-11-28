// Console IO is a wrapper between the actual in and output and the console code
// In an embedded system, this might interface to a UART driver.

#include "consoleIo.h"
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

// We have to get access to the UART handle defined in main.c
extern UART_HandleTypeDef huart1;

uint8_t receivedChar;
// Size of buffer has been slightly arbitrarily chosen.
// The buffer only needs to large enough to store characters that are received
// between ConsoleIoReceive calls. It's unlikely this will be as many as 10 characters.
uint8_t receivedBuffer[10];
volatile uint8_t nextCharIdx;


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  // Echo back the received character. This should maybe also use transmit using interrupts.
  HAL_UART_Transmit(huart, &receivedChar, 1, 10);

  receivedBuffer[nextCharIdx] = receivedChar;
  // If we fill the buffer we start again at the beginning.
  // This would mean we lose characters. If this happens frequently we'd need to increase
  // the size of the buffer.
  if (++nextCharIdx == 10) {
    nextCharIdx = 0;
  }

  HAL_UART_Receive_IT(huart, &receivedChar, 1);
}

eConsoleError ConsoleIoInit()
{
  HAL_UART_Receive_IT(&huart1, &receivedChar, 1);

	return CONSOLE_SUCCESS;
}

eConsoleError ConsoleIoReceive(uint8_t *buffer, const uint32_t bufferLength, uint32_t *readLength)
{
	uint32_t i = 0;
	uint8_t ch;
	
	// Disable the UART interrupts while we transfer characters from the receivedBuffer
	// so that we don't risk losing in transit characters.
	HAL_NVIC_DisableIRQ(USART1_IRQn);
	ch = receivedBuffer[i];
	while (( i < bufferLength ) && ( i < nextCharIdx ))
	{
		buffer[i] = (uint8_t) ch;
		i++;
		ch = receivedBuffer[i];
	}
	*readLength = i;

	nextCharIdx = 0;
	HAL_NVIC_EnableIRQ(USART1_IRQn);

	return CONSOLE_SUCCESS;
}

eConsoleError ConsoleIoSendString(const char *buffer)
{
  uint32_t i = 0;
  uint8_t ch;

  ch = (uint8_t) buffer[i];
  while (ch != '\0')
  {
    HAL_UART_Transmit(&huart1, &ch, 1, 10);
    i++;
    ch = buffer[i];
  }

	return CONSOLE_SUCCESS;
}



#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "codec.h"
#include "stm32f4xx_it.h"
#include <math.h>

#define CTRL_REG1 0x20
#define OUTX 0xA9
#define OUTY 0xAB
#define OUTZ 0xAD

#define GREENLED LED4
#define ORANGELED LED3
#define BLUELED LED6
#define REDLED LED5

#define DECAY_FACTOR 0.9996		// Factor by which averaged values will be multiplied for the fading effect of a note
#define DURATION 24100			// Duration for a note that is neither long nor short
#define DURATION_DELAY 55100 	// Duration for a long note
#define DURATION_END 55100		// Duration for a note that ends a melody
#define OUTBUFFERSIZE 44100		// Size of buffer to hold the note waveform
#define DACBUFFERSIZE 1200		// Default size of buffer to be used to synthesize a single note

// Configuration function prototypes
void RCC_Configuration(void);
void RNG_Configuration(void);
void GPIO_Configuration(void);
void Button_GPIO_Configuration(void);
void SPI_Configuration(void);
void peripheralsInit(void);

// Control function prototypes
void delay_ms(uint32_t milli);
int32_t writeAccelByte(uint8_t regAdr, uint8_t data);
void checkAcc(void);
void updatePitchAndSpeed(uint16_t amount);
void setNoteFrequency(char notes[], uint8_t i);
void generateNote(void);

//Melody function prototypes
void playTwinkle(void);
void playLittleLamb(void);


// Status variables
uint8_t playing = 0; 	// 0 - Paused, 1 - playing
uint8_t enabled = 0;	// 0 - enabled 1 - disabled - Used to enable manipulation of sound by the accelerometer
__IO uint8_t mode = 5; 	// 0 - Algorithm mode, 1 - USB mode
uint8_t currentNote;	// Used to track position of melody when paused

__IO uint8_t outBuffer[OUTBUFFERSIZE];	// Buffer that stores synthesized note waveform

__IO float octave = 3;				// default note pitch variable
__IO float noteFreq = 20.6;			// Default note frequency
__IO float amplitude = 5.0;			// Determines how loud the sound is
__IO uint32_t duration = 44100;		// Global note duration variable

// Melody arrays
__IO char twinkle[42] = {'C','C','G','G','A','A','G','F','F','E','E','D','D','C','G','G','F','F','E','E','D','G','G','F','F','E','E','D','C','C','G','G','A','A','G','F','F','E','E','D','D','C'};
__IO char littleLamb[26] = {'E','D', 'C','D', 'E','E','E','D','D','D','E','G','G','E','D','C','D','E','E','E','E','D','D','E','D','C'};

// Accelerometer variables
__IO int32_t accTemp = 0;
__IO float accX, accY, accZ = 0;
 __IO float roll, pitch = 0;

volatile uint32_t sampleCounter = 0;
volatile int16_t sample = 0;
volatile uint8_t DACBuffer[DACBUFFERSIZE];		// Buffer used for synthesis
volatile uint8_t tempBuffer[DACBUFFERSIZE];		// Buffer used for synthesis
volatile uint8_t noiseBuffer[DACBUFFERSIZE];	// Buffer to store random numbers to refill DACBuffer for the synthesis of each note
uint16_t DACBufferSize;


void peripheralsInit(void)
{
	// Initialize LEDs
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_LEDInit(LED5);
	STM_EVAL_LEDInit(LED6);

	// Initialize user button
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);

	RCC_Configuration();

	// Codec peripheral initialization
	RNG_Configuration();
	codec_init();
	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);

	// Accelerometer Peripheral Initialization
	GPIO_Configuration();
	Button_GPIO_Configuration();
	SPI_Configuration();


	// Delay 3ms so that Accelerometer can startup
	delay_ms(3);

	// Send setup byte to Control Register 1
	(void)writeAccelByte(CTRL_REG1, 0x47);
}

int main(void)
{
	// Initialize micro-controller to a default state
	SystemInit();

	peripheralsInit();

	// Calculation of buffer length corresponding to note that needs to be played (using default values here)
	// duration/(note frequency x 2^octave) if odd, add 1
	DACBufferSize = (uint16_t)(((float)44100/(noteFreq*pow(2, octave))));
	if(DACBufferSize & 0x00000001)
		DACBufferSize +=1;

	// Loop variables
	int16_t i;
	uint16_t n, m;

	// Random numbers variable
	uint32_t random = 0;

	// Fill buffer with random values
	for (n = 0; n<DACBUFFERSIZE; n++)
	{
		while(RNG_GetFlagStatus(RNG_FLAG_DRDY) == 0);
		random = RNG_GetRandomNumber();

		noiseBuffer[n] = (uint8_t)(((0xFF+1)/2)*(2*(((float)random)/0xFFFFFFFF)));
		DACBuffer[n] = noiseBuffer[n];
		RNG_ClearFlag(RNG_FLAG_DRDY);
	}


	while(1)
	{
		// Read play button input pin
		uint8_t playValue = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);

		if ( playValue == 1) // Change between play and pause
		{

			if((playing == 0)) //play mode
			{
				GPIO_SetBits(GPIOD,GPIO_Pin_5); 	// Light up play LED

				playing = 1; 	// Toggle play/pause status
				playTwinkle();	// Start playing melody
			}

			else if(playing == 1)
			{
				GPIO_ResetBits(GPIOD,GPIO_Pin_5);
				playing = 0;//return to play mode
			}
		}

	}
	return 0;
}

// Set frequency of single note based on symbol in the melody array i.e. either C,F,G etc
void setNoteFrequency(char notes[], uint8_t i)
{
	if(notes[i] == 'G')
	{
		noteFreq = 24.5;
	}
	else if(notes[i] == 'C')
	{
		noteFreq = 16.35;
	}
	else if(notes[i] == 'D')
	{
		noteFreq = 18.35;
	}
	else if(notes[i] == 'A')
	{
		noteFreq = 27.5;
	}
	else if(notes[i] == 'E')
	{
		noteFreq = 20.60;
	}
	else if(notes[i] == 'F')
	{
		noteFreq = 21.83;
	}
}

// Update pitch and speed of audio playing based on accelerometer orientation
 void updatePitchAndSpeed(uint16_t amount)
 {
	checkAcc();

	if(roll < 0)
	{
		if(roll < 0 && roll > -45)
		{
			duration = amount;
		}
		else if(roll <= -45 && roll > -90)
		{
			duration = amount/2;
		}
		else if(roll <= -90 && roll > -135)
		{
			duration = amount/3;
		}
		else if(roll <= -135 && roll >= -160)
		{
			duration = amount/4;
		}
	}
	else if(roll > 0)
	{
		if(roll > 0 && roll < 45)
		{
			duration = amount;
		}
		else if(roll >= 45 && roll < 90)
		{
			duration = amount*2;
		}
		else if(roll >= 90 && roll < 135)
		{
			duration = amount*3;
		}
		else if(roll >= 135 && roll <= 160)
		{
			duration = (amount*3)+80000;
		}
	}
	else if(roll == 0)
	{
		duration  = amount;
	}


	if(pitch > 0)
	{
		if(pitch > 0 && pitch < 18)
		{
			octave = 3;
		}
		else if(pitch >= 18 && pitch < 36)
		{
			octave = 3.5;
		}
		else if(pitch >= 36 && pitch < 54)
		{
			octave = 4;
		}
		else if(pitch >= 54 && pitch < 72)
		{
			octave = 4.5;
		}
		else if(pitch >= 72 && pitch < 90)
		{
			octave = 5;
		}
	}
	else if(pitch < 0)
	{
		if(pitch < 0 && pitch > -18)
		{
			octave = 3;
		}
		else if(pitch <= -18 && pitch > -36)
		{
			octave = 2.5;
		}
		else if(pitch <= -36 && pitch > -54)
		{
			octave = 2;
		}
		else if(pitch <= -54 && pitch > -72)
		{
			octave = 1.5;
		}
		else if(pitch <= -72)
		{
			octave = 1.2;
		}
	}
	else if(pitch == 0)
	{
		octave = 3;
	}
 }

 // Get accelerometer orientation data
void checkAcc(void)
{
	/* Read X Acceleration */
	temp = writeAccelByte(OUTX, 0x00);
	accX = (18/1000.0) * 9.81 * temp;

	/* Read Y Acceleration */
	temp = writeAccelByte(OUTY, 0x00);
	accY = (18/1000.0) * 9.81 * temp;

	/* Read Z Acceleration */
	temp = writeAccelByte(OUTZ, 0x00);
	accZ = (18/1000.0) * 9.81 * temp;


	// Calculate angles
	pitch = atan2(accY,accZ); // Changes the pitch of the synthesized audio
	pitch = pitch*((180/M_PI));
	roll = atan2(-accX,(sqrt((accY*accY) + (accZ*accZ)))); // Changes the speed of the synthesized audio
	roll = roll*((float)(180/M_PI));
}

// Play 'Mary Had a Little Lamb' melody
void playLittleLamb(void)
{
	// Loop variable
	int8_t g = 0;

	int size = sizeof (littleLamb) / sizeof (char);
	for(g = 0 ; g < size ; g++)
	{
		// Change of mode between synthesis and USB mode meant to be implemented at this point. See comments at bottom of this file

		// Check next button input pin
		uint8_t next = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);

		if(next == 1) // Skip to next melody
		{
			// Break out of loop
			g = size+1;
		}

		// Check play/pause button input
		uint8_t isPlaying = GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_0);

		if((isPlaying == 1) && (playing == 1)) // Pause
		{
			currentNote = g;					// Store current note
			GPIO_ResetBits(GPIOD,GPIO_Pin_5);	// Turn off play/pause LED
			playing = 0;
			break;
		}
		else if((isPlaying == 1) && (playing == 0)) // Play
		{
			g = currentNote; // Resume
		}

		// Set frequency of note
		setNoteFrequency(littleLamb, g);

		// Check enable button input
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		if( enabled == 1)
		{
			updatePitchAndSpeed(DURATION);

			if(g >= 4 && g <= 12)
			{

				updatePitchAndSpeed(DURATION_DELAY);
			}
			else if(g == size)
			{

				updatePitchAndSpeed(DURATION_END);
			}
			else
			{
				updatePitchAndSpeed(DURATION);
			}

		}

		// Generate note
		generateNote();
	}

	// Play next melody once complete
	playTwinkle();
}

void playTwinkle(void)
{
	// Loop variable
	int8_t g = 0;

	int size = sizeof (twinkle) / sizeof (char);
	for(g = 0 ; g < size ; g++)
	{
		// Change of mode between synthesis and USB mode meant to be implemented at this point. See comments at bottom of this file

		// Check next button input pin
		uint8_t next = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);

		if(next == 1) // Skip to next melody
		{
			// Break out of loop
			g = size+1;
		}

		// Check play/pause button input
		uint8_t isPlaying = GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_0);

		if((isPlaying == 1) && (playing == 1)) // Pause
		{
			currentNote = g;					// Store current note
			GPIO_ResetBits(GPIOD,GPIO_Pin_5);	// Turn off play/pause LED
			playing = 0;
			break;
		}
		else if((isPlaying == 1) && (playing == 0)) // Play
		{
			g = currentNote; // Resume
		}

		// Set frequency of note
		setNoteFrequency(twinkle, g);

		//enable button
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		// Check enable button input
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		if( enabled == 1)
		{
			updatePitchAndSpeed(DURATION);

			if(g >= 4 && g <= 12)
			{

				updatePitchAndSpeed(DURATION_DELAY);
			}
			else if(g == size)
			{

				updatePitchAndSpeed(DURATION_END);
			}
			else
			{
				updatePitchAndSpeed(DURATION);
			}

		}

		// Generate note
		generateNote();
	}

	// Play next melody once complete
	playLittleLamb();

}

// Synthesize note and play it
void generateNote(void)
{
	uint16_t n, m;

	// update buffer length according to note desired
	DACBufferSize = (uint16_t)(((float)44100/(noteFreq*pow(2, octave))));
	if(DACBufferSize & 0x00000001)
		DACBufferSize +=1;

	// Loop variables
	volatile uint16_t i;
	volatile uint16_t j = 0;
	for(i = 0; i < duration; i++)
	{
		// send silence to audio DAC while note is being synthesized (gets rid of static noise)
		if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
		{
			SPI_I2S_SendData(CODEC_I2S, 0);
		}

		// Karplus-Strong algorithm
		if(j != DACBufferSize-1)
		{
			tempBuffer[j] = (uint8_t)(((DACBuffer[j]+DACBuffer[j+1])/2.0)*0.999996);
		}
		else
		{
			tempBuffer[j] = (uint8_t)(((DACBuffer[j]+tempBuffer[0])/2.0)*0.999996);
		}
		outBuffer[i] = (uint8_t)tempBuffer[j];

		j++;

		// values synthesized are re-used to synthesize newer values (simulates a queue)
		if(j == DACBufferSize)
		{
			for(n=0; n < DACBufferSize; n++)
			{
				DACBuffer[n] = tempBuffer[n];

				// send silence to audio DAC while note is being synthesized (gets rid of static noise)
				if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
				{
					SPI_I2S_SendData(CODEC_I2S, 0);
				}
			}
			j = 0;
		}

	}

	/* 'n = 1000' determines where in the output buffer we should start sending values to audioDAC
	 * helps to get rid of some delay
	 */

	n = 1000;

	int8_t v;

	for(v = 0 ; v < 14400 ; v++)
	{

		if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
		{
			// Output sound
			SPI_I2S_SendData(CODEC_I2S, sample);

			// Only update on every second sample to ensure that L & R channels have the same sample value
			if (sampleCounter & 0x00000001)
			{
				if(n >= duration)
				{
					n = 0;
					if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
					{
						SPI_I2S_SendData(CODEC_I2S, 0);
					}

					break;	// exit loop when note reaches its duration
				}

				sample = (int16_t)(amplitude*outBuffer[n]);

				// change increment value to change frequency (should not be used)
				n += 1;
			}
			sampleCounter++;
		}

		if (sampleCounter == 96000)
		{
			sampleCounter = 0;
			//break;
		}
	}

	// fill buffer with random numbers
	for(m=0; m < DACBufferSize; m++)
	{
		DACBuffer[m] = noiseBuffer[m];

		// send silence to audio DAC while no note is being played (gets rid of static noise)
		if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
		{
			SPI_I2S_SendData(CODEC_I2S, 0);
		}
	}
}

// Configure system clocks
void RCC_Configuration(void)
{
	// enable clock for GPIOA, GPIOB, GPIOC, GPIOE and DMA2
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOE|RCC_AHB1Periph_DMA2, ENABLE);

	// enable clock for Random Number Generator
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);

	// enable SPI for accelerometer
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
}

// Configure accelerometer peripheral pin inputs and outputs
void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	/* Pack the struct */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	//Init CS-Pin
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIO_SetBits(GPIOE, GPIO_Pin_3);  //Set CS high when idle

	/* Set Af config */
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource5 , GPIO_AF_SPI1);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource6 , GPIO_AF_SPI1);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource7 , GPIO_AF_SPI1);

}

// Configure button pin inputs and outputs
void Button_GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// Next button input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// Play button input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// Mode button input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	// Enable button input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	// Play button LED pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// Mode button LED pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

// Configure SPI peripheral
void SPI_Configuration(void)
{
	SPI_InitTypeDef  SPI_InitStructure;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1, ENABLE);
}

int32_t writeAccelByte(uint8_t regAdr, uint8_t data)
{
	uint8_t dummyVar;
	int32_t val = 0;

	//Pull CS low
	GPIO_ResetBits(GPIOE, GPIO_Pin_3);

	//Wait for flags
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));
	//Send register address
	SPI_I2S_SendData(SPI1, regAdr);

	//Wait for flags
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE));
	//Read dummy data
	dummyVar = SPI_I2S_ReceiveData(SPI1);

	//Wait for flags
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));
	// Send slave data byte
	SPI_I2S_SendData(SPI1, data);

	//Wait for flags
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE));

	val = (int8_t)SPI_I2S_ReceiveData(SPI1);

	//Pull CS high again
	GPIO_SetBits(GPIOE, GPIO_Pin_3);

	return val;
}

// Random Number Generator setup
void RNG_Configuration(void)
{
	RNG_Cmd(ENABLE);
}


void delay_ms(uint32_t milli)
{
	uint32_t delay = milli * 17612;              // approximate loops per ms at 168 MHz, Debug config
	for(; delay != 0; delay--);
}


/*
 * Callback used by stm32f4_discovery_audio_codec.c.
 * Refer to stm32f4_discovery_audio_codec.h for more info.
 */
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size){
  /* TODO, implement your code here */
  return;
}

/*
 * Callback used by stm324xg_eval_audio_codec.c.
 * Refer to stm324xg_eval_audio_codec.h for more info.
 */
uint16_t EVAL_AUDIO_GetSampleCallBack(void){
  /* TODO, implement your code here */
  return -1;
}


/*
  Code for mode change to be implemented at specified locations in the melody functions
  ----------------------------------------------------------------------------------------

 // Mode changed to USB

	if( (mode == 1) && (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_1) == 1))
	{
		// Change mode between USB and algorithm

		/*uint8_t modeValue = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_1);

		if(modeValue == 1)
		{
			if((mode == 5) || (mode == 0))
			{
				GPIO_SetBits(GPIOD,GPIO_Pin_15);
				mode=1;
				//code for karplus synthesis
			}

			else if(mode == 1)
			{
				GPIO_ResetBits(GPIOD,GPIO_Pin_15);
				mode=0;
				break;
				// swicth to USb mode
			}


			// Debouncing

			uint32_t smalldelay = 10000;
			for(;smalldelay > 0; smalldelay--);
		}

----------------------------------------------------------------------------------------------------------------------
*/

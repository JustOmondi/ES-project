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
#define DECAY_FACTOR 0.99999996
#define OUTBUFFERSIZE 88200
#define DACBUFFERSIZE 5000

//Function prototypes
void RCC_Configuration(void);
void RNG_Configuration(void);
void GPIO_Configuration(void);
void SPI_Configuration(void);
void delay_ms(uint32_t milli);
int32_t writeAccelByte(uint8_t regAdr, uint8_t data);
void checkAcc(void);

int play = 0;


__IO uint8_t outBuffer[OUTBUFFERSIZE];	// stores synthesized note waveform

// Note variables
__IO float octave = 2;				// default octave
__IO float noteFreq = 20.6;			// default note frequency
__IO float amplitude = 5.0;			// volume
__IO uint32_t duration = 44100;		//  duration of note

__IO char melody[42] = {'C','C','G','G','A','A','G','F','F','E','E','D','D','C','G','G','F','F','E','E','D','G','G','F','F','E','E','D','C','C','G','G','A','A','G','F','F','E','E','D','D','C'};

// Accelerometer variables
__IO int32_t temp = 0;
__IO float accX, accY, accZ = 0;
 float roll, pitch = 0;



void checkAcc(void)
{
	// Read X Acceleration
	temp = writeAccelByte(OUTX, 0x00);
	accX = (18/1000.0) * 9.81 * temp;

	// Read Y Acceleration
	temp = writeAccelByte(OUTY, 0x00);
	accY = (18/1000.0) * 9.81 * temp;

	// Read Z Acceleration
	temp = writeAccelByte(OUTZ, 0x00);
	accZ = (18/1000.0) * 9.81 * temp;

	// Used to control pitch
	pitch = atan2(accY,accZ);
	pitch = pitch*((180/M_PI));

	// Used to control speed
	roll = atan2(-accX,(sqrt((accY*accY) + (accZ*accZ))));
	roll = roll*((float)(180/M_PI));

 float r = pitch;
 float y = roll;


	if(y > 0)
	{
		int f = 4;
	}
	else if(r < 0)
	{
		int p = 9;
	}

	/* Do the LED pitch thing */
//	if(pitch < 0)
//	{
//		STM_EVAL_LEDOn(LED6);
//		STM_EVAL_LEDOff(LED3);
//	}
//	else
//	{
//		STM_EVAL_LEDOff(LED6);
//		STM_EVAL_LEDOn(LED3);
//	}
//	if(roll < 0)
//	{
//		STM_EVAL_LEDOn(LED4);
//		STM_EVAL_LEDOff(LED5);
//	}
//	else
//	{
//		STM_EVAL_LEDOff(LED4);
//		STM_EVAL_LEDOn(LED5);
//	}
}

int main(void)
{
	SystemInit();

	/* Initialize LEDs */
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_LEDInit(LED5);
	STM_EVAL_LEDInit(LED6);

	//Initialize user button
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);


	RCC_Configuration();

	RNG_Configuration();

	codec_init();
	codec_ctrl_init();

	I2S_Cmd(CODEC_I2S, ENABLE);

	// Delay 3ms so that Accelerometer can startup
	delay_ms(3);

	// Accelerometer Peripheral Initialization
   GPIO_Configuration();
   SPI_Configuration();

   // Send setup byte to Control Register 1
	(void)writeAccelByte(CTRL_REG1, 0x47);


	//uint16_t dacBuffer[SIZE] = {678, 807, 539, 215, 967, 308, 984, 589, 957, 952, 816, 581, 185, 501, 511, 492, 679, 140, 716, 174, 558, 30, 494, 642, 54, 913, 712, 14, 314, 917, 1, 381, 753, 79, 302, 554, 557, 272, 27, 416, 733, 380, 613, 717, 520, 57, 629, 80, 165, 521, 126, 100, 764, 687, 262, 388, 173, 896, 980, 261, 33, 826, 314, 529, 743, 126, 633, 173, 892, 67, 904, 807, 244, 689, 521, 105, 537, 397, 487, 42, 698, 803, 240, 172, 887, 589, 286, 555, 261, 419, 558, 332, 820, 896, 860, 512, 184, 300, 404, 247, 293, 438, 500, 410, 315, 182, 885, 374, 344, 996, 0, 810, 166, 258, 595, 603, 951, 618, 551, 414, 767, 461, 844, 766, 710, 243, 713, 449, 929, 47, 667, 372, 459, 557, 874, 548, 337, 78, 718, 350, 659, 535, 169, 67, 575, 451, 529, 550, 773, 856, 980, 8, 291, 372, 500, 191, 732, 588, 623, 480, 864, 686, 102, 870, 723, 191, 257, 161, 202, 363, 990, 489, 639, 148, 497, 768, 460, 371, 983, 83, 509, 870, 143, 272, 339, 85, 780, 916, 717, 652, 89, 955, 109, 895, 718, 429, 535, 294, 239, 399, 46, 918, 661, 299, 564, 969, 522, 464, 848, 3, 772, 0, 444, 350, 763, 358, 803, 620, 787, 544, 363, 454, 549, 406, 451, 856, 274, 705, 31, 346, 851, 132, 584, 932, 532, 260, 542, 310, 383, 447, 765, 635, 537, 937, 399, 903, 746, 727, 366, 467, 940, 542, 255, 572, 878, 97, 538, 843, 817, 284, 92, 89, 511, 91, 655, 425, 415, 876, 768, 753, 171, 407, 765, 708, 677, 528, 662, 701, 938, 65, 810, 970, 560, 305, 666, 640, 295, 740, 706, 659, 652, 321, 387, 555, 70, 722, 923, 780, 726, 249, 619, 475, 286, 926, 783, 449, 394, 678, 267, 699, 644, 260, 137, 622, 538, 469, 408, 559, 963, 399, 933, 312, 27, 996, 728, 0, 983, 878, 898, 486, 579, 28, 620, 517, 148, 232, 669, 710, 272, 561, 740, 269, 658, 524, 64, 111, 616, 113, 300, 720, 131, 313, 732, 179, 629, 682, 443, 525, 11, 44, 942, 29, 317, 423, 135, 974, 129, 714, 386, 657, 521, 893, 676, 876, 330, 768, 972, 832, 541, 413, 857, 865, 343, 413, 400, 808, 200, 118, 147, 528, 719, 576, 283, 588, 619, 845, 76, 734, 904, 751, 164, 236, 461, 35, 75, 325, 411, 422, 429, 76, 432, 200, 698, 27, 285, 397, 416, 450, 558, 969, 720, 520, 677, 437, 340, 228, 41, 542, 422, 925, 140, 93, 432, 366, 628, 48, 724, 657, 190, 990, 259, 916, 797, 739, 294, 402, 576, 590, 860, 744, 859, 242, 119, 285, 37, 907, 875, 647, 605, 735, 192, 750, 911, 24, 188, 956, 981, 100, 675, 395, 123, 336, 329, 846, 385, 594, 589, 272, 218, 26, 996, 931, 741, 542, 101, 438, 975, 846, 185, 147, 660, 869, 654, 573, 730, 507, 292, 540, 928, 446};
	//float dacBuffer[SIZE];

	//-------------------------------------------------------------

	volatile uint32_t sampleCounter = 0;
	volatile int16_t sample = 0;
	volatile uint8_t DACBuffer[DACBUFFERSIZE];		// buffer used for synthesis algorithm; length determines note frequency and octave
	volatile uint8_t tempBuffer[DACBUFFERSIZE];
	volatile uint8_t noiseBuffer[DACBUFFERSIZE];

	// Calculation of buffer length corresponding to note that needs to be played (using default values here)
	// Size = duration/(note frequency x 2^octave) if odd, add 1
	uint16_t DACBufferSize = (uint16_t)(((float)44100/(noteFreq*pow(2, octave))));
	if(DACBufferSize & 0x00000001)
		DACBufferSize +=1;


	// Loop variables
	int16_t i;
	uint16_t n, m;

	// Random number variable
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

	volatile int8_t g = 0;
	while(1)
	{

		int mSize = sizeof (melody) / sizeof (char);
		for(g = 0 ; g < mSize ; g++)
		{
			if(melody[g] == 'G')
			{
				noteFreq = 24.5;
			}
			else if(melody[g] == 'C')
			{
				noteFreq = 16.35;
			}
			else if(melody[g] == 'D')
			{
				noteFreq = 18.35;
			}
			else if(melody[g] == 'A')
			{
				noteFreq = 27.5;
			}
			else if(melody[g] == 'E')
			{
				noteFreq = 20.60;
			}
			else if(melody[g] == 'F')
			{
				noteFreq = 21.83;
			}

			checkAcc();

			if(roll < 0)
			{
				if(roll < 0 && roll > -45)
				{
					duration = 44100;
				}
				else if(roll <= -45 && roll > -90)
				{
					duration = 22050;
				}
				else if(roll <= -90 && roll > -135)
				{
					duration = 11025;
				}
				else if(roll <= -135 && roll >= -160)
				{
					duration = 5512;
				}
			}
			else if(roll > 0)
			{
				if(roll > 0 && roll < 45)
				{
					duration = 44100;
				}
				else if(roll >= 45 && roll < 90)
				{
					duration = 47000;
				}
				else if(roll >= 90 && roll < 135)
				{
					duration = 53150;
				}
				else if(roll >= 135 && roll <= 160)
				{
					duration = 55175;
				}
			}
			else if(roll == 0)
			{
				duration  = 44100;
			}



			if(pitch > 0)
			{
				octave = 2.5;
			}
			else
			{
				octave = 2;
			}

			if(pitch > 0)
			{
				if(pitch > 0 && pitch < 22.5)
				{
					octave = 4;
				}
				else if(pitch >= 22.5 && pitch < 45)
				{
					octave = 3;
				}
				else if(pitch >= 45 && pitch < 67.5)
				{
					octave = 2;
				}
			}
			else if(pitch < 0)
			{
				if(pitch < 0 && pitch > -22.5)
				{
					octave = 4;
				}
				else if(pitch <= -22.5 && pitch > -45)
				{
					octave = 5;
				}
				else if(pitch <= 45 && pitch > -67.5)
				{
					octave = 6;
				}
			}
			else if(pitch == 0)
			{
				octave = 4;
			}


		// Update buffer length according to note desired
		DACBufferSize = (uint16_t)(((float)44100/(noteFreq*pow(2, octave))));
		if(DACBufferSize & 0x00000001)
			DACBufferSize +=1;

		// Synthesize note
		volatile uint16_t i;
		volatile uint16_t j = 0;
		for(i = 0; i < duration+10000; i++)
		{
			// Send silence to audio DAC while note is being synthesized (gets rid of static noise)
			if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
			{
				SPI_I2S_SendData(CODEC_I2S, 0);
			}

			// Karplus-Strong algorithm
			if(j != DACBufferSize-1)
			{
				tempBuffer[j] = (uint8_t)(((DACBuffer[j]+DACBuffer[j+1])/2.0)*DECAY_FACTOR);
			}
			else
			{
				tempBuffer[j] = (uint8_t)(((DACBuffer[j]+tempBuffer[0])/2.0)*DECAY_FACTOR);
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

		// determines where in the output buffer we should start sending values to audioDAC
		// helps get rid of some delay
		n = 1000;

		// Loop variable
		volatile int8_t v;


		for(v = 0 ; v < 14400 ; v++)
		{

			if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
			{
				// output sound
				SPI_I2S_SendData(CODEC_I2S, sample);

				//only update on every second sample to ensure that L & R channels have the same sample value
				if (sampleCounter & 0x00000001)
				{
					if(n >= duration)
					{
						n = 0;
						if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
						{
							SPI_I2S_SendData(CODEC_I2S, 0);
						}

						break;	// exit loop when note ends
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

		// fill buffer with random values again
		for(m = 0; m < DACBufferSize; m++)
		{
			DACBuffer[m] = noiseBuffer[m];

			// send silence to audio DAC while no note is being played (gets rid of static noise)
			if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
			{
				SPI_I2S_SendData(CODEC_I2S, 0);
			}
		}

	}
	}

	return 0;

}

void RCC_Configuration(void)
{
	// enable clock for GPIOA, GPIOB, GPIOC, GPIOE and DMA2
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOE|RCC_AHB1Periph_DMA2, ENABLE);

	// enable clock for Random Number Generator
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);

	// enable clock for timer 2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	// enable clock for SYSCFG for EXTI
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG|RCC_APB2Periph_ADC1, ENABLE);

	// enable SPI for accelerometer
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	// Pack the struct
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Init CS-Pin
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIO_SetBits(GPIOE, GPIO_Pin_3);  //Set CS high when idle

	// Set Af config
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource5 , GPIO_AF_SPI1);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource6 , GPIO_AF_SPI1);
	GPIO_PinAFConfig (GPIOA, GPIO_PinSource7 , GPIO_AF_SPI1);

}

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
	uint32_t delay = milli * 17612;        // approximate loops per ms at 168 MHz, Debug config
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



#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "codec.h"
#include "stm32f4xx_it.h"
#include <math.h>

#define DELAY 60
#define CTRL_REG1 0x20
#define OUTX 0xA9
#define OUTY 0xAB
#define OUTZ 0xAD


#define GREENLED LED4
#define ORANGELED LED3
#define BLUELED LED6
#define REDLED LED5

#define SAMPLE_RATE 44100
#define DECAY_FACTOR 0.996
#define DURATION 24100
#define DURATION_DELAY 55100
#define DURATION_END 55100
/* Private Macros */
#define OUTBUFFERSIZE 44100
#define DACBUFFERSIZE 1200

//Function prototypes
void RCC_Configuration(void);
void RNG_Configuration(void);
void GPIO_Configuration(void);
void Button_GPIO_Configuration(void);
void SPI_Configuration(void);
void delay_ms(uint32_t milli);
int32_t writeAccelByte(uint8_t regAdr, uint8_t data);
void checkAcc(void);
void updatePitchAndSpeed(uint16_t amount);
void setNoteFrequency(char notes[], uint8_t i);
void generateNote(void);
void peripheralInit(void);

//Melody function prototypes
void playTwinkle(void);
void playLittleLamb(void);
void playJingleBells(void);
void playHappyBirthday(void);
void playNokiaTune(void);


//void EXTI1_IRQHandler(void);


// Status variables
uint8_t playing = 4; // 0 - Paused, 1 - playing
uint8_t enabled = 0;
__IO uint8_t mode = 5; // 0 - Algorithm mode, 1 - USb mode
uint8_t currentSong = 1;
uint8_t currentNote;




__IO uint8_t outBuffer[OUTBUFFERSIZE];	// stores synthesized note waveform

__IO float octave = 3;
__IO float noteFreq = 20.6;			// Default note frequency
__IO float amplitude = 5.0;			// Volume
__IO float volume = 0.5;
__IO uint32_t duration = 44100;		// Duration of note

// Melody arrays
__IO char twinkle[42] = {'C','C','G','G','A','A','G','F','F','E','E','D','D','C','G','G','F','F','E','E','D','G','G','F','F','E','E','D','C','C','G','G','A','A','G','F','F','E','E','D','D','C'};
__IO char littleLamb[26] = {'E','D', 'C','D', 'E','E','E','D','D','D','E','G','G','E','D','C','D','E','E','E','E','D','D','E','D','C'};
__IO char jingleBells[51] = {'E','E','E','E','E','E','E','G','C','D','E','F','F','F','F','F','E','E','E','E','E','D','D','E','D','G','E','E','E','E','E','E' ,'E','G','C','D','E','F','F','F','F','F','E','E','E','E','G','G','F','D','C'};
__IO char happyBirthday[25] = {'G','G','A','G','C','B','G','G','A','G','D','C','G','G','G','E','C','B','A','F','F','E','C','D','C'};
__IO char nokiaTune[13] = {'E','D','F','G','C','B','D','E','B','A','C','E','A'};


__IO int32_t temp = 0;
__IO int32_t temp1 = 0;
__IO int32_t temp2 = 0;
__IO float accX, accY, accZ = 0;
 float roll, pitch = 0;

volatile uint32_t sampleCounter = 0;
volatile int16_t sample = 0;
volatile uint8_t DACBuffer[DACBUFFERSIZE];		// buffer used for synthesis algorithm; length determines note frequency and octave
volatile uint8_t tempBuffer[DACBUFFERSIZE];
volatile uint8_t noiseBuffer[DACBUFFERSIZE];
uint16_t DACBufferSize;


void peripheralInit(void)
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
	SystemInit();

	peripheralInit();

	//uint16_t dacBuffer[SIZE] = {678, 807, 539, 215, 967, 308, 984, 589, 957, 952, 816, 581, 185, 501, 511, 492, 679, 140, 716, 174, 558, 30, 494, 642, 54, 913, 712, 14, 314, 917, 1, 381, 753, 79, 302, 554, 557, 272, 27, 416, 733, 380, 613, 717, 520, 57, 629, 80, 165, 521, 126, 100, 764, 687, 262, 388, 173, 896, 980, 261, 33, 826, 314, 529, 743, 126, 633, 173, 892, 67, 904, 807, 244, 689, 521, 105, 537, 397, 487, 42, 698, 803, 240, 172, 887, 589, 286, 555, 261, 419, 558, 332, 820, 896, 860, 512, 184, 300, 404, 247, 293, 438, 500, 410, 315, 182, 885, 374, 344, 996, 0, 810, 166, 258, 595, 603, 951, 618, 551, 414, 767, 461, 844, 766, 710, 243, 713, 449, 929, 47, 667, 372, 459, 557, 874, 548, 337, 78, 718, 350, 659, 535, 169, 67, 575, 451, 529, 550, 773, 856, 980, 8, 291, 372, 500, 191, 732, 588, 623, 480, 864, 686, 102, 870, 723, 191, 257, 161, 202, 363, 990, 489, 639, 148, 497, 768, 460, 371, 983, 83, 509, 870, 143, 272, 339, 85, 780, 916, 717, 652, 89, 955, 109, 895, 718, 429, 535, 294, 239, 399, 46, 918, 661, 299, 564, 969, 522, 464, 848, 3, 772, 0, 444, 350, 763, 358, 803, 620, 787, 544, 363, 454, 549, 406, 451, 856, 274, 705, 31, 346, 851, 132, 584, 932, 532, 260, 542, 310, 383, 447, 765, 635, 537, 937, 399, 903, 746, 727, 366, 467, 940, 542, 255, 572, 878, 97, 538, 843, 817, 284, 92, 89, 511, 91, 655, 425, 415, 876, 768, 753, 171, 407, 765, 708, 677, 528, 662, 701, 938, 65, 810, 970, 560, 305, 666, 640, 295, 740, 706, 659, 652, 321, 387, 555, 70, 722, 923, 780, 726, 249, 619, 475, 286, 926, 783, 449, 394, 678, 267, 699, 644, 260, 137, 622, 538, 469, 408, 559, 963, 399, 933, 312, 27, 996, 728, 0, 983, 878, 898, 486, 579, 28, 620, 517, 148, 232, 669, 710, 272, 561, 740, 269, 658, 524, 64, 111, 616, 113, 300, 720, 131, 313, 732, 179, 629, 682, 443, 525, 11, 44, 942, 29, 317, 423, 135, 974, 129, 714, 386, 657, 521, 893, 676, 876, 330, 768, 972, 832, 541, 413, 857, 865, 343, 413, 400, 808, 200, 118, 147, 528, 719, 576, 283, 588, 619, 845, 76, 734, 904, 751, 164, 236, 461, 35, 75, 325, 411, 422, 429, 76, 432, 200, 698, 27, 285, 397, 416, 450, 558, 969, 720, 520, 677, 437, 340, 228, 41, 542, 422, 925, 140, 93, 432, 366, 628, 48, 724, 657, 190, 990, 259, 916, 797, 739, 294, 402, 576, 590, 860, 744, 859, 242, 119, 285, 37, 907, 875, 647, 605, 735, 192, 750, 911, 24, 188, 956, 981, 100, 675, 395, 123, 336, 329, 846, 385, 594, 589, 272, 218, 26, 996, 931, 741, 542, 101, 438, 975, 846, 185, 147, 660, 869, 654, 573, 730, 507, 292, 540, 928, 446};
	//float dacBuffer[SIZE];

	//-------------------------------------------------------------


	// Calculation of buffer length corresponding to note that needs to be played (using default values here)
	// duration/(note frequency x 2^octave) if odd, add 1
	DACBufferSize = (uint16_t)(((float)44100/(noteFreq*pow(2, octave))));
	if(DACBufferSize & 0x00000001)
		DACBufferSize +=1;


	int16_t i;

	uint16_t n, m;
	uint32_t random = 0;
	uint8_t b = 0;

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
		// Change between play and pause
		uint8_t playValue = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);
		if ( playValue == 1)
		{

			if((playing == 0)|| (playing == 4)) //play mode
			{

				GPIO_SetBits(GPIOD,GPIO_Pin_5);
				playing = 1; // next time button is pressed set in pause mode
				playTwinkle();

				//code to stop audio
			}
			else if(playing == 1)
			{
				GPIO_ResetBits(GPIOD,GPIO_Pin_5);
				playing = 0;//return to play mode
			}
			//debounce
//			uint32_t smalldelay = 100000;
//			for(;smalldelay > 0; smalldelay--);

		}

	}

	return 0;

}

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

void playLittleLamb(void)
{
	// Loop variable
	volatile int8_t g = 0;
	int size = sizeof (littleLamb) / sizeof (char);
	for(g = 0 ; g < size ; g++)
	{
		// Change of mode between synthesis and USB mode meant to be implemented at this point. See comments at bottom of this file


		//using next button
	/*----------------------------------------------------------------------------------------------------------------------*/
		uint8_t next = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
		if(next == 1)
		{
			STM_EVAL_LEDOn(ORANGELED);
			break;
		}
		else
		{
			STM_EVAL_LEDOff(ORANGELED);
		}

/*----------------------------------------------------------------------------------------------------------------------*/



		//play and pause
/*----------------------------------------------------------------------------------------------------------------------*/
		if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 1)) // paused
		{
			currentNote = g;
			GPIO_ResetBits(GPIOD,GPIO_Pin_5);
			playing = 0;
			break;
		}
		/*if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 0)) // play from pause
		{
			g= currentNote;

		}
*/



		// Set frequency of note
		setNoteFrequency(littleLamb, g);


		//enable button
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		//ENABLE ACCELEROMETER IF ENABLE BUTTON IS PRESSED
		if( enabled == 1)
		{

			STM_EVAL_LEDOn(GREENLED);
			updatePitchAndSpeed(DURATION);


			if(g >= 4 && g <= 12)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_DELAY);
			}
			else if(g == size)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_END);
			}
			else
			{
				updatePitchAndSpeed(DURATION);
			}

		}

		else
		{
			STM_EVAL_LEDOff(GREENLED);
			//code to disable accelerometer
		}

		// Generate note
		generateNote();

	}

}

void playTwinkle(void)
{
	// Loop variable
		volatile int8_t g = 0;
	int size = sizeof (twinkle) / sizeof (char);
	for(g = 0 ; g < size ; g++)
	{
		// Change of mode between synthesis and USB mode meant to be implemented at this point. See comments at bottom of this file


		//using next button
	/*----------------------------------------------------------------------------------------------------------------------*/
		uint8_t next = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
		if(next == 1)
		{
			STM_EVAL_LEDOn(ORANGELED);
			break;
		}
		else
		{
			STM_EVAL_LEDOff(ORANGELED);
		}

/*----------------------------------------------------------------------------------------------------------------------*/



		//play and pause
/*----------------------------------------------------------------------------------------------------------------------*/
		if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 1)) // paused
		{
			currentNote = g;
			GPIO_ResetBits(GPIOD,GPIO_Pin_5);
			playing = 0;
			break;
		}
		/*if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 0)) // play from pause
		{
			g= currentNote;

		}
*/



		// Set frequency of note
		setNoteFrequency(twinkle, g);


		//enable button
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		//ENABLE ACCELEROMETER IF ENABLE BUTTON IS PRESSED
		if( enabled == 1)
		{

			STM_EVAL_LEDOn(GREENLED);
			updatePitchAndSpeed(DURATION);


			if(g == 6 || g == 13 || g == 20 || g == 27 || g == 34)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_DELAY);
			}
			else if(g == 41)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_END);
			}
			else
			{
				updatePitchAndSpeed(DURATION);
			}

		}

		else
		{
			STM_EVAL_LEDOff(GREENLED);
			//code to disable accelerometer
		}

		// Generate note
		generateNote();

	}

	playLittleLamb();

}

void playNokiaTune(void)
{
	// Loop variable
		volatile int8_t g = 0;
	int size = sizeof (nokiaTune) / sizeof (char);
	for(g = 0 ; g < size ; g++)
	{
		// Change of mode between synthesis and USB mode meant to be implemented at this point. See comments at bottom of this file


		//using next button
	/*----------------------------------------------------------------------------------------------------------------------*/
		uint8_t next = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
		if(next == 1)
		{
			STM_EVAL_LEDOn(ORANGELED);
			break;
		}
		else
		{
			STM_EVAL_LEDOff(ORANGELED);
		}

/*----------------------------------------------------------------------------------------------------------------------*/



		//play and pause
/*----------------------------------------------------------------------------------------------------------------------*/
		if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 1)) // paused
		{
			currentNote = g;
			GPIO_ResetBits(GPIOD,GPIO_Pin_5);
			playing = 0;
			break;
		}
		/*if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 0)) // play from pause
		{
			g= currentNote;

		}
*/



		// Set frequency of note
		setNoteFrequency(nokiaTune, g);


		//enable button
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		//ENABLE ACCELEROMETER IF ENABLE BUTTON IS PRESSED
		if( enabled == 1)
		{

			STM_EVAL_LEDOn(GREENLED);
			updatePitchAndSpeed(DURATION);


			if(g == 6 || g == 13 || g == 20 || g == 27 || g == 34)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_DELAY);
			}
			else if(g == 41)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_END);
			}
			else
			{
				updatePitchAndSpeed(DURATION);
			}

		}

		else
		{
			STM_EVAL_LEDOff(GREENLED);
			//code to disable accelerometer
		}

		// Generate note
		generateNote();

	}
}

void playJingleBells(void)
{
	// Loop variable
	volatile int8_t g = 0;

	int size = sizeof (jingleBells) / sizeof (char);
	for(g = 0 ; g < size ; g++)
	{
		// Change of mode between synthesis and USB mode meant to be implemented at this point. See comments at bottom of this file


		//using next button
	/*----------------------------------------------------------------------------------------------------------------------*/
		uint8_t next = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
		if(next == 1)
		{
			STM_EVAL_LEDOn(ORANGELED);
			break;
		}
		else
		{
			STM_EVAL_LEDOff(ORANGELED);
		}

/*----------------------------------------------------------------------------------------------------------------------*/



		//play and pause
/*----------------------------------------------------------------------------------------------------------------------*/
		if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 1)) // paused
		{
			currentNote = g;
			GPIO_ResetBits(GPIOD,GPIO_Pin_5);
			playing = 0;
			break;
		}
		/*if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 0)) // play from pause
		{
			g= currentNote;

		}
*/


		// Set frequency of note
		setNoteFrequency(jingleBells, g);


		//enable button
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		//ENABLE ACCELEROMETER IF ENABLE BUTTON IS PRESSED
		if( enabled == 1)
		{

			STM_EVAL_LEDOn(GREENLED);
			updatePitchAndSpeed(DURATION);


			if(g == 2 || g == 5 || g == 10 || g == 13 || g == 25 || g == 26 || g == 29 || g == 32 || g == 36 || g == 39)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_DELAY);
			}
			else if(g == size-1)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_END);
			}
			else
			{
				updatePitchAndSpeed(DURATION);
			}

		}

		else
		{
			STM_EVAL_LEDOff(GREENLED);
			//code to disable accelerometer
		}

		// Generate note
		generateNote();

	}
}

void playHappyBirthday(void)
{
	// Loop variable
		volatile int8_t g = 0;
	int size = sizeof (happyBirthday) / sizeof (char);
	for(g = 0 ; g < size ; g++)
	{
		// Change of mode between synthesis and USB mode meant to be implemented at this point. See comments at bottom of this file

		//using next button
	/*----------------------------------------------------------------------------------------------------------------------*/
		uint8_t next = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
		if(next == 1)
		{
			STM_EVAL_LEDOn(ORANGELED);
			break;
		}
		else
		{
			STM_EVAL_LEDOff(ORANGELED);
		}

/*----------------------------------------------------------------------------------------------------------------------*/



		//play and pause
/*----------------------------------------------------------------------------------------------------------------------*/
		if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 1)) // paused
		{
			currentNote = g;
			GPIO_ResetBits(GPIOD,GPIO_Pin_5);
			playing = 0;
			break;
		}
		/*if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)==1) && (playing == 0)) // play from pause
		{
			g= currentNote;

		}
*/

		// Set frequency of note
		setNoteFrequency(happyBirthday, g);

		//enable button
		uint8_t enabled = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5);

		//ENABLE ACCELEROMETER IF ENABLE BUTTON IS PRESSED
		if( enabled == 1)
		{

			STM_EVAL_LEDOn(GREENLED);
			updatePitchAndSpeed(DURATION);


			if((g >= 2 && g <= 5) || (g >= 8 && g <= 11) || g == 18 || (g >= size-4 && g <= size-1))
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_DELAY);
			}
			else if(g == size-1)
			{
				// Update pitch & speed depending on accelerometer orientation
				updatePitchAndSpeed(DURATION_END);
			}
			else
			{
				updatePitchAndSpeed(DURATION);
			}

		}

		else
		{
			STM_EVAL_LEDOff(GREENLED);
			//code to disable accelerometer
		}

		// Generate note
		generateNote();

	}
}

void generateNote(void)
{
	uint16_t n, m;

	// update buffer length according to note desired
	DACBufferSize = (uint16_t)(((float)44100/(noteFreq*pow(2, octave))));
	if(DACBufferSize & 0x00000001)
		DACBufferSize +=1;

	// Synthesize note
	volatile uint16_t i;
	volatile uint16_t j = 0;
	for(i = 0; i < duration+10000; i++)
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

	// determines where in the output buffer we should start sending values to audioDAC
	// helps get rid of some delay
	n = 1000;
	volatile int8_t v;
	// output sound
	for(v = 0 ; v < 14400 ; v++)
	{
		// output sound
		if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
		{
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

	// fill buffer with white noise again
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

void RCC_Configuration(void)
{
	// enable clock for GPIOA, GPIOB, GPIOC, GPIOE and DMA2
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOE|RCC_AHB1Periph_DMA2, ENABLE);

	// enable clock for Random Number Generator
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);

	// enable SPI for accelerometer
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
}

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

void Button_GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Pins connected to our button input */

	//nextButton input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//PlayButton input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//modeButton input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//enableButton input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_Init(GPIOE, &GPIO_InitStructure);


	//Pins to send output to external circuit

	//Output for play button LED - flash @1hz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//output for mode button LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(GPIOD, &GPIO_InitStructure);
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

	//Complete
	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1, ENABLE);
}

//void PE1_Configuration(void)
//{
//    /* Set variables used */
//    GPIO_InitTypeDef GPIO_InitStruct;
//    EXTI_InitTypeDef EXTI_InitStruct;
//    NVIC_InitTypeDef NVIC_InitStruct;
//
//    /* Set pin as input */
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
//    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
//    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
//    GPIO_Init(GPIOE, &GPIO_InitStruct);
//
//    /* Tell system that you will use PE4 for EXTI_Line4 */
//    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource1);
//
//    /* PE4 is connected to EXTI_Line4 */
//    EXTI_InitStruct.EXTI_Line = EXTI_Line1;
//    /* Enable interrupt */
//    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
//    /* Interrupt mode */
//    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
//    /* Triggers on rising and falling edge */
//    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
//    /* Add to EXTI */
//    EXTI_Init(&EXTI_InitStruct);
//
//    /* Add IRQ vector to NVIC */
//    /* PE4 is connected to EXTI_Line4, which has EXTI4_IRQn vector */
//    NVIC_InitStruct.NVIC_IRQChannel = EXTI1_IRQn;
//    /* Set priority */
//    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00;
//    /* Set sub priority */
//    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
//    /* Enable interrupt */
//    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
//    /* Add to NVIC */
//    NVIC_Init(&NVIC_InitStruct);
//}
//
//void EXTI1_IRQHandler(void)
//{
//    /* Make sure that interrupt flag is set */
//    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
//    {
//    	if((mode == 5) || (mode == 0))
//		{
//			GPIO_SetBits(GPIOD,GPIO_Pin_15);
//			mode=1;
//			//code for karplus synthesis
//		}
//
//
//
//		else if(mode == 1)
//		{
//			GPIO_ResetBits(GPIOD,GPIO_Pin_15);
//			mode=0;
//			// swicth to USb mode
//		}
//    }
//}
int32_t writeAccelByte(uint8_t regAdr, uint8_t data)
{
	uint8_t dummyVar;
	int32_t val = 0;

	/* Put your code here */

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

/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
  Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
  Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/

//add comment to test git

#include <Bela.h>
#include <cmath>
#include <Scope.h>

Scope scope;

int gAudioFramesPerAnalogFrame = 0;
float gInverseSampleRate;
float gPhase;

float gAmplitude;
float gFrequency;

float gIn1;
float gIn2;

// For this example you need to set the Analog Sample Rate to 
// 44.1 KHz which you can do in the settings tab.
float average;
int sampleCount;


void dumpAverage()
{
	float avg=average/sampleCount;
	rt_printf("average: %f\n",avg);
	average=0.0f;
	sampleCount=0;
}


void storeSample(float sample)
{
	average+=sample;
	sampleCount++;
	
	if(sampleCount==10000)
	   dumpAverage();
}


bool setup(BelaContext *context, void *userData)
{
    average=0.0f;
    sampleCount=0;
    
	// setup the scope with 3 channels at the audio sample rate
	scope.setup(3, context->audioSampleRate);
	
	if(context->analogSampleRate > context->audioSampleRate)
	{
		fprintf(stderr, "Error: for this project the sampling rate of the analog inputs has to be <= the audio sample rate\n");
		return false;
	}
	if(context->analogInChannels < 2)
	{
		fprintf(stderr, "Error: for this project you need at least two analog inputs\n");
		return false;
	}

	gInverseSampleRate = 1.0 / context->audioSampleRate;
	gPhase = 0.0;

	return true;
}

void render(BelaContext *context, void *userData)
{
    
	for(unsigned int n = 0; n < context->audioFrames; n++) {

		// read analog inputs and update frequency and amplitude
		gIn1 = analogRead(context, n, 0);
		gIn2 = analogRead(context, n, 1);
		storeSample(gIn1);
		gIn1-=0.4192f;
		
		// generate a sine wave with the amplitude and frequency 
		/*float out = gAmplitude * sinf(gPhase);
		gPhase += 2.0f * (float)M_PI * gFrequency * gInverseSampleRate;
		if(gPhase > M_PI)
			gPhase -= 2.0f * (float)M_PI;
        */
        float out=0.0f;		// log the sine wave and sensor values on the scope
		scope.log(out, gIn1, gIn2);

		// pass the sine wave to the audio outputs
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{

}

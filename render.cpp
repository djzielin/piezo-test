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
#include "simple_moving_average.h"
Scope scope;

int gAudioFramesPerAnalogFrame = 0;
float gInverseSampleRate;
float gPhase;

float gAmplitude=1.0f;
float gFrequency;

float gIn1;
float gIn2;

// For this example you need to set the Analog Sample Rate to 
// 44.1 KHz which you can do in the settings tab.
float average;
int sampleCount;
simple_moving_average *SMA;

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

    SMA=new simple_moving_average(10);
    
	return true;
}

#define STATE_WAITING 0
#define STATE_TRIGGERED 1

int state=STATE_WAITING;
int timeout;
unsigned int zero_counter=0;
bool zero_reported=false;
float max_found=0.0f;    
float zero_ratio;
int note_time=0;    	
    	
int negCount=0;
int maxPos=0;
float slope;

void render(BelaContext *context, void *userData)
{
    float trigThreshold=-0.05f; //magic numbers
    int timeOutAmount=1000;   
    
	for(unsigned int n = 0; n < context->audioFrames; n++) {

		// read analog inputs and update frequency and amplitude
		gIn1 = analogRead(context, n, 0);
		gIn2 = analogRead(context, n, 1);
		
		gIn1-=0.4218f;
		//storeSample(gIn1);
		float smoothed=SMA->tick(gIn1);
		
		float sample=gIn1;
		if(sample<-0.4f) sample=-0.4f;
		if(sample>0.4f) sample=0.4f;
		
		if(sample<0.0f)
		  negCount++;
		if(sample>=0.0f)
		  negCount=0;
		  
		if(state==STATE_WAITING)
		{
			if(sample<trigThreshold)
		    {
		       //rt_printf("note triggered at level: %f\n",gIn1);
		       state=STATE_TRIGGERED;
		       timeout=timeOutAmount;
		       zero_counter=0;
		       zero_reported=false;
		       max_found=0.0f;
	     	}
		}
		if(state==STATE_TRIGGERED)
		{

            if(sample<max_found)
            {
               max_found=sample;
               maxPos=zero_counter;
            }
			if(zero_reported==false)
			{
				zero_counter++;

			    if(sample>0.0f)
			    {
			    	rt_printf("zero crossing. sample: %d\n",zero_counter);
			    	//rt_printf("neg count: %d\n",negCount);
			    	
			    	if(zero_counter>110)  zero_ratio=1.0f;
			    	else                 zero_ratio=0.0f;
			    	
			    	
			    	 
			    	if(zero_ratio>0.6f)
			    	   gFrequency=400;
			    	else
			    	   gFrequency=100;
			    	note_time=2000;
			    	//rt_printf("zero ratio: %.02f\n",zero_ratio);
			    	
			    	//rt_printf("max found: %f\n", max_found);
			    	float slope=fabs(max_found-trigThreshold)/maxPos*25.0f;
			    	if(slope>1.0f) slope=1.0f;
			    	//rt_printf("slope: %f\n",slope);

			      	//gAmplitude=max_found/-0.4f;
				    gAmplitude=slope;
			    	zero_reported=true;
			    }
			}

			timeout--;
			if(timeout<0)
			{
				

				
				rt_printf("vol: %.02f center: %.02f\n",gAmplitude,zero_ratio);
				state=STATE_WAITING;
				zero_reported=false;
			}
		}
		float out=0.0f;
		if(note_time>0)
		{
			note_time--;
		// generate a sine wave with the amplitude and frequency 
		out = gAmplitude * sinf(gPhase);
		gPhase += 2.0f * (float)M_PI * gFrequency * gInverseSampleRate;
		if(gPhase > M_PI)
			gPhase -= 2.0f * (float)M_PI;
		}	
			
        
		scope.log(out, gIn1, smoothed);

		// pass the sine wave to the audio outputs
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{

}

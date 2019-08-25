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
simple_moving_average *SMA1;
simple_moving_average *SMA2;
simple_moving_average *SMA3;

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

    SMA1=new simple_moving_average(100);
    SMA2=new simple_moving_average(200);
    SMA3=new simple_moving_average(300);
    
	return true;
}

#define STATE_WAITING 0
#define STATE_TRIGGERED 1
#define STATE_RELEASE 2

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
float prevSample=0;

int classification;
float clamp(float in, float min, float max)
{
	if(in<min) in=min;
	if(in>max) in=max;
	
	return in;
}

int last_zero=-1;
int timeSinceLastZero=-1;
float valueAtTrip;
int hitnum=0;

void render(BelaContext *context, void *userData)
{
    float trigThreshold=-0.03f; //magic numbers
    int timeOutAmount=500;   

    
	for(unsigned int n = 0; n < context->audioFrames; n++) {

		// read analog inputs and update frequency and amplitude
		gIn1 = analogRead(context, n, 0);
		gIn2 = analogRead(context, n, 1);
		
		gIn1-=0.4218f;
		gIn2-=0.4218f;
		//storeSample(gIn1);
		float sample=clamp(gIn1*1.66f,-1,1);
		float sample2=clamp(gIn2*1.66f,-1,1);
		
		float trigThreshold=-SMA1->tick(fabs(sample))-0.05f;
		//float smoothed2=-SMA2->tick(fabs(sample));
		//float smoothed3=-SMA3->tick(fabs(sample));
		
        if(sample<0 && prevSample>=0)
           timeSinceLastZero=0;
		
		if(state==STATE_WAITING)
		{
			if(sample<trigThreshold)
		    {
		    	classification=0;
		    	valueAtTrip=sample;
		    	timeout=timeOutAmount;
     		    zero_counter=0;
	     		zero_reported=false;
		    	max_found=0.0f;
		    	//SMA1->fill(1.0f);
		    
		    	state=STATE_TRIGGERED;
		    }
		}
		
		if(state==STATE_TRIGGERED)
		{
            if(sample<max_found)
            {
               max_found=sample;
               maxPos=zero_counter;
            }
			if(zero_counter==50)
			{
			    //if(sample>0.0f)
			    //{
			    	slope=fabs(max_found-valueAtTrip)/maxPos;
                     
				    gAmplitude=clamp(slope*slope*500.0f,0.0,1.0);
				    
				    //float test=50.0f*gAmplitude + (float)zero_counter -115.0f; //use line to seperate data
				  
				    if(gAmplitude>=0.9)
				    {
				       gFrequency=400;
				       note_time=3000;
				    }
				    else
				    {
				      gFrequency=200;
			    	  note_time=3000;
			    	}
			    	gPhase=0;
			    	
			        rt_printf("HIT: %d Volume: %.04f, Zero Crossing: %d\n", hitnum,gAmplitude,zero_counter);
				    hitnum++;
			     	state=STATE_RELEASE;
			    //}
			}
			zero_counter++;
		}
        if(state==STATE_RELEASE)
        {
			timeout--;
			if(timeout<0)
			{
				state=STATE_WAITING;
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
		else
		   gAmplitude=0;
		
			
        
		scope.log(sample,trigThreshold,gAmplitude);

		// pass the sine wave to the audio outputs
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
		
		prevSample=sample;		
		if(timeSinceLastZero>=0)
		   timeSinceLastZero++;
	}
}

void cleanup(BelaContext *context, void *userData)
{

}

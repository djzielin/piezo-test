/***** simple_moving_average.h *****/
class simple_moving_average
{
private:
	float *buffer;
	int sma_index;
	float sma_running_total;
	int sma_size;
	float total_factor;

public:
	simple_moving_average(int size)
	{
		sma_size=size;
		buffer=new float[sma_size];
		total_factor=1.0/(float)sma_size;
		
		reset();
	}

	float tick(float val)
	{ 
		sma_running_total-= buffer[sma_index]; //remove out oldest sample from running total
		sma_running_total+= val;        //add in newest sample to running total
		buffer[sma_index]=val;          //store newest sample 
		sma_index=(sma_index+1)%sma_size;
      
		float current_avg=sma_running_total*total_factor; //divide by number of samples in buffer
		
		return current_avg;
	}
	
	void reset()
	{
		sma_index=0; 
     	sma_running_total=0;
     	
        for(int i=0;i<sma_size;i++)
		{
			buffer[i]=0.0f;
		}
	}
};
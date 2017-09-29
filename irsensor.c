/***************************************************************
IRSensor.c Version 0.0.0
This is a small program that uses the bcm2835.h library along
with the MLX90614 IR sensor attached to i2c pins through a
jumper wire. The board I used for testing was the standard
i2c pins on the Raspberry pi 3.
This program reads in the tempurature from the MLX90614 and
writes it to a log file in Celsius. This should repeat once
per minute as long as the program is running, and the program
will create a new log file every day.
Credit to Eddie Fiorentine at Codedump.io for the sample code
on using the bcm2835 library.
**************************************************************/ 


#include <bcm2835.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define ADDRESS 0x5a
#define AMBIENT 0x06
#define OBJECT 0x07

int main(int argc, char *argv[])
{
	int exit = 0, nextTime;
	double temp = 0;
	char mlxbuf[6], measurement = AMBIENT;
	struct tm curTime;
	time_t t;
	FILE *flog;
	char buffer[50];

	//initialize bcm2835
	bcm2835_init();
	
	//begin i2c connection
	bcm2835_i2c_begin();
	
	//set baudrate to 25000 bits per second
	bcm2835_i2c_set_baudrate(25000);

	//set slave address to defined address
	bcm2835_i2c_setSlaveAddress(ADDRESS);

	//get current time
	t = time(NULL);
	curTime = *localtime(&t);

	//open log
	sprintf(buffer, "logs/%d-%d-%d.csv", curTime.tm_mon + 1, curTime.tm_mday, curTime.tm_year + 1900);
	if(access(buffer, R_OK) != -1) 
	{
		flog = fopen(buffer, "a");
	}

	else
	{
		flog = fopen(buffer, "w");
		fprintf(flog, "(hour:minute:second), temperature(C)\n");
		fflush(flog);
	}

	//begin main loop
	while(!exit) //not sure when this will be true since the program runs indefinitely
	{
		//check if the time is 12:00 am. If it is, open a new csv file
		if(curTime.tm_min == 0 && curTime.tm_hour == 0)
		{
			fclose(flog);
			sprintf(buffer, "logs/%d-%d-%d.csv", curTime.tm_mon, curTime.tm_mday, curTime.tm_year + 1900);
			flog = fopen(buffer, "w");
			fprintf(flog, "(hour:minute:second), (tempurature(C)\n");
			fflush(flog);
		}

		//begin i2c connection again
		bcm2835_i2c_begin();

		//write which registry to get measurement from (AMBIENT, or OBJECT)
		bcm2835_i2c_write(&measurement, 1);

		//read the returned value
		bcm2835_i2c_read_register_rs(&measurement, &mlxbuf[0], 3);

		//convert i2c bits to celsius temp
		temp = (double) (((mlxbuf[1]) << 8) + mlxbuf[0]);
		temp = (temp * 0.02) - 0.01;
		temp = temp - 273.15;

		//write time and temp to flog
		t = time(NULL);
		curTime = *localtime(&t);
		fprintf(flog, "(%d:%d:%d), %.3lf", curTime.tm_hour, curTime.tm_min, curTime.tm_sec, temp);
		fflush(flog);

		//wait for one minute until reading next
		if(curTime.tm_min == 59)
		{
			nextTime = curTime.tm_hour + 1;
			while (curTime.tm_hour < nextTime)
			{
				t = time(NULL);
				curTime = *localtime(&t);
			}
		}

		else
		{
			nextTime = curTime.tm_min + 1;
			while(curTime.tm_min < nextTime)
			{
				t = time(NULL);
				curTime = *localtime(&t);
			}
		}
	}

	fclose(flog);
	return 0;
}

#include<LPC17xx.h>

#include<stdio.h>

char to_disp[10];
float x;
int j1;
int cmd=0x0, data=0x00, flag=0, j,i;
#define RS_CTRL 0x08000000;	//1.27
#define EN_CTRL 0x10000000;	//1.28
#define DT_CTRL 0x07800000;	//1.23-0.26

void init(void); //LCD initialisation
void clearPorts(void);
void writeCmd(void); //Writing commands and data
void writeData(void);
void disp(char *);
extern int flag, cmd, data;
void pwm_init(void);
double f[3]={92.7,93.5,98.3};
float diff,freq;
int main(void)
{
	LPC_SC->PCONP &= (0x1<<12)|(0x1<<15);
	LPC_PINCON->PINSEL3 = (3<<28);	//P1.30 fn3 for channel4
	init();
	pwm_init();
	
	while(1)
	{
		flag=0;
		data = 0x80;//Move to line1 column1
		writeData();
		for(j1=0;j1<3000;j1++);
		LPC_ADC->ADCR = 0x01200010;
		for(j1=0;j1<2000;j1++);
		if(LPC_ADC->ADGDR & 0x1<<31)
		{
			x = (LPC_ADC->ADGDR & 0xFFF0)>>4;
			x = 90.0 + ((int)(x/(4096/95)))*0.1;
			if(x>=90.0 && x<=99.50)
			{
				sprintf(to_disp, "%3.2f FM", x);
				for(i=0;i<3;i++)
				{
					diff=f[i]-x;
					if(diff<0) 
						diff =-diff;
					if(diff <= 0.1)//Max intensity if diff is less than 0.04
					{	
						LPC_PWM1->MR4 = 30000;
						LPC_PWM1->LER = 0X000000FF;
						break;				
					}
					else if(diff <= 0.2)//Medium intensity if diff is less than 0.1
					{	
						LPC_PWM1->MR4 = 10000;
						LPC_PWM1->LER = 0X000000FF;
						break;				
					}
					else if(diff <= 0.4)//Min intensity if diff is less than 0.2
					{	
						LPC_PWM1->MR4 = 2000;
						LPC_PWM1->LER = 0X000000FF;
						break;				
					}
					else//LED does not glow if diff is greater
					{
						LPC_PWM1->MR4 = 0;
						LPC_PWM1->LER = 0X000000FF;
					}	
				}
			}disp(&to_disp[0]);//Display the message
		}
	}
}

void init()
{
	LPC_PINCON->PINSEL0 &= 0xFFFFF00F;
	LPC_GPIO0->FIODIRH = 0x1FF0;
	clearPorts();
	flag=0;
	for(j=0;j<3200;j++);
	//LCD initialisation commands
	for(i=0;i<3;i++)//Wake up
	{
		cmd = 0x3<<4;
		writeCmd();
		for(j=0;j<30000;j++);
	}
	cmd = 0x2<<4;//Return home
	writeCmd();
	for(j=0;j<30000;j++);
	
	data = 0x28;//Inform that there are 2 lines, default font
	writeData();
	for(j=0;j<30000;j++);
	
	data = 0x01;//Clear display
	writeData();
	for(j=0;j<10000;j++);
	
	data = 0x06;//Increment cursor after writing, don't shift data
	writeData();
	for(j=0;j<800;j++);
	
	data = 0x80;//Move to first line, first column
	writeData();
	for(j=0;j<800;j++);
	
	data = 0x0F;//Switch on LCD, show cursor and blink
	writeData();
	for(j=0;j<800;j++);
}

void clearPorts()
{
	LPC_GPIO0->FIOCLRH = 0x1F80;
}

//Function to write the command/data
void writeCmd()
{
	clearPorts();
	LPC_GPIO0->FIOPIN = cmd;
	if(flag==0)
	{
		LPC_GPIO0->FIOCLR = RS_CTRL;
	}
	else if(flag==1)
	{
		LPC_GPIO0->FIOSET = RS_CTRL;
	}
	LPC_GPIO0->FIOSET = EN_CTRL;
	for(j=0;j<50;j++);
	LPC_GPIO0->FIOCLR = EN_CTRL;
}

//Function to extract command/data
void writeData()
{
	cmd = (data & 0xF0)<<0;
	writeCmd();
	cmd = (data & 0x0F)<<4;
	writeCmd();
	for(j=0;j<1000;j++);
}

//Function used to dislay the data
void disp(char *to_disp)
{
	i = 0;
	while(to_disp[i]!='\0')
	{
		data = to_disp[i];
		flag=1;
		writeData();
		i++;
	}
}

void pwm_init()
{
	LPC_SC->PCONP |= (1<<6);  //Power on PWM1 (can also be done through system config wizard)
	LPC_PINCON->PINSEL3 |= 0X00008000; //PWM1.4 is selected for the P1.23
	
	LPC_PWM1->PR = 0;// set PR to 0
	LPC_PWM1->PCR = 0X00001000; //Enable PWM1.4 for SINGLE EDGE modulation
	LPC_PWM1->MCR = 0X00000002; //Reset on PWM match0 (one complete cycle)
	LPC_PWM1->MR0 = 30000; //Setup MR0 for length of cycle
	LPC_PWM1->MR4 = 0X00000000; 
	LPC_PWM1->LER = 0X000000FF; //Enable shadow copy register ie load enable register
	LPC_PWM1->TCR = 0X00000002; //Reset
	LPC_PWM1->TCR = 0X00000009; //Enable PWM and counter
	return;
}

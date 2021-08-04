#include <reg52.h>
#include "ds1302_driver.h"
#include "oled_iic_driver.h"

/*��������״̬��־*/
#define show_state 0		//��ʾģʽ
#define menu_state  1        //�˵�ģʽ
#define set_alarm  2        //��������
#define set_time   3   		//ʱ������
#define alarm_noisy 4

//�������µĵ�ƽ״̬
#define key_down 0			
#define key_up   1

sbit menu_key = P2^7;//����������P3^2 �ⲿ�ж�0����
sbit shift_key = P2^6;
sbit add_key = P2^5;
sbit save_key = P2^4;
sbit alarm = P3^4;

unsigned char temp_buf[10];		//������
unsigned char alarm_time[2][3] ={{0x00, 0x30,0x08},{0x00, 0x20,0x08}};//����ʱ��2�鿪�ط�ʱbcd����
unsigned char *time_ascii;				//���ڴ��ת����ascii���ʱ������
unsigned char run_state = show_state;	//��ʼ����������״̬Ϊ��ʾģʽ
unsigned char timer0_count = 1;			//��ʱ���������
unsigned char key_pressed = 0;			//���ް�������
unsigned char tc = 0;					//��������
unsigned char my_8bit_flag = 0x00;		//8λ��־λ
										//��ʾ��־ �˵���־ �����ӱ�־ ��ʱ���־ 
										//��˸��־ alarm ѡ��time set  ѡ��alarm set
int position = 0;//����ʱ��ʱ��Ĺ��λ��
unsigned char blink_count = 0;

//***********************************************************************************
void alarm_show(void);

void delay1ms(void);
	
unsigned char* ds1302_time_dat_to_ascii(unsigned char time[]);/* *��ds1302����������ת��Ϊascii��*/	

void show_time(unsigned char ascii_time[],unsigned char page_drift);/* *������ʾʱ��*/

void bcd_add_1(unsigned char *bcd_dat,unsigned char position);//����ʱ�������ʱ���ã���Ӧλ�õ�bcd���һ

void Timer0Init(void);//50���붨ʱ��0��ʼ��@12.000MHz		


void main(void)
{
	
	P3 |= 0x10;//���η����˵ķ�����
	//ds1302_init();
	oled_init();
	Timer0Init();
	IT0=0; //�ⲿ�ж�0�͵�ƽ��Ч
	EX0=1; //�ⲿ�жϿ���
	ET0=1; //������ʱ��0�ж�
	TR0 = 0;
	EA=1;  //���жϿ���
	while(1)
	{
		switch(run_state)
		{
			case show_state:
				if((my_8bit_flag&0x80)==0x00)//�ս���ʱ ��һ����
				{	
					oled_all_fill(0x00);
					my_8bit_flag |= 0x80;
					oled_ascii_8x16_str(0,6,"Alarm\0");
					if(alarm_time[0][0])
						oled_ascii_8x16_str(42,6,"1 ON\0");
					else
						oled_ascii_8x16_str(42,6,"1 OFF\0");
					if(alarm_time[1][0])
						oled_ascii_8x16_str(85,6,"2 ON\0");
					else
						oled_ascii_8x16_str(85,6,"2 OFF\0");
						
				}
				ds1302_read_all_time();
				time_ascii = ds1302_time_dat_to_ascii(TIME);
				show_time(time_ascii,0);
				if(alarm_time[0][0])
					if((TIME[1]==alarm_time[0][1]) && (TIME[2]==alarm_time[0][2]) && (TIME[0]==0x00))
						run_state = alarm_noisy;
				if(alarm_time[1][0])
					if((TIME[1]==alarm_time[1][1]) && (TIME[2]==alarm_time[1][2]) && (TIME[0]==0x00))
						run_state = alarm_noisy;
				break;
			case alarm_noisy:
				alarm = ~alarm;
				timer0_count = 0;
				{
					unsigned char tc = 200;
					while(tc--)
						delay1ms();
				}
				if((shift_key==key_down)||(add_key==key_down)||(save_key==key_down))
				{
					unsigned char tc = 80;
					key_pressed = 1;
					while(!(tc--)) delay1ms();
						
					if((shift_key==key_down)||(add_key==key_down)||(save_key==key_down))
					{	
						alarm = 1;
						run_state = show_state;
					}

				}
				break;
			case menu_state:
				if((my_8bit_flag&0x40)==0x00)//�ս���ʱ ��һ����ֻ��ʾһ�β˵�������ѭ����ʾ�Գ�������ϴ���ʱ
				{	
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"<Option Menu>\0");
					oled_ascii_8x16_str(2,2,"1.Time Set\0");
					oled_ascii_8x16_str(2,4,"2.Alarm Set\0");
					oled_ascii_8x16_str(0,6,"@Author:CTBU^Shy\0");
					my_8bit_flag |= 0x40;
				}
				/* *������� */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					
					tc = 20;//��������
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed)
				{
					tc--;
					if(!tc)//��������
					{
						if(add_key == key_down)//add_key����
						{
							my_8bit_flag |= 0x01;//alarm set
							my_8bit_flag &= 0xfd;//off time set
							timer0_count = 0;
							key_pressed = 0;
							oled_ascii_8x16_str(80,2,"    \0");
							oled_ascii_8x16_str(88,4,"<-<-\0");
						}
						else if(shift_key == key_down)//shift_key����
						{
							my_8bit_flag |= 0x02;//time set
							my_8bit_flag &= 0xfe;//off alram set
							timer0_count = 0;
							key_pressed = 0;
							oled_ascii_8x16_str(80,2,"<-<-\0");
							oled_ascii_8x16_str(88,4,"    \0");
						}
						else if(save_key == key_down)//save_key����
						{
							if((my_8bit_flag&0x01) == 0x01)//alarm set
							{
								timer0_count = 0;
								TR0 = 1;
								EX0 = 0;
								my_8bit_flag |= 0x03;//�˳�ʱ���ѡ�������
								run_state = set_alarm;
								key_pressed = 0;
								break;//�˳��˵�
							}
							else if((my_8bit_flag&0x02) == 0x02)//time set
							{
								timer0_count = 0;
								TR0 = 1;
								EX0 = 0;
								my_8bit_flag |= 0x03;//�˳�ʱ���ѡ�������
								run_state = set_time;
								key_pressed = 0;	
								break;//�˳��˵�
								
							}
							else
								key_pressed = 0;
						}
							
						else
							key_pressed = 0;//�ж�Ϊ����
							
					}
				}
				break;
			case set_alarm:
				if((my_8bit_flag&0x20)==0x00)//ֻ��ʾһ�β˵�������ѭ����ʾ�Գ�������ϴ���ʱ
				{
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"< Alarm Set >\0");
					oled_ascii_8x16_str(2,2,"Alarm1@\0");
					oled_ascii_8x16_str(2,4,"Alarm2@\0");
					alarm_show();
					my_8bit_flag |= 0x20;
					position = 0;
					
					TIME[0] = alarm_time[0][0];
					TIME[1] = alarm_time[0][1];
					TIME[2] = alarm_time[0][2];
					TIME[3] = alarm_time[1][0];
					TIME[4] = alarm_time[1][1];
					TIME[5] = alarm_time[1][2];
				}
				/*��˸�������ӵ�ʵ��*///12.16
				if(my_8bit_flag&0x08)
				{
					if(--blink_count==0)
						my_8bit_flag&=0xf7;
				}
				else
				{
					if(++blink_count==10)
						my_8bit_flag|=0x08;
				}
				switch(position)
				{
					case 0:
						temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
						temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //ʱ
						temp_buf[2]='\0';
						oled_ascii_8x16_str(58,4,temp_buf);//����ǰһλ
						
						if(my_8bit_flag&0x08)
						{
							if(alarm_time[0][0] == 0x00)
								oled_ascii_8x16_str(104,2,"OFF");
							else
								oled_ascii_8x16_str(104,2," ON");
						}
						else
						{
								oled_ascii_8x16_str(104,2,"   ");
						}
						break;
					case 1:
						if(alarm_time[0][0] == 0x00)
							oled_ascii_8x16_str(104,2,"OFF");
						else
							oled_ascii_8x16_str(104,2," ON");
						if(my_8bit_flag&0x08)
						{
							temp_buf[3]=(alarm_time[0][1]&0x70)/16 + '0';
							temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//��
							temp_buf[5]='\0';
							oled_ascii_8x16_str(58+24,2,&temp_buf[3]);
						}
						else
						{
							oled_ascii_8x16_str(58+24,2,"  ");
						}
						
						break;
					case 2:
						temp_buf[3]=(alarm_time[0][1]&0x70)/16 + '0';
						temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//��
						temp_buf[5]='\0';
						oled_ascii_8x16_str(58+24,2,&temp_buf[3]);
						if(my_8bit_flag&0x08)
						{
							temp_buf[0]=(alarm_time[0][2]&0x30)/16 + '0';
							temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //ʱ
							temp_buf[2]='\0';
							oled_ascii_8x16_str(58,2,temp_buf);
						}
						else
						{
							oled_ascii_8x16_str(58,2,"  ");
						}
						break;
					
					
					case 3:
						temp_buf[0]=(alarm_time[0][2]&0x30)/16 + '0';
						temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //ʱ
						temp_buf[2]='\0';
						oled_ascii_8x16_str(58,2,temp_buf);
						if(my_8bit_flag&0x08)
						{
							if(alarm_time[1][0] == 0x00)
								oled_ascii_8x16_str(104,4,"OFF");
							else
								oled_ascii_8x16_str(104,4," ON");
						}
						else
						{
								oled_ascii_8x16_str(104,4,"   ");
						}
						break;
					case 4:
						if(alarm_time[1][0] == 0x00)
								oled_ascii_8x16_str(104,4,"OFF");
						else
								oled_ascii_8x16_str(104,4," ON");
						if(my_8bit_flag&0x08)
						{
							temp_buf[3]=(alarm_time[1][1]&0x70)/16 + '0';
							temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//��
							temp_buf[5]='\0';
							oled_ascii_8x16_str(58+24,4,&temp_buf[3]);
						}
						else
						{
							oled_ascii_8x16_str(58+24,4,"  ");
						}
						break;
					case 5:
						temp_buf[3]=(alarm_time[1][1]&0x70)/16 + '0';
						temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//��
						temp_buf[5]='\0';
						oled_ascii_8x16_str(58+24,4,&temp_buf[3]);
						if(my_8bit_flag&0x08)
						{
							temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
							temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //ʱ
							temp_buf[2]='\0';
							oled_ascii_8x16_str(58,4,temp_buf);
						}
						else
						{
							oled_ascii_8x16_str(58,4,"  ");
						}
						break;
					default:
						break;
				}
				/* *������� */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					tc=2;
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed&&!(tc--))
				{
					if(add_key == key_down)//add_key����
					{
						timer0_count = 0;
						key_pressed = 0;//���Ű�������
						if(position < 3)
						{
							if(position == 0)
								alarm_time[0][0] = ~alarm_time[0][0];//����״̬�ı�
							else
								bcd_add_1(alarm_time[0],position);//ʱ������
						}
						else
						{
							if(position == 3)
								alarm_time[1][0] = ~alarm_time[1][0];//����״̬�ı�
							else
								bcd_add_1(alarm_time[1],position-3);//ʱ������
						}
						alarm_show();//ˢ����ʾ����	
					}
					else if(shift_key == key_down)//shift_key����
					{
						position++;
						if(position>5)//6������λ
							position = 0;
						
						timer0_count = 0;
						
						key_pressed = 0;//���Ű�����ȡ
					}
					else if(save_key == key_down)//save_key����
					{
						key_pressed = 0;//���Ű�����ȡ
						my_8bit_flag&=0xf7;
						run_state = show_state;//������ʾʱ��ģʽ
					}
							
					else
						key_pressed = 0;//�ж�Ϊ����
				}
				break;
			case set_time:
				/*��˸����ʱ���ʵ��*///12.16
				if(my_8bit_flag&0x08)
				{
					if(--blink_count==0)
						my_8bit_flag&=0xf7;
				}
				else
				{
					if(++blink_count==10)
						my_8bit_flag|=0x08;
				}
				switch(position)
				{
					unsigned char temp[3];
					case 6:
						{//��ʾǰһλ��˸��ֵ
						temp[0] = time_ascii[18];
						temp[1] = time_ascii[19];	
						temp[2] = '\0';
						oled_ascii_8x16_str(31+24+24,5,temp);//��ʾmiao
						}
						
						if(my_8bit_flag&0x08)
						{

							temp[0] = time_ascii[2];
							temp[1] = time_ascii[3];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16,2,temp);//��ʾ��
						}
						else
						{
							oled_ascii_8x16_str(16,2,"  ");
						}
						break;
					case 5:
						{
							temp[0] = time_ascii[2];
							temp[1] = time_ascii[3];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16,2,temp);//��ʾ��
						}
						if(my_8bit_flag&0x08)
						{
							oled_chinese_characters_16x16(10*8+16*2,2,WEEK[time_ascii[11]-'0'+1]);//��
						}
						else
						{
							oled_ascii_8x16_str(10*8+16*2,2,"  ");
						}
						break;
					case 4:
						{
							oled_chinese_characters_16x16(10*8+16*2,2,WEEK[time_ascii[11]-'0'+1]);//��
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[5];
							temp[1] = time_ascii[6];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24,2,temp);//��ʾ��
						}
						else
						{
							oled_ascii_8x16_str(16+24,2,"  ");
						}
						
						break;
					case 3:
						{
							temp[0] = time_ascii[5];
							temp[1] = time_ascii[6];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24,2,temp);//��ʾ��
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[8];
							temp[1] = time_ascii[9];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24+24,2,temp);//��ʾ��
						}
						else
						{
							oled_ascii_8x16_str(16+24+24,2,"  ");
						}
						break;
					case 2:
						{
							temp[0] = time_ascii[8];
							temp[1] = time_ascii[9];	
							temp[2] = '\0';
							oled_ascii_8x16_str(16+24+24,2,temp);//��ʾ��
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[12];
							temp[1] = time_ascii[13];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31,5,temp);//��ʾʱ
						}
						else
						{
							oled_ascii_8x16_str(31,5,"  ");
						}
						break;
					case 1:
						{
							temp[0] = time_ascii[12];
							temp[1] = time_ascii[13];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31,5,temp);//��ʾʱ
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[15];
							temp[1] = time_ascii[16];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24,5,temp);//��ʾ��
						}
						else
						{
							oled_ascii_8x16_str(31+24,5,"  ");//��ʾ��
						}			
						break;
					case 0:
						{
							temp[0] = time_ascii[15];
							temp[1] = time_ascii[16];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24,5,temp);//��ʾ��
						}
						if(my_8bit_flag&0x08)
						{
							temp[0] = time_ascii[18];
							temp[1] = time_ascii[19];	
							temp[2] = '\0';
							oled_ascii_8x16_str(31+24+24,5,temp);//��ʾmiao
						}
						else
						{
							oled_ascii_8x16_str(31+24+24,5,"  ");//��ʾmiao
						}			
						break;	
					default:
						break;
				}
				
				if((my_8bit_flag&0x10)==0x00)//ֻ��ʾһ�β˵�������ѭ����ʾ�Գ�������ϴ���ʱ
				{
					oled_all_fill(0x00);
					oled_ascii_8x16_str(12,0,"< Time  Set >\0");
					my_8bit_flag |= 0x10;
					ds1302_read_all_time();//��һ��ʱ�䵽timeȫ�ֱ���
					time_ascii = ds1302_time_dat_to_ascii(TIME);
					show_time(time_ascii,2);//��ʾһ�ε�ǰ��ʱ��
					position = 6;//��ʼ��������λ��
				}
				/* *������� */
				if((key_pressed==0)&&((shift_key==key_down)||(add_key==key_down)||(save_key==key_down)))
				{
					tc=2;
					key_pressed = 1;
				}
				delay1ms();
				if(key_pressed&&!(tc--))
				{
					if(add_key == key_down)//add_key����
					{
						timer0_count = 0;
						bcd_add_1(TIME,position);
						key_pressed = 0;//���Ű�������
						time_ascii = ds1302_time_dat_to_ascii(TIME);//ˢ����ʾ
						//show_time(time_ascii,2);
					}
					else if(shift_key == key_down)//shift_key����
					{
						
						position--;
						if(position==-1)//7������λ
							position = 6;
						timer0_count = 0;
						
						key_pressed = 0;//���Ű�����ȡ
					}
					else if(save_key == key_down)//save_key����
					{
						ds1302_init();//����д�������õ�ʱ�䵽1302
						key_pressed = 0;//���Ű�����ȡ
						position=0;
						my_8bit_flag&=0xf7;
						run_state = show_state;//������ʾʱ��ģʽ
					}	
					else
						key_pressed = 0;//�ж�Ϊ����
				}
				
				break;
			default:
				break;
				
		}
		
	}
}

unsigned char* ds1302_time_dat_to_ascii(unsigned char time[])
{
	static unsigned char time_ascii[] ="2020-11-09-120:00:12\0";
	//���ת�����ascii����-��-��-��ʱ���֣���
	unsigned char* TIME = time;
	time_ascii[19] = (TIME[0]&0x0f) + '0';		//��
	time_ascii[18] = (TIME[0]&0x70)/16 + '0';
	
	time_ascii[16] = (TIME[1]&0x0f) + '0';		//��
	time_ascii[15] = (TIME[1]&0x70)/16 + '0';
		
	time_ascii[13] = (TIME[2]&0x0f) + '0';		//ʱ
	time_ascii[12] = (TIME[2]&0x30)/16 + '0';
		
	time_ascii[11] = (TIME[5]&0x07) + '1';		//��
		
	time_ascii[9] = (TIME[3]&0x0f) + '0';		//��
	time_ascii[8] = (TIME[3]&0x30)/16 + '0';
		
	time_ascii[6] = (TIME[4]&0x0f) + '0';		//��
	time_ascii[5] = (TIME[4]&0x10)/16 + '0';
		
	time_ascii[3] = (TIME[6]&0x0f) + '0';		//��
	time_ascii[2] = (TIME[6]&0xf0)/16 + '0';
	
	time_ascii[1] =  '0';
	time_ascii[0] =  '2';
	return time_ascii;
}
void show_time(unsigned char ascii_time[],unsigned char page_drift)
{


	unsigned char* temp = ascii_time;
	temp[10] = '\0';//ȡ������
	oled_ascii_8x16_str(0,page_drift,temp);//��ʾ������
	oled_chinese_characters_16x16(10*8,page_drift,WEEK[0]);//��
	oled_chinese_characters_16x16(10*8+16,page_drift,WEEK[1]);//��
	oled_chinese_characters_16x16(10*8+16*2,page_drift,WEEK[ascii_time[11]-'0'+1]);//��
	temp = ascii_time;
	temp = temp+12;//ȡʱʱ����
	oled_ascii_8x16_str(31,3+page_drift,temp);//��ʾʱ����
}

void Timer0Init(void)		//50����@12.000MHz
{
	TMOD = 0x01;		//���ö�ʱ��Ϊ16λ
	TL0 = 0xB0;		//���ö�ʱ����ֵ
	TH0 = 0x3C;		//���ö�ʱ����ֵ
	TF0 = 0;		//����жϱ�־λ
	TR0 = 0;		//�رն�ʱ��
}
void exint0(void) interrupt 0		//�ⲿ�ж�0���жϷ�����
{
	TR0 = 1;//�򿪶�ʱ��
	EX0 = 0;
}

void timer0(void) interrupt 1		//��ʱ��0���жϷ�����
{
	TL0 = 0xB0;		//���ö�ʱ����ֵ
	TH0 = 0x3C;		//���ö�ʱ����ֵ
	if(menu_key == key_down)//���ð���������
	{
		
		if(run_state != menu_state)
			my_8bit_flag &= 0x0f;//��4λ��־����
		run_state = menu_state;
	}
	else
		EX0 = 1;
	if(run_state != show_state)
		timer0_count++;
	if(timer0_count >200)//���10��û�����ò������˳��˵�
	{
		
		if(run_state == set_alarm)//�����������˻���
		{
			alarm_time[0][0] = TIME[0];//����������
			alarm_time[0][1] = TIME[1];
			alarm_time[0][2] = TIME[2];
			alarm_time[1][0] = TIME[3];
			alarm_time[1][1] = TIME[4];
			alarm_time[1][2] = TIME[5];
			
		}
		ds1302_read_all_time();//��һ��ʱ�䵽timeȫ�ֱ��� ��ֹ����
		run_state = show_state;
		timer0_count = 0;
		TR0 = 0;
		EX0 = 1;
		my_8bit_flag |= 0x0f;//�˳�ʱ���ѡ�������
		key_pressed = 0;
		my_8bit_flag&=0xf7;
	}
}

void bcd_add_1(unsigned char *bcd_dat,unsigned char position)
{
	bcd_dat[position]++;
	if((bcd_dat[position]&0x0f)>9)					 //BCD��λ��ʽ
	{
		bcd_dat[position]=bcd_dat[position]+6;
	}
	switch(position)
	{
		case 0:		//��
			if(bcd_dat[position] >= 0x60)		//59 ��
				bcd_dat[position] &= 0x80;		//��������
			break;
		case 1:		//��
			if(bcd_dat[position] >= 0x60)		//59 ��
				bcd_dat[position] &= 0x80;		//��������
			break;
		case 2:		//ʱ
			if(bcd_dat[position] >= 0x24)		//23 ʱ
				bcd_dat[position] &= 0xc0;		//ʱ������
			break;
		case 3:		//��
			if(bcd_dat[position] >= 0x32)		//31 ��
				bcd_dat[position] &= 0xc0;		//������
			break;
		case 4:		//��
			if(bcd_dat[position] >= 0x13)		//12 ��
				bcd_dat[position] &= 0xe0;		//������
			break;
		case 5:		//��
			if(bcd_dat[position] >= 0x07)		// 7 ��
				bcd_dat[position] &= 0xf0;		//������
			break;
		case 6:		//��
			if(bcd_dat[position] > 0x99)		//99 ��
				bcd_dat[position] &= 0x00;		//������
			break;
	}	
}

void delay1ms()		//@12.000MHz
{
	unsigned char i, j;

	i = 12;
	j = 169;
	do
	{
		while (--j);
	} while (--i);
}

void alarm_show(void)
{
	temp_buf[0]=(alarm_time[0][2]&0x30)/16 + '0';
	temp_buf[1]=(alarm_time[0][2]&0x0f) + '0';    //ʱ
	temp_buf[2]=':';
	temp_buf[3]=(alarm_time[0][1]&0x70)/16 + '0';
	temp_buf[4]=(alarm_time[0][1]&0x0f) + '0';//��
	temp_buf[5]='\0';
	oled_ascii_8x16_str(58,2,temp_buf);
	temp_buf[0]=(alarm_time[1][2]&0x30)/16 + '0';
	temp_buf[1]=(alarm_time[1][2]&0x0f) + '0';    //ʱ
	temp_buf[2]=':';
	temp_buf[3]=(alarm_time[1][1]&0x70)/16 + '0';
	temp_buf[4]=(alarm_time[1][1]&0x0f) + '0';//��
	temp_buf[5]='\0';
	oled_ascii_8x16_str(58,4,temp_buf);
	if(alarm_time[0][0] == 0x00)
		oled_ascii_8x16_str(104,2,"OFF");
	else
		oled_ascii_8x16_str(104,2," ON");
	if(alarm_time[1][0] == 0x00)
		oled_ascii_8x16_str(104,4,"OFF");
	else
		oled_ascii_8x16_str(104,4," ON");
}
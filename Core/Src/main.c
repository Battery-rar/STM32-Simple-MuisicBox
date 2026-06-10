/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include"stdint.h"
#include "song.h"
#include "font.h"
#include "gpio.h"
#include "i2c.h"
#include "tim.h"
#include "oled.h"
#include "multi_button.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef enum { STOPPED,PLAYING,PAUSED,MACHINE_STOP} State;
typedef enum {Null_Song, Start_Music, Haruhikage, Blossom, Cheerleader } SongList;
typedef enum { Menu1, Menu2,Volume_Control_Menu, Song_Display_Menu } MenuState;

static Button KEY1;
static Button KEY2;
static Button KEY3;
static Button KEY4;

volatile State PlayerState = STOPPED; 
volatile MenuState CurrentMenu;
volatile SongList Selected_Song=Null_Song;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//声明变量
Song *Current_Song_Sheet;			  // 当前播放的歌曲铺
volatile uint32_t Current_Song_Time_Length;	// 当前播放的歌曲时间长度
volatile uint32_t Current_Song_Note_Num;// 当前播放的歌曲的音符数量
volatile uint32_t currentNote=0;   // 当前播放的音符
volatile uint32_t elapsedTime=0;    // 当前音符已经播放的时间
volatile uint32_t current_Play_Time = 0;//当前歌曲播放时间
volatile uint16_t is_Doubled_Playing = 0;//是否快速播放
volatile uint16_t Volume=50;//音量大小
volatile uint16_t is_Move_First_Cursor=0, is_Move_Second_Cursor=0;//是否移动一级光标
volatile uint16_t is_Enter_Secondary_Menu=0,is_Leave_Secondary_Menu=0;//是否进入二级菜单
volatile uint16_t is_Enter_Song=0,is_Leave_Song=0;//是否进入/离开歌曲
volatile uint16_t is_next10s_Playing=0,is_previous10s_Playing=0;//是否快进
volatile uint16_t First_Cursor_Position=7,Second_Cursor_Position=7;

//声明函数
void Player_Start(Song *sheet, uint32_t length, int note_num);
void Player_Pause(void);
void Player_Resume(void);
void Player_Stop(void);
void Player_Update(void);
uint32_t Get_Song_Time_Length(Song sheet[],int note_num);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
//uint8_t read_button_gpio(uint8_t button_id);
void Machine_Start();
void First_Menu();
void Second_Menu();
void Volume_Control();
void Song_Display();

// 播放指定歌曲
void Player_Start(Song *sheet, uint32_t length, int note_num) {
  Current_Song_Sheet = sheet;
  Current_Song_Time_Length = length;
  Current_Song_Note_Num = note_num;
  currentNote = 0;
  elapsedTime = 0;
  current_Play_Time = 0;
  PlayerState = PLAYING;
  return;
}


// 暂停
void Player_Pause(void) {
    PlayerState = PAUSED;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0); // 静音
}

// 继续
void Player_Resume(void) {
  if (PlayerState == PAUSED) {
    PlayerState = PLAYING;
  }
}

// 停止
void Player_Stop(void) {
  PlayerState = STOPPED;
  currentNote = 0;
  elapsedTime = 0;
  current_Play_Time = 0;
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0); // 静音
}

void Set_BPM_Time(int BPM){
	note_time[_1_16]=125*120/BPM;
	note_time[_1_8]=2*note_time[_1_16];
	note_time[_1_4]=2*note_time[_1_8];
	note_time[_1_2]=2*note_time[_1_4];
	note_time[_1_1]=2*note_time[_1_2];
	return;
}

// SysTick 每 1ms 调用一次
void Player_Update(void) {
  if (Current_Song_Sheet == NULL||Selected_Song ==Null_Song) return;

  Song current = Current_Song_Sheet[currentNote];
  uint32_t note_Duration = note_time[current.note_len];

	// 播放音符
	if(current.note_pitch == Rest||PlayerState==PAUSED||PlayerState==STOPPED){
		__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
	}else{
		__HAL_TIM_SET_AUTORELOAD(&htim4,1000000/note_freqs[current.note_pitch]-1); // 重装载值

		if(0<=elapsedTime&&elapsedTime<100)	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (int)1000000 / note_freqs[current.note_pitch]*Volume/5/100); // 占空比
		if(100<=elapsedTime&&elapsedTime<150) __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (int)5*Volume/100); // 占空比
		if(150<=elapsedTime&&elapsedTime<200) __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (int)3*Volume/100);
		if(200<=elapsedTime&&elapsedTime<250) __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (int)1*Volume/100);
		if(elapsedTime>=250) __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
	}

//倍速播放
  if(is_Doubled_Playing==1 && 0 <= current_Play_Time && current_Play_Time < Current_Song_Time_Length && PlayerState==PLAYING){
    elapsedTime+=2;
    current_Play_Time+=2;
  }else if(is_Doubled_Playing==0 && 0<=current_Play_Time && current_Play_Time < Current_Song_Time_Length && PlayerState==PLAYING){
    elapsedTime+=1;
    current_Play_Time+=1;
  }

  // 快进功能
  //快进 10 秒
  if (is_next10s_Playing == 1) {
      is_next10s_Playing = 0; // 恢复标志

      int tmp_Skip_Note = 0;
      int total_Skip_Note_Time = 0;

      while (1) {
          if (total_Skip_Note_Time < 10000) {
              if (tmp_Skip_Note == 0) {
                  total_Skip_Note_Time += note_Duration - elapsedTime;  // 当前音符剩余时间
              } else {
                  // 若快进越界到结尾，直接到末尾退出
                  if (currentNote + tmp_Skip_Note >= Current_Song_Note_Num||current_Play_Time+total_Skip_Note_Time>=Current_Song_Time_Length) {
                      currentNote = Current_Song_Note_Num - 1;
                      current_Play_Time = Current_Song_Time_Length;
                      elapsedTime = note_Duration;
                      break;
                  } else {
                      total_Skip_Note_Time += note_time[Current_Song_Sheet[currentNote + tmp_Skip_Note].note_len];
                  }
              }
              tmp_Skip_Note++;
          } else {
              // 计算快进到的新位置
              currentNote += tmp_Skip_Note - 1;
              elapsedTime = note_time[Current_Song_Sheet[currentNote].note_len] - (total_Skip_Note_Time - 10000);
              if (Current_Song_Time_Length-current_Play_Time > 10000)
            	  current_Play_Time += 10000;
              else
            	  current_Play_Time = Current_Song_Time_Length;
              break;
          }
      }
  }


  // ==================== 后退 10 秒 ====================
  if (is_previous10s_Playing == 1) {
      is_previous10s_Playing = 0; // 恢复标志

      int tmp_Skip_Note = 0;
      int total_Skip_Note_Time = 0;

      while (1) {
          if (total_Skip_Note_Time < 10000) {
              if (tmp_Skip_Note == 0) {
                  total_Skip_Note_Time += elapsedTime;  // 当前音符已播放部分
              } else {
                  // 若后退越界到开头，直接回到第一个音符
                  if (currentNote - tmp_Skip_Note < 0||(int)current_Play_Time-total_Skip_Note_Time<=0) {
                      currentNote = 0;
                      elapsedTime = 0;
                      current_Play_Time = 0;
                      break;
                  } else {
                      total_Skip_Note_Time += note_time[Current_Song_Sheet[currentNote - tmp_Skip_Note].note_len];
                  }
              }
              tmp_Skip_Note++;
          } else {
              // 计算后退到的新位置
              currentNote -=(tmp_Skip_Note - 1);
              elapsedTime =total_Skip_Note_Time - 10000;
              if (current_Play_Time > 10000)
                  current_Play_Time -= 10000;
              else
                  current_Play_Time = 0;
              break;
          }
      }
  }


  // 音符播放完毕，切换到下一个音符
  if (elapsedTime >= note_Duration) {
	elapsedTime = 0;
	currentNote++;
  }

  //歌曲播放结束,重新开始
  if (currentNote >= Current_Song_Note_Num&&Selected_Song!=Start_Music) {
	  elapsedTime =0;
	  currentNote=0;
	  current_Play_Time=0;
      return;
    }
  if (currentNote >= Current_Song_Note_Num&&Selected_Song==Start_Music) {
  	  Player_Stop();
  	  return;
  }
}

//获取歌曲时长
uint32_t Get_Song_Time_Length(Song sheet[],int note_num){

	int Total_Song_Time_Length=0;

	for(int i=0;i<note_num;i++){
		Total_Song_Time_Length+=note_time[sheet[i].note_len];
	}
	return Total_Song_Time_Length;
}

//按键功能
void KEY1_single_click_handler(){

  if(CurrentMenu==Song_Display_Menu){
      if(PlayerState==PLAYING){
        Player_Pause();
      }else if(PlayerState==PAUSED){
        Player_Resume();
      }
    }
}

void KEY1_double_click_handler(){
  if(CurrentMenu==Song_Display_Menu){
      Player_Start(Current_Song_Sheet, Current_Song_Time_Length, Current_Song_Note_Num);
    }

}
void KEY1_long_press_Start_handler(){
	if(Selected_Song==Haruhikage&&CurrentMenu==Song_Display_Menu){
		Selected_Song=Blossom;
		Set_BPM_Time(Blossom_BPM);
		Player_Start((Song*)Blossom_Sheet, Get_Song_Time_Length(Blossom_Sheet,Blossom_Note_Num),Blossom_Note_Num);
	}
	if(Selected_Song==Blossom&&CurrentMenu==Song_Display_Menu){
		Selected_Song=Cheerleader;
		Set_BPM_Time(Cheerleader_BPM);
	    Player_Start((Song*)Cheerleader_Sheet, Get_Song_Time_Length(Cheerleader_Sheet,Cheerleader_Note_Num),Cheerleader_Note_Num);
	}
	if(Selected_Song==Cheerleader&&CurrentMenu==Song_Display_Menu){
	    Selected_Song=Haruhikage;
	    Set_BPM_Time(Haruhikage_BPM);
		Player_Start((Song*)Haruhikage_Sheet, Get_Song_Time_Length(Haruhikage_Sheet,Haruhikage_Note_Num),Haruhikage_Note_Num);
	}

}
void KEY2_single_click_handler(){
	if(CurrentMenu==Song_Display_Menu){
		is_previous10s_Playing=1;

	}
	if(CurrentMenu==Volume_Control_Menu){
		if(Volume-10>=0) Volume-=10;
		if(Volume-10<0) Volume=0;
	}
}

void KEY2_double_click_handler(){
	if(CurrentMenu==Song_Display_Menu||CurrentMenu==Volume_Control_Menu){
	    if(Volume-10>=0) Volume-=10;
	    if(Volume-10<0) Volume=0;
	  }
}

void KEY2_long_press_start_handler(){

}

void KEY3_single_click_handler(){
	if(CurrentMenu==Song_Display_Menu){
	    is_next10s_Playing=1;
	}
	if(CurrentMenu==Volume_Control_Menu){
		if(Volume+10<=100) Volume+=10;
		if(Volume+10>100) Volume=100;
	}
}

void KEY3_double_click_handler(){
	if(CurrentMenu==Song_Display_Menu||CurrentMenu==Volume_Control_Menu){
	    if(Volume+10<=100) Volume+=10;
	    if(Volume+10>100) Volume=100;
	}
}

void KEY3_long_press_hold_handler(){
  if(CurrentMenu==Song_Display_Menu){
    is_Doubled_Playing=1;
  }
}

void KEY3_release_handler() {
	if (CurrentMenu == Song_Display_Menu) {
	   if(is_Doubled_Playing ==1) is_Doubled_Playing = 0;  // 松开 -> 恢复正常播放
    }
}

void KEY4_single_click_handler(){
	if(CurrentMenu==Menu1){
		  if(First_Cursor_Position>=27){
		  	 First_Cursor_Position=7;
		  }else{
			  First_Cursor_Position+=20;
		  }
	  }
	  if(CurrentMenu==Menu2){
		  if(Second_Cursor_Position>=47){
			  Second_Cursor_Position=7;
		  }else{
		      Second_Cursor_Position+=20;
		  }
	  }

}
void KEY4_double_click_handler(){
	if(CurrentMenu==Menu2||CurrentMenu==Volume_Control_Menu){
		is_Leave_Secondary_Menu=1;
	}
	if(CurrentMenu==Song_Display_Menu){
		is_Leave_Song=1;
	}
}

void KEY4_long_press_start_handler(){
	if(CurrentMenu==Menu1){
		is_Enter_Secondary_Menu=1;
	}
	if(CurrentMenu==Menu2){
		is_Enter_Song=1;
	}
}


uint8_t read_button_gpio(uint8_t button_id)
{
    switch (button_id) {
        case 1:
            return HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
        case 2:
        	return HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
        case 3:
            return HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin);
        case 4:
            return HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin);
        default:
            return 0;
    }
}

void Machine_Start(){
  //呼吸灯
  for(int i=0;i<50;i++){
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,i);
    HAL_Delay(10);
  }	
  for(int i=49;i>=0;i--)	{
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,i);
    HAL_Delay(10);
  }
  for(int i=0;i<50;i++){
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,i);
    HAL_Delay(10);
  }	
  for(int i=49;i>=0;i--){
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,i);
    HAL_Delay(10);
  }
  for(int i=0;i<50;i++){
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,i);
    HAL_Delay(10);
  }	
  for(int i=49;i>=0;i--){
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,i);
    HAL_Delay(10);
  }

  //播放开场音乐
  Selected_Song=Start_Music;
  Set_BPM_Time(Start_Music_BPM);
  Player_Start((Song*)Start_Music_Sheet, Get_Song_Time_Length(Start_Music_Sheet,Start_Music_Note_Num), Start_Music_Note_Num);
  //开场动画
  for(int i=0;i<40;i++){
	  OLED_NewFrame();
      OLED_DrawCircle(64,32,i,OLED_COLOR_NORMAL);
      OLED_DrawCircle(64,32,i*3,OLED_COLOR_NORMAL);
      OLED_DrawImage(43,-30+i,&UESTCImg,OLED_COLOR_NORMAL);
      OLED_PrintString(64-1*i,42,"Welcome!",&font16x16,OLED_COLOR_NORMAL);
      OLED_ShowFrame();
      HAL_Delay(70);
  }
  CurrentMenu=Menu1;
  return;
 }

//一级菜单
void First_Menu(){

	  OLED_NewFrame();
    //绘制LIST
	  OLED_DrawFilledRectangle(0,0,60,15,0);
	  OLED_PrintString(3,0,"LIST",&font13x13,1);
    //绘制CONTROL
	  OLED_DrawFilledRectangle(0,20,60,15,0);
	  OLED_PrintString(3,20,"CONTROL",&font15x15,1);
    //绘制光标
	  OLED_DrawLine(115,First_Cursor_Position,120,First_Cursor_Position,0);
	  OLED_DrawLine(115,First_Cursor_Position+1,120,First_Cursor_Position+1,0);
	  OLED_DrawLine(115,First_Cursor_Position+2,120,First_Cursor_Position+2,0); 

	  OLED_ShowFrame();

    if(is_Enter_Secondary_Menu==1&&First_Cursor_Position==7){
      CurrentMenu=Menu2;
      Second_Cursor_Position=7;
      is_Enter_Secondary_Menu=0;
	}

    if(is_Enter_Secondary_Menu==1&&First_Cursor_Position==27){
          CurrentMenu=Volume_Control_Menu;
          is_Enter_Secondary_Menu=0;
    }


    return;
}

//二级菜单
void Second_Menu(){

    OLED_NewFrame();
    //第一首歌
    OLED_DrawFilledRectangle(0,0,80,15,0);
    OLED_PrintString(3,0,"1 Haruhikage",&font13x13,1);
    //第二首歌
    OLED_DrawFilledRectangle(0,20,80,15,0);
    OLED_PrintString(3,20,"2 Blossom",&font13x13,1);
    //第三首歌
    OLED_DrawFilledRectangle(0,40,80,15,0);
    OLED_PrintString(3,40,"3 Cheerleader",&font13x13,1);
    //绘制光标 
    OLED_DrawLine(115,Second_Cursor_Position,120,Second_Cursor_Position,0);
    OLED_DrawLine(115,Second_Cursor_Position+1,120,Second_Cursor_Position+1,0);
    OLED_DrawLine(115,Second_Cursor_Position+2,120,Second_Cursor_Position+2,0);
      
   	OLED_ShowFrame();
    //进入歌曲播放界面
    if(is_Enter_Song==1&&Second_Cursor_Position==7){
      CurrentMenu=Song_Display_Menu;
      Selected_Song=Haruhikage;
      Set_BPM_Time(Haruhikage_BPM);
      Player_Start((Song*)Haruhikage_Sheet, Get_Song_Time_Length(Haruhikage_Sheet,Haruhikage_Note_Num),Haruhikage_Note_Num);
      is_Enter_Song=0;
    }
    if(is_Enter_Song==1&&Second_Cursor_Position==27){
      CurrentMenu=Song_Display_Menu;
      Selected_Song=Blossom;
      Set_BPM_Time(Blossom_BPM);
      Player_Start((Song*)Blossom_Sheet, Get_Song_Time_Length(Blossom_Sheet,Blossom_Note_Num),Blossom_Note_Num);
      is_Enter_Song=0;
    }
    if(is_Enter_Song==1&&Second_Cursor_Position==47){
      CurrentMenu=Song_Display_Menu;
      Selected_Song=Cheerleader;
      Set_BPM_Time(Cheerleader_BPM);
      Player_Start((Song*)Cheerleader_Sheet, Get_Song_Time_Length(Cheerleader_Sheet,Cheerleader_Note_Num),Cheerleader_Note_Num);
      is_Enter_Song=0;
    } 
    //返回一级菜单(长按KEY1)
    if(is_Leave_Secondary_Menu==1){
      HAL_Delay(50);
      CurrentMenu=Menu1;
      is_Leave_Secondary_Menu=0;
    }
  return;
}

void Volume_Control(){
	//显示音量大小
	OLED_NewFrame();
	OLED_DrawRectangle(60, 10, 10, 50, OLED_COLOR_NORMAL);
	OLED_DrawFilledRectangle(62, (int)(12+(100-Volume)*46/100), 6, (int)Volume*46/100, OLED_COLOR_NORMAL);//height:12px-48px--->filled height:volume/100*36
	OLED_ShowFrame();
	if(is_Leave_Secondary_Menu==1){
		HAL_Delay(50);
	    CurrentMenu=Menu1;
	    is_Leave_Secondary_Menu=0;
	}

}
//显示歌曲信息
void Song_Display(){
	 OLED_NewFrame();
    //显示歌曲名称
    if(Selected_Song==Haruhikage){
      OLED_PrintString(5,0,"Haruhikage",&font13x13,0);
    }
    if(Selected_Song==Blossom){
      OLED_PrintString(5,0,"Blossom",&font13x13,0);
    }
    if(Selected_Song==Cheerleader){
      OLED_PrintString(5,0,"Cheerleader",&font13x13,0);
    }
	  
    //显示进度条
    int FilledWidth= current_Play_Time*1.0/Current_Song_Time_Length*108;
    OLED_DrawRectangle(7,54,112,6,OLED_COLOR_NORMAL);
    OLED_DrawFilledRectangle(9,56,FilledWidth,4,OLED_COLOR_NORMAL);
    //显示音量大小
    OLED_DrawRectangle(100, 10, 10, 40, OLED_COLOR_NORMAL);
    OLED_DrawFilledRectangle(102, (int)(12+(100-Volume)*36/100), 6, (int)Volume*36/100, OLED_COLOR_NORMAL);//height:12px-48px--->filled height:volume/100*36
    OLED_ShowFrame();

	//短按 KEY2 返回上一级菜单
	if(is_Leave_Song==1){
      Player_Stop();
      is_Leave_Song=0;
      CurrentMenu=Menu2;
      return;
	}
  return;
}

void timer_5ms_interrupt_handler(void)
{
    button_ticks();

}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TIM3_Init();
  MX_I2C2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_4);
  //按键初始化





  button_init(&KEY1, read_button_gpio, 0, 1);
  button_attach(&KEY1, BTN_SINGLE_CLICK, KEY1_single_click_handler);
  button_attach(&KEY1, BTN_DOUBLE_CLICK, KEY1_double_click_handler);
  button_attach(&KEY1,BTN_LONG_PRESS_START,KEY1_long_press_Start_handler);

  button_init(&KEY2, read_button_gpio, 0, 2);
  button_attach(&KEY2, BTN_SINGLE_CLICK, KEY2_single_click_handler);
  button_attach(&KEY2,BTN_DOUBLE_CLICK,KEY2_double_click_handler);
  button_attach(&KEY2, BTN_LONG_PRESS_START, KEY2_long_press_start_handler);

  button_init(&KEY3, read_button_gpio, 0, 3);
  button_attach(&KEY3, BTN_SINGLE_CLICK, KEY3_single_click_handler);
  button_attach(&KEY3, BTN_DOUBLE_CLICK, KEY3_double_click_handler);
  button_attach(&KEY3, BTN_LONG_PRESS_HOLD, KEY3_long_press_hold_handler);
  button_attach(&KEY3, BTN_PRESS_UP, KEY3_release_handler);

  button_init(&KEY4, read_button_gpio, 0, 4);
  button_attach(&KEY4, BTN_SINGLE_CLICK, KEY4_single_click_handler);
  button_attach(&KEY4, BTN_DOUBLE_CLICK, KEY4_double_click_handler);
  button_attach(&KEY4,BTN_LONG_PRESS_START, KEY4_long_press_start_handler);

  button_start(&KEY1);
  button_start(&KEY2);
  button_start(&KEY3);
  button_start(&KEY4);

  Machine_Start();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while(1){
	  //ButtonEvent debug_btnstate = button_get_event(&KEY1);
	  if(CurrentMenu==Menu1){
	       First_Menu();
	     }
	     if(CurrentMenu==Menu2){
	       Second_Menu();
	     }
	     if(CurrentMenu==Volume_Control_Menu){
	    	Volume_Control();
	     }
	     if(CurrentMenu==Song_Display_Menu){
	       Song_Display();
	     }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

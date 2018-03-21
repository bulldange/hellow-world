#include <LEDMatrix.h>
#include <IRremote.h>

#include <ConfigOnEEPROM.h>

#define _DEBUG_

//#define _MEGA_
#define _UNO_

#define READY        1
#define NOT_READY    0
#define IR_RCVING    2        // 신호를 수신중

#define HEADER_SIZE  2
#define BUTTON_IR_RCV    1    // IR_Button_On
#define BUTTON_NOCHANGE  0    

#define DETECT       1
#define NOT_DETECT   0

#define NOT_DETECT_TIME  4000  // 4초 * 10^3
#define DETECT_RAT       0.8   // 80%

#define ON  0
#define OFF 1

volatile int statusOfSystem = NOT_READY ; // 0: 초기, 1:초기화 완료
volatile int buttonStatus   = BUTTON_NOCHANGE;
int statusOfDetect          = NOT_DETECT;

int ledTimer      = 0;
int notDectectCnt = 0; 
int DectectCnt    = 0; 
/*
 *  button interrupt pin
 */
#ifdef _MEGA_
uint8_t IRRcvInterruptPin = 2;  // pin 21  : 리모컨 신호 수신
uint8_t InitInterruptPin  = 20;  // Interrupt 3  : 초기화
uint8_t IRSndPin          = 9;
#endif

#ifdef _UNO_
uint8_t IRRcvInterruptPin = 0;  // pin 2  : 리모컨 신호 수신
uint8_t InitInterruptPin  = 20;  // Interrupt 3  : 초기화
uint8_t IRSndPin          = 3;
#endif

int distAnalogPin     = A0;  
uint8_t redPinOut     = 4;  // 1초에 두번씩 깜박이는 경우는 초기화 준비, 1초에 3번 깜빡이는 경우 초기화 확인.
uint8_t greenPinOut   = 5;  // 사람을 감지 한 경우 1초에 한번씩 점멸.
uint8_t bluePinOut    = 6;  // 감지하지 못 하는 경우 1초에 한번씩 점멸.
uint8_t triggerPin    = 7;
uint8_t echoPin       = 8;
uint8_t IRRcvPin      = 10;
uint8_t LedCLKpin     = 11;
uint8_t LedCSpin      = 12;
uint8_t LedDINpin     = 13;

unsigned long redDelayTime   = -1;  
unsigned long greenDelayTime = -1;
unsigned long blueDelayTime  = -1;

unsigned long redOldTime,greenOldTime,blueOldTime;

uint8_t redLedStatus;
uint8_t greenLedStatus;
uint8_t blueLedStatus;

volatile unsigned long IRRcvTime = 0; 
int DISTANCE       =  150;  // 100cm
/*
 * 적외선 관련 
 */
int  Sig[3][100]    = {0};
char SigLen[3]      = {0};    // 시그날 크기
int  SigCnt         = 0;      // 시그날 갯수
int  AddrEEPROM     = 0;      // EEOROM 주소
IRrecv irrecv(IRRcvPin);
IRsend irsend;

/*
 *  초음파 관련
 */
unsigned long dectectStartTime=0;

/*
 * LEDMatrix
 */
LEDMatrix Led;

/*
*  Config On EEPROM
*  환경저장하기
*/
ConfigOnEEPROM Config;

int readIRSig()
{  
  decode_results  results;        // Somewhere to store the results
  if (irrecv.decode(&results)) {    // Grab an IR code
    for (int i = 1;  i < results.rawlen;  i++) 
       Sig[SigCnt][i-1] = results.rawbuf[i] * USECPERTICK;      
    SigLen[SigCnt] = results.rawlen-1;  
    irrecv.resume();
    SigCnt++;                    // 시그널 갯수 증가
    return IR_RCVING;            // 데이타를 수신중.       
  }
  return NOT_READY;            
}

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(9600);
  
  IRrecv irrecv(IRRcvPin);
  irrecv.enableIRIn();  // Start the receiver  

  Led.Init_MAX7219(LedCLKpin,LedCSpin,LedDINpin);
  // utrasonic  
  pinMode(triggerPin,OUTPUT);
  pinMode(echoPin,INPUT);

  // led pinMode
  pinMode(redPinOut,OUTPUT);
  pinMode(greenPinOut,OUTPUT);
  pinMode(bluePinOut,OUTPUT);

  // button pinMode
  pinMode(redPinOut,OUTPUT);
  pinMode(greenPinOut,OUTPUT);
  pinMode(bluePinOut,OUTPUT);

  // Interrupt 
  attachInterrupt(IRRcvInterruptPin,IRRcvButtonOn,RISING);

  // Button Status
  buttonStatus=BUTTON_NOCHANGE;
  
  int address=0;
  int r;int i=0;
  do {
    r=Config.readinitFromEEPROM(&address,Sig[i],&SigLen[i]);      
    i++;
  }while(r>0);

  if ( r == 0 )
  {
    statusOfSystem = READY;
    SigCnt = i;
  }

  dectectStartTime = millis();
  redOldTime       = millis();
  greenOldTime     = millis();
  blueOldTime      = millis();
}

void IRRcvButtonOn()
{
   if ( statusOfSystem == NOT_READY )
   {
      Serial.println("IRRcvButtonOn 이미 호출됨 "); 
      return; 
   }
     
   if (millis() - IRRcvTime < 2000)
   {
      Serial.println("IRRcvButtonOn 연속 호출 < 2초 "); 
      return; 
   }

   if ( statusOfSystem == IR_RCVING && SigCnt > 0 )
   {
      statusOfSystem = READY;
      return; 
   }
   
   IRRcvTime   = millis();   
   buttonStatus= BUTTON_IR_RCV; 
   irrecv.enableIRIn();  // Start the receiver 

   SigCnt         = 0;      // 시그날 갯수
   AddrEEPROM     = 0;      // EEOROM 주소    
}

/*
 *  상태 출력
*/
void BlinkLed()
{
 
   unsigned long currentTime=millis();
   if ( redDelayTime > 1 &&redDelayTime != 9999  && redDelayTime < (currentTime - redOldTime) )
   { 
      redLedStatus = (redLedStatus == HIGH ? LOW : HIGH);
      redOldTime=currentTime;
   }
   if ( greenDelayTime > 1 && greenDelayTime != 9999  && greenDelayTime < (currentTime - greenOldTime) )
   {
      greenLedStatus = (greenLedStatus == HIGH ? LOW : HIGH);
      greenOldTime=currentTime;
   }

   if ( blueDelayTime > 1 && blueDelayTime != 9999  && blueDelayTime < (currentTime - blueOldTime) )
   {
      blueLedStatus = (blueLedStatus == HIGH ? LOW : HIGH);
      blueOldTime=currentTime;
   }
           
   digitalWrite(redPinOut  ,redLedStatus);
   digitalWrite(greenPinOut,greenLedStatus); 
   digitalWrite(bluePinOut ,blueLedStatus); 
}

void SetLedTimer(char RGB,unsigned long DelayTime)  // 0 이면 off, 1 = on, 1 보다 크면 딜레이 타임. 9999 변화 없음. 
{
  switch(RGB)
  {
    case 'R':
      redDelayTime   = DelayTime;
      if ( redDelayTime == 9999 )
         ;
      else if ( redDelayTime == 0 ) 
         redLedStatus = LOW;
      else if (redDelayTime == 1 ) 
         redLedStatus = HIGH;
      break;
    case 'G':
      greenDelayTime = DelayTime;
         
      if ( greenDelayTime == 9999 )
         ;
      else if ( greenDelayTime == 0 ) 
         greenLedStatus = LOW;
      else if (greenDelayTime == 1 ) 
         greenLedStatus = HIGH;
    
      break;
      
    case 'B':

      blueDelayTime = DelayTime;

      if ( blueDelayTime == 9999 )
         ;
      else if ( blueDelayTime == 0 ) 
         blueLedStatus = LOW;
      else if (blueDelayTime == 1 ) 
         blueLedStatus = HIGH;
  }
}

void DisplayLed()
{
  if ( statusOfSystem == NOT_READY )
     SetLedTimer('R',500);
  else if ( statusOfSystem == READY )
     SetLedTimer('R',1);

  if ( statusOfDetect == DETECT )
     SetLedTimer('B',1);
  else if ( statusOfDetect == NOT_DETECT )
     SetLedTimer('B',500);

  BlinkLed();  
}

void sendIR(int onOff)
{
  int khz = 38; // 38kHz carrier frequency for the NEC protocol
  if ( onOff == ON )
      irsend.sendRaw(Sig[0], SigLen[0] , khz); //Note the approach used to automatically calculate the size of the array.
   else {
      for (int i=1;i<SigCnt;i++){
        irsend.sendRaw(Sig[i], SigLen[i] , khz); //Note the approach used to automatically calculate the size of the array.   
        delay(2000);
      }
   }
   SetLedTimer('G',(onOff==ON)?1:0);
}

void checkDistance()
{
  int distance;
  unsigned long   currentTime =  millis();

  // put your main code here, to run repeatedly:
  digitalWrite(triggerPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin,LOW);
  distance=pulseIn(echoPin,HIGH)/58;

  float ratio = 0;
  if ( distance <= DISTANCE )
      ratio=(1-1.0*distance/DISTANCE)*8;
  Led.DispProgressBar((int)ratio);
  
  // 일정시간동안 물체를 찾을 수 없으면 끄기 신호를 보낸다.
  if ( statusOfDetect == DETECT )
  {
       if ( distance > DISTANCE )  
          notDectectCnt ++; 
       else
          DectectCnt ++; 

       if ( (currentTime- dectectStartTime ) > NOT_DETECT_TIME )
       {   
        #ifdef _DEBUG_
           Serial.println("DETECT : " +String(DectectCnt) +" " +  String(notDectectCnt) +" "+ String(1.0*notDectectCnt / (notDectectCnt+DectectCnt) ));
        #endif   
           if ( 1.0*notDectectCnt / (notDectectCnt+DectectCnt)  > DETECT_RAT )
           {
             statusOfDetect = NOT_DETECT;
             sendIR(OFF);
           }
           Serial.println("Distance is(cm) " + String(distance)+" "+min((int)((1-1.0*distance/DISTANCE)*8),0));
           Serial.println("DETECT(DISTANCE) (" +String(DISTANCE)+")"+String(DectectCnt) +" " +  String(notDectectCnt) +" "+ String(1.0*notDectectCnt / (notDectectCnt+DectectCnt) ));

           dectectStartTime = currentTime;
           notDectectCnt=0;
           DectectCnt   =0;
       }

    }
    else
    {
       if ( distance <= DISTANCE )      
          DectectCnt ++; 
       else
          notDectectCnt ++; 
       if ( (currentTime- dectectStartTime ) > NOT_DETECT_TIME )
       {   
        #ifdef _DEBUG_
           Serial.println("NOT_DETECT(DISTANCE) (" +String(DISTANCE)+")"+String(DectectCnt) +" " +  String(notDectectCnt) +" "+ String(1.0*DectectCnt / (notDectectCnt+DectectCnt) ));
        #endif   

           if ( 1.0*DectectCnt / (notDectectCnt+DectectCnt)  > DETECT_RAT )
           {
             statusOfDetect = DETECT;
             sendIR(ON);
           }
           Serial.println("Distance is(cm) " + String(distance));
           Serial.println("NOT_DETECT(DISTANCE) (" +String(DISTANCE)+")"+String(DectectCnt) +" " +  String(notDectectCnt) +" "+ String(1.0*DectectCnt / (notDectectCnt+DectectCnt) ));

           dectectStartTime = currentTime;
           notDectectCnt=0;
           DectectCnt   =0;
       }
  
    }
   delay(10);
}

void loop() {
  
  // 초기화가 안되었으면 초기화 코드를 호출한다.
  while ( statusOfSystem == NOT_READY || statusOfSystem == IR_RCVING)
  {
    statusOfSystem=readIRSig();
    if ( statusOfSystem == IR_RCVING )
    {
       Config.writeInitIntoEEPROM(AddrEEPROM,Sig[SigCnt-1],SigLen[SigCnt-1]);
       AddrEEPROM+=HEADER_SIZE + sizeof(int)*SigLen[SigCnt-1];
    }
    DisplayLed();
    Led.DisplayOnLed(0,SigCnt);  
  }

  DISTANCE =  map(analogRead(distAnalogPin),0,1023,2,400);
  Led.DisplayOnLed(DISTANCE/100,(DISTANCE-DISTANCE/100*100)/10);

  // 버튼입력 대기
  if ( buttonStatus == BUTTON_IR_RCV )
  {
     statusOfSystem = NOT_READY;
     buttonStatus   = BUTTON_NOCHANGE;
  }

  if ( statusOfSystem == READY )
    checkDistance();
  
  // NOT DECTECT 상태에서  거리가 탐지거리 이하이면 DECTECT 상태로 변경.
  DisplayLed();
   
}  


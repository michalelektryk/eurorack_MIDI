#include <EEPROM.h>

#define LEARN_CC_PIN_1 51
#define LEARN_CC_PIN_2 52
#define GATE_PIN 53

byte CCdata[2];
byte note;
byte velocity;
bool gate=false;

byte CCnumber[2];

void saveCCToEEPROM(){
  EEPROM.update(0,CCnumber[0]);
  EEPROM.update(1,CCnumber[1]);
}

void loadCCFromEEPROM(){
  CCnumber[0]=EEPROM.read(0);
  CCnumber[1]=EEPROM.read(1);
}

void setup(){
  Serial .begin(115200);
  Serial1.begin(31250);
  loadCCFromEEPROM();
  pinMode(LEARN_CC_PIN_1,INPUT_PULLUP);
  pinMode(LEARN_CC_PIN_2,INPUT_PULLUP);
  pinMode(GATE_PIN,OUTPUT);
}

enum state {idle=0,first,second};

byte dane[3];
bool learnCC[2]={false,false};
state stan=idle;
unsigned char pom;
int notesOn=0;

void parseMessage(){
  switch(dane[0]&0xf0){
    case 0x90:
    if(dane[2]>0)//note on
    {
      Serial.println("note on ");
      note=dane[1];
      velocity=dane[2];
      notesOn++;
      digitalWrite(GATE_PIN,HIGH);
      Serial.print(dane[1]);
      Serial.write('\t');
      Serial.print(dane[2]);
      Serial.write('\n');
      break;
    }//velocity = 0, effectively note off
    case 0x80:
    {
      Serial.println("note off");
      notesOn--;
      if(notesOn==0)
        digitalWrite(GATE_PIN,LOW);
    }
    break;
    case 0xb0:
    {
      for(byte i=0;i<2;i++){
        if(learnCC[i]){
          learnCC[i]=false;
          CCnumber[i]=dane[1];
          saveCCToEEPROM();
          Serial.print("saved CC: ");
          Serial.println(CCnumber[i]);
        }
        if(CCnumber[i]==dane[1]){
          CCdata[i]=dane[2];
          Serial.print("recieved CC data: ");
          Serial.println(CCdata[i]);
        }
      }
    }
    break;
  }
}

void loop(){
  if(Serial1.available()){
    pom=Serial1.read();
    switch(stan){
      case idle:
      {
        if(pom>127){
          dane[0]=pom;
          pom=(pom&0xf0);//usuniecie channel
          if(pom==0x80||pom==0x90||pom==0xa0||pom==0xb0||pom==0xe0)//note on or note off or aftertouch or CC or pitch wheel
            stan=first;
        }
      }
      break;
      case first:
      {
        if(pom>127)
          stan=idle;
        else{
          stan=second;
          dane[1]=pom;
        }
      }
      break;
      case second:
      {
        stan=idle;
        if(pom<128){
          dane[2]=pom;
          parseMessage();
        }
      }
      break;
    }
  }
  learnCC[0]=digitalRead(LEARN_CC_PIN_1)==LOW?true:learnCC[0];
  learnCC[1]=digitalRead(LEARN_CC_PIN_2)==LOW?true:learnCC[1];
}

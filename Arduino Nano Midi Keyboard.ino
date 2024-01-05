#include <Mux.h>
#include <MIDI.h> //menambahkan libraries midi

using namespace admux;
Mux mux(Pin(A0, INPUT_PULLUP, PinType::Digital), Pinset(10, 11, 12, 13)); // multiplexer

// inisialisasi jumlah matriks
#define matrix1 8
#define matrix2 8
#define matrix3 8

// penting biar midi jalan
struct HairlessMidiSettings : public midi::DefaultSettings
{
   static const bool UseRunningStatus = false;
   static const long BaudRate = 115200;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, HairlessMidiSettings);
//MIDI_CREATE_DEFAULT_INSTANCE();

// menghitung waktu untuk velocity
unsigned long waktu;
unsigned long waktuMulai[matrix1 * matrix2];
unsigned long waktuSelesai[matrix1 * matrix2];
unsigned long totalWaktu[matrix1 * matrix2];

bool mulaiWaktu [matrix1 * matrix2]; // untuk memulai waktu setelah 1 tombol tuts di tekan

// velocity
unsigned long rangeBawah = 2552;
unsigned long rangeAtas = 80000;
unsigned long rangeVelocity ;

byte velocity;

bool pedal;
bool pedalState;

byte mulaiNote = 36; // note awal

// untuk input pullup tuts
bool currentState3[matrix1 * matrix2];
bool currentState2[matrix1 * matrix2];
bool requestState3[matrix1 * matrix2];
bool requestState2[matrix1 * matrix2];

byte bacaBaris[8][2] = {
    {0b00000100,0b11111100},
    {0b00001000,0b11111100},
    {0b00010000,0b11111100},
    {0b00100000,0b11111100},
    {0b01000000,0b11111100},
    {0b10000000,0b11111100},
    {0b00000000,0b11111101},
    {0b00000000,0b11111110},
    };

void setup() 
{
  //inisialisasi multiplexer sebagai input pullup digital
  //mux.signalPin(A1, INPUT_PULLUP, DIGITAL);

  DDRD = 0b00000000; //set port d ke input pin 2,3,4,5,6,7
  DDRB = 0b11111100; // set port b ke input pin 8,9
  pinMode(A1,INPUT_PULLUP);
  /*/matriks 1
  pinMode(2,INPUT_PULLUP); //1
  pinMode(3,INPUT_PULLUP); //2
  pinMode(4,INPUT_PULLUP); //3
  pinMode(5,INPUT_PULLUP); //4
  pinMode(6,INPUT_PULLUP); //5
  pinMode(7,INPUT_PULLUP); //6
  pinMode(8,INPUT_PULLUP); //7
  pinMode(9,INPUT_PULLUP); //8
  */
  Serial.begin(115200);
  
  MIDI.begin();
  
}

void loop() 
{
  readKeys();
  sustain();
  writeKeys();

}

void readKeys()
{
  waktu = micros(); //memulai waktu untuk velocity
  for(byte i=0; i<matrix2; i++)
  {
    DDRD = bacaBaris[i][0];
    DDRB = bacaBaris[i][1];
    /*/---------matrix1 pins-----------
    pinMode(2,INPUT);
    pinMode(3,INPUT);
    pinMode(4,INPUT);
    pinMode(5,INPUT);
    pinMode(6,INPUT);
    pinMode(7,INPUT);
    pinMode(8,INPUT);
    pinMode(9,INPUT);    
    */
    
    // pembacaan input matriks 2 tombol tuts atas
    requestState2[i] = !mux.read(0);
    requestState2[i+8*1] = !mux.read(1);
    requestState2[i+8*2] = !mux.read(2);
    requestState2[i+8*3] = !mux.read(3);
    requestState2[i+8*4] = !mux.read(4);
    requestState2[i+8*5] = !mux.read(5);
    requestState2[i+8*6] = !mux.read(6);
    requestState2[i+8*7] = !mux.read(7);

    for(byte j=0; j< matrix2; j++)
    {
      //Serial.print(requestState2[i+8*j]);
      if (requestState2[i+8*j] == true)
      {
        
        // pembacaan input matriks 3 tombol tuts bawah
        requestState3[i+8*j] = !mux.read(j+8);
        /*requestState3[i+8*1] = !mux.read(9);
        requestState3[i+8*2] = !mux.read(10);
        requestState3[i+8*3] = !mux.read(11);
        requestState3[i+8*4] = !mux.read(12);
        requestState3[i+8*5] = !mux.read(13);
        requestState3[i+8*6] = !mux.read(14);
        requestState3[i+8*7] = !mux.read(15);*/
      }
    }
  }
}

void writeKeys()
{
  
  
  for(byte i=0; i<matrix1 * matrix2; i++)
  {
    if(requestState2[i]==true  && currentState2[i] == false && mulaiWaktu[i] == false)
    {
      waktuMulai[i] = waktu; // waktu mulai di rekam dalam variabel
      mulaiWaktu[i] = true;
      currentState2[i] = true;
    }
    if (requestState2[i] == true && requestState3[i] == true && currentState3[i] == false && currentState2[i] == true && mulaiWaktu[i] == true)
       {
         waktuSelesai[i] = waktu; // waktu selesai di rekam
         totalWaktu[i] = waktuSelesai[i] - waktuMulai[i];
         //Serial.println(totalWaktu[i]);
         //Serial.println(i);
         velocity = pembagianRange(totalWaktu[i]);
         //Serial.println(velocity); 
         MIDI.sendNoteOn(i + mulaiNote, velocity, 1); // kirim sinyal midi
         
         mulaiWaktu[i] = false;
         currentState3[i] = true;
       }
    if(requestState2[i] == false && requestState3[i] == false && mulaiWaktu[i] == true && currentState3[i] == false && currentState2[i] == true)
      {
        mulaiWaktu[i] = false;
        currentState2[i] = false;
      }
    if(requestState2[i] == false && currentState2[i] == true && currentState3[i] == true )
    {
      //Serial.println("-");
      MIDI.sendNoteOff(i + mulaiNote, 0, 1);
      currentState2[i] = false;
      currentState3[i] = false;

    }
    
  }
  
}
void sustain()
{
  pedal = !digitalRead(A1);
  
  //Serial.print(pedal);
  if (pedal == true && pedalState == false )
  {
    MIDI.sendControlChange(64,127,1);
    pedalState = true;
  }else if(pedal == false && pedalState == true)
  {
    MIDI.sendControlChange(64,0,1);
    pedalState = false;
  }
  
}

int pembagianRange (long hitung)
{
  if (hitung > rangeBawah && hitung < rangeAtas)
  {
    rangeVelocity = map(hitung, rangeBawah,rangeAtas,115,0); // mapping velocity
    return rangeVelocity;
  }else if (hitung > rangeAtas)
  {    
    return 0;   
  }else{
    return 120;
  }
}

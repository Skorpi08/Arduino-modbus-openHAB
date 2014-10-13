#include <SimpleModbusSlave.h>

/***************************************************
 * / Input/output pins
/***************************************************/
#include <OneWire.h>
OneWire  ds(A3);
#define PIN_TASTER       9

#define ON  8
#define OFF 7

#define INTERVAL 1000 
#define BUTTON_DEBOUNCE_DELAY 20
long lastDebounceTime = 0;
long previousMillis = 0;


/***************************************************
 * / Modbus globals
/***************************************************/
#define MB_TXENPIN   A2
#define MB_SLAVE_ID  2
#define MB_BAUDRATE  9600
#define MB_PARITY  SERIAL_8N2


//////////////// registers of your slave ///////////////////
enum 
{     
  // just add or remove registers and your good to go...
  // The first register starts at address 0
  LICHT,
  CELSIUS,
  HOLDING_REGS_SIZE // leave this one
  // total number of registers
  // i.e. the same address space
};

unsigned int holdingRegs[HOLDING_REGS_SIZE]; // function 3 and 6 register array



////////////////////////////////////////////////////////////

void setup()
{
    // configure pins
  pinMode(PIN_TASTER, INPUT);
  pinMode(ON, OUTPUT);  
  pinMode(OFF, OUTPUT); 
  
  modbus_configure(&Serial, MB_BAUDRATE, MB_PARITY, MB_SLAVE_ID, MB_TXENPIN, HOLDING_REGS_SIZE, holdingRegs);
  // modbus_update_comms(baud, byteFormat, id) is not needed but allows for easy update of the
  // port variables and slave id dynamically in any function.
  modbus_update_comms(MB_BAUDRATE, MB_PARITY, MB_SLAVE_ID);

}

void loop()
{  
  
    modbus_update();
  byte i;
  byte present = 0;
  byte type_s =0;
  byte data[12];
  byte addr[8] = {0x28, 0x97, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00}; // Adresse vom DS18B20 eingeben
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
  
//  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  if(millis() - previousMillis < INTERVAL) return;
    previousMillis = millis();   
 present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  Serial.print(present,HEX);
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
  }

  // convert the data to actual temperature
  unsigned int raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // count remain gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
holdingRegs[CELSIUS] = raw*625L/1000;  // Zwischenrechnung ist long, Ergebnis passt in int
  

  // modbus_update() is the only method used in loop(). It returns the total error
  // count since the slave started. You don't have to use it but it's useful
  // for fault finding by the modbus master.
  
  
   if ((digitalRead(PIN_TASTER) == HIGH) && ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY))
  {
    // invert LED state
    holdingRegs[LICHT] = !holdingRegs[LICHT];

    lastDebounceTime = millis();
  }

    if (holdingRegs[LICHT]==1){
      // ON //
  digitalWrite(ON,1);
  digitalWrite(OFF,0);
  delay(10);
  digitalWrite(ON,0);
  digitalWrite(OFF,0);
} 
  else{
    // OFF //
  digitalWrite(ON,0);
  digitalWrite(OFF,1);
  delay(10);
  digitalWrite(ON,0);
  digitalWrite(OFF,0); 
}

}


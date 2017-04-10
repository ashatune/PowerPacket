/*
  Handles control of relays, analog sensors, and reading and writing of SD card data
  @Author: Mark Pinion
  @Last Revision: April 8 2017
 */
 
#include <Wire.h>
float voltageReading;
float currentReading;
float powerReading;
char incomingByte;
word handle;
int timeSlice;  //Affects much of how the code will operate. Determines how often certain things will happen (sensor sampling, checking for input, etc...)
vector<float> voltageDataBuffer;
vector<float> currentDataBuffer;
vector<float> powerDataBuffer;
#define timeBetweenSamples 1000 //CHANGE THIS NUMBER BASED ON HOW OFTEN SAMPLES ARE COLLECTED (MAKE SURE TO NOTE UNITS)

//Some declarations for the current sensor module
int mVperAmp = 185; // use 185 for 5A Module, 100 for 20A Module and 66 for 30A Module
int RawValue= 0;
int ACSoffset = 2500; 
float Voltage = 0;

//Define which pins on the arduino board will control which output lines on the relay module board CHANGE THESE DEPENDING ON HOW RELAYS ARE PLUGGED IN
#define relayOut1  7
#define relayOut2  6
#define relayOut3  5
#define relayOut4  4
#define voltageSensPin 1  //CHANGE THIS NUMBER BASED ON WHAT PIN THE VOLTAGE SENSOR IS CONNECTED TO
#define currentSensPin 2 //CHANGE THIS NUMBER BASED ON WHAT PIN THE CURRENT SENSOR IS CONNECTED TO


void setup() {
  // Open serial communications and wait for port to open (not needed in final project, mainly for debugging purposes)
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  

  Serial.println("Goodnight moon!");  //This will print when serial connection is established (for debugging purposes)

  //Configure the GPIO pins that control the relay module as output pins
  pinMode(relayOut1,OUTPUT);
  pinMode(relayOut2,OUTPUT);
  pinMode(relayOut3,OUTPUT);
  pinMode(relayOut4,OUTPUT);

}


void loop() { // run over and over

  voltageReading = getVoltageReading(); //Get a voltage reading from the voltage sensor (units in Volts)
  currentReading = getCurrentReading(); //Get a current reading from the current sensor (units in Amps)
  powerReading = voltageReading * currentReading; //Calculate power reading based on current and voltage readings (units in Watts)


  //Will print current iterations values for the current and voltage sensors to the serial terminal (for debugging purposes)
  Serial.print("Voltage Reading: ");
  Serial.print(voltageReading);
  Serial.println(" Volts");
  Serial.print("Current Reading: ");
  Serial.print(currentReading);
  Serial.println(" Amps");
  Serial.print("Power Reading: ");
  Serial.print(powerReading);
  Serial.println(" Watts");

  // Generates the string of data to be written to the SD card
  String dataString = generateStringToWrite(voltageReading,currentReading,powerReading);

  // Write the data string to a txt file on the SD card and then close the file
  handle = Display.file_Open("DATA_LOG.txt",'a');
  delay(2000);
  Display.file_Write(15,dataString,handle);
  Display.file_Close(handle);
  delay(2000);


/*
 * If this flag is toggled on, that means we want to read from the card. Function call needs the row in the txt file we want to start at (essentially what time we want to start looking at) 
 * and the number of points to be looked at (could be a day, week, hour...) - Stores the values in the 3 vectors initialized earlier
 */
  if(readFlag){
    readFromSDCard(index,numPoints,voltageDataBuffer,currentDataBuffer,powerDataBuffer);
  }

     
   
/*  
 *   Code below handles relay controls. Current set up to accept input from serial terminal.
 *   TO-DO: Make button presses and wireless communications control relays.
 */
if (Serial.available() > 0) {
		// read the incoming byte
		incomingByte = Serial.read();

		// Print incoming byte in serial terminal (for debugging purposes)
		Serial.print("I received: ");
		Serial.println(incomingByte);
		
		// Switch statement to control the relay modules based on serial input
		switch(incomingByte){
		    
		    case '1': digitalWrite(relayOut1,!(digitalRead(relayOut1)));break;
		    case '2': digitalWrite(relayOut2,!(digitalRead(relayOut2)));break;
		    case '3': digitalWrite(relayOut3,!(digitalRead(relayOut3)));break;
		    case '4': digitalWrite(relayOut4,!(digitalRead(relayOut4)));break;
		}
	}

 delay(timeSlice); //Delay parameter *ALTER THIS TO CHANGE HOW OFTEN SENSORS GET POLLED*
   
}

/*
 * @Return (a sensor reading from the analog voltage sensor in the form of a float)
 */
float getVoltageReading(){
  float vSensorVal = analogRead(voltageSensPin);
  vSensorVal = vSensorVal / 40.4;
  return vSensorVal;
}

/*
 * @Return (a sensor reading from the analog current sensor in the form of a float)
 */
float getCurrentReading(){
  RawValue = analogRead(currentSensPin);
  Voltage = (RawValue / 1023.0) * 5000; // Gets you mV
  float iSensorVal = ((Voltage - ACSoffset) / mVperAmp);
  return iSensorVal;
}

/*
 * @Param (voltage value to be put into string, current value to be put into string, power value to be put into string)
 * @Return a string that will be written to the SD card for the purposes of data storage
 * String will appear in the following form: v.vv,i.ii,p.pp where v represents the voltage values, i represents the current values, and p represents the power values. Newline character is also appended to the end of each string.
 */
String generateStringToWrite(float v, float i, float p){
  String toWrite = String(v,2) + ",";
  toWrite += String(i,2) + ",";
  toWrite += String(p,2) + 0xA;
  return toWrite;
}

/*
 * @Param (an integer representing the row in the text file to be read, the number of data points to be read from the text file,a pointer to the location where the data points will be stored)
 * @Return (a string holding the data in a single line)
 * file_Seek places the file pointer at the appropriate place in the file based on what the value of rowIndex passed to the function is
 */

void readFromSDCard(int rowIndex, int numDataPoints, vector<float> &vBuffer, vector<float> &iBuffer, vector<float> &pBuffer){
  word fileHandle = Display.file_Open("DATA_LOG.txt",r);  //Opens the file
  char* nxtPtr;
  float voltageData, currentData, powerData;
  Display.file_Seek(fileHandle,0,(rowIndex*15));          //Set the file pointer to the correct place in the file
  for(int i = 0; i < numDataPoints; i++)
  {
  Display.file_GetS(readData,15,handle);                  //Grabs a line from the text file, containing a triple in the form of v.vv,i.ii,p.pp (voltage,current,and power)
  voltageData = strtof(readData,&nxtPtr);                 //Takes the voltage portion of the string and converts it to a float value, nxtPtr points to the beginning of the rest of the string
  currentData = strtof(nxtPtr, &nxtPtr);                  //Takes the current portion of the string and converts it to a float value, nxtPtr points to the beginning of the rest of the string
  powerData = strtof(nxtPtr, NULL);                       //Takes the power portion of the string and converts it to a float value
  vBuffer.push_back(voltageData);                         //Fills the data buffers with the values just obtained
  iBuffer.push_back(currentData);
  pBuffer.push_back(powerData);
  }
  Display.file_Close(fileHandle);
  return readData;
}

/*
 * @Param (a reference to the vector containing the data to be averaged, passed as constant reference since we do not want to change any values in the buffer)
 * @Return (a float equal to the average value of the data in the vector)
 * 
 */
float getAvgValue(vector<float> const &Buffer){
  int accumulator = 0;
  for(int i=0; i < Buffer.size(); i++){
    accumulator+=Buffer[i];
  }
  float avg = accumulator / Buffer.size();
  return avg;
}


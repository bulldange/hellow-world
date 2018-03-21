// these constants describe the pins. They won't change:
const int xpin = A1;                  // x-axis of the accelerometer
const int ypin = A2;                  // y-axis
const int zpin = A3;                  // z-axis (only on 3-axis models)
 
int sampleDelay = 500;   //number of milliseconds between readings
void setup()
{
  // initialize the serial communications:
  Serial.begin(9600);
 
  //Make sure the analog-to-digital converter takes its reference voltage from
  // the AREF pin
  analogReference(EXTERNAL);
 
  pinMode(xpin, INPUT);
  pinMode(ypin, INPUT);
  pinMode(zpin, INPUT);
}
 
void loop()
{
  int x = analogRead(xpin);
 
  //add a small delay between pin readings.  I read that you should
  //do this but haven't tested the importance
    delay(1);
 
  int y = analogRead(ypin);
 
  //add a small delay between pin readings.  I read that you should
  //do this but haven't tested the importance
    delay(1);
 
  int z = analogRead(zpin);
 
  //zero_G is the reading we expect from the sensor when it detects
  //no acceleration.  Subtract this value from the sensor reading to
  //get a shifted sensor reading.
  float zero_G = 512.0;
 
  //scale is the number of units we expect the sensor reading to
  //change when the acceleration along an axis changes by 1G.
  //Divide the shifted sensor reading by scale to get acceleration in Gs.
  float scale = 102.3;
 
/*  Serial.print(((float)x - zero_G)/scale);
  Serial.print("t");
 
  Serial.print(((float)y - zero_G)/scale);
  Serial.print("t");
 
  Serial.println(((float)z - zero_G)/scale);
  */
  float xsintheta =constrain(mapinfloat(x,403,608,-1,1),-1,1);
  float ysintheta =constrain(mapinfloat(y,403,608,-1,1),-1,1);
  float zsintheta =constrain(mapinfloat(z,403,608,-1,1),-1,1);

  float xtheta =asin(xsintheta)*180/PI;
  float ytheta =asin(ysintheta)*180/PI;
  float ztheta =asin(zsintheta)*180/PI;

 Serial.println(String(xtheta) + ","+String(ytheta)+","+String(ztheta));      
 Serial.println(String(x) + ","+String(y)+","+String(z)); 
  // delay before next reading:
  delay(sampleDelay);
}

float mapinfloat(float i,float imin,float imax,float omin,float omax){
  float o=(i-imin)*(omax-omin)/(imax-imin)+omin;
  return o;
}


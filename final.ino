#include  <WiFi.h>
#include <FirebaseESP32.h>
#include <Servo.h>

#include "DHT.h"
#include <NTPClient.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>

//flotteur
#define  flotteur 12
//FIREBASE

#define FIREBASE_HOST "https://enit-smart-garden-default-rtdb.firebaseio.com"
#define WIFI_SSID "Aziz Gaddour" // Change the name of your WIFI
#define WIFI_PASSWORD "20182018" // Change the password of your WIFI
#define FIREBASE_Authorization_key "dqcVeK2sLUnEaOteSnnDa1rl8eBwYfK7RWbcfUvi"

FirebaseData firebaseData;
FirebaseJson json;

WiFiUDP ntpUDP;
NTPClient temps(ntpUDP, "fr.pool.ntp.org", 3600, 60000);

#define DHTPIN 14  

#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);

const int trigPin = 5;
const int echoPin = 18;

#define SOUND_SPEED 0.034

long duration;
float distanceCm;
float distancePercent,distancePercentStock;

float tabtemp1[5]= {0,0,0,0,0};
float tabtemp2[5]= {0,0,0,0,0};
float tablevel[5]= {0,0,0,0,0};

#define  RELAIS_POMPE 4
#define  RELAIS_LAMPES_AV 17
#define  RELAIS_LAMPES_AR 16
#define  RELAIS_GENERAL 0
#define  BUTTON_AV 26
#define  BUTTON_AR 27
#define  BAT_AV 35
#define  BAT_GEN 34
#define  BUZZER 15
Servo myservo1,myservo2;

String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

int LumAr,LumAv,Robinet,ButtonAr = 0,ButtonAv = 0;

unsigned long temps_depart;
int etat_FLOT;
int pos;
float Bat_Av,Bat_Ar, Bat_Gen;


unsigned long  epochTime;
  int heure, minutes, seconde;
  int day, mois, annee;
  String mois_chaine;



void setup() {

 Serial.begin(115200);
   WiFi.begin (WIFI_SSID, WIFI_PASSWORD);
   Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  Firebase.begin(FIREBASE_HOST,FIREBASE_Authorization_key);
  
  temps.begin();
  pinMode(BUTTON_AV, INPUT);
  pinMode(BUTTON_AR, INPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(flotteur, INPUT);
  pinMode(BAT_GEN, INPUT);
  pinMode(BAT_AV, INPUT);
  pinMode(RELAIS_POMPE, OUTPUT);
  pinMode(RELAIS_LAMPES_AV, OUTPUT);
  pinMode(RELAIS_LAMPES_AR, OUTPUT);
  pinMode(RELAIS_GENERAL, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  myservo1.attach(32);
  myservo2.attach(33);

  dht.begin();
}

void loop() 
{
  Temperature();
  Flotteur();
  Niveau();
  Acquisition();
  Vanne();
  Arrosage();
  Battery();
  Allumer1();
  Allumer2();

  delay (200);
}

void Battery()
{
Bat_Av = (digitalRead(BAT_AV)/4095)*37.5;
  Bat_Gen = (digitalRead(BAT_AV)/4095)*37.5;
  Bat_Ar = Bat_Gen - Bat_Av ;
  Firebase.setFloat(firebaseData, "/Enit/Bat_Av", Bat_Av);
  Firebase.setFloat(firebaseData, "/Enit/Bat_Ar", Bat_Ar);
  Firebase.setFloat(firebaseData, "/Enit/Bat_Gen", Bat_Gen);

    Serial.println(Bat_Av);

  // Alarme et coupure recharge par panneau  
  while (Bat_Av > 12.5)
  {
    Serial.println("programme lumiere");
    digitalWrite(RELAIS_GENERAL, LOW);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(RELAIS_LAMPES_AV, HIGH);
    digitalWrite(RELAIS_LAMPES_AR, HIGH);
    Temperature();
    Flotteur();
    Niveau();
    Acquisition();
    Vanne();
    Arrosage();
    Allumer1();
    Allumer2();
  }
}

void Allumer1()
{ 
    if (Firebase.RTDB.getInt(&firebaseData, F("/Enit/LumAv")))
   {
   String(firebaseData.to<int>()).c_str();
   LumAv =  firebaseData.intData();
   Serial.print("lumAv = ");
    Serial.println(LumAv);

   }
   //Lecture des boutons posés sur la table 
      ButtonAv = digitalRead(BUTTON_AV);
  //Allumer les lampes 
  if (( ButtonAv == HIGH)||(LumAv == 1)) 
  {
          Serial.println("programme lumiere av");
    Serial.println("button 1 activé");
    temps_depart = millis();
    digitalWrite(RELAIS_LAMPES_AV, HIGH);
    while ((millis() - temps_depart) <= 600)
    {
    Temperature();
    Flotteur();
    Niveau();
    Acquisition();
    Vanne();
    Arrosage();     
    Allumer2();
    Battery();
    }
    digitalWrite(RELAIS_LAMPES_AV, LOW);
    Firebase.setInt(firebaseData, "/Enit/LumAv", 0);
    }
}
void Allumer2()
{
  if (Firebase.RTDB.getInt(&firebaseData, F("/Enit/LumAr")))
   {
   String(firebaseData.to<int>()).c_str();
   LumAr =  firebaseData.intData();
   Serial.print("lumAr = ");
   Serial.println(LumAr);

   }
      ButtonAr = digitalRead(BUTTON_AR);
  if (( ButtonAr == HIGH)||(LumAr == 1))  
  {
              Serial.println("programme lumiere ar");

    Serial.println("button 2 activé");
    temps_depart = millis();
    digitalWrite(RELAIS_LAMPES_AR, HIGH);
    while ((millis() - temps_depart) <= 600)
    {
    Temperature();
    Flotteur();
    Niveau();
    Acquisition();
    Vanne();
    Arrosage();      
    Allumer1();
    Battery();
    }
    digitalWrite(RELAIS_LAMPES_AR, LOW);
    Firebase.setInt(firebaseData, "/Enit/LumAr", 0);
    }         
}     
void Vanne()
{
  if (Firebase.RTDB.getInt(&firebaseData, F("/Enit/Robinet")))
  {
    String(firebaseData.to<int>()).c_str();
    Robinet =  firebaseData.intData();
    Serial.print("Robinet=");
    Serial.println(Robinet);

  }
  if(Robinet == 1)
  {
    for (pos = 0; pos <= 90; pos ++) 
    { 
      myservo1.write(pos);   
      myservo2.write(pos);                 
      delay(15); 
    }
  }
  else
  {
    for (pos = 90; pos >= 0; pos --) 
    { 
      myservo1.write(pos);   
      myservo2.write(pos);                 
      delay(15); 
    }
  }  
}
void Flotteur ()
{
  etat_FLOT = digitalRead(flotteur);
  if (etat_FLOT == 1)
    {
      digitalWrite(RELAIS_POMPE,HIGH);
    }
  else  
    {
    digitalWrite(RELAIS_POMPE,LOW);
    }
}
void Temps ()
{
  //Temps et date 
  temps.update();
  epochTime = temps.getEpochTime();
  heure = temps.getHours();
  minutes = temps.getMinutes();
  seconde = temps.getSeconds();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  day = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(day);

  mois = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(mois);

  mois_chaine = months[mois-1];
  Serial.print("Month name: ");
  Serial.println(mois_chaine);

  annee = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(annee);
}

void Acquisition ()
{
  Temps();
  //acquisition du niveau d'eau chaque jour:
  if (heure == 12 && minutes ==0 && (seconde == 0 || seconde == 1 || seconde == 2|| seconde == 3 ))
  { 
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    
    // Calculate the distance
    distanceCm = duration * SOUND_SPEED/2;
    Serial.print("distance: ");
    Serial.println(distanceCm);
    distancePercentStock = 100 - distanceCm/70*100;
    tablevel[4] = tablevel[3];
    tablevel[3] = tablevel[2];
    tablevel[2] = tablevel[1];
    tablevel[1] = tablevel[0];
    tablevel[0] = (int)distancePercentStock;
    Firebase.setInt(firebaseData, "/Enit/jour1c", tablevel[0]);
    Firebase.setInt(firebaseData, "/Enit/jour2c", tablevel[1]);
    Firebase.setInt(firebaseData, "/Enit/jour3c", tablevel[2]);
    Firebase.setInt(firebaseData, "/Enit/jour4c", tablevel[3]);
    Firebase.setInt(firebaseData, "/Enit/jour5c", tablevel[4]);
  }
  //acquisition de la température chaque jour à 12h :
  if (heure == 12 && minutes ==0 && (seconde == 0 || seconde == 1 || seconde == 2|| seconde == 3))
  { 
   float tempstock1 = dht.readTemperature();  
   tabtemp1[4] = tabtemp1[3];
   tabtemp1[3] = tabtemp1[2];
   tabtemp1[2] = tabtemp1[1];
   tabtemp1[1] = tabtemp1[0];
   tabtemp1[0] = tempstock1;
    Firebase.setFloat(firebaseData, "/Enit/jour1a", tabtemp1[0]);
    Firebase.setFloat(firebaseData, "/Enit/jour2a", tabtemp1[1]);
    Firebase.setFloat(firebaseData, "/Enit/jour3a", tabtemp1[2]);
    Firebase.setFloat(firebaseData, "/Enit/jour4a", tabtemp1[3]);
    Firebase.setFloat(firebaseData, "/Enit/jour5a", tabtemp1[4]);
  }

//acquisition de la température chaque jour à 00h :

  if (heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 || seconde == 2 || seconde == 3))
  { 
    float tempstock2 = dht.readTemperature();  
    tabtemp2[4] = tabtemp2[3];
    tabtemp2[3] = tabtemp2[2];
    tabtemp2[2] = tabtemp2[1];
    tabtemp2[1] = tabtemp2[0];
    tabtemp2[0] = tempstock2;
    Firebase.setFloat(firebaseData, "/Enit/jour1b", tabtemp2[0]);
    Firebase.setFloat(firebaseData, "/Enit/jour2b", tabtemp2[1]);
    Firebase.setFloat(firebaseData, "/Enit/jour3b", tabtemp2[2]);
    Firebase.setFloat(firebaseData, "/Enit/jour4b", tabtemp2[3]);
    Firebase.setFloat(firebaseData, "/Enit/jour5b", tabtemp2[4]);
  }
}

void Arrosage()
{
  Temps();
  //Arrosage automatique 
  if ((mois == 1 && day == 2) || (mois == 1 && day == 9) || (mois == 1 && day == 16) || (mois == 1 && day == 23) || (mois == 1 && day == 30) )
  {
   if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
   {
    for (pos = 0; pos <= 90; pos += 1) 
    { 
      myservo1.write(pos);   
      myservo2.write(pos);                 
      delay(15); 
    }
   }
   if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
   {
    for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 2 && day == 6)|| (mois == 2 && day == 13)|| (mois == 2 && day == 20 )|| (mois == 2 && day == 27))
  {
   if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
   {
    for (pos = 0; pos <= 90; pos += 1) 
    { 
      myservo1.write(pos);   
      myservo2.write(pos);                 
      delay(15); 
    }  
   }
  if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
    for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }

  if ((mois == 12 && day == 4) || (mois == 12 && day == 11 )|| (mois == 12 && day == 18 )|| (mois == 12 && day == 25))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }  
    }
  if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
    for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 11 && day == 6) || (mois == 11 && day == 13)|| (mois == 11 && day == 20)|| (mois == 11 && day == 27))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
    for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
       delay(15); 
      } 
    }
    if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 10 && day == 2) || (mois == 10 && day == 9 )|| (mois == 10 && day == 16 )|| (mois == 10 && day == 23 )|| (mois == 10 && day == 30))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                
        delay(15); 
      } 
    }
  if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
    for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 9 && day == 2 )|| (mois == 9 && day == 4 )||( mois == 9 && day == 11) || (mois == 9 && day == 18 )|| (mois == 9 && day == 25))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      } 
    }
    if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
      {
      for (pos = 90; pos >= 0; pos --) 
        { 
          myservo1.write(pos);   
          myservo2.write(pos);                 
          delay(15); 
        }
    }
  }
  
  if ((mois == 3 && day == 3) ||( mois == 3 && day == 6 )|| (mois == 3 && day == 10 )|| (mois == 3 && day == 13 )|| (mois == 3 && day == 17) ||( mois == 3 && day == 20) ||( mois == 3 && day == 24) || (mois == 3 && day == 27) || (mois == 3 && day == 31))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 4 && day == 3 )||( mois == 4 && day == 7) ||( mois == 4 && day == 10) ||( mois == 4 && day == 14) ||( mois == 4 && day == 17 )||( mois == 4 && day == 21) ||( mois == 4 && day == 24) ||( mois == 4 && day == 28))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
    if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
      {
      for (pos = 90; pos >= 0; pos --) 
        { 
          myservo1.write(pos);   
          myservo2.write(pos);                 
          delay(15); 
        }
      }
  }
  
  if ((mois == 5 && day == 1 )||( mois == 5 && day == 3 )|| (mois == 5 && day == 6 )|| (mois == 5 && day == 8) || (mois == 5 && day == 10 )|| (mois == 5 && day == 13) || (mois == 5 && day == 15 )|| (mois == 5 && day == 17) ||( mois == 05 && day == 20 )||( mois == 5 && day == 22) ||( mois == 5 && day == 24) ||( mois == 5 && day == 27 )|| (mois == 5 && day == 29 )||( mois == 5 && day == 31))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
    if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 6 && day == 3 )||( mois == 6 && day == 5) || (mois == 6 && day == 8 ) || (mois == 6 && day == 10) ||( mois == 6 && day == 12 )|| (mois == 6 && day == 14 )|| (mois == 6 && day == 17) || (mois == 6 && day == 19 )||( mois == 6 && day == 21 )||( mois == 6 && day == 24) || (mois == 6 && day == 26) || (mois == 6 && day == 28))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 7 && day == 1 )|| (mois == 7 && day == 3)|| (mois == 7 && day == 5 )|| (mois == 7 && day == 7 )|| (mois == 7 && day == 9 )|| (mois == 7 && day == 10 )|| (mois == 7 && day == 12 )|| (mois == 7 && day == 14 )|| (mois == 7 && day == 16) || (mois == 7 && day == 17 )|| (mois == 7 && day == 19) ||( mois == 7 && day == 21) ||( mois == 7 && day == 23) || (mois == 7 && day == 24) || (mois == 7 && day == 26) ||( mois == 7 && day == 28 )|| (mois == 7 && day == 31))
  {
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
       myservo1.write(pos);   
       myservo2.write(pos);                
       delay(15); 
      }
    }
  if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
    for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
  
  if ((mois == 8 && day == 2) || (mois == 8 && day == 5 )||( mois == 8 && day == 7) ||( mois == 8 && day == 9) || (mois == 8 && day == 12) || (mois == 8 && day == 14) ||( mois == 8 && day == 16 )|| (mois == 8 && day == 19) ||( mois == 8 && day == 21) || (mois == 8 && day == 23) || (mois == 8 && day == 26 )||( mois == 8 && day == 28) ||( mois == 8 && day == 30))
  { 
    if(heure == 0 && minutes ==0 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
      for (pos = 0; pos <= 90; pos += 1) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  if(heure == 0 && minutes ==15 && (seconde == 0 || seconde == 1 ||seconde == 2 || seconde == 3))
    {
    for (pos = 90; pos >= 0; pos --) 
      { 
        myservo1.write(pos);   
        myservo2.write(pos);                 
        delay(15); 
      }
    }
  }
}

void Niveau()
{
  //Niveau eau ultrason 
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  Serial.print("distance: ");
  Serial.println(distanceCm);
  distancePercent = 100 - distanceCm/70*100;
  Serial.println(distancePercent);
  Firebase.setFloat(firebaseData, "/Enit/Level", (int)distancePercent);
}

void Temperature()
{
  // temperature 
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();  
  if (isnan(hum) || isnan(temp)  )
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print("°C");
  Serial.print(" Humidity: ");
  Serial.print(hum);
  Serial.print("%");
  Serial.println();

  Firebase.setFloat(firebaseData, "/Enit/Temp", temp);
  Firebase.setFloat(firebaseData, "/Enit/Humidity", hum);
}

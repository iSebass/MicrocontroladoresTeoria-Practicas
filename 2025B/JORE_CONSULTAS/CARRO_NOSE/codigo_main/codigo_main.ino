#include "PINES.h"
#include "MOTORES.h"


#define SensorCarro 11

int S1;
int S2;
int S3;
int S4;
int S5;
int S6;

int sensorCarro;



char aux[50]=" ";


void setup(){

  //CONFIGURAMOS LOS PINES ANALOGICOS COMO ENTRADA
  /*
  pinMode(ANALOG_SENSOR1, INPUT);
  pinMode(ANALOG_SENSOR2, INPUT);
  pinMode(ANALOG_SENSOR3, INPUT);
  pinMode(ANALOG_SENSOR4, INPUT);
  pinMode(ANALOG_SENSOR5, INPUT);
  pinMode(ANALOG_SENSOR6, INPUT);
  */


  //CONFIGURAMOS LOS PINES DEL MOTOR COMO SALIDA
  pinMode(MA, OUTPUT);
  pinMode(MB, OUTPUT);
  pinMode(MC, OUTPUT);

  //CONFIGURAMOS EL SENSOR DETECTOR DE CARRO
  pinMode(13, INPUT);

  //CONFIGURAMOS LE PUERTO SERIE
  Serial.begin(9600);


  

}

void loop(){

  S1 = digitalRead( ANALOG_SENSOR1 );
  S2 = digitalRead( ANALOG_SENSOR2 );
  S3 = digitalRead( ANALOG_SENSOR3 );
  S4 = digitalRead( ANALOG_SENSOR4 );
  S5 = digitalRead( ANALOG_SENSOR5 );
  S6 = digitalRead( ANALOG_SENSOR6 );

  sensorCarro = digitalRead(SensorCarro);

  sprintf(aux, "SensorCarro: %d", sensorCarro);
  Serial.println(aux);

  if( !(sensorCarro == 1) ){
    Frenar();
    delay(5000);
    digitalWrite(MC, HIGH);
    delay(5000);
    digitalWrite(MC, LOW);
    //while( 1 );
  }

  if( S1 == 1 && S2 == 1 && S3  == 1 && S4 == 1  && S5 == 1 && S6 == 1   ){
    Avanzar();
  }
  else if( S1 == 1 && S2 == 1 && S3  == 1 && S4 == 1  && S5 == 1 && S6 == 1){

  }
  else if( S1 == 1 && S2 == 1 && S3  == 1 && S4 == 1  && S5 == 1 && S6 == 1 ){

  }
  else{

  }

  sprintf(aux, "%d %d %d %d %d %d", S1, S2, S3, S4, S5, S6);
  Serial.println(aux);

  delay(1000);

}

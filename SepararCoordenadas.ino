
//String pruebas="123.456_78.9";
//String prueba;

struct area {
  String puntos[20];
};

struct area Jardin;

struct area jardines[5];



double Latitude(String coordenadas){
  int split=coordenadas.indexOf("_");
  double lat=coordenadas.substring(0, split).toDouble();
  return lat;
}

double Longitude(String coordenadas){
  int split=coordenadas.indexOf("_");
  double lon=coordenadas.substring(split+1).toDouble();
  return lon;
}



void setup() {
  //double Lo = Longitude(pruebas);
  //double lat = Latitude(pruebas);
  //Serial.println(Lo,5);
  //Serial.println(lat,5);

  Serial.begin(9600);

  jardines[0]=CrearJardin();

  Serial.println("Jardin creado:");
  
  for (int k=0; k<3;k++){
    Serial.println(jardines[0].puntos[k]);
  }

}


struct area CrearJardin(){
  
  struct area jardinNuevo;
  Serial.println("introduzca coordenadas");
  
  int i=0; 
  while (i<3){
    if (Serial.available()>0){  
      
  String prueba = Serial.readString();
  
  jardinNuevo.puntos[i]=prueba;
  Serial.println(jardinNuevo.puntos[i]);

  Serial.println(Latitude(jardinNuevo.puntos[i]),6);
  Serial.println(Longitude(jardinNuevo.puntos[i]),6);

  i++;
  }
  } 
  return jardinNuevo;
}


void loop() {
 /*
  if (Serial.available()>0){   
  prueba = Serial.readString();
  Serial.println(Latitude(prueba),6);
  Serial.println(Longitude(prueba),6);

  Jardin.puntos[i]=prueba;
  Serial.println(Jardin.puntos[i]);
  }
 */ 


}

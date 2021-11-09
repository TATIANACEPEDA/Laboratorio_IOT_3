# include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>                    // incluye libreria de Servo para la placa
#include "DHT.h"                      //incluyo la libretia para el sensor de temperatura
#include "ESP32_MailClient.h"        // incluyo la libreria para enviar correos

#define DHTPIN 15
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);             //creo objeto para el sensor de temperatura
Servo servo1;                         // crea objeto para el servo
// defino variables del sensor ultrasonico
const int trigPin = 2;               // son las entradas para el sensor de distancia
const int echoPin = 5;
// defino variables del servomotor
int PINSERVO = 26;                       // pin 34 conectado a señal del servo
int PULSOMIN = 900;                     // pulso minimo en microsegundos
int PULSOMAX = 2000;                    // pulso maximo en microsegundos

// defino demas variables
float duration;
int D_ant,bandera,mnum,BANDERA;
double Distance;
int LED = 14;                           //para prender el bombillo 1
int LED1 = 12;                          //para prender el bombillo 2
int VENT = 13;                          //para prender el ventilador
int LED_MAIL = 18;                      //para prender el led indicador que se envio el mensaje
int BOTON ;                          //una bandera
int SENSOR = 4;                         //para leer la apertura de la puerta
int mnuman = 0;                         //para almacenas el valor de temperatura aterior en entero
int cont = 500 ;
int BANDERA1 = 0;

String stringdistance,str,tem,hum,mens,PUERTA,EMAIL;
float  t,h,h_ant,t_ant;

SMTPData datosSMTP;

//**************
//**** MQTT CONFIG *****
//**************
const char *mqtt_server = "node02.myqtthub.com";
const int   mqtt_port =1883 ;
const char *mqtt_user = "IOT_PRUEBA";
const char *mqtt_pass = "IOT_PRUEBA";
const char *root_topic_subscribe = "Temperatura/input";
const char *root_topic_publish = "Temperatura/output";



//**************
//**** WIFICONFIG ******
//**************
const char* ssid = "DANIEL Y HELEN 2.4";
const char* password =  "159357DH";



//**************
//**** GLOBALES   ******
//**************
WiFiClient espClient;
PubSubClient client(espClient);
char msg[76];
char msgtem[25];
char msghum[25];
//long count=0;


//********
//* F U N C I O N E S **
//********
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();

    void setup() 
  {
    Serial.begin(9600);
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port); 
    client.setCallback(callback);
    servo1.attach(PINSERVO, PULSOMIN, PULSOMAX);  // inicializacion de servo
    pinMode(trigPin, OUTPUT); // defino como salidad el pin tring del ultrasoido
    pinMode(echoPin, INPUT);  // defino como entrada el pin echo del ultrasoido
    pinMode(LED, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(VENT, OUTPUT);
    pinMode(BOTON, OUTPUT);
    pinMode(LED_MAIL, OUTPUT);    
    pinMode(SENSOR, INPUT);     //Pulsador o sensor como entrada   
    digitalWrite(LED, LOW);     //les doy un alto iniciando para que no se prendan los relay ya que prenden con ceros
    digitalWrite(LED1, LOW);
    digitalWrite(VENT, HIGH);
    digitalWrite(LED_MAIL, LOW);
    dht.begin();                        //inicio el modulo dht11
  }

void loop() 
{
    
    if (!client.connected())         // si el cliente no se conecta entra
    {
      reconnect();
    }
  
      if (client.connected())
      { 
         h = dht.readHumidity();          //leo la humedad del sensor y lo guardo en h
         t = dht.readTemperature();        //leo la temperatura del sensor y lo guardo en t
         
         // limpia los pines de trigpin 
        digitalWrite(trigPin, LOW);
        delayMicroseconds(10);
    
        // coloca el trigpin en alto durante 10 microsegundos
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
    
        // lee el pin echo y debuelvo el tiempo de ida y vuelta de la onda en microsegundos
        duration = pulseIn(echoPin, HIGH);
    
        // Calculo la distancio con esta ecuacion
        Distance = duration * 0.034 / 2;
        if(BOTON==0) PUERTA="ABIERTA";  // son para escribir si esta abierta o cerrada la puerta segun el caso
        else PUERTA="CERRADA";
         str = " Distancia: " + String (Distance) + " Cm "+"Humedad: "+ h +"% Temperatura: "+ t +"°C puerta: "+ PUERTA;        
         tem = "tem:"+ String (t) +"°C "; 
         hum = "Humedad: "+ String (h) +"% "; 
          // muestro en pantalla las variables
          if(D_ant != Distance || h_ant != h || t_ant != t) //si hay algun cambio muestre en pantalla
            {
             str.toCharArray(msg,76);                 //convierta str en arreclo char con 57 carateres 
             client.publish(root_topic_publish,msg);  
            }
            
          D_ant=Distance;      //guarda el cambio anterior para la funcion si cambia la variables muestra
          h_ant=h;
          t_ant=t;
          
          BOTON = digitalRead(SENSOR);  //lee si esta abierta la puerta
          if(BOTON == 0 && BANDERA1 < 1)
            {
            correo();                   //se llama a correo para decir que esta abierta la puerta
            Serial.print("se abrio la puerta con el boton \n ");
            Serial.print(   BANDERA1  );
            BANDERA1 ++;
            }
          casos();
       }
  client.loop();
  
    
}

 //***********
  //*    CONEXION WIFI      *
  //*********** 
  void setup_wifi()
  {
    delay(10);
    // Nos conectamos a nuestra red Wifi
    Serial.println();
    Serial.print("Conectando a ssid: ");
    Serial.println(ssid);
  
    WiFi.begin(ssid, password);
  
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        Serial.print(".");
      }
  
    Serial.println("");
    Serial.println("Conectado a red WiFi!");
    Serial.println("Dirección IP: ");
    Serial.println(WiFi.localIP());
  }



//***********
//*    CONEXION MQTT      *
//***********

void reconnect() 
{

  while (!client.connected()) 
  {
    Serial.print("Intentando conexión Mqtt...");
    // Creamos un cliente ID
    String clientId = "IOT_PRUEBA";
    clientId += String(random(0xffff), HEX);
    // Intentamos conectar
      if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) //aqui
      {
        Serial.println("Conectado!");
        // Nos suscribimos
          if(client.subscribe(root_topic_subscribe))     //aqui es donde 
          {
            Serial.println("Suscripcion ok");
          }
          
          else
          {
            Serial.println("fallo Suscripciión");
          }
      } 
      else 
      {
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");
      delay(5000);
     }
  }
}


//***********   desifra la informacion del brouquer
//*       CALLBACK        *    en formato char 
//***********

  void callback(char* topic, byte* payload, unsigned int length)
  {
    String incoming = "";
    Serial.print("Mensaje recibido desde: ");
    Serial.print(topic);
    Serial.println("");
      for (int i = 0; i < length; i++) 
      {
        incoming += (char)payload[i];
      }
      mens=incoming;                                           //guardo en una variable para poder usarlo  como lectura
    incoming.trim();
    Serial.println("Mensaje: " + incoming);
      casos();                                                 // llama la funcion y dependiendo la orden ejecuta
  
  }
  
   void casos()                                                // funcion "funcion casos"
      { 
          //estos if son para prender y apagar el bombillo de la abitacion # 1
          if (mens == "ventilador_off") //if (mens == "cuarto1off")
             {
               digitalWrite(VENT, HIGH); 
                //Serial.println("entro a apagar cuarto1 " );
             }
           if (mens == "ventilador_on") //if (mens == "cuarto1on")
             {
               digitalWrite(VENT, LOW); 
                //Serial.println("entro a prender cuarto1"); 
             } 
           //estos if son para prender y apagar el bombillo de la abitacion # 2  
           if (mens == "bombillo_off") //if (mens == "cuarto2off")
             {
                digitalWrite(LED1, HIGH); 
                //Serial.println("entro a apagar cuarto2"); 
             } 
           if (mens == "bombillo_on") //if (mens == "cuarto2on")
             {
                digitalWrite(LED1, LOW); 
               // Serial.println("entro a prender cuarto2"); 
             } 
           //estos if son para prender y apagar el ventilador  
           if (mens == "c0") //if (mens == "ventiladoroff")
             {
                digitalWrite(VENT, HIGH); 
               // Serial.println("entro a apagar ventilador"); 
             } 
           if (mens == "c1")  //if (mens == "ventiladoron")
             {
                digitalWrite(VENT, LOW); 
               // Serial.println("entro a prender ventilador"); 
             } 
            //estos if son para cerrar o abrir la puerta principal de la casa   
           if (mens == "puerta_off"&& BANDERA1 > 0)  //if (mens == "cerrar")
             {
                servo1.write(0);    // ubica el servo a 0 grados
                delay(800);   
                BANDERA1 = 0;
                if(BOTON == 0 )BANDERA1++;
                Serial.println("entro a cerrar puerta"); 
             } 
             
           if (mens == "puerta_on" && BANDERA1 < 1) //if (mens == "abrir")
             {  
                servo1.write(180);    // ubica el servo a 0 grados
                  correo(); 
                  BANDERA1++;                //reseteo la bandera 
                  Serial.println("entro a abrir puerta"); 
                delay(200); 
             }     
           //este if es para cuando el cliente desea poner una temeratura para que se prende el ventilador
           if (mens[0] == 't' && mens[1] == 'e' && mens[2] == 'm' && mens[3] == ':')
             {
               mnum = ((mens[4]-48)*10) +mens[5]-48;                //guardo el numero para usarlo en la temperatura
               Serial.println("entro al tem:");
                  if (t>=mnum)                                      //si la tempratura es mayor o igual a la designada por el usuario entro
                     {
                      digitalWrite(VENT, LOW);                       //prendo el ventilaror(trabaja con ceros)
                      //Serial.println("entro al mayor que");
                      delay(1000); 
                     } 
                  else  digitalWrite(VENT, HIGH);  
                  
                if(mnum != mnuman)             //si cambio la tempratura
                {
                  BANDERA = 1;                 //uso esta bandera para indicar que es para la temperatura
                  correo(); 
                  BANDERA = 0;                //reseteo la bandera 
                }
                mnuman = mnum;                //para que no entre si no cuando la temperatura cambie             
                                         
             }  
             
      }  


       void correo()
  { 
    Serial.print("Iniciando correo!!!");
    delay(1000);
    EMAIL="SE ACABA DE ABRIR LA PUERTA"; 
     if(BANDERA==1) EMAIL="CAMBIO LA TEMPERATURA";          //este if es para cuando se esta cambiando la temperaturo y no abriendo la puerta
     digitalWrite(LED_MAIL, HIGH);                         //prendo el led indicando que se esta enviendo un email
    datosSMTP.setLogin("smtp.gmail.com", 465, "electivaiotunisangil@gmail.com", " ElectivaIOT20");//Configuración del servidor de correo electrónico SMTP, host, puerto, cuenta y contraseña
    datosSMTP.setSender("Cristian_Nossa", "electivaiotunisangil@gmail.com");     //coloca el nombre del que envia el mensaje y el correo electrónico
    datosSMTP.setPriority("High");                            // Establezca la prioridad o importancia del correo electrónico High, Normal, Low o 1 a 5 (1 es el más alto)
    datosSMTP.setSubject("ALERTA DE SEGURIDAD");               // Establecer el asunto
    datosSMTP.setMessage(EMAIL, false);                       // escribe el contenido del correo
    datosSMTP.addRecipient("cristiannossa@unisangil.edu.co");   // aqui se envia el correo al destinatario     
    if (!MailClient.sendMail(datosSMTP))                       //si tiene datos comience a enviar correo electrónico.
    Serial.println("Error enviando el correo, " + MailClient.smtpErrorReason());
    datosSMTP.empty();           //Borrar todos los datos del objeto datosSMTP para liberar memoria
      delay(5000);                 // esperao 9min a que se envie el mensaje
     digitalWrite(LED_MAIL, LOW);    //apago led despues de enviar el mensaje
     BANDERA==0;
  }

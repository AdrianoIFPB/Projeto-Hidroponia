//Bibliotecas WiFi / RTC=========================================================
#include <WiFi.h>
#include "time.h"

//Bibliotecas DHT================================================================
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//Bibliotecas de propósito geral=================================================
#include <string.h>
#include <stdio.h>
#include <stdint.h>

//PROTOTIPOS DE FUNCOES==========================================================
void printLocalTime();
void conectaWifi();
void desconectaWifi();
void getTemperaturaUmidade();
void limpaVariaveis();
void dataHoraAgora();
void acionaSolenoideHora();
void desligaSolenoideHora();
void dataHoraUltimaLeitura();
void desligaSolenoideRemoto();
void acionaSolenoideRemoto();
void leituraDiaria(uint8_t dia);
void getUmidadeSolo();
void getVazao();

//MAPEAMENTO DE HARDWARE=========================================================
#define DHTPIN 25
#define PINOSOLENOIDE 27 
#define PINOMEDSOLO 19
#define PINOMEDVAZAO 23

//DEFINICOES GLOBAIS/CONSTANTES - WiFi =====================================================
const char* ssid     = "PROXXIMA - 193283-2.4Ghz";
const char* password = "30660512";
WiFiServer server(80);

//DEFINICOES GLOBAIS/CONSTANTES RTC ========================================================
const char* ntpServer = "br.pool.ntp.org";
const long  gmtOffset_sec = -10800;
const int   daylightOffset_sec = -10800;//GMT-3:00

//DECLARAÇÃO DE OBJETOS =========================================================
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//DECLARAÇÃO DE CONSTANTES ======================================================
const int horaLigar [4] = {5,12,16,19};// 4 acionamentos automáticos por dia
const int minutoLigar [4]  = {0,0,0,0};
const int horaLeitura [4] = {5,12,16,18};// 4 leituras automáticas por dia
const int minutoLeitura [4]  = {10,10,10,06};

//DECLARAÇÃO DE STRUCT ======================================================
struct leituraDiaria{
  float temperatura[4];
  float vazaoDiaria[4];
  int umidadeAr[4];
  int umidadeSolo[4];  
};


//VARIAVEIS GLOBAIS =============================================================
double calculoVazao;
int contador;
float fluxoAcumulado;
int umidade = 0x00;
int umidadeSolo = 0x00;
float temperatura = 0x00;
float vazaoDiaria = 0x00;
struct tm dataHora;
int value = 0;
int horaAtual = 0x00;
int minutoAtual = 0x00;
int diaAtual = 0x00;
int mesAtual = 0x00;
int anoAtual = 0x00;
int horaUltimaLeitura = 0x00;
int minutoUltimaLeitura = 0x00;
int diaUltimaLeitura = 0x00;
int mesUltimaLeitura = 0x00;
int anoUltimaLeitura = 0x00;
uint8_t diaSemana = 0x00;
uint8_t controleSolenoideHora = 0x00;
uint8_t statusSolenoide = 0x00;
uint8_t minutoAgora = 0x00;
uint8_t posDiaLeitura = 3;
struct leituraDiaria domingo;
struct leituraDiaria segunda;
struct leituraDiaria terca;
struct leituraDiaria quarta;
struct leituraDiaria quinta;
struct leituraDiaria sexta;
struct leituraDiaria sabado;
bool controleLeituraDiaria = false;
uint8_t acionamentoRemoto = 0x00;


// the setup function runs once when you press reset or power the board
void setup() {
  
  pinMode(PINOMEDSOLO,INPUT);
  pinMode(PINOMEDVAZAO,INPUT);  
  pinMode(DHTPIN, INPUT_PULLUP);
  pinMode(PINOSOLENOIDE, OUTPUT);  

 
  
  // initialize...
  Serial.begin(115200);
  dht.begin();  
  
  conectaWifi();//chama a função para conectar o WiFi
  
  server.begin();//inicializa ESP32 como Servidor Web

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while(adjustLocalTime() == false);  
 
  getLocalTime(&dataHora);// inicializa dataHora
  diaSemana = dataHora.tm_wday; // recebe o dia da semana: 0-6(dom-sab)
  getTemperaturaUmidade(); // inicializa Temperatura e Umidade    
}

// the loop function runs over and over again forever
void loop() {
  dataHoraAgora();
  getVazao();
  acionaSolenoideHora();
  desligaSolenoideHora();
  leituraDiaria(diaSemana);
  
  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            //client.println("Content-type:text/html");
            client.println("<!DOCTYPE html>");
            client.println();

            client.println("<html lang='pt-br'>");
            client.println("<head><meta charset='UTF-8'>");
            client.println("<meta content=\"widh=device-width, initial-scale=1\">");
            client.println("<style>html { margin: 0px auto; text-align: center;}");
            client.println(".botao_atualiza { background-color: #0000FF; color: black; padding: 15px 40px; border-radius: 25px}");
            client.println(".botao_liga { background-color: #00FF00; color: black; padding: 15px 40px; border-radius: 25px}");
            client.println(".botao_desliga { background-color: #FF0000; color: black; padding: 15px 40px; border-radius: 25px}</style></head>");
            client.println("<body><h1>Monitoramento Remoto com ESP32</h1>");
            client.println(" <br />");
            client.println("<h2>Variáveis do Sistema de Irrigação</h2>");           
            client.printf("<h3>Temperatura: %.2f", temperatura);            
            client.println("°C</h3>");           
            client.printf("<h3>Umidade do Ar: %d", umidade);
            client.println("%</h3>");
            client.printf("<h3>Umidade do Solo: %d", umidadeSolo);
            client.println("%</h3>");
            if(statusSolenoide){
              client.println("<h3 style = 'color:Green;'>Estado do Solenoide: Ligado.</h3>");
            }
            else{
              client.println("<h3 style = 'color:red;'>Estado do Solenoide: Desligado.</h3>");
            }
            client.printf("<h3>Vazão Diária Acumulada: %.2f", vazaoDiaria);
            client.println(" litros.</h3>");
            client.printf("<h4 style = 'color: gray;'><i><u>Leituras realizadas em %d/%d/%d às %02d:%02d.</u></i></h4>", diaUltimaLeitura, mesUltimaLeitura, anoUltimaLeitura, horaUltimaLeitura, minutoUltimaLeitura);
            //client.printf("%d/%d/%d às %02d:%02d", diaUltimaLeitura, mesUltimaLeitura, anoUltimaLeitura, horaUltimaLeitura, minutoUltimaLeitura);
            client.print(" <br />");             
            
            // the content of the HTTP response follows the header:
            client.println("<h2>Atualização Remota das Variáveis</h2>"); 
            client.print("<p><a href=\"/A\"><button class=\"botao_atualiza\">ATUALIZA</button></a></p>");
            client.print(" <br />");

             //Tabela com horários de leitura
            client.println("<style>html { margin: 0px auto; text-align: center;}");
            client.println(".table_hora{ margin-left: auto; margin-right: auto}</style>");
            client.println("<table class=\"table_hora\" border = '1px'><caption><h4>Horários para Leitura Automatizada</h4></caption><thead>");
            client.println("<tr><th>Horário_1</th><th>Horário_2</th><th>Horário_3</th><th>Horário_4</th></tr></thead>");
            client.println("<tbody><tr>");
            for(int i=0; i<4; i++){
               client.printf("<td>%d:%02d</td>",horaLeitura[i], minutoLeitura[i]);  
            }
            client.println("</tr></tbody></table>");
            client.println(" <br />");
            
                                 
            client.println(" <br />");
            client.println("<h2>Acionamento Remoto do Solenoide</h2>");  
            client.print("<p><a href=\"/H\"><button class=\"botao_liga\">LIGA</button></a></p>");
            client.print("<p><a href=\"/L\"><button class=\"botao_desliga\">DESLIGA</button></a></p>");

            //Tabela com horários de acionamento
            client.println("<style>html { margin: 0px auto; text-align: center;}");
            client.println(".table_hora{ margin-left: auto; margin-right: auto}</style>");
            client.println("<table class=\"table_hora\" border = '1px'><caption><h4>Horários para Irrigação Automatizada</h4></caption><thead>");
            client.println("<tr><th>Horário_1</th><th>Horário_2</th><th>Horário_3</th><th>Horário_4</th></tr></thead>");
            client.println("<tbody><tr>");
            for(int i=0; i<4; i++){
               client.printf("<td>%d:%02d</td>",horaLigar[i], minutoLigar[i]);  
            }
            client.println("</tr></tbody></table>");
            client.println(" <br />");
            //=============================TABELA OPCIONAL================================
            client.println("<h2>Histórico Semanal</h2>");
            client.println("<style>html { margin: 0px auto; text-align: center;}");
            client.println(".table_historico{ margin-left: auto; margin-right: auto}</style>");
            client.println("<table class=\"table_historico\" border = '1px'><caption></caption>");
            client.println("<thead><tr><th colspan='5'>Domingo</th></tr></thead>");
            client.println("<thead><tr><th>Horário</th><th>Temperatura</th><th>Umidade do Ar</th><th>Umidade do Solo</th><th>Vazão Diária</th></tr></thead>");
            client.println("<tbody>");
            for(int i=0; i<4; i++){
              client.println("<tr>");
              client.printf("<td>%dh%dmin</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>",horaLeitura[i], minutoLeitura[i], domingo.temperatura[i], 
              domingo.umidadeAr[i], domingo.umidadeSolo[i], domingo.vazaoDiaria[i]);  
              client.println("</tr>");
            }            
            client.println("<tr><td style='font-weight:bold; text-align: center'; colspan='5'>Segunda</td></tr>");
            for(int i=0; i<4; i++){
              client.println("<tr>");
              client.printf("<td>%dh%dmin</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>",horaLeitura[i], minutoLeitura[i], segunda.temperatura[i], 
              segunda.umidadeAr[i], segunda.umidadeSolo[i], segunda.vazaoDiaria[i]);  
              client.println("</tr>");
            }
            client.println("<tr><td style='font-weight:bold; text-align: center'; colspan='5'>Terça</td></tr>");
            for(int i=0; i<4; i++){
              client.println("<tr>");
              client.printf("<td>%dh%dmin</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>",horaLeitura[i], minutoLeitura[i], terca.temperatura[i], 
              terca.umidadeAr[i], terca.umidadeSolo[i], terca.vazaoDiaria[i]);  
              client.println("</tr>");
            }
            client.println("<tr><td style='font-weight:bold; text-align: center'; colspan='5'>Quarta</td></tr>");
            for(int i=0; i<4; i++){
              client.println("<tr>");
              client.printf("<td>%dh%dmin</td><td>%.2f</td><td>%d</td><td>%d</td><td>%.2f</td>",horaLeitura[i], minutoLeitura[i], quarta.temperatura[i], 
              quarta.umidadeAr[i], quarta.umidadeSolo[i], quarta.vazaoDiaria[i]);  
              client.println("</tr>");
            }
            client.println("<tr><td style='font-weight:bold; text-align: center'; colspan='5'>Quinta</td></tr>");
            for(int i=0; i<4; i++){
              client.println("<tr>");
              client.printf("<td>%dh%dmin</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>",horaLeitura[i], minutoLeitura[i], quinta.temperatura[i], 
              quinta.umidadeAr[i], quinta.umidadeSolo[i], quinta.vazaoDiaria[i]);  
              client.println("</tr>");
            }
            client.println("<tr><td style='font-weight:bold; text-align: center'; colspan='5'>Sexta</td></tr>");
            for(int i=0; i<4; i++){
              client.println("<tr>");
              client.printf("<td>%dh%dmin</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>",horaLeitura[i], minutoLeitura[i], sexta.temperatura[i], 
              sexta.umidadeAr[i], sexta.umidadeSolo[i], sexta.vazaoDiaria[i]);  
              client.println("</tr>");
            }
            client.println("<tr><td style='font-weight:bold; text-align: center'; colspan='5'>Sábado</td></tr>");
            for(int i=0; i<4; i++){
              client.println("<tr>");
              client.printf("<td>%dh%dmin</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>",horaLeitura[i], minutoLeitura[i], sabado.temperatura[i], 
              sabado.umidadeAr[i], sabado.umidadeSolo[i], sabado.vazaoDiaria[i]);  
              client.println("</tr>");
            }                                  
            client.println("</tbody></table>");
            client.println(" <br />");
            //=============================TABELA OPCIONAL================================
            // the content of the footer
            client.println("<footer style = 'font-weight: bold; font-size: 14px;'><nav><a href='https://ifpb.edu.br/'>IFPB</a><br />");
            client.println("<a href='https://github.com/AdrianoIFPB/Projeto-Hidroponia'>Código: GitHub</a>  <br /></nav>");          
            client.println("&copy; 2022 - Adriano Soares; João Igor; José Domingos</footer>");    
            
            client.println("</body></html>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
           if (!(statusSolenoide)){
            acionaSolenoideRemoto();
            acionamentoRemoto = 1;
          }                        
        }
        if (currentLine.endsWith("GET /L")) {
          if (digitalRead(PINOMEDSOLO) == 0){
            desligaSolenoideRemoto();
            acionamentoRemoto = 0;
          }          
        }
        if (currentLine.endsWith("GET /A")) {         
          getTemperaturaUmidade();
          getVazao(); 
        //chama getUmidadeSolo() para pegar valor atualizado          
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");      
  }   
}

//FUNCOES========================================================

boolean adjustLocalTime(){
  struct tm timeinfo; 
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time!");
    delay(3000);
    return false;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return true;
}

void conectaWifi(){
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

void desconectaWifi(){
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi disconnected.");
}

void getTemperaturaUmidade(){
  if ((!(isnan(temperatura = dht.readTemperature()))) && (!(isnan(umidade = dht.readHumidity())))){
    dataHoraUltimaLeitura(); // atualiza data e hora da leitura 
  }
  else{ //tenta ler 10 vezes em caso de falha
    for(int i = 0; i < 10; i ++){
      Serial.println("Falha ao ler Sensor DHT!");
      temperatura = dht.readTemperature();
      umidade = dht.readHumidity();
      delay(1000);
      if((!(isnan(temperatura))) && (!(isnan(umidade)))){
        dataHoraUltimaLeitura();
        return;
      } 
    }    
  }
}

void limpaVariaveis(){
  umidade = 0x00;
  temperatura = 0x00;
  horaAtual = 0x00;
  minutoAtual = 0x00;
  diaAtual = 0x00;
  mesAtual = 0x00;
  anoAtual = 0x00;
}

void dataHoraAgora(){
  getLocalTime(&dataHora);
  horaAtual = dataHora.tm_hour;
  minutoAtual = dataHora.tm_min;
  diaAtual = dataHora.tm_mday;
  mesAtual = dataHora.tm_mon + 1;
  anoAtual = dataHora.tm_year + 1900;
}

void dataHoraUltimaLeitura(){ // registra data e hora da leitura 
  getLocalTime(&dataHora);
  horaUltimaLeitura = dataHora.tm_hour;
  minutoUltimaLeitura = dataHora.tm_min;
  diaUltimaLeitura = dataHora.tm_mday;
  mesUltimaLeitura = dataHora.tm_mon + 1;
  anoUltimaLeitura = dataHora.tm_year + 1900;
}

void acionaSolenoideHora(){  
  for (int i=0; i<4; i++){
    if (horaAtual == horaLigar[i] && minutoAtual == minutoLigar[i]){
      controleLeituraDiaria = false;//habilita registro diário na próxima leitura      
      if(statusSolenoide == 0){
        digitalWrite(PINOSOLENOIDE, HIGH);
        //ligadoDesde = millis();
        controleSolenoideHora = 1;
        break;
      }      
    }   
  }
}

void desligaSolenoideHora(){
  uint8_t tempControleSolenoide = 0;
  for (int i=0; i<4; i++){
    if (horaAtual == horaLigar[i] && minutoAtual == minutoLigar[i]){
      tempControleSolenoide = 0;
      break;
    }
    else{
      tempControleSolenoide = 1;
    }    
  }
  if (tempControleSolenoide && controleSolenoideHora){
    digitalWrite(PINOSOLENOIDE, LOW);
    controleSolenoideHora = 0;
  }  
}

void acionaSolenoideRemoto(){
  digitalWrite(PINOSOLENOIDE, HIGH);              
  statusSolenoide = 1;  
}

void desligaSolenoideRemoto(){
  digitalWrite(PINOSOLENOIDE, LOW);               
  statusSolenoide = 0;  
}

void leituraDiaria(uint8_t dia){ 
  if (horaAtual == horaLeitura[posDiaLeitura] && minutoAtual == minutoLeitura[posDiaLeitura] && controleLeituraDiaria == false){
    getTemperaturaUmidade();
    //chama getVazaoDiaria() para pegar valor atualizado
    //chama getUmidadeSolo() para pegar valor atualizado
    switch(dia){
      case 0:
        domingo.temperatura[posDiaLeitura] = temperatura;
        domingo.umidadeAr[posDiaLeitura] = umidade;
        domingo.vazaoDiaria[posDiaLeitura] = vazaoDiaria;
        domingo.umidadeSolo[posDiaLeitura] = umidadeSolo;
        controleLeituraDiaria = true;      
        break;
      case 1:
        segunda.temperatura[posDiaLeitura] = temperatura;
        segunda.umidadeAr[posDiaLeitura] = umidade;
        segunda.vazaoDiaria[posDiaLeitura] = vazaoDiaria;
        segunda.umidadeSolo[posDiaLeitura] = umidadeSolo;
        controleLeituraDiaria = true;      
        break;
      case 2:
        terca.temperatura[posDiaLeitura] = temperatura;
        terca.umidadeAr[posDiaLeitura] = umidade;
        terca.vazaoDiaria[posDiaLeitura] = vazaoDiaria;
        terca.umidadeSolo[posDiaLeitura] = umidadeSolo;
        controleLeituraDiaria = true;        
        break;
      case 3:
        quarta.temperatura[posDiaLeitura] = temperatura;
        quarta.umidadeAr[posDiaLeitura] = umidade;
        quarta.vazaoDiaria[posDiaLeitura] = vazaoDiaria;
        quarta.umidadeSolo[posDiaLeitura] = umidadeSolo;
        controleLeituraDiaria = true;      
        break;
      case 4:
        quinta.temperatura[posDiaLeitura] = temperatura;
        quinta.umidadeAr[posDiaLeitura] = umidade;
        quinta.vazaoDiaria[posDiaLeitura] = vazaoDiaria;
        quinta.umidadeSolo[posDiaLeitura] = umidadeSolo;
        controleLeituraDiaria = true;      
        break;
      case 5:
        sexta.temperatura[posDiaLeitura] = temperatura;
        sexta.umidadeAr[posDiaLeitura] = umidade;
        sexta.vazaoDiaria[posDiaLeitura] = vazaoDiaria;
        sexta.umidadeSolo[posDiaLeitura] = umidadeSolo;
        controleLeituraDiaria = true;      
        break;
      case 6:
        sabado.temperatura[posDiaLeitura] = temperatura;
        sabado.umidadeAr[posDiaLeitura] = umidade;
        sabado.vazaoDiaria[posDiaLeitura] = vazaoDiaria;
        sabado.umidadeSolo[posDiaLeitura] = umidadeSolo;
        controleLeituraDiaria = true;      
        break;  
      default:
        break;        
    }
    if (posDiaLeitura == 3){
      posDiaLeitura = 0;
    }
    else{
      posDiaLeitura++;  
    }    
  }
}

void getUmidadeSolo(){
}

void getVazao(){
  if(digitalRead(PINOMEDSOLO) == 1 || acionamentoRemoto == 1){
     attachInterrupt (23,Vazao,RISING );
     contador = 0;
     interrupts();
     delay(1000); 
     noInterrupts();
     calculoVazao = (contador * 2.25);
     fluxoAcumulado = fluxoAcumulado + (calculoVazao/1000);
     calculoVazao = calculoVazao * 60;
     calculoVazao = calculoVazao/1000;
     Serial.println("VAZAO");
     Serial.println(calculoVazao);
     Serial.println("ACUMULADO:");
     Serial.println(fluxoAcumulado);
     digitalWrite(PINOSOLENOIDE, HIGH);
     statusSolenoide = 1;  
  }
  else if(acionamentoRemoto == 0){
     digitalWrite(PINOSOLENOIDE, LOW);
     statusSolenoide = 0;  
  }
  vazaoDiaria = fluxoAcumulado;
}

void Vazao(){
    contador++;
}

//Programa : Temperatura e umidade com o DHT11 e LCD 16x2
//Autor : Mari, Pedro, Rafa

#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h> //Carrega a biblioteca LCD
#include <SoftwareSerial.h> //Biblioteca bluetooth

LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);
//SoftwareSerial mySerial(2, 3); // rx e tx, do bluetooth

#include <DHT.h> //Carrega a biblioteca DHT
#include <DS1307.h>
#include <string.h>

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//defines:

//Define a ligação ao pino de dados do sensor
#define DHTPIN A0
 
//Define o tipo de sensor DHT utilizado
#define DHTTYPE DHT11

#define buttonUp 8 // botao up no pino digital 8
#define buttonOk 9 // botao ok no pino digital 10
#define buttonDown 10 // botao down no pino digital 9
#define buttonEsc 11 // botao esc

#define pinBuzzer 6
#define pinVibracall 7

#define pinLDR A1

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//variaveis globais

DHT dht(DHTPIN, DHTTYPE);
DS1307 rtc(A3, A2);

//state = qual menu eu to: 1 = inicio, 2 = escolher alterar horario ou alarme, 3 = alterar horario, 4 = alterar alarme
int state = 1;

//essa variável é uma flag para ver quando está despertando
//0 = nao despertando, 1 = despertando
boolean despertando = false;
boolean soneca = false;

//variaveis dos botoes
int bUp = 0, bOk = 0, bDown = 0, bESC = 0;
int bUp_ant = 0, bOk_ant = 0, bDown_ant = 0, bESC_ant = 0;
int val_LDR = 500;
int val_LDR_ant = 500;

unsigned long delay1 = 0;
unsigned long delay_buzzer = 0;
unsigned long delay_alarme = 0;
unsigned long tempo_LDR = 0;

//horas, minutos e segundos que estão aparecendo no relogio (são atribuidos valores a eles no setup)
int h = 0, m = 0, s = 0; 

//horas e minutos configurados do alarme
int h_alarme = 0, m_alarme = 0;

//horas, minutos e segundos que o usuario está configurando no menu (se ele confirmar
//as alterações, os valores dessas variáveis vão para as variáveis do alarme de fato)
//int h_alarme_provisorio = 0, m_alarme_provisorio = 0, s_alarme_provisorio = 0; 

//Array simbolo grau
byte grau[8] ={ B00001100,
                B00010010,
                B00010010,
                B00001100,
                B00000000,
                B00000000,
                B00000000,
                B00000000,};

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//funções dos menus

void checaAlarme()
{
  if(rtc.getTime().hour == h_alarme && rtc.getTime().min == m_alarme)
  {
    if((millis() - delay_alarme) > 80000) //1 minuto e 20s
    {
      despertando = true;
      soneca = false;
      int contador_LDR = 1;
      while(despertando)
      {
        bUp = digitalRead(buttonUp);
        bOk = digitalRead(buttonOk);
        bDown = digitalRead(buttonDown);
        bESC = digitalRead(buttonEsc);
        val_LDR = analogRead(pinLDR);
        
        if(val_LDR < 300) //se mirar com o laser, para de despertar até algo acontecer
        {
          if(val_LDR_ant > 300)
          {
            tempo_LDR = millis(); 
            delay_buzzer = millis();
            digitalWrite(pinBuzzer, LOW);
            digitalWrite(pinVibracall, LOW);
          }
          if((millis() - tempo_LDR) > (contador_LDR*1000)) //tempo segurando laser no LDR > 1 segundo
          {
            if(!soneca)
            {
              //faz rotina soneca
              if(m_alarme < 55)
              {
                m_alarme += 5;
              }
              else
              {
                m_alarme -= 55;
                h_alarme += 1;
              }
              soneca = true;
              //contador_LDR += 1;
            }

            if((millis() - delay_buzzer) > 1000)
            {
              digitalWrite(pinBuzzer, HIGH);
              digitalWrite(pinVibracall, HIGH);
            }
            
            if((millis() - delay_buzzer) < 1000)
            {
              digitalWrite(pinBuzzer, LOW);
              digitalWrite(pinVibracall, LOW);
            }
            
            if((millis() - delay_buzzer) > 1200)
            {
              delay_buzzer = millis();
              contador_LDR += 1;
            }
          }
          if(contador_LDR == 4)// depois de 3 segundos segurando laser no LDR
          {
            despertando = false;
            digitalWrite(pinBuzzer, LOW); // buzzer para de tocar
            digitalWrite(pinVibracall, LOW); // vibracall para de vibrar
            h_alarme = EEPROM.read(0);
            m_alarme = EEPROM.read(1);
          }
        }
        else //se nao estiver mirando com o laser, continua despertando
        {
          if(soneca == true)
          {
            despertando = false;
          }
          else
          {
            if((millis() - delay_buzzer) > 400)
            {
              digitalWrite(pinBuzzer, HIGH);
              digitalWrite(pinVibracall, HIGH);
            }
            
            if((millis() - delay_buzzer) < 400)
            {
              digitalWrite(pinBuzzer, LOW);
              digitalWrite(pinVibracall, LOW);
            }
            
            if((millis() - delay_buzzer) > 1000)
              delay_buzzer = millis();
          }
          
        }
        val_LDR_ant = val_LDR;
        
  
        if((bOk == LOW && bOk_ant == HIGH))
        {
          despertando = false;
          digitalWrite(pinBuzzer, LOW); // buzzer para  
          digitalWrite(pinVibracall, LOW); // vibracall para
          
          //se o alarme for o da soneca, volta pro que tava configurado antes
          h_alarme = EEPROM.read(0);
          m_alarme = EEPROM.read(1);
          
        }
        else if(bESC == LOW && bESC_ant == HIGH)
        {
          //rotina soneca
          despertando = false;
          soneca = true;
          if(m_alarme < 55)
          {
            m_alarme += 5;
          }
          else
          {
            m_alarme -= 55;
            h_alarme += 1;
          }
        }
        bOk_ant = bOk;
        bESC_ant = bESC;
      }
      delay_alarme = millis();
    }  
  }
}

void telaInicial(float temperatura)
{
  //printar coisas no display (arrumar)
  lcd.setCursor(0,0);
  lcd.print("Hora: ");
  lcd.print(" ");
  lcd.setCursor(7,0);
  lcd.print(rtc.getTimeStr());
  lcd.setCursor(12,0);
  lcd.setCursor(0,1);
  lcd.print("Temp: ");
  lcd.print(" ");
  lcd.setCursor(7,1);
  lcd.print(temperatura,1);
  lcd.setCursor(12,1);
  lcd.write((byte)0); //talvez isso aqui dê problema (qualquer coisa é só comentar)

  
}

void escolherAlteracao()
{
  int vertical_position = 0;
   
  while(state != 1 && state != 3 && state != 4)
  {
    bUp = digitalRead(buttonUp);
    bOk = digitalRead(buttonOk);
    bDown = digitalRead(buttonDown);
    bESC = digitalRead(buttonEsc);
    
    checaAlarme();
    
    lcd.setCursor(0,0);
    if(vertical_position == 0) //seta está no primeiro
    {
      lcd.print(">");
    }
    else
    {
      lcd.print(" ");
    }
    lcd.setCursor(1,0);
    lcd.print("Alterar horario");
    lcd.setCursor(0,1);
    if(vertical_position == 1) //seta do está no segundo
    {
      lcd.print(">");
    }
    else
    {
      lcd.print(" ");
    }
    lcd.print(" ");
    lcd.setCursor(1,1);
    lcd.print("Alterar alarme");
    lcd.print(" ");

    //se o usuário estiver no state 2 e apertar o botão pra cima, muda
    //a posição vertical da seta
    if(bUp == LOW && bUp_ant == HIGH) //para que a pessoa consiga clicar só uma vez sem desencadear mais de 1 evento (da pra segurar o botao sem problemas)
    {
      if((millis() - delay1) > 10)
      {
        if(vertical_position == 0)
        {
          vertical_position = 1;
          Serial.print("alterar alarme");
        }
        else if(vertical_position == 1)
        {
          vertical_position = 0;
          Serial.print("alterar horario");
        }  
        delay1 = millis();
      }
    }
    else if(bOk == LOW && bOk_ant == HIGH) //se o usuário estiver no state 2 e apertar "OK"
    {
      if((millis() - delay1) > 100) //na verdade eu nem sei se precisa desse delay. veremos amanha no teste
      {
        if(despertando == 1)
        {
          despertando = 0;
        }
        else
        {
          if(vertical_position == 0)
          {
            state = 3;
            Serial.print("foi para o state = 3 (horario)");
            lcd.clear();
          }
          else if(vertical_position == 1)
          {
            state = 4;
            Serial.print("foi para o state = 4 (alarme)");
            lcd.clear();
          }
        }
        delay1 = millis();
      }
    }
    else if(bDown == LOW && bDown_ant == HIGH)
    {
      if(vertical_position == 0)
      {
        vertical_position = 1;
        Serial.print("alterar alarme");
      }
      else if(vertical_position == 1)
      {
        vertical_position = 0;
        Serial.print("alterar horario");
      }  
    }
    else if(bESC == LOW && bESC_ant == HIGH)
    {
      if((millis() - delay1) > 100)
      {
        state = 1;
        Serial.print("foi para o state = 1");
        lcd.clear();
        delay1 = millis();
      }
    }
    bESC_ant = bESC;
    bDown_ant = bDown;
    bOk_ant = bOk;
    bUp_ant = bUp;
  }
  
}

void printarTelaAlterarHorario(int posicao_selecionada, int h, int m, int s)
{
    String hora = String("");
    String minuto = String("");
    String segundo = String("");
    
    hora += h;
    minuto += m;
    segundo += s;

    /*--- Primeira Linha ---*/

    lcd.setCursor(0,0);
    lcd.print("    ");
    lcd.setCursor(4,0);
    if(posicao_selecionada == 4)
      lcd.print("__");
    else
      lcd.print("  ");
    lcd.setCursor(6,0);
    lcd.print(" ");
    lcd.setCursor(7,0);
    if(posicao_selecionada == 7)
      lcd.print("__");
    else
      lcd.print("  ");
    lcd.setCursor(9,0);
    lcd.print(" ");
    lcd.setCursor(10,0);
    if(posicao_selecionada == 10)
      lcd.print("__");
    else
      lcd.print("  ");
    lcd.setCursor(12,0);
    lcd.print("    ");
    
    /*--- Segunda Linha ---*/

    lcd.setCursor(0,1);
    lcd.print("    ");
    lcd.setCursor(4,1);
    if(h < 10)
    {
      lcd.print("0");
      lcd.setCursor(5,1);
      if(h == 0)
      {
        lcd.print("0");  
      }
      else
      {
        lcd.print(hora);
      }
    }
    else
      lcd.print(hora);
    lcd.setCursor(6,1);
    lcd.print(":");
    lcd.setCursor(7,1);
    if(m < 10)
    {
      lcd.print("0");
      lcd.setCursor(8,1);
      if(m == 0)
      {
        lcd.print("0");  
      }
      else
      {
        lcd.print(minuto);
      }
    }
    else
      lcd.print(minuto);
    lcd.setCursor(9,1);
    lcd.print(":");
    lcd.setCursor(10,1);
    if(s < 10)
    {
      lcd.print("0");
      lcd.setCursor(11,1);
      if(s == 0)
      {
        lcd.print("0");  
      }
      else
      {
        lcd.print(segundo);
      }
    }
    else
      lcd.print(segundo);
    lcd.setCursor(12,1);
    lcd.print("    ");
}

void printarAlteracaoCancelada()
{
  lcd.clear();
  lcd.setCursor(0,3);
  lcd.print("Alteracao");
  lcd.setCursor(1,3);
  lcd.print("Cancelada");
}

void alterarHorario()
{
  int lateral_position = 0; //0 = coluna da hora, 1 = coluna do minuto, 2 = coluna do segundo
  int posicao_selecionada = 4; //posição para colocar o "__" na coluna selecionada no display
  //seta valores iniciais antes do loop
  h = rtc.getTime().hour;
  m = rtc.getTime().min;
  s = rtc.getTime().sec;
  
  while(state != 1)
  {
    bUp = digitalRead(buttonUp);
    bOk = digitalRead(buttonOk);
    bDown = digitalRead(buttonDown);
    bESC = digitalRead(buttonEsc);
    
    printarTelaAlterarHorario(posicao_selecionada, h, m, s);
    
    if(bUp == LOW && bUp_ant == HIGH)
    {
      if(lateral_position == 0) //está na coluna da hora (HH)
      {
        if(h < 23)
        {
          h += 1;
          Serial.print("aumentou hora mesmo\n");
          Serial.print(h);
        }
        else if(h == 23)
        {
          h = 0;
          Serial.print("aumentou hora mesmo\n");
          Serial.print(h);
        }
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
      else if(lateral_position == 1) //está na coluna do minuto (MM)
      {
        if(m < 59)
        {
          m += 1;
          Serial.print("aumentou minuto mesmo\n");
          Serial.print(m);
        }
        else if(m == 59)
        {
          m = 0;
          Serial.print("aumentou minuto mesmo\n");
          Serial.print(m);
        }
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
      else if(lateral_position == 2) //está na coluna do segundo (SS)
      {
        if(s < 59)
        {
          s += 1;
          Serial.print("aumentou segundo mesmo\n");
          Serial.print(s);
        }
        else if(s == 59)
        {
          s = 0;
          Serial.print("aumentou segundo mesmo\n");
          Serial.print(s);
        }
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
    }
    else if(bOk == LOW && bOk_ant == HIGH)
    {
      if(lateral_position == 0) //está na coluna da hora (HH)
      {
        lateral_position = 1; //vai para a coluna do minuto (MM)
        Serial.print("alterando minuto");
        posicao_selecionada = 7;
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
      else if(lateral_position == 1) //está na coluna do minuto (MM)
      {
        lateral_position = 2; //vai para a coluna do segundo (SS)
        Serial.print("alterando segundo");
        posicao_selecionada = 10;
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
      else if(lateral_position == 2) //está na coluna do segundo (SS)
      {
        rtc.setTime(h, m, s); //coloca horario no rtc
        Serial.print("confirmou horario mesmo\n");
        state = 1;
        Serial.print("foi para o state = 1 (tela inicial\n");
        lcd.clear();
      }
    }
    else if(bDown == LOW && bDown_ant == HIGH)
    {
      if(lateral_position == 0) //está na coluna da hora (HH)
      {
        if(h > 0)
        {
          h -= 1;
          Serial.print("diminuiu hora mesmo\n");
          Serial.print(h);
        }
        else if(h == 0)
        {
          h = 23;
          Serial.print("diminuiu hora mesmo\n");
          Serial.print(h);
        }
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
      else if(lateral_position == 1) //está na coluna do minuto (MM)
      {
        if(m > 0)
        {
          m -= 1;
          Serial.print("diminuiu minuto mesmo\n");
          Serial.print(m);
        }
        else if(m == 0)
        {
          m = 59;
          Serial.print("diminuiu minuto mesmo\n");
          Serial.print(m);
        }
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
      else if(lateral_position == 2) //está na coluna do segundo (SS)
      {
        if(s > 0)
        {
          s -= 1;
          Serial.print("diminuiu segundo mesmo\n");
          Serial.print(s);
        }
        else if(s == 0)
        {
          s = 59;
          Serial.print("diminuiu segundo mesmo\n");
          Serial.print(s);
        }
        printarTelaAlterarHorario(posicao_selecionada, h, m, s);
      }
    }
    else if(bESC == LOW && bESC_ant == HIGH)
    {
      Serial.print("nao confirmou horario mesmo\n");
      state = 1;
      Serial.print("foi para o state = 1 (tela inicial)\n");
      lcd.clear();
    }
    bESC_ant = bESC;
    bDown_ant = bDown;
    bOk_ant = bOk;
    bUp_ant = bUp;
  }
}

void printarTelaAlterarAlarme(int posicao_selecionada, int h_alarme_provisorio, int m_alarme_provisorio)
{
    String hora = String("");
    String minuto = String("");
    
    hora += h_alarme_provisorio;
    minuto += m_alarme_provisorio;

    /*--- Primeira Linha ---*/

    lcd.setCursor(0,0);
    lcd.print("     ");
    lcd.setCursor(5,0);
    if(posicao_selecionada == 5)
      lcd.print("__");
    else
      lcd.print("  ");
    lcd.setCursor(7,0);
    lcd.print(" ");
    lcd.setCursor(8,0);
    if(posicao_selecionada == 8)
      lcd.print("__");
    else
      lcd.print("  ");
    lcd.setCursor(10,0);
    lcd.print("      ");
    
    /*--- Segunda Linha ---*/

    lcd.setCursor(0,1);
    lcd.print("     ");
    lcd.setCursor(5,1);
    if(h_alarme_provisorio < 10)
    {
      lcd.print("0");
      lcd.setCursor(6,1);
      if(h_alarme_provisorio == 0)
      {
        lcd.print("0");  
      }
      else
      {
        lcd.print(hora);
      }
    }
    else
      lcd.print(hora);
    lcd.setCursor(7,1);
    lcd.print(":");
    lcd.setCursor(8,1);
    if(m_alarme_provisorio < 10)
    {
      lcd.print("0");
      lcd.setCursor(9,1);
      if(m_alarme_provisorio == 0)
      {
        lcd.print("0");  
      }
      else
      {
        lcd.print(minuto);
      }
    }
    else
      lcd.print(minuto);
    lcd.setCursor(10,1);
    lcd.print("      ");
}

void alterarAlarme()
{
  int lateral_position = 0; //0 = coluna da hora, 1 = coluna do minuto
  int posicao_selecionada = 5; //posição para colocar o "__" na coluna selecionada no display
  //seta valores iniciais antes do loop
  int h_alarme_provisorio = h_alarme;
  int m_alarme_provisorio = m_alarme;
  
  while(state != 1)
  { 
    bUp = digitalRead(buttonUp);
    bOk = digitalRead(buttonOk);
    bDown = digitalRead(buttonDown);
    bESC = digitalRead(buttonEsc);
    
    printarTelaAlterarAlarme(posicao_selecionada, h_alarme_provisorio, m_alarme_provisorio);
    
    if(bUp == LOW && bUp_ant == HIGH)
    {
      if(lateral_position == 0) //está na coluna da hora (HH)
      {
        if(h_alarme_provisorio < 23)
        {
          h_alarme_provisorio += 1;
          Serial.print("aumentou hora alarme\n");
          Serial.print(h_alarme_provisorio);
        }
        else if(h_alarme_provisorio == 23)
        {
          h_alarme_provisorio = 0;
          Serial.print("aumentou hora alarme\n");
          Serial.print(h_alarme_provisorio);
        }
        printarTelaAlterarAlarme(posicao_selecionada, h_alarme_provisorio, m_alarme_provisorio);
      }
      else if(lateral_position == 1) //está na coluna do minuto (MM)
      {
        if(m_alarme_provisorio < 59)
        {
          m_alarme_provisorio += 1;
          Serial.print("aumentou minuto alarme\n");
          Serial.print(m_alarme_provisorio);
        }
        else if(m_alarme_provisorio == 59)
        {
          m_alarme_provisorio = 0;
          Serial.print("aumentou minuto alarme\n");
          Serial.print(m_alarme_provisorio);
        }
        printarTelaAlterarAlarme(posicao_selecionada, h_alarme_provisorio, m_alarme_provisorio);
      }
    }
    else if(bOk == LOW && bOk_ant == HIGH)
    {
      if(lateral_position == 0) //está na coluna da hora (HH)
      {
        lateral_position = 1; //vai para a coluna do minuto (MM)
        posicao_selecionada = 8;
        printarTelaAlterarAlarme(posicao_selecionada, h_alarme_provisorio, m_alarme_provisorio);
        Serial.print("alterando minuto alarme");
      }
      else if(lateral_position == 1) //está na coluna do minuto (MM)
      {
        h_alarme = h_alarme_provisorio;
        m_alarme = m_alarme_provisorio;
        EEPROM.write(0, h_alarme); //salva hora do alarme no endereço 0 do arduino
        EEPROM.write(1, m_alarme); // salva minuto do alarme no endereço 1 do arduino
        Serial.print("confirmou horario alarme\n");
        state = 1;
        Serial.print("foi para o state = 1 (tela inicial\n");
        lcd.clear();
      }
    }
    else if(bDown == LOW && bDown_ant == HIGH)
    {
      if(lateral_position == 0) //está na coluna da hora (HH)
      {
        if(h_alarme_provisorio > 0)
        {
          h_alarme_provisorio -= 1;
          Serial.print("diminuiu hora alarme\n");
        }
        else if(h_alarme_provisorio == 0)
        {
          h_alarme_provisorio = 23;
          Serial.print("diminuiu hora alarme\n");
        }
        printarTelaAlterarAlarme(posicao_selecionada, h_alarme_provisorio, m_alarme_provisorio);
      }
      else if(lateral_position == 1) //está na coluna do minuto (MM)
      {
        if(m_alarme_provisorio > 0)
        {
          m_alarme_provisorio -= 1;
          Serial.print("diminuiu minuto alarme\n");
        }
        else if(m_alarme_provisorio == 0)
        {
          m_alarme_provisorio = 59;
          Serial.print("diminuiu minuto alarme\n");
        }
        printarTelaAlterarAlarme(posicao_selecionada, h_alarme_provisorio, m_alarme_provisorio);
      }
    }
    else if(bESC == LOW && bESC_ant == HIGH)
    {
      Serial.print("nao confirmou horario mesmo\n");
      state = 1;
      Serial.print("foi para o state = 1 (tela inicial)\n");
      //fazer um print de alterações canceladas
      //printarAlteracaoCancelada();
      //delay(1000);
      lcd.clear();
    }
    bESC_ant = bESC;
    bDown_ant = bDown;
    bOk_ant = bOk;
    bUp_ant = bUp;
  }
}


void setup()
{
//  mySerial.begin(38400); //Inicializa o bluetooth
  Serial.begin(9600); //Inicializa a serial
  lcd.begin(16,2); //Inicializa LCD
  lcd.clear(); //Limpa o LCD
  //Cria o caractere customizado com o simbolo do grau
  lcd.createChar(0, grau);

  //Led como alarme 
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinVibracall, OUTPUT);
  pinMode(buttonUp, INPUT);
  pinMode(buttonOk, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(buttonEsc, INPUT);
  
  //Aciona o relógio
  rtc.halt(false);
  
  //As linhas abaixo setam a data e hora do modulo
  //e podem ser comentada apos a primeira utilizacao
  rtc.setDOW(SATURDAY);      //Define o dia da semana
  //rtc.setTime(13, 45, 0);     //Define o horario
  rtc.setDate(30, 11, 2019);   //Define o dia, mes e ano

  //atribui às variáveis de hora, minuto e segundo os valores que estão no rtc
  h = rtc.getTime().hour;
  m = rtc.getTime().min;
  s = rtc.getTime().sec;

  h_alarme = EEPROM.read(0);
  m_alarme = EEPROM.read(1);

  //Definições do pino no SQW/Out
  rtc.setSQWRate(SQW_RATE_1);
  rtc.enableSQW(true);
  digitalWrite(pinBuzzer, LOW);
  digitalWrite(pinVibracall, LOW);

  dht.begin();
}
 
void loop()
{
//variaveis com temperatura e umidade
//float umidade = dht.readHumidity(); //Le o valor da umidade
float temperatura = dht.readTemperature(); //Le o valor da temperatura
Serial.print(temperatura);
Serial.print("\n");


bUp = digitalRead(buttonUp);
bOk = digitalRead(buttonOk);
bDown = digitalRead(buttonDown);
bESC = digitalRead(buttonEsc);

switch(state)
{
  case 1: //tela inicial com horario e temperatura

    telaInicial(temperatura);
    checaAlarme();
    
    if(bOk == LOW && bOk_ant == HIGH) //se apertar o botao "OK" vai para o menu de configurações (state = 2)
    {
      if((millis() - delay1) > 10)
      {
        if(despertando == 1)
        {
          despertando = 0;
        }
        else
        {
          state = 2; 
          Serial.print("foi para o estado 2\n");
          lcd.clear();
          //delay(1000);
        }
        delay1 = millis();
      }
    }
    bOk_ant = bOk;
    break;
  case 2:
    escolherAlteracao();
    break;
  case 3:
    alterarHorario();
    break;
  case 4:
    alterarAlarme();
    break;
}
 
//Mostra o simbolo do grau quadrado
//lcd.print((char)223);

//Mostra o simbolo do grau formado pelo array
  //lcd.write((byte)0);

}

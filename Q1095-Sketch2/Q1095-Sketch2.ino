/****************************************************************************************
  Video Q1095 - Tutorial Maker - Display de LEDs
  
  Desenvolvido pela Fábrica de Programas - Brincando com Ideias (www.brincandocomideias.com)
  www.youtube.com/c/BrincandoComIdeias

  Autor Flavio Guimaraes
*****************************************************************************************/

//********************** PARAMETRIZAÇÃO DO PROJETO ***********************
#define pinBotaoDireito 2    //Porta do Arduino onde um dos terminais do botão direito esta conectado. O segundo terminal do botao deve ser ligado em GND do Arduino
#define pinBotaoEsquerdo 4  //Onde um dos terminais do botão ESQUERDO esta conectado. O segundo terminal do botao deve ser ligado em GND do Arduino

#define pinBuzzer 19       //Onde o terminal de sinal do buzzer esta conectado.
#define nivelBuzzer HIGH  //Nivel lógico em que o buzzer é acionado
#define tempoBeep 200     //Tempo em milisegundos de cada beep do alarme
#define qtdeBeeps 10      //Quantidade de Beeps por alarme

#define MAX_DEVICES 6  //Numero de modulos MAX7219 utilizados

#define pinDATA 16  //Porta do Arduino em que o pino DIN (data in) do primeiro módulo MAX7219 esta ligado
#define pinCS 17    //Onde o pino CS do primeiro módulo MAX7219 esta ligado
#define pinCLK 18   //Onde o pino CLX (clock) do primeiro módulo MAX7219 esta ligado

#define SCROLL_DELAY 80      //Velocidade da Rolagem do Texto, em milisegundos. Quanto menor for este valor, mais rápida é a rolagem.
#define DISPLAY_INTENSITY 5  //Define o nivel de luminosidade do display. Varia entre 0 e 10, sendo 0 o menos luminoso e 15 o mais luminoso.

#define BUF_SIZE 135  //Tamanho máximo em caracteres para o texto de rolagem.
#define QTD_TEXTOS 2  //Define a quantidade de textos que estarao disponiveis para rolagem

#define TIMER_INTERVAL_US 10000 //Tempo para execução do Timer

String texto[QTD_TEXTOS] = { "BRINCANDO COM IDEIAS           O CANAL DA INTERNET DAS COISAS          ",
                             "TE VEJO NO PROXIMO VIDEO!!!           TCHAU!!!!           " };

//********************** FINAL DA PARAMETRIZAÇÃO DO PROJETO ***********************

//BIBLIOTECAS
#include <MD_MAX72xx.h>
#include "esp_timer.h"
#include "BluetoothSerial.h"


//DEFINICOES FIXAS
#define CHAR_SPACING 1
#define tempoSegFun 1500  //define o tempo em milisegundos, que o botao deve ficar apertado para acionar sua segunda função

//OBJETOS PUBLICOS
BluetoothSerial serialBT;
MD_MAX72XX mx = MD_MAX72XX(pinDATA, pinCLK, pinCS, MAX_DEVICES);

//VARIAVEIS PUBLICAS
String device_name = "ESP32-BT";
String txtBT;
int textoSelect = 0;
bool textoEnviado = false;
String textoAd;
byte funcaoSelect = 0;  //0=Display 1=Cronometro 2=Contagem Regressiva 3=Placar
byte funcaoSelectAnt = 1;
int placar1 = 0;
int placar1Ant = 1;
int placar2 = 0;
int placar2Ant = 1;
char curMessage[BUF_SIZE];
long contador = 0;
long contadorAnt = 1;
bool contadorStatus = false;

#define qtdeTempos 12
long tempoRegressiva[qtdeTempos] = { 1000, 2000, 3000, 6000, 12000, 18000, 24000, 30000, 60000, 120000, 180000, 360000 };
long tempoApp = -1;
int tempoSelect = 0;
int tocaAlarmes = 0;
unsigned long delayAlarme;


uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t) {
  static char *p = curMessage;
  static uint8_t state = 0;
  static uint8_t curLen, showLen;
  static uint8_t cBuf[8];
  uint8_t colData;

  switch (state) {
    case 0:
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state++;
      if (*p == '\0') {
        p = curMessage;
      }
    case 1:
      colData = cBuf[curLen++];
      if (curLen == showLen) {
        showLen = CHAR_SPACING;
        curLen = 0;
        state = 2;
      }
      break;
    case 2:
      colData = 0;
      curLen++;
      if (curLen == showLen)
        state = 0;
      break;
    default:
      state = 0;
  }
  return (colData);
}

void scrollText(void) {
  static uint32_t prevTime = 0;
  if (millis() - prevTime >= SCROLL_DELAY) {
    mx.transform(MD_MAX72XX::TSR);
    prevTime = millis();
  }
}

void mostraNumero(byte numero, int coluna) {
  mx.setColumn(coluna - 1 - 4, 0);
  mx.setColumn(coluna + 5 - 4, 0);

  if (numero == 1) {
    mx.setColumn(coluna - 4, 0);
    mx.setColumn(coluna + 1 - 4, 0);
  }
  mx.setChar(coluna, char(numero + 48));
}

void mostraPlacar(int valor1, int valor2) {
  mx.setChar(25, 'x');

  if (valor1 <= 9) {
    mostraNumero(valor1, 11);
  } else if (valor1 <= 99) {
    mostraNumero(valor1 / 10, 8);
    mostraNumero(valor1 % 10, 14);
  } else {
    mostraNumero(valor1 / 100, 5);
    mostraNumero((valor1 % 100) / 10, 11);
    mostraNumero((valor1 % 100) % 10, 17);
  }

  if (valor2 <= 9) {
    mostraNumero(valor2, 39);
  } else if (valor2 <= 99) {
    mostraNumero(valor2 / 10, 36);
    mostraNumero(valor2 % 10, 42);
  } else {
    mostraNumero(valor2 / 100, 33);
    mostraNumero((valor2 % 100) / 10, 39);
    mostraNumero((valor2 % 100) % 10, 45);
  }
}

void mostraCronometro(int minutos, int segundos, int decimas) {
  int nL;

  if (minutos > 0) {
    if ((minutos / 10) > 0) {
      mostraNumero(minutos / 10, 8);
    } else {
      for (nL = 0; nL <= 9; nL++) {
        mx.setColumn(nL, 0);
      }
    }
    mostraNumero(minutos % 10, 14);
    mx.setColumn(16, B00010100);
  } else {
    for (nL = 0; nL <= 16; nL++) {
      mx.setColumn(nL, 0);
    }
  }

  if (segundos > 0) {
    if ((segundos / 10) > 0) {
      mostraNumero(segundos / 10, 22);
    } else {
      if (minutos > 0) {
        mostraNumero(0, 22);
      } else {
        for (nL = 17; nL <= 23; nL++) {
          mx.setColumn(nL, 0);
        }
      }
    }
    mostraNumero(segundos % 10, 28);
  } else {
    if (minutos > 0) {
      mostraNumero(0, 22);
    } else {
      for (nL = 17; nL <= 23; nL++) {
        mx.setColumn(nL, 0);
      }
    }
    mostraNumero(0, 28);
  }

  mx.setColumn(30, B00010100);

  mostraNumero(decimas / 10, 36);
  mostraNumero(decimas % 10, 42);
}

void periodic_timer_callback(void* arg) {
  if (contadorStatus) {
    if (funcaoSelect == 1) {
      contador++;
    }

    if (funcaoSelect == 2) {
      contador--;

      if (contador <= 0) {
        tocaAlarmes = qtdeBeeps;
        contadorStatus = false;
        if (tempoApp != -1) {
          contador = tempoApp;
        } else {
          contador = tempoRegressiva[tempoSelect];
        }
        delayAlarme = millis();
      }
    }
  }
}

byte botaoDireitoApertado() {
#define tempoDebounce 20  //(tempo para eliminar o efeito Bounce EM MILISEGUNDOS)

  static bool estadoBotaoAnt;
  static unsigned long delayBotao = 0;
  static unsigned long botaoApertado;
  static byte fase = 0;

  bool estadoBotao;
  byte estadoRet;

  estadoRet = 0;
  if ((millis() - delayBotao) > tempoDebounce) {
    estadoBotao = digitalRead(pinBotaoDireito);
    if (estadoBotao && (estadoBotao != estadoBotaoAnt)) {
      delayBotao = millis();
      botaoApertado = millis();
      fase = 1;
    }

    if ((fase == 1) && ((millis() - botaoApertado) > tempoSegFun)) {
      fase = 0;
      estadoRet = 2;
    }

    if (!estadoBotao && (estadoBotao != estadoBotaoAnt)) {
      delayBotao = millis();

      if (fase == 1) {
        estadoRet = 1;
      }
      fase = 0;
    }

    estadoBotaoAnt = estadoBotao;
  }

  return estadoRet;
}

byte botaoEsquerdoApertado() {
#define tempoDebounce 20  //(tempo para eliminar o efeito Bounce EM MILISEGUNDOS)

  static bool estadoBotaoAnt;
  static unsigned long delayBotao = 0;
  static unsigned long botaoApertado;
  static byte fase = 0;

  bool estadoBotao;
  byte estadoRet;

  estadoRet = 0;
  if ((millis() - delayBotao) > tempoDebounce) {
    estadoBotao = digitalRead(pinBotaoEsquerdo);
    if (estadoBotao && (estadoBotao != estadoBotaoAnt)) {
      delayBotao = millis();
      botaoApertado = millis();
      fase = 1;
    }

    if ((fase == 1) && ((millis() - botaoApertado) > tempoSegFun)) {
      fase = 0;
      estadoRet = 2;
    }

    if (!estadoBotao && (estadoBotao != estadoBotaoAnt)) {
      delayBotao = millis();

      if (fase == 1) {
        estadoRet = 1;
      }
      fase = 0;
    }

    estadoBotaoAnt = estadoBotao;
  }

  return estadoRet;
}

void setup() {
  pinMode(pinBotaoDireito, INPUT);
  pinMode(pinBotaoEsquerdo, INPUT);

  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinBuzzer, !nivelBuzzer);

  mx.begin();
  mx.setShiftDataInCallback(scrollDataSource);

  // Define o nivel de luminosidade
  mx.control(MD_MAX72XX::INTENSITY, DISPLAY_INTENSITY);
  texto[textoSelect].toCharArray(curMessage, BUF_SIZE);

  // Configuração do timer periódico usando esp_timer
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &periodic_timer_callback,
      .name = "periodic"
  };
  esp_timer_handle_t periodic_timer;
  esp_err_t err = esp_timer_create(&periodic_timer_args, &periodic_timer);
  if (err != ESP_OK) {
    Serial.println("Erro ao criar timer");
  }
  
  err = esp_timer_start_periodic(periodic_timer, TIMER_INTERVAL_US);
  if (err != ESP_OK) {
    Serial.println("Erro ao iniciar timer periódico");
  }

  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  Serial.println("Sketch Iniciado!");

  serialBT.begin(device_name);
  Serial.printf("O dispositivo de nome \"%s\" foi iniciado.\nO pareamento com Bluetooth esta disponivel!\n", device_name.c_str());
}

void loop() {
  byte estadoBotao;
  char chrBT = 0;

  //CONTROLE DO BLUETOOTH
  if (serialBT.available()) {
    chrBT = serialBT.read();
    Serial.print(chrBT);
    if (chrBT != '`') {
      txtBT += chrBT;
    }

    //Ignorar as mensagens de conecção do módulo bluetooth
    if (txtBT.charAt(0) == '+') {
      if (txtBT.length() > 3) {
        if (txtBT.substring(txtBT.length() - 4, txtBT.length() - 2) == "OK") {
            txtBT = "";
        }
      }
    }
  }

  if (chrBT == '`') {
    int comando = 0;

    //Solicitação do App pelo estado atual
    if (txtBT.charAt(0) == '*') {
      char funcEnvio;
      switch (funcaoSelect) {
        case 0:
          funcEnvio = '0';
          break;
        case 1:
          funcEnvio = '1';
          break;
        case 2:
          funcEnvio = '2';
          break;
        case 3:
          funcEnvio = '3';
          break;
      };
      serialBT.write(funcEnvio);
    }

    if (txtBT.charAt(0) == 'S') {
      txtBT = txtBT.substring(1);
      comando = txtBT.toInt();

      if (comando == funcaoSelect) {
        if (funcaoSelect == 0) {
          textoSelect++;

          if (textoEnviado) {
            if (textoSelect > QTD_TEXTOS) textoSelect = 0;
          } else {
            if (textoSelect == QTD_TEXTOS) textoSelect = 0;
          }

          //Limpa o buffer
          for (int nL; nL < BUF_SIZE; nL += 2) {
            curMessage[nL] = ' ';
            curMessage[nL + 1] = '.';
          }
          //for (int nL; nL < 10; nL++) mx.transform(MD_MAX72XX::TSR);

          if (textoSelect < QTD_TEXTOS) {
            texto[textoSelect].toCharArray(curMessage, BUF_SIZE);
          } else {
            textoAd.toCharArray(curMessage, BUF_SIZE);
          }
          mx.clear();
        }
      } else {
        funcaoSelect = comando;

        switch (funcaoSelect) {
          case 1:
            contador = 0;
            contadorAnt = 1;
            contadorStatus = false;
            placar1 = 0;
            placar2 = 0;
            mx.clear();
            break;
          case 2:
            if (tempoApp != -1) {
              contador = tempoApp;
            } else {
              contador = tempoRegressiva[tempoSelect];
            }
            contadorAnt = 1;
            contadorStatus = false;
            placar1 = 0;
            placar2 = 0;
            mx.clear();
            break;
          case 3:
            contador = 0;
            contadorAnt = 1;
            contadorStatus = false;
            placar1 = 0;
            placar2 = 0;
            placar1Ant = 1;
            mx.clear();
            break;
        }
      }
    }

    if (txtBT.charAt(0) == '#') {
      txtBT = txtBT.substring(1);
      txtBT += "          ";
      textoAd = txtBT;

      textoEnviado = true;
      textoSelect = QTD_TEXTOS;

      //Limpa o buffer
      for (int nL; nL < BUF_SIZE; nL += 2) {
        curMessage[nL] = ' ';
        curMessage[nL + 1] = '.';
      }
      //for (int nL; nL < 10; nL++) mx.transform(MD_MAX72XX::TSR);

      textoAd.toCharArray(curMessage, BUF_SIZE);
      mx.clear();
    }

    if (txtBT.charAt(0) == 'C') {
      txtBT = txtBT.substring(1);
      comando = txtBT.toInt();

      switch (comando) {
        case 1:
          {  //Cronometro Play
            contadorStatus = true;
            break;
          }
        case 2:
          {  //Cronometro Pause
            contadorStatus = false;
            break;
          }
        case 3:
          {  //Cronometro Stop
            contador = 0;
            contadorAnt = 1;
            contadorStatus = false;
            mx.clear();
            break;
          }

        case 4:
          {  //Regressiva Play
            contadorStatus = true;
            break;
          }
        case 5:
          {  //Regressiva Pause
            contadorStatus = false;
            break;
          }
        case 6:
          {  //Regressiva Stop
            if (tempoApp != -1) {
              contador = tempoApp;
            } else {
              contador = tempoRegressiva[tempoSelect];
            }
            contadorAnt = 1;
            contadorStatus = false;
            mx.clear();
            break;
          }

        case 7:
          {  //Placar 1
            placar1++;
            break;
          }

        case 8:
          {  //Placar 2
            placar2++;
            break;
          }

        case 9:
          {  //Placar Stop
            placar1 = 0;
            placar1Ant = 1;
            placar2 = 0;
            mx.clear();
            break;
          }
      }
    }

    if (txtBT.charAt(0) == 'T') {
      txtBT = txtBT.substring(1);

      int localPonto = txtBT.indexOf(':');
      String txtHoras = txtBT.substring(0, localPonto);
      String txtMinutos = txtBT.substring(localPonto + 1);

      int valHoras = txtHoras.toInt();
      int valMinutos = txtMinutos.toInt();

      funcaoSelect = 2;
      tempoApp = ((long(valMinutos) * 6000) + (long(valHoras) * 360000));
      contador = tempoApp;
      contadorAnt = 1;
      contadorStatus = false;
      placar1 = 0;
      placar2 = 0;
      mx.clear();
    }

    txtBT = "";
  }

  //CONTROLE DO BOTAO ESQUERDO
  estadoBotao = botaoEsquerdoApertado();
  if (estadoBotao == 1) {
    switch (funcaoSelect) {
      case 0:
        {  //Texto Rolando
          textoSelect--;
          if (textoSelect < 0) {
            textoSelect = (QTD_TEXTOS - 1);
          }
          texto[textoSelect].toCharArray(curMessage, BUF_SIZE);
          mx.clear();
          break;
        }

      case 1:
        {  //Cronometro
          break;
        }

      case 2:
        {  //Contagem Regressiva
          if (!contadorStatus) {
            tempoSelect++;
            if (tempoSelect > (qtdeTempos - 1)) {
              tempoSelect = 0;
            }
            contador = tempoRegressiva[tempoSelect];
            tempoApp = -1;
            contadorAnt = 0;
          }
          break;
        }

      case 3:
        {  //Placar
          placar1++;
          break;
        }
    }
  }
  if (estadoBotao == 2) {
    funcaoSelect++;
    if (funcaoSelect > 3) {
      funcaoSelect = 0;
    }

    if (funcaoSelect == 1) {
      contador = 0;
    } else {
      if (tempoApp != -1) {
        contador = tempoApp;
      } else {
        contador = tempoRegressiva[tempoSelect];
      }
    }
    contadorAnt = 1;
    contadorStatus = false;
    placar1 = 0;
    placar2 = 0;
    mx.clear();
  }

  //CONTROLE DO BOTAO DIREITO
  estadoBotao = botaoDireitoApertado();
  if (estadoBotao == 1) {
    switch (funcaoSelect) {
      case 0:
        {  //Texto Rolando
          textoSelect++;

          if (textoEnviado) {
            if (textoSelect > QTD_TEXTOS) textoSelect = 0;
          } else {
            if (textoSelect == QTD_TEXTOS) textoSelect = 0;
          }

          //Limpa o buffer
          for (int nL; nL < BUF_SIZE; nL += 2) {
            curMessage[nL] = ' ';
            curMessage[nL + 1] = '.';
          }
          //for (int nL; nL < 10; nL++) mx.transform(MD_MAX72XX::TSR);

          if (textoSelect < QTD_TEXTOS) {
            texto[textoSelect].toCharArray(curMessage, BUF_SIZE);
          } else {
            textoAd.toCharArray(curMessage, BUF_SIZE);
          }
          mx.clear();
          break;
        }

      case 1:
        {  //Cronometro
          if (contadorStatus) {
            contadorStatus = false;
          } else {
            contadorStatus = true;
          }
          break;
        }

      case 2:
        {  //Placar
          if (contadorStatus) {
            contadorStatus = false;
          } else {
            contadorStatus = true;
          }
          break;
        }

      case 3:
        {  //Placar
          placar2++;
          break;
        }
    }
  }
  if (estadoBotao == 2) {
    switch (funcaoSelect) {
      case 1:
        {  //Cronometro
          contador = 0;
          contadorAnt = 1;
          contadorStatus = false;
          mx.clear();
          break;
        }

      case 2:
        {  //Regressiva
          if (tempoApp != -1) {
            contador = tempoApp;
          } else {
            contador = tempoRegressiva[tempoSelect];
          }
          contadorAnt = 0;
          contadorStatus = false;
          mx.clear();
          break;
        }

      case 3:
        {  //Placar
          placar1 = 0;
          placar2 = 0;
          mx.clear();
          break;
        }
    }
  }

  //CONTROLE DO DISPLAY
  switch (funcaoSelect) {
    case 0:
      {  //Display com Rolagem de Texto
        scrollText();
        break;
      }

    case 1:
      {  //Cronometro
        if (contador != contadorAnt) {
          mostraCronometro((contador / 100) / 60, (contador / 100) % 60, contador % 100);
        }
        contadorAnt = contador;
        break;
      }

    case 2:
      {  //Contagem Regressiva
        if (contador != contadorAnt) {
          mostraCronometro((contador / 100) / 60, (contador / 100) % 60, contador % 100);
        }
        contadorAnt = contador;
        break;
      }

    case 3:
      {  //Placar
        if ((placar1 != placar1Ant) || (placar2 != placar2Ant)) {
          mostraPlacar(placar1, placar2);
        }
        placar1Ant = placar1;
        placar2Ant = placar2;
        break;
      }
  }

  //CONTROLE DO ALARME
  if (tocaAlarmes > 0) {
    if ((millis() - delayAlarme) < tempoBeep) {
      digitalWrite(pinBuzzer, nivelBuzzer);
    } else {
      digitalWrite(pinBuzzer, !nivelBuzzer);
    }

    if ((millis() - delayAlarme) > (tempoBeep * 2)) {
      tocaAlarmes--;
      delayAlarme = millis();
    }
  }
}

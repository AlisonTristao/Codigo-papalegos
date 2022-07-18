//Autor: Alison de Oliveira
//Email: AlisonTristao@hotmail.com

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it 
#endif 

BluetoothSerial SerialBT;

//right
int fd1 = 26;
int fd2 = 27;
int dpwm = 13;

//left
int fe1 = 25;
int fe2 = 19;
int epwm = 21;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Esp32 car");//nome do dispositivo

  //define os pinos
  pinMode(fe1, OUTPUT);
  pinMode(fe2, OUTPUT);
  pinMode(epwm, OUTPUT);

  pinMode(fd1, OUTPUT);
  pinMode(fd2, OUTPUT);
  pinMode(dpwm, OUTPUT);

  //configuração para controlar a ponte H
  ledcSetup(0, 5000, 12);//canal para esquerdo
  ledcSetup(1, 5000, 12);//canal para o direito
  ledcAttachPin(epwm, 0);
  ledcAttachPin(dpwm, 1);
  //inicia como 0
  ledcWrite(0, 0);
  ledcWrite(1, 0);
}

//gambiarrinha pra filtrar os dados bluetooth 
//nao consegui filtrar a string recebida por bluetooth de outro jeito
int contador = 0;
String esq = "";
String dir = "";

//recebe a char do bt e separa ela em valor esquerdo e direito
void filtraDadosRecebidos(char a){
  if( a == '[' and contador == 0){//inicio
      contador = 1;
    }else if(a != ',' and contador == 1){//valor do esquerdo
      esq += a;  
    }else if(a == ',' and contador == 1){//virgula
      contador = 2;  
    }else if (a != ']' and contador == 2){//lado direito
        dir += a;
    }else{//fim (quando ser igual a ])
      contador = 0;
  }
}

int* setMovimento(int valores[]){
  //recebe um vetor com os valores da potencia do motor
  //[esquerdo, direito]
  
  if(valores[0] > 0){
    //motor esquerdo frente
    digitalWrite(fe1, HIGH);
    digitalWrite(fe2, LOW);
  }else if (valores[0] < 0){
    //valor esquerdo pra tras
    digitalWrite(fe1, LOW);
    digitalWrite(fe2, HIGH);

    //retira o sinal de negativo pois a ponte H não tem valore negativo
    valores[0] = valores[0]*-1;   
  }else{
    //parado  
    digitalWrite(fe1, LOW);
    digitalWrite(fe2, LOW);
  }

  if(valores[1] > 0){
    //motor direito frente
    digitalWrite(fd1, HIGH);
    digitalWrite(fd2, LOW);
  }else if (valores[1] < 0){
    //valor direito pra tras
    digitalWrite(fd1, LOW);
    digitalWrite(fd2, HIGH);

    //retira o sinal de negativo pois a ponte H não tem valore negativo
    valores[1] = valores[1]*-1;  
  }else{
    //parado  
    digitalWrite(fd1, LOW);
    digitalWrite(fd2, LOW);
  }

  //retorna os valores sem os sinais negativos
  return valores;
}

void loop() {
  //enquanto ele estiver recebendo dados do bt ele repete o loop
  if(SerialBT.available()){
    //fitra a string recebida
    filtraDadosRecebidos(SerialBT.read());

    /* -----------------------------------------------------------------------
        Verifica se ele recebeu os dados inteiros (se os dados forem diferente de nulo)
      
        Apenas verificar se o dado do motor direito não é nulo ja basta pois 
      se o direito não é nulo, o esquerdo tbm não é 

        Verifica se tbm não é o sinal de menos "-" pq se o valor ser negativo ele recebe 
      o sinal em um char diferente do numero, entao no loop do menos ele verifica q não é nulo
      e pula o numero

        Caso for usar numero com duas casas decimais precisa fazer diferente, talvez
      verificar se o lengh do dir é igual a 2 
       ----------------------------------------------------------------------*/
    if(dir != "" and dir != "-"){ 
      int valores[] = {esq.toInt(), dir.toInt()};

      /* -----------------------------------------------------------------------
          Pra não fazer um switch case gingante verificando qual valor de -9 a 9 é
        e controlar a velocidade eu fiz uma conta

          A velocidade vai de 2500 ate 4095 (abaixo de 2500 a ponte H n fornece 
        energia suficiente para os motores andarem) e temos 9 velocidades, entao

          Cada velocidade é = (4095-2500)/9 => 177 aproximadamente

          (valores[1 ou 2]*177)+2500 = velocidade

          Porem pra ficar mais facil de controlar ele em uma velocidade baixa 
        eu deixei com 8 velocidade normais ate 3200 e a 9 velocidade como 4095 (turbo)

          valores[1 ou 2]*77+2500

          Acho que era melhor usar .map() porem não sabia que existia 
       ----------------------------------------------------------------------*/

      //cria uma variavel nova pq deu conflito int* com int[2]
      int* val = setMovimento(valores);

      //determina a potencia de cada motor
      if(val[0] != 9){//velocidade 9 é turbo
        val[0] = (valores[0]*77)+2500;
      }else{
        //turbo
          val[0] = 4095;
      }

      if(val[1] != 9){//velocidade 9 é turbo
        val[1] = (valores[1]*77)+2500;
      }else{
        //turbo
          val[1] = 4095;
      }

      //manda o comando pra ponte H
      ledcWrite(0, val[0]);
      ledcWrite(1, val[1]);

      //limpa as variaveis pro proximo loop
      esq = "";
      dir = "";
    }
  }
}

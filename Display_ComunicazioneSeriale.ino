#include <DMD2.h>
//------------FONTS---------------//

//#include "fonts/Arial14.h"
//#include "fonts/Arial_Black_16.h"

#include "fonts/Droid_Sans_12.h"       //Character height 9            B O M B O L O N I   _   E U R O   _   1 , 2 0
                                       //                   width      5 7 9 5 7 5 7 7 3  10   4 6 5 7  10   3 2 5 5
//Total width (keeping in mind the 1 pixel of spacing after each char):      63                   25            18

const int WIDTH = 2;     //32 pixel
const int HEIGHT = 1;    //16 pixel

//------------Font Selection----------------//
//const uint8_t *FONT = Arial_Black_16;
//const uint8_t *FONT = Arial14;
const uint8_t *FONT = Droid_Sans_12;

SoftDMD dmd(WIDTH, HEIGHT);  
DMD_TextBox box(dmd, 0, 0, 32, 16);
const int OE_PIN = 9;  // Assuming it is connected to the OE pin of the P10 display

String mydata, text, price;
char d;
short int nProdotti;
char carattere[38] = {' ', ',', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'X', 'Y', 'Z', 'W', 'J', 'K', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
int pixelWidth[38] = {11,   3,   8,   6,   7,   7,   5,   6,   7,   7,   4,   6,  10,   8,   8,   6,   8,   6,   6,   8,   7,   8,   8,   8,   6,   12,   3,   7,   4,   6,   6,   7,   6,   6,   6,   6,   6,   6};

void setup() {

  Serial.begin(9600);
  pinMode(OE_PIN, OUTPUT);  // Set pin 9 (OE) as output
  digitalWrite(OE_PIN, HIGH);  // Enables the output (to work it has to be high)
  Serial.println("Setting Brightness");
  dmd.setBrightness(250);
  dmd.selectFont(FONT);
  dmd.begin();
  Serial.println("DMD begun");

}

void loop() {

  //----Serial reading----//
  if(Serial.available()){
    do{
      if(Serial.available()){
        d = Serial.read();  //Reads the char sent on the serial
        if(d != '\n'){
          mydata += d;  //Appends the char to the string that will contain the whole message
        }
      }
    }while(d != '\n');  //Keeps looking at the serial until the special character (enter)
    ItemsNumber();  //Finds the number of products
  }
  int previousIndex=0;

  DisplayWrite(previousIndex);  //Prints the message after checking, formatting and aligning it
}

//------------Communication code for the serial-------------------//
/*

Product  Price      Product     Price
   vvvv   vvv      vvvvvvvvvv    vvv
  abcdef:1,20?1!Bomboloni_Crema:15,69     
              ^                      
              Symbol in numerical format (on the display you can only put one before the price)

  '!' Separate the different products 
  '_' Separate the words that correspond to a single product (empty spaces included), needed in case the text size exceeds the 64 pixels available (see previous exampple)
  ':' Comes before the price (decimal separated by ',')
  '?' Comes before the symbols -> Available symbols: heart=1; pizza=2; donut=3.

*/

void ItemsNumber(){

  //---Counts the number of products (needed for next cycles)---//
  //Returns the number of products

  nProdotti=1;
  for(int i=0; i < mydata.length(); i++){
    if(mydata.charAt(i) == '!'){
      nProdotti++;
    }
  }
}

int LetterSizes(String txt, int length){

  //-----Finds the pixel width for a String knowing the number of elements-----//
  //Returns the size in pixels

  int sizeCounter = 0;

  for (int ind=0; ind < length; ind++){
    for(int i=0; i<38; i++){
      if(txt.charAt(ind) == carattere[i]){
        sizeCounter += pixelWidth[i];
      }
      else
      {
        text = "ERR NO CHAR"; //A char not present in the usable chars vector was found
      }
    }
  }
  return sizeCounter;
}
  

void WordsCutter(int previousIndex){

//-----------------Cutting the substring of the text and price------------------//

  text = mydata.substring(previousIndex, mydata.indexOf(':',previousIndex)-1);
  price = mydata.substring(previousIndex, mydata.indexOf('?',previousIndex)-1);

}

String PixelWidthCheck(String msg){

//---Size of the word or price on the display---//

  int msgL = text.length();
  int wordSize = LetterSizes(msg, msgL); //Width in pixels of the string
  
  if(wordSize>64){
    msg = "SIZE ERROR"; //If one of the words exceeds the 64 pixels the display shows "SIZE ERROR"
  }
  return msg;
}

int Nwords(int previousIndex){
  
  //-----Counts how many words there are for a single product-----//

  int indexUnderscore = 0, count = 1;

  for(int k=0; k > 0; k = indexUnderscore){

    indexUnderscore = text.indexOf('_',k)-1;
    count++;

  }
  
  return count;
}

String Message(int index){
  return text.substring(index, text.indexOf('_',index)-1);  //Cuts the single word from the text
}

int SymbolSelector(int previousIndex){

  //------Extracts the number to identify the wanted symbol to print-----//
  //Returns the code of the symbol

  String simbolo = mydata.substring(mydata.indexOf(':',previousIndex) + 1, mydata.indexOf('?',previousIndex) - 1);
  return simbolo.toInt();
}

//------------Function to write on the display----------------------------------//

void DisplayWrite(int previousIndex){
  
  do{
    int nw = Nwords(previousIndex); //number of words (excluding price)
    WordsCutter(previousIndex); 
    int indice = 0;
    for(int j=0; j<nw; j++){  //Print each word in the text string
      dmd.clearScreen();
      String message = Message(indice); //Gets the substring
      message = PixelWidthCheck(message); //Checks it
      int messageL = message.length();
      dmd.drawString(floor((64 - LetterSizes(message, messageL))/2), 3, message); //Prints the product name/s
      indice += message.length() + 2; //Goes to the next word of the product (if exists)

      //1s delay
      int previousMillis = millis();
      while(millis()<(1000-previousMillis));
    }
    
    price = PixelWidthCheck(price); //Checks the price string width in pixels is correct
    int pricex = floor((64 - LetterSizes(price, price.length()))/2);  //Calculate xcoordinates for the text to center it
    dmd.clearScreen();
    dmd.drawString(pricex, 3, price); //Prints the price

    //----Draw symbol----//

    int sym = SymbolSelector(previousIndex); 

    switch(sym){
      case 0:
        break;
      case 1: 
      //------Heart------//
        if(pricex > 18){  
          dmd.drawLine(pricex-18,6,pricex-18,4);
          dmd.drawLine(pricex-18,3,pricex-15,0);
          dmd.drawLine(pricex-6,0,pricex-2,3);
          dmd.drawLine(pricex-2,4,pricex-2,6);
          dmd.drawLine(pricex-3,7,pricex-3,8);
          dmd.drawLine(pricex-5,9,pricex-10,14);
          dmd.drawLine(pricex-11,14,pricex-0,9);
          dmd.drawLine(pricex-17,24,pricex-17,23);
          dmd.drawLine(pricex-15,0,pricex-14,0);
          dmd.drawLine(pricex-6,0,pricex-9,0);
          dmd.drawLine(pricex-8,0,pricex-10,2);
          dmd.drawLine(pricex-13,0,pricex-11,2);
        }
        else{
          sym = 0;
        }
        break;
      case 2:
      //------Pizza------//
        if(pricex > 16){
          dmd.drawLine(pricex-2,16,pricex-2,13);
          dmd.drawLine(pricex-3,12,pricex-3,9);
          dmd.drawLine(pricex-4,8,pricex-4,5);
          dmd.drawLine(pricex-5,4,pricex-5,2);
          dmd.drawLine(pricex-5,2,pricex-8,2);
          dmd.drawLine(pricex-9,3,pricex-10,3);
          dmd.drawLine(pricex-11,4,pricex-14,7);
          dmd.drawLine(pricex-15,8,pricex-15,9);
          dmd.drawLine(pricex-16,10,pricex-16,13);
          dmd.drawLine(pricex-5,4,pricex-7,4);
          dmd.drawLine(pricex-8,5,pricex-9,5);
          dmd.drawLine(pricex-10,6,pricex-12,8);
          dmd.drawLine(pricex-13,9,pricex-13,10);
          dmd.drawLine(pricex-14,11,pricex-14,13);
          dmd.drawLine(pricex-16,13,pricex-14,13);
          dmd.drawLine(pricex-13,14,pricex-10,14);
          dmd.drawLine(pricex-9,15,pricex-4,15);
          dmd.drawLine(pricex-5,16,pricex-2,16);
          dmd.drawLine(pricex-7,14,pricex-4,14);
          dmd.drawLine(pricex-6,13,pricex-5,13);
          dmd.drawLine(pricex-6,10,pricex-6,7);
          dmd.drawLine(pricex-5,10,pricex-5,7);
          dmd.drawLine(pricex-7,9,pricex-7,8);
          dmd.drawLine(pricex-11,12,pricex-10,12);
          dmd.drawLine(pricex-12,11,pricex-9,11);
          dmd.drawLine(pricex-12,10,pricex-9,10);
          dmd.drawLine(pricex-11,9,pricex-10,9);
        }
        else{
          sym = 0;
        }
        break;
      case 3:
      //------Donut-----//
        if(pricex > 16){
          dmd.drawLine(pricex-6,14,pricex-12,14);
          dmd.drawLine(pricex-4,13,pricex-14,13);
          dmd.drawLine(pricex-3,12,pricex-15,12);
          dmd.drawLine(pricex-2,11,pricex-16,11);
          dmd.drawLine(pricex-2,10,pricex-7,10);
          dmd.drawLine(pricex-2,9,pricex-5,9);
          dmd.drawLine(pricex-2,8,pricex-5,8);
          dmd.drawLine(pricex-2,7,pricex-6,7);
          dmd.drawLine(pricex-2,6,pricex-7,6);
          dmd.drawLine(pricex-3,5,pricex-11,5);
          dmd.drawLine(pricex-4,4,pricex-10,4);
          dmd.drawLine(pricex-5,3,pricex-13,3);
          dmd.drawLine(pricex-7,2,pricex-11,2);
          dmd.drawLine(pricex-11,10,pricex-16,10);
          dmd.drawLine(pricex-13,9,pricex-16,9);
          dmd.drawLine(pricex-14,4,pricex-16,6);
          dmd.drawLine(pricex-13,4,pricex-16,7);
          dmd.drawLine(pricex-13,8,pricex-10,6);
          dmd.drawLine(pricex-14,8,pricex-11,6);
          dmd.drawLine(pricex-15,8,pricex-16,8);
          dmd.drawLine(pricex-15,7,pricex-15,8);
        }
        break;
      default:
        break;
    }

    

    //-------Coin Drawing------------//
    int coinx = pricex + LetterSizes(price, price.length()) + 8;

    dmd.drawCircle(coinx,7,7);
    //            X_center, Y_center, radius

    //EURO symbol

    dmd.drawLine(coinx-2,3,coinx+2,3); //Line AB
    //           X_A,Y_A,X_B,Y_B
    dmd.drawLine(coinx-2,11,coinx+2,11); 
    dmd.drawLine(coinx-3,4,coinx-3,10);
    dmd.drawLine(coinx-4,6,coinx-4,6);
    dmd.drawLine(coinx-4,8,coinx,8);
    //---------------------------------//

    //1s delay
    int previousMillis = millis();
    while(millis()<(1000-previousMillis));


    previousIndex = (mydata.indexOf('!',previousIndex) + 1);  //After writing the first product goes to the next one

  }while((previousIndex - 1) >= 0);
}



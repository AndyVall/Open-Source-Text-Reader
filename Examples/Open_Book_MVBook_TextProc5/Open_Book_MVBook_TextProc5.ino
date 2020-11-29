/////////////////////////////////////////////
// Author: Andy Vall
// Using the Open Book Hardware
// https://github.com/joeycastillo/The-Open-Book
// Will take txt files from any source, like project Gutenberg
// Will create a CSV file of all *.txt file listings
// Along with, whether file is being read = "opened," or not = "closed"
// Along with, character placement
// It only works with ASCII, so special characters may not display right
// The CSV file is only written at the beginning,
// or when actively beeing read, and the "select" (middle) or "lock" (by usb plug) is pressed
// to save file writes to the SD card.

#include <OpenBook.h>
#include "bitmap.h"

#include <Adafruit_SPIFlash.h>
#include <SPI.h>
#include <SdFat.h>
#include "sdios.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MP3.h>

OpenBook *book;

const uint8_t chipSelect = SS;
SdFat sd;
SdFile root;
SdFile file;
File myFile;

Adafruit_NeoPixel pixel(1, OPENBOOK_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_MP3 player;

void setup()
{
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(5, 0, 0)); //red
  pixel.show();
}

void loop()
{
  unsigned long bookcharacter;
  unsigned long bookstart;
  unsigned int directorylist;
  unsigned int opnbkcsvlist;
  unsigned int opnbk_open;
  unsigned int opnbk_old;
  unsigned int opnbk_comp;
  int selection = 0;
  int selectionopen = -1;
  int oldselection = 0;
  bool firstRun = true;
  int menuitems = 19;
  unsigned int csvchar;
  unsigned long csvholder;
  unsigned long txtholder;

  char filecontents[700];
  unsigned int filecontentsnum[700];
  unsigned int ichar = 0; // 700
  unsigned int linestart = 0; // 700
  unsigned int ibook = 0; // 700
  unsigned int lastword = 0;
  unsigned int linechar = 20;
  unsigned int pageline = 22;
  unsigned int currentline = 0;
  bool usechar = false;
  bool isword = false;
  bool useword = false;
  bool isline = false;
  bool useline = false;
  bool csvextra = false;
  opnbk_open = -1;
  String csvfilenames[32] = {};
  String csvopened[32] = {};
  unsigned long csvstartchar[32] = {0};
  unsigned long csvendchar[32] = {0};

  char *menu[] = {
  "The Open Book", "A:", "B:", "C:", "D:", "E:", "F:", "G:", "H:", "I:", "J:", "K:", "L:",
  "M:", "N:", "O:", "P:", "Q:", "R:"};
  
  String selected = "[x]: ";
  String endofline = "\n";
  String charbuffer;
  static String FileNames[22];
  String OBKCVSlines[32];
  String bufferFileName;

  int containstxt;
  int containshtm;
  int containscsv;
  
  book = new OpenBook();
  book->configureScreen();
  delay(1000);
  book->configureBabel();
  book->configureShiftButtons();

  book->getTypesetter()->setTextColor(EPD_BLACK);
  
  // the display is the Adafruit_GFX object we use to manipulate any pixels on the screen.
  Adafruit_EPD *display = book->getDisplay();
  // the Babel typesetter has its own reference to the display, which it uses to display multilingual text.
  BabelTypesetter *typesetter = book->getTypesetter();
  BabelDevice *babel = typesetter->getBabel();

  uint8_t buttons = book->readButtons();

  // bitRead(buttons,0) is left button
  // bitRead(buttons,1) is down button
  // bitRead(buttons,2) is up button
  // bitRead(buttons,3) is right button
  // bitRead(buttons,4) is select button
  // bitRead(buttons,5) is previous button
  // bitRead(buttons,6) is next button
  // bitRead(buttons,7) is lock button

  //////////////////////////////////////////////
  // Get Files Names
  containstxt = -1;
  containshtm = -1;
  containscsv = -1;
  directorylist = 1;
  txtholder = 0;
  sd.begin(OPENBOOK_SDCS, SD_SCK_MHZ(50));
  //sd.remove("opnbk.csv");
  root.open("/");
  //Serial.begin(115200);
  //Serial.println("reading file list");
  while (file.openNext(&root, O_RDONLY)) { //  && directorylist <= menuitems
    if (! file.isDir()) {
      char filename[64];
      file.getName(filename, sizeof(filename));
      FileNames[directorylist] = String(filename);
      containstxt = FileNames[directorylist].indexOf(".txt");
      containshtm = FileNames[directorylist].indexOf(".htm");
      if (containscsv == -1){
        containscsv = FileNames[directorylist].indexOf("opnbk.csv");
      }
      if (containstxt != -1 && directorylist <= menuitems){
        bitWrite(txtholder,directorylist,1);
        directorylist++;
      }
    }
    file.close();
  }
  root.close();
  bitWrite(txtholder,directorylist,0);
  //Serial.println(String(txtholder));

  ///////////////////////////////////////////////////
  // No opnbk.csv available, initialize
  for (int x = 0; x <= 700; x++) {
    filecontents[x] = 0;
    filecontentsnum[x] = 0;
  }
  opnbkcsvlist = 0;
  //Serial.println(String(containscsv));
  if (containscsv == -1){
    //Serial.println("creating opnbk.csv");
    myFile = sd.open("opnbk.csv", FILE_WRITE);
    if (!myFile.isOpen()) {
      //Serial.println("opnbk.csv not created");
    }
    else{
      for (int i = 1; i < directorylist; i++){
        if (i == 1){
          bufferFileName = FileNames[i] + ",opened,200,500";
        }
        else{
          bufferFileName = FileNames[i] + ",closed,0,0";
        }
        myFile.println(bufferFileName);
        //Serial.println(bufferFileName);
      }
      //myFile.println("testing.txt,opened,0,0");
      //Serial.println("testing.txt,opened,0,0");
    }
    myFile.close();
  }
  ////////////////////////////////////////////
  // opnbk.csv is available, read
  // and compare with txt files on the SD card
  else{
    csvholder = 0; // used to track what is in csv file
    csvchar = 0;
    opnbk_open = -1;
    //Serial.println( "Reading opnbk.csv");
    myFile = sd.open("opnbk.csv", FILE_READ);
    if (myFile) {
      while(myFile.available() && csvchar < 700 && opnbkcsvlist < 30){
        filecontentsnum[csvchar] =  myFile.read();
        filecontents[csvchar] = filecontentsnum[csvchar];
        if (filecontents[csvchar] == '\n'){
          filecontentsnum[csvchar] = 0;
          filecontents[csvchar] = filecontentsnum[csvchar];
          OBKCVSlines[opnbkcsvlist] = filecontents;
          bool csventrypresent = 0;
          unsigned int oindex = OBKCVSlines[opnbkcsvlist].indexOf(",",0);
          unsigned int sindex = OBKCVSlines[opnbkcsvlist].indexOf(",",oindex+1);
          unsigned int eindex = OBKCVSlines[opnbkcsvlist].indexOf(",",sindex+1);
          csvfilenames[opnbkcsvlist] = OBKCVSlines[opnbkcsvlist].substring(0,oindex);
          csvopened[opnbkcsvlist] = OBKCVSlines[opnbkcsvlist].substring(oindex+1,sindex);
          String startnum = OBKCVSlines[opnbkcsvlist].substring(sindex+1,eindex);
          String endnum = OBKCVSlines[opnbkcsvlist].substring(eindex+1);
          csvstartchar[opnbkcsvlist] = startnum.toInt();
          csvendchar[opnbkcsvlist] = endnum.toInt();
          //Serial.print(csvfilenames[opnbkcsvlist] + ",");
          //Serial.print(csvopened[opnbkcsvlist] + ",");
          //Serial.print(String(csvstartchar[opnbkcsvlist]) + ",");
          //Serial.println(String(csvendchar[opnbkcsvlist]));
          ///////////////////////////////////////////////////////////////////////////////
          // opened logic
          if (opnbk_open == -1 && OBKCVSlines[opnbkcsvlist].indexOf(",opened,",0) != -1){
            for (int i = 1; i < directorylist; i++){
              if (OBKCVSlines[opnbkcsvlist].indexOf(FileNames[i],0) != -1){
                bitWrite(txtholder,i,0);
                opnbk_open = opnbkcsvlist;
                csventrypresent = 1;
                selection = i;
                selectionopen = i;
                unsigned int oindex = OBKCVSlines[opnbkcsvlist].indexOf(",opened,",0);
                unsigned int sindex = OBKCVSlines[opnbkcsvlist].indexOf(",",oindex+1);
                unsigned int eindex = OBKCVSlines[opnbkcsvlist].indexOf(",",sindex+1);
                String startnum = OBKCVSlines[opnbkcsvlist].substring(sindex+1,eindex);
                String endnum = OBKCVSlines[opnbkcsvlist].substring(eindex+1);
                bookstart = startnum.toInt();
                bookcharacter = endnum.toInt();
                //Serial.println(String(bookstart));
                //Serial.println(String(bookcharacter));
                //Serial.println(String(selection));
              }
            }
            if (opnbk_open == -1){
              bitWrite(csvholder,opnbkcsvlist,0);
              csventrypresent = 0;
            }
          }
          /////////////////////////////////////////////////////////////////////////////////////
          // Find errant entries
          for (int i = 1; i <= directorylist; i++){
            if (OBKCVSlines[opnbkcsvlist].indexOf(FileNames[i],0) != -1 && bitRead(txtholder,i)){
              bitWrite(txtholder,i,0);
              csventrypresent = 1;
            }
          }
          //Serial.println(String(csventrypresent));
          if (csventrypresent){
            bitWrite(csvholder,opnbkcsvlist,1);
          }
          else{
            bitWrite(csvholder,opnbkcsvlist,0);
            csvextra = 1;
          }
          opnbkcsvlist++;
          csvchar = 0;
        }
        else{
          csvchar++;
        }
      }
      myFile.close();
      for (int i = 0; i < 30; i++){
        //Serial.println(OBKCVSlines[i]);
      }
      //Serial.println(String(csvholder));
      //Serial.println(String(txtholder));
      if (txtholder != 0 || csvextra){
        csvextra = 0;
        for (int i = 1; i <= directorylist; i++){
          // Find if any txt files are missing, add to CSV buffer
          if (bitRead(txtholder,i)){
            bufferFileName = FileNames[i] + ",closed,0,0";
            OBKCVSlines[opnbkcsvlist] = bufferFileName;
            bitWrite(csvholder,opnbkcsvlist,1);
            bitWrite(txtholder,i,0);
            opnbkcsvlist++;
          }
        }
        //Serial.println(String(csvholder));
        //Serial.println(String(txtholder));
        //Serial.println("removing opnbk.csv");
        sd.remove("opnbk.csv");
        //Serial.println("recreating opnbk.csv");
        myFile = sd.open("opnbk.csv", FILE_WRITE);
        if (!myFile.isOpen()) {
          //Serial.println("opnbk.csv not created");
        }
        else{
          for (int i = 0; i < opnbkcsvlist; i++){
            // Rewrite with new file list
            if (bitRead(csvholder,i)){
              bufferFileName = OBKCVSlines[i];
              myFile.println(bufferFileName);
              unsigned int oindex = OBKCVSlines[i].indexOf(",",0);
              unsigned int sindex = OBKCVSlines[i].indexOf(",",oindex+1);
              unsigned int eindex = OBKCVSlines[i].indexOf(",",sindex+1);
              csvfilenames[i] = OBKCVSlines[i].substring(0,oindex);
              csvopened[i] = OBKCVSlines[i].substring(oindex+1,sindex);
              String startnum = OBKCVSlines[i].substring(sindex+1,eindex);
              String endnum = OBKCVSlines[i].substring(eindex+1);
              csvstartchar[i] = startnum.toInt();
              csvendchar[i] = endnum.toInt();
              //Serial.print(csvfilenames[i] + ",");
              //Serial.print(csvopened[i] + ",");
              //Serial.print(String(csvstartchar[i]) + ",");
              //Serial.println(String(csvendchar[i]));
            }
          }
        }
        myFile.close();
      }
    }
    else{
      //Serial.println("Error Opening opnbk.csv");
    }
  }
  if (opnbk_open != -1){
    goto MENUSCREEN;
  }

SPLASHSCREEN:
  book->getDisplay()->setRotation(1);
  display->clearBuffer();
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(100, 100, 0)); //yellow
  pixel.show();
  display->drawBitmap(0, 0, OpenBookSplash, 400, 300, EPD_BLACK);
  display->display();
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(1, 1, 0)); //yellow
  pixel.show();
  
SPLASHHOLD:
  buttons = book->readButtons();
  //while (buttons!=0){buttons = book->readButtons();}
  // bitRead(buttons,4) is select button
  // bitRead(buttons,7) is lock button
  if ((bitRead(buttons,4) || bitRead(buttons,7))){ // && (opnbk_open == -1)
    goto MENUSCREEN;
  }
//  if ((bitRead(buttons,4) || bitRead(buttons,7)) && (opnbk_open != -1)){
//    goto MENUSCREEN;
//  }
  goto SPLASHHOLD;
  
MENUSCREEN:
  book->getDisplay()->setRotation(0);
  //Serial.println("menu started");
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(0, 100, 0));
  pixel.show();
  display->clearBuffer();
  display->setTextWrap(false);

  // Get Files Names
  containstxt = -1;
  containshtm = -1;
  containscsv = -1;
  directorylist = 1;

  root.open("/");
  //Serial.println("reading file list");
  while (file.openNext(&root, O_RDONLY) && directorylist <= menuitems) {
    if (! file.isDir()) {
      char filename[64];
      file.getName(filename, sizeof(filename));
      FileNames[directorylist] = String(filename);
      containstxt = FileNames[directorylist].indexOf(".txt");
      containshtm = FileNames[directorylist].indexOf(".htm");
      containscsv = FileNames[directorylist].indexOf("opnbk.csv");
      if (containstxt != -1){
        directorylist++;
      }
    }
    file.close();
  }
  root.close();
   
  for (int i = 0; i < directorylist; i++) {
    //Serial.println(menu[i]);
    if (selection == i){
      goto SELECTEDTEXT;
    }
    else{
      goto NOTSELECTED;
    }
NOTSELECTED:
    charbuffer = menu[i]+FileNames[i]; //+endofline
    goto SELECTDONE;
SELECTEDTEXT:
    charbuffer = selected+menu[i]+FileNames[i]; //+endofline
    goto SELECTDONE;
SELECTDONE:
    charbuffer.toCharArray(filecontents,700);
    size_t len = babel->utf8_codepoint_length(filecontents);
    BABEL_CODEPOINT *buf = (BABEL_CODEPOINT *)malloc(len);
    babel->utf8_parse(filecontents, buf);
    typesetter->setItalic(false);
    if (i == selection){
      typesetter->setBold(true);
      if (i==0){
        typesetter->setCursor(0,0);
        typesetter->setTextSize(2);
      }
      else{
        typesetter->setCursor(0,15+i*20);
        typesetter->setTextSize(1);
      }
      typesetter->setTextColor(EPD_BLACK);
      //display->print(charbuffer);
      book->getTypesetter()->print(filecontents);
    }
    else{
      typesetter->setBold(false);
      if (i==0){
        typesetter->setCursor(25,0);
        typesetter->setTextSize(2);
      }
      else{
        typesetter->setCursor(25,15+i*20);
        typesetter->setTextSize(1);
      }
      typesetter->setTextColor(EPD_BLACK);
      //display->print(charbuffer);
      book->getTypesetter()->print(filecontents);
    }
  }
  display->display();
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(0, 1, 0));
  pixel.show();
  oldselection = selection;
  //Serial.begin(115200);
  
MENUHOLD:  
  buttons = book->readButtons();
  //while (buttons!=0){buttons = book->readButtons();}
  if (bitRead(buttons,1)){ // bitRead(buttons,1) is down button
    pixel.begin();
    pixel.setPixelColor(0, pixel.Color(0, 3, 0));
    pixel.show();
    selection = selection + 1;
    if (selection >= menuitems){
      selection = 0;
    }
    buttons = book->readButtons();
    while (buttons!=0){buttons = book->readButtons();}
    pixel.begin();
    pixel.setPixelColor(0, pixel.Color(0, 1, 0));
    pixel.show();
    goto MENUSCREEN;
  }
  if (bitRead(buttons,2)){ // bitRead(buttons,2) is up button
    pixel.begin();
    pixel.setPixelColor(0, pixel.Color(0, 3, 0));
    pixel.show();
    selection = selection - 1;
    if (selection < 0){
      selection = menuitems - 1;
    }
    buttons = book->readButtons();
    while (buttons!=0){buttons = book->readButtons();}
    pixel.begin();
    pixel.setPixelColor(0, pixel.Color(0, 1, 0));
    pixel.show();
    goto MENUSCREEN;
  }
  if (bitRead(buttons,4) && selection != oldselection){ // bitRead(buttons,4) is select button
    goto MENUSCREEN; // new selection != old selection
  }
  if (bitRead(buttons,4) && selection == 0 && selection == oldselection){ // bitRead(buttons,4) is select button
    goto SPLASHSCREEN; // splash screen selected
  }
  if (bitRead(buttons,4) && selection > 0 && selection == oldselection){ // bitRead(buttons,4) is select button
    if (selection != selectionopen){ // change open txt file in csv
      for (int i = 0; i < opnbkcsvlist; i++){
        if (csvfilenames[i].indexOf(FileNames[selectionopen],0) != -1){
          csvopened[i] = "closed";
          //Serial.print(String(i) + ',' + csvfilenames[i] + ",");
          //Serial.print(csvopened[i] + ",");
          //Serial.print(String(csvstartchar[i]) + ",");
          //Serial.println(String(csvendchar[i]));
        }
      }
    }
    for (int i = 0; i < opnbkcsvlist; i++){
      if (csvfilenames[i].indexOf(FileNames[selection],0) != -1){
        csvopened[i] = "opened";
        bookstart = csvstartchar[i];
        bookcharacter = csvendchar[i];
        opnbk_open = i;
        //Serial.print(String(i) + ',' + csvfilenames[i] + ",");
        //Serial.print(csvopened[i] + ",");
        //Serial.print(String(csvstartchar[i]) + ",");
        //Serial.println(String(csvendchar[i]));
      }
    }
    selectionopen = selection;
    goto READFILE; // Text File Has Been Selected
  }
  goto MENUHOLD;

  // Read Text File
READFILE:
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(0, 0, 100));
  pixel.show();
  
  book->getDisplay()->setRotation(0);
  display->clearBuffer();
  typesetter->setCursor(0,0);
  typesetter->setTextSize(2);
  display->setTextWrap(false);
  
  myFile = sd.open(FileNames[selection]);

  // ~22 lines with text size of 2
  // ~20 characters per line

  //bookcharacter = 61;
  myFile.seek(bookstart);
  
  for (int x = 0; x <= 700; x++) {
    filecontents[x] = 0;
    filecontentsnum[x] = 0;
  }
  linechar = 14;
  pageline = 9;
  ichar = 0;
  ibook = 0;
  lastword = 0;
  linestart = 0;
  currentline = 0;
  if (myFile.available()) {
FINDWORD:
    isword = false;
    while (!isword && ichar < 700 && ibook < 700) {
      filecontentsnum[ibook] =  myFile.read();
      if (filecontentsnum[ibook]!=239 && filecontentsnum[ibook]!=187 && filecontentsnum[ibook]!=191){
        if (ibook > 2){
          if (filecontentsnum[ibook]!=32 || filecontentsnum[ibook]==32 && filecontentsnum[ibook-1]!=32){
            usechar = true;
          }
          else{
            usechar = false;
          }
        }
        else{
          usechar = true;
        }
      }
      if (usechar){
        filecontents[ichar] = filecontentsnum[ibook];
        if (filecontents[ichar] == ' ' || filecontents[ichar] == '/'){
          isword = true;
          isline = false;
        }
        else if (filecontents[ichar] != '\n' && filecontents[ichar-1] == '\n'  && filecontents[ichar-2] != '\n'){
          isword = true;
          isline = false;
        }
        else if (filecontents[ichar] == '\n' && filecontents[ichar-1] == '\n'){
          isword = true;
          isline = true;
        }
        else{
          ichar = ichar + 1;
        }
      }
      ibook = ibook + 1;
    }
    //Serial.println(String(currentline) + " " + String(ibook) + " " + String(ichar) + " " + String(isword) + " " + String(isline));
    if (isword && isline){
      //Serial.println("Find Page");
      goto FINDPAGE;
    }
    else if (isword && !isline){
      //Serial.println("Find Line");
      goto FINDLINE;
    }
    else{
      //Serial.println("Done Page");
      goto DONEPAGE;
    }
FINDLINE:
    if (filecontents[ichar] == ' '){
      if (ichar - linestart < linechar){
        isword = false;
        lastword = ichar;
        ichar = ichar + 1;
        //Serial.println("Find Word sp");
        goto FINDWORD;
      }
      else{
        linestart = lastword;
        if (filecontents[lastword] == ' ' || filecontents[lastword] == '/'){
          filecontents[lastword] = '\n';
        }
        lastword = ichar;
        isword = false;
        //Serial.println("Find Page");
        goto FINDPAGE;
      }
    }
    else if (filecontents[ichar] != '\n' && filecontents[ichar-1] == '\n'  && filecontents[ichar-2] != '\n'){
      if (ichar - 1 - linestart < linechar){
        filecontents[ichar-1] = ' ';
        lastword = ichar - 1;
        isword = false;
        ichar = ichar + 1;
        //Serial.println("Find Word nl");
        goto FINDWORD;
      }
      else{
        linestart = lastword;
        lastword = ichar;
        isword = false;
        //Serial.println("Find Page");
        goto FINDPAGE;
      }
    }
FINDPAGE:
    if (currentline < pageline){
      if (filecontents[ichar] == '\n' && filecontents[ichar-1] == '\n' && filecontents[ichar-2] == '\n'){
        currentline = currentline + 1;
        ichar = ichar + 1;
      }
      else{
        currentline ++;
        ichar = ichar + 1;
      }
      isword = false;
      isline = false;
      goto FINDWORD;
    }
    else{
      isword = false;
      isline = false;
      goto DONEPAGE;
    }
DONEPAGE:
    isword = false;
    isline = false;
  }

  //delay(1000);
  Serial.println("display");
  book->getTypesetter()->print(filecontents);
  //display->print(String(filecontents));
  display->display();
  pixel.begin();
  pixel.setPixelColor(0, pixel.Color(0, 0, 1));
  pixel.show();
  myFile.close();
  buttons = book->readButtons();
  bookcharacter = bookstart + ibook;

  csvstartchar[opnbk_open] = bookstart;
  csvendchar[opnbk_open] = bookcharacter;
  //Serial.print(String(opnbk_open) + ',' + csvfilenames[opnbk_open] + ",");
  //Serial.print(csvopened[opnbk_open] + ",");
  //Serial.print(String(csvstartchar[opnbk_open]) + ",");
  //Serial.println(String(csvendchar[opnbk_open]));
  
READHOLD:
  buttons = book->readButtons();
  //while (buttons!=0){buttons = book->readButtons();}
  
  if (bitRead(buttons,5)){ // bitRead(buttons,5) is previous button
    if (bookstart > 500){
      bookstart = bookstart - 500;
    }
    else{
      bookstart = 0;
    }
    goto READFILE; // Continue Reading
  }
  if (bitRead(buttons,6)){ // bitRead(buttons,6) is next button
    bookstart = bookcharacter;
    goto READFILE; // Continue Reading
  }
  if (bitRead(buttons,4)){ // bitRead(buttons,4) is select button
    csvopened[opnbk_open] = "closed";
    //Serial.println("removing opnbk.csv");
    sd.remove("opnbk.csv");
    //Serial.println("recreating opnbk.csv");
    myFile = sd.open("opnbk.csv", FILE_WRITE);
    if (!myFile.isOpen()) {
      //Serial.println("opnbk.csv not created");
    }
    else{
      for (int i = 0; i < opnbkcsvlist; i++){
        // Rewrite with new file list
        if (bitRead(csvholder,i)){
          bufferFileName = csvfilenames[i] + "," + csvopened[i];
          bufferFileName = bufferFileName + "," + String(csvstartchar[i]);
          bufferFileName = bufferFileName + "," + String(csvendchar[i]);
          myFile.println(bufferFileName);
          //Serial.println(bufferFileName);
        }
      }
    }
    myFile.close();
    goto MENUSCREEN; // Exit Out Of Reading
  }
  if (bitRead(buttons,7)){ // bitRead(buttons,7) is lock button
    //Serial.println("removing opnbk.csv");
    sd.remove("opnbk.csv");
    //Serial.println("recreating opnbk.csv");
    myFile = sd.open("opnbk.csv", FILE_WRITE);
    if (!myFile.isOpen()) {
      //Serial.println("opnbk.csv not created");
    }
    else{
      for (int i = 0; i < opnbkcsvlist; i++){
        // Rewrite with new file list
        if (bitRead(csvholder,i)){
          bufferFileName = csvfilenames[i] + "," + csvopened[i];
          bufferFileName = bufferFileName + "," + String(csvstartchar[i]);
          bufferFileName = bufferFileName + "," + String(csvendchar[i]);
          myFile.println(bufferFileName);
          //Serial.println(bufferFileName);
        }
      }
    }
    myFile.close();
    goto SPLASHSCREEN; // Exit Out Of Reading
  }
  goto READHOLD;
}

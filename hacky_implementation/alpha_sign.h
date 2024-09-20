/*
Av's terrible Alpha LED Sign communications library
Based on the Visual Basic code, which is why all of the code sucks.
One day, this should be rewritten to not use strings.
Use it as you please.
*/

#pragma once
#include "esphome.h"

const int ALPHA_TEXT = 1;
const int ALPHA_STRING = 2;

const int LINE_MIDDLE = 0;
const int LINE_TOP = 1;
const int LINE_BOTTOM = 2;
const int LINE_FILL = 3;

#define ARRAYSIZE 10

class AlphaSign : public Component, public UARTDevice {
  public:
    AlphaSign(UARTComponent *parent) : UARTDevice(parent) {}

    // Setup routines for the Alpha sign.
    // To use the Alpha sign, you first define the types of data you want.

    // Start a new configuration
    void NewConfig() {
      //log("New Config");
      pendingMemory = ""; // Clear
    }

    // Create a string object.  Length limit is 125 bytes.
    void MakeString(String Label, int Length) {
      allocateMemory(Label, ALPHA_STRING, Length);
    }

    // Start a new text file.
    void NewTextFile() {
      pendingText = "";
      SetCurrentLine(LINE_MIDDLE);
    }

    void AddText(String text) {
      pendingText += makeAlpha(text);
    }

    // Set the current line on which text appears.
    // Need at least a 2-line sign to use this.
    void SetCurrentLine(int lineOpt) {
      switch (lineOpt) {
        case LINE_MIDDLE: currentLine = " "; return;
        case LINE_TOP: currentLine = "\""; return;
        case LINE_BOTTOM: currentLine = "&"; return;
        case LINE_FILL: currentLine = "0"; return;
      }
    }

    // Finish the text file and add it to the data to send to the sign
    void FinishTextFile(String Label) {
      int mem = pendingText.length() + 8; // Allocate a bit extra just in case

      log("Finishing text file " + Label + " by allocating " + String(mem, DEC) + " bytes of memory.");
      // never play
      // allocateMemory FileLabel, ALPHA_TEXT, nMem, True, , , , "FE", "00"
      allocateMemory(Label, ALPHA_TEXT, mem);
      addData("A" + Label + pendingText);
      NewTextFile();
    }

    // Send the accumulated data to the sign.
    void SendToSign() {
      // Begin with the memory data.
      if (pendingMemory.length() > 0) {
          sendMemory();
          //delay(500);
      }

      // Now send the other data.
      sendPackets();
    }

    void SendBeep(int beepcount) {
      signPacket("E(2022" + String(beepcount - 1, HEX));
      //signOut "1" ' 3 beeps
      //        2   - tone
      //         02 - frequency (not supported on PPD)
      //           2 - duration, 0 to F
      //            2 - repeat (so 2 means beep 3 times)
    }

    // Sets the serial address of all currently connected LED signs.  Disconnect signs you don't want to set.
    // Values are text in the range of 00 to FF.
    void SetSerialAddress(String NewAddress) {
      // E -  COMMAND CODE: Special function
      // 7 - Set Address
      log("Setting serial address to " + NewAddress);
      signPacket("E7" + NewAddress);
    }

    // Set the order of the pages the sign is to display.
    // This can be a single page or multiple pages, like "A", or "AAAAB", or "ABC".
    void SetRunSequence(String Sequence) {
        // Commands here are:
        // E - Special Function
        // . - Set run sequence
        // T - Run based on times - S run in sequence
        // U - Unlocked
        signPacket("E.TU" + Sequence);
    }

    // Change the ID that this class addresses.
    void SetMyID(String ID) {
      mySignID = ID;
    }

    void SetString(String Label, String Value) {
      log("Setting " + Label + " to " + Value);
      //If Len(mEnc) > 125 Then mEnc = Left(mEnc, 125) ' limit to 125 characters
      signPacket("G" + Label + makeAlpha(Value));
    }

  private:
    String pendingMemory = "";
    String pendingText = "";
    String currentLine = "";
    String pendingData[ARRAYSIZE]; // TODO: Make this better
    int pendingCount = 0;
    bool init = false;
    String mySignID = "00"; // Default to all signs

    void log(String msg) {
      return;
      //return;
      char copy[500];
      int c = 0;
      for(int i = 0; i < msg.length(); i++ ) {
        if (msg[i] < 32) {
          copy[c++] = '[';
          copy[c++] = (msg[i] / 10) + '0';
          copy[c++] = (msg[i] % 10) + '0';
          copy[c++] = ']';
        } else {
          copy[c++] = msg[i];
        }
      }
      for (int i = c; i < 500; i++) {
        copy[i] = 0;
      }
    }

    // Add data to the pending list
    void addData(String s) {
      if (pendingCount > ARRAYSIZE - 1) {
        log("Can't add another pending item. Increase ARRAYSIZE");
        return;
      }
      pendingData[pendingCount] = s;
      pendingCount++;
    }

    // Send the pending data to the sign
    void sendPackets() {

      // Use the nested packet format, maybe it will be more reliable.
      //transBegin(false);

      for (int i = 0; i < pendingCount; i++) {
        log("Sending packet " + String(i, DEC) + " of " + String(pendingCount, DEC));

        //write_byte(0x02); // start of text (STX)
        //delay(110); // 100 ms delay required after STX

        //signOut(pendingData[i]);

        signPacket(pendingData[i]);

        //delay(100);
        //niceDelay(1000);
        pendingData[i] = ""; // Clear
        SendBeep(1);
        /*if (i < pendingCount - 1) {
          write_byte(0x03); // End of Text (ETX)
        }*/
      }
      //delay(500);
      //transEnd();

      // Clear the count
      pendingCount = 0;
      //SendBeep(1);
    }

    void niceDelay(long d) {
      long t = millis();
      while (millis() < t + d) {
        // do nothing
      }
    }

    // Allocate memory on the Alpha sign.
    void allocateMemory(String Label, int type, int size) {

      String s = Label;
      switch (type) {
        case ALPHA_TEXT:
          s += "A";        // Type: Text
          s += "L"; //"L";        // U for Unlocked, L for locked
          s += makeHex(size, 4);  // size in hex bytes
          s += "FF00"; // Start time and stop time, default to FF00
          break;
        case ALPHA_STRING:
          s += "B";        // Type: String
          s += "L";        // Strings are always locked
          s += makeHex(size, 4);  // size in hex bytes
          s += "0000";     // Placeholder
          break;
      }

      // Add it to the pending memory queue.
      pendingMemory += s;
    }

    void sendMemory() {

      log("Flushing memory queue: " + pendingMemory);

      // Send the pending memory dump to the sign.
      // E - Special Function
      // $ - Set memory
      signPacket("E$" + pendingMemory);
      pendingMemory = "";
    }

    // signPacket sends a packet to the sign
    void signPacket(String data) {
      //log(data);
      transBegin(false);
      signOut(data);
      transEnd();
    }

    void transBegin(bool usesNested) {
      // Send 20 null bytes to set baud rate on the sign
      // Minimum requirement is 5
      for (int i = 0; i < 5; i++) {
        write_byte(0x00);
      }

      write_byte(0x01); // start of header
      write_byte(0x5A);  // type code: all signs (Z)

      // Sign ID -- 00 is all signs
      signOut(mySignID);
      if (!usesNested) write_byte(0x02); // start of text, used in all cases unless we are doing nested packets
    }

    void transEnd() {
      write_byte(0x04); // end of transmission
    }

    void signOut(String data) {
      // Write the characters to the serial port.
      for(int i = 0; i < data.length(); i++ ) {
        write_byte(data[i]);
      }
    }

    // Create a hex number padded to the desired number of places.
    String makeHex(int num, int len) {
      String s = String(num, HEX);
      s.toUpperCase();
      while (s.length() < len) {
        s = "0" + s;
      }
      return s;
    }

    String makeAlpha(String s) {
      const String cc = String((char)27);

      // Use these special codes in your message to send commands to the Alpha sign
      s.replace("<SPEED:1>", String((char)21));
      s.replace("<SPEED:2>", String((char)22));
      s.replace("<SPEED:3>", String((char)23));
      s.replace("<SPEED:4>", String((char)24));
      s.replace("<SPEED:5>", String((char)25));

      // COLORS
      s.replace("<C:RED>", String((char)28) + "1");
      s.replace("<C:GREEN>", String((char)28) + "2");
      s.replace("<C:AMBER>", String((char)28) + "3");
      s.replace("<C:DIMRED>", String((char)28) + "4");
      s.replace("<C:DIMGREEN>", String((char)28) + "5");
      s.replace("<C:BROWN>", String((char)28) + "6");
      s.replace("<C:ORANGE>", String((char)28) + "7");
      s.replace("<C:YELLOW>", String((char)28) + "8");

      s.replace("<C:RAIN1>", String((char)28) + "9");
      s.replace("<C:RAIN2>", String((char)28) + "A");
      s.replace("<C:COLORMIX>", String((char)28) + "B");
      s.replace("<C:AUTO>", String((char)28) + "C");

      // FONTS
      s.replace("<F:SANS5>", String((char)26) + "1");
      s.replace("<F:SANS7>", String((char)26) + "3");
      s.replace("<F:SERIF7>", String((char)26) + "5");
      s.replace("<F:SERIF16>", String((char)26) + "8");
      s.replace("<F:SANS16>", String((char)26) + "9");

      // WIDE MODES
      s.replace("<WIDE:ON>", String((char)29) + "01");
      s.replace("<WIDE:OFF>", String((char)29) + "00");
      s.replace("<DWIDE:ON>", String((char)29) + "11");
      s.replace("<DWIDE:OFF>", String((char)29) + "10");


      // FIXED WIDTH
      s.replace("<FIXEDWIDTH:ON>", String((char)29) + "41");
      s.replace("<FIXEDWIDTH:OFF>", String((char)29) + "40");

      s.replace("<FIXED:ON>", String((char)30) + "1");
      s.replace("<FIXED:OFF>", String((char)30) + "0");

      //s.replace("<TRUE:ON>", String(6) + "1") // True Descenders
      //s.replace("<TRUE:OFF>", String(6) + "0")

      //s.replace("<DHIGH:ON>", String(5) + "1") // Double High
      //s.replace("<DHIGH:OFF>", String(5) + "0")

      // EFFECTS
      s.replace("<SCROLL>", cc + currentLine + "a");
      s.replace("<HOLD>", cc + currentLine + "b");
      s.replace("<FLASH>", cc + currentLine + "c");
      s.replace("<ROLL:UP>", cc + currentLine + "e");
      s.replace("<ROLL:DOWN>", cc + currentLine + "f");
      s.replace("<ROLL:LEFT>", cc + currentLine + "g");
      s.replace("<ROLL:RIGHT>", cc + currentLine + "h");
      s.replace("<ROLL:IN>", cc + currentLine + "p");
      s.replace("<ROLL:OUT>", cc + currentLine + "q");

      s.replace("<WIPE:UP>", cc + currentLine + "i");
      s.replace("<WIPE:DOWN>", cc + currentLine + "j");
      s.replace("<WIPE:LEFT>", cc + currentLine + "k");
      s.replace("<WIPE:RIGHT>", cc + currentLine + "l");
      s.replace("<WIPE:IN>", cc + currentLine + "r");
      s.replace("<WIPE:OUT>", cc + currentLine + "s");

      s.replace("<2LINESCROLLUP>", cc + currentLine + "m");      // Rolls one line at a time up from the bottom, 2-line display mode.
      s.replace("<AUTO>", cc + currentLine + "o");               // Just picks random effects
      s.replace("<TWINKLE>", cc + currentLine + "n" + "0");      // The message appears, and then twinkles.
      s.replace("<SPARKLE>", cc + currentLine + "n" + "1");      // Sorta like a dissolve
      s.replace("<SNOW>", cc + currentLine + "n" + "2");         // Message "snows" from the top.
      s.replace("<INTERLOCK>", cc + currentLine + "n" + "3");    // Pulls apart sideways and reassembles
      s.replace("<SWITCH>", cc + currentLine + "n" + "4");       // Like Interlock, except top-bottom
      s.replace("<SLIDE>", cc + currentLine + "n" + "5");        // Spells message from right
      s.replace("<SPRAY>", cc + currentLine + "n" + "6");        // Firehoses from the right
      s.replace("<STARBURST>", cc + currentLine + "n" + "7");    // Firework bangs to appear

      // ANIMATIONS
      s.replace("<ANIM:WELCOME>", cc + currentLine + "n" + "8");
      s.replace("<ANIM:SLOTS>", cc + currentLine + "n" + "9");
      s.replace("<ANIM:THANKYOU>", cc + currentLine + "n" + "S");
      s.replace("<ANIM:NOSMOKING>", cc + currentLine + "n" + "U");
      s.replace("<ANIM:DRINKDRIVE>", cc + currentLine + "n" + "V");
      s.replace("<ANIM:HORSE>", cc + currentLine + "n" + "W");
      s.replace("<ANIM:FIREWORKS>", cc + currentLine + "n" + "X");
      s.replace("<ANIM:TURBOCAR>", cc + currentLine + "n" + "Y");
      s.replace("<ANIM:CHERRYBOMB>", cc + currentLine + "n" + "Z");

      // OTHER
      s.replace("<DATE>", String((char)11) + "8");
      s.replace("<TIME>", String((char)19));
      s.replace("<NOHOLD>", String((char)9));
      s.replace("\\p", String((char)12));
      s.replace("\\n", String((char)13));

      // Calling up other stuff
      //s.replace("<PIC>", String((char)20));    // Must be followed by file label
      s.replace("<STRING>", String((char)16)); // Must be followed by file label

      return s;
    }

};
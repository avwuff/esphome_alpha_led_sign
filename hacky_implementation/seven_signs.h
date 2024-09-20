#include "alpha_sign.h"
#define NUM_SIGNS 7

class SevenSigns : public Component, public UARTDevice {
  public:
    SevenSigns(UARTComponent *parent) : UARTDevice(parent) {
        parentUart = parent;
    }

    // Create an array of AlphaSigns.  Item 0 represents all signs.
    // Items 1-7 address individual signs.
    AlphaSign* Signs[NUM_SIGNS + 1];

    // Called every so often? Maybe.
    void Tick() {

    }

    // Set the text on the sign
    void Update(int num, std::string mode, std::string top, std::string bottom) {
        // Switch the sign into the right mode
        if (mode == "Time") {
            setTime(num);
        } else if (mode == "Text") {
            twoLineText(num, String(top.c_str()), String(bottom.c_str()));
        } else if (mode == "Header") {
            twoLineHeader(num, String(top.c_str()), String(bottom.c_str()));
        } else if (mode == "OneLine") {
            oneLineScrollUp(num, String(top.c_str()));
        } else if (mode == "Scroll") {
            twoLineBottomScroll(num, String(top.c_str()), String(bottom.c_str()));
        } else if (mode == "Twinkle") {
            twoLineTwinkle(num, String(top.c_str()), String(bottom.c_str()));
        } else if (mode == "Slots") {
            slots(num);
        } else {
            twoLineText(num, " ", " ");
        }
    }

    void SetID(std::string ID) {
        String id = String(ID.c_str());
        if (id.length() > 0) {
            Signs[0]->SetSerialAddress(id);
            twoLineText(0, "ID set to", id);
        }
    }

    // Called when instantiated
    void Boot() {
        log("Boot");

        // Set up the array of signs.
        for (int i = 0; i < NUM_SIGNS + 1; i++) {
            Signs[i] = new AlphaSign(parentUart);
            Signs[i]->SetMyID("0" + String(i, DEC));
        }
    }

    // Called when the ESP has booted.
    void Start() {
        log("Startup");

        // Set up the base sign.
        setupBase();
    }

    void setupBase() {
        auto s = Signs[0];

        s->NewConfig();

        // STRINGS
        s->MakeString("d", 50); // These letters are the IDs of the strings in the Alpha's memory
        s->MakeString("e", 50);
        s->MakeString("i", 10);

        s->MakeString("1", 125);   // Top line part 1
        s->MakeString("2", 125);   // Top line part 2
        s->MakeString("3", 125);   // Second line part 1
        s->MakeString("4", 125);   // Second line part 2
        s->MakeString("5", 125);    // Second line part 3

            // File !: Sign is ready.
        s->NewTextFile();
        s->AddText("<HOLD>Sign booting...\\n");
        s->AddText("Sign <STRING>i\\p");
        s->FinishTextFile("!");

        // File B: Debug / Startup messages
        s->NewTextFile();
        s->SetCurrentLine(LINE_TOP);
        s->AddText("<HOLD><F:SANS7>");
        s->AddText("<STRING>d<NOHOLD>");
        s->SetCurrentLine(LINE_BOTTOM);
        s->AddText("<HOLD><F:SANS7>");
        s->AddText("<STRING>e<NOHOLD>");
        s->AddText("\\p");
        s->FinishTextFile("B");

        // File O: 1 line scroll up
        s->NewTextFile();
        s->SetCurrentLine(LINE_FILL);
        s->AddText("<HOLD><F:SANS7>");
        s->AddText("<2LINESCROLLUP>");
        s->AddText("<STRING>1<STRING>2<STRING>3<STRING>4<STRING>5");
        s->AddText("\\p");
        s->FinishTextFile("O");

        // File T: 2 line hold
        s->NewTextFile();
        s->SetCurrentLine(LINE_TOP);
        s->AddText("<HOLD><F:SANS7>");
        s->AddText("<STRING>1<STRING>2");
        s->SetCurrentLine(LINE_BOTTOM);
        s->AddText("<HOLD><F:SANS7>");
        s->AddText("<STRING>3<STRING>4<STRING>5");
        s->AddText("\\p");
        s->FinishTextFile("T");

        // File H: 2 line bottom scroll with bold header
        s->NewTextFile();
        s->SetCurrentLine(LINE_TOP);
        s->AddText("<HOLD><F:SANS7><WIDE:ON>");
        s->AddText("<STRING>1<STRING>2<WIDE:OFF>");
        s->SetCurrentLine(LINE_BOTTOM);
        s->AddText("<WIPE:RIGHT><F:SANS7>");
        s->AddText("<STRING>3<STRING>4<STRING>5");
        s->AddText("\\p");
        s->FinishTextFile("H");

        //File C: 2 line Twinkle
        s->NewTextFile();
        s->SetCurrentLine(LINE_FILL);
        s->AddText("<TWINKLE>");
        s->AddText("<STRING>1<STRING>2\\n");
        s->AddText("<STRING>3<STRING>4<STRING>5<NOHOLD>");
        s->AddText("\\p");
        s->FinishTextFile("C");

        s->NewTextFile();
        s->SetCurrentLine(LINE_FILL);
        s->AddText("<ROLL:DOWN><SPEED:1><F:SANS7>");
        s->AddText("<STRING>1<STRING>2");
        s->AddText("<ROLL:UP>");
        s->AddText("<STRING>3<STRING>4<STRING>5");
        s->AddText("<ROLL:UP>");
        s->FinishTextFile("Q");

        // File D: 2 line, bottom scroll
        s->NewTextFile();
        s->SetCurrentLine(LINE_TOP);
        s->AddText("<HOLD><F:SANS7>");
        s->AddText("<STRING>1<STRING>2");
        s->SetCurrentLine(LINE_BOTTOM);
        s->AddText("<WIPE:RIGHT><F:SANS7>");
        s->AddText("<STRING>3<STRING>4<STRING>5");
        s->AddText("\\p");
        s->FinishTextFile("D");

        // File E: Slots
        s->NewTextFile();
        s->AddText("<ANIM:SLOTS>");
        s->FinishTextFile("E");

        // SEND THE NEW PROGRAMMING TO THE SIGN!
        s->SendToSign();

        s->SetString("i", "???");

        // Indicate the sign numbers
        for (int i = 1; i <= NUM_SIGNS; i++) {
            Signs[i]->SetString("i", String(i, DEC));
        }

        s->SetRunSequence("!");

    }

  private:
    UARTComponent *parentUart;
    String lastPage[NUM_SIGNS + 1]; // the last page we switched to
    String lastTop[NUM_SIGNS + 1]; // last message on top
    String lastBot[NUM_SIGNS + 1];

    void log(String msg) {
        char copy[100];
        msg.toCharArray(copy, 100);
        ESP_LOGD("sevensigns", "%s", copy);
    }

    // Display the current time on this sign
    void setTime(int sign) {
        auto time = id(homeassistant_time).now();
        String top = String(time.strftime("%a %B ").c_str()) + remLeadZero(String(time.strftime("%d").c_str())); // Sat August 02
        String bot = remLeadZero(String(time.strftime("%I").c_str())) + String(time.strftime(":%M %p").c_str());
        twoLineText(sign, top, bot);
    }

    // Remove the leading zero since the time functions don't support it
    String remLeadZero(String p) {
        if (p.substring(0, 1) == "0") {
            return p.substring(1);
        }
        return p;
    }

    // Displays a message with the top and bottom lines.
    void twoLineText(int id, String top, String bottom) {
        if (top != lastTop[id]) {
            lastTop[id] = top;
            setInto(Signs[id], top, "1", "2", "", "", "");
        }
        if (bottom != lastBot[id]) {
            lastBot[id] = bottom;
            setInto(Signs[id], bottom, "3", "4", "5", "", "");
        }
        setPage(id, "T");
    }

    // Displays a message with a bold header
    void twoLineHeader(int id, String top, String bottom) {
        if (top != lastTop[id]) {
            lastTop[id] = top;
            setInto(Signs[id], top, "1", "2", "", "", "");
        }
        if (bottom != lastBot[id]) {
            lastBot[id] = bottom;
            setInto(Signs[id], bottom, "3", "4", "5", "", "");
        }
        setPage(id, "H");
    }

    void oneLineScrollUp(int id, String msg) {
        setInto(Signs[id], msg, "1", "2", "3", "4", "5");
        setPage(id, "O");
    }

    void twoLineBottomScroll(int id, String top, String bottom) {
        setInto(Signs[id], top, "1", "2", "", "", "");
        setInto(Signs[id], bottom, "3", "4", "5", "", "");
        setPage(id, "D");
    }

    void twoLineTwinkle(int id, String top, String bottom) {
        setInto(Signs[id], top, "1", "2", "", "", "");
        setInto(Signs[id], bottom, "3", "4", "5", "", "");
        setPage(id, "C");
    }

    void slots(int id) {
        setPage(id, "E");
    }


    void setPage(int id, String page) {
        if (page == lastPage[id]) {
            return; // no need to change it
        }
        lastPage[id] = page;
        Signs[id]->SetRunSequence(page);
    }

    void setInto(AlphaSign* s, String msg, String f1, String f2, String f3, String f4, String f5) {
      // take the first bit and put it into the first file.
        msg = setRemain(s, msg, f1);
        msg = setRemain(s, msg, f2);
        msg = setRemain(s, msg, f3);
        msg = setRemain(s, msg, f4);
        setRemain(s, msg, f5);
    }

    String setRemain(AlphaSign* s, String msg, String file) {
        // if no file was specified, do nothing
        if (file.length() == 0) return "";

        // is it more than 125 characters?
        if (msg.length() > 125) {
            s->SetString(file, msg.substring(0, 125));
            return msg.substring(125); // return everything after character 125
        }
        // less than 125 characters, just set it and then return blank.
        s->SetString(file, msg);
        return "";
    }
};
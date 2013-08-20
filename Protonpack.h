
#ifndef Protonpack_h
#define Protonpack_h

#include "Arduino.h"

typedef struct _powercell {
    const static int num_leds = 6;
    const static int UPDATE_RATE = 60;
    const static int MAX_BRIGHTNESS = 500;
    int leds[num_leds];
    int last_updated;
    int current_led;
    int current_brightness;
    boolean initializing;

} Powercell;

typedef struct _cyclotron {
    int leds[4];
    int last_updated;
    int current_led;
    int current_brightness;
    int fade_started;
    int fade_duration;
} Cyclotron;

typedef struct _protonpack {
    int last_updated;
    int now;
    int started_at;
    Powercell powercell;
    Cyclotron cyclotron;
} Pack;

typedef void (*updatePowercellCallback)(Pack*, Powercell*);
typedef void (*updateCyclotronCallback)(Pack*, Cyclotron*);

class Protonpack {
public:
    Protonpack();
    void update();
    void initialize();
    void setPowercellUpdateCallback(updatePowercellCallback);
    void setCyclotronUpdateCallback(updateCyclotronCallback);
private:
    Pack _pack;
    Powercell _cell;
    Cyclotron _cyclotron;
    updatePowercellCallback _update_powercell_cb;
    updateCyclotronCallback _update_cyclotron_cb;
    void _updatePowercell();
    void _updateCyclotron();
};

#endif
